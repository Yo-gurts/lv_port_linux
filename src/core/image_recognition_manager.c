#include "core/image_recognition_manager.h"

#include "image_recognize/image_recognize.h"
#include "mlog.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int initialized;
    int stop_thread;
    int task_pending;

    image_recognition_state_t state;
    int error_code;

    char input_path[256];
    char prompt[1024];
    char result_text[4096];
} image_recognition_ctx_t;

static image_recognition_ctx_t g_ctx = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
    .state = IMAGE_RECOGNITION_STATE_IDLE,
};

static void* image_recognition_worker(void* arg)
{
    (void)arg;

    while (1) {
        char input_path[256];
        char prompt[1024];
        int ret;
        image_recognizer_t* recognizer;
        char local_result[4096] = { 0 };

        pthread_mutex_lock(&g_ctx.mutex);
        while (!g_ctx.stop_thread && !g_ctx.task_pending) {
            pthread_cond_wait(&g_ctx.cond, &g_ctx.mutex);
        }

        if (g_ctx.stop_thread) {
            pthread_mutex_unlock(&g_ctx.mutex);
            break;
        }

        snprintf(input_path, sizeof(input_path), "%s", g_ctx.input_path);
        snprintf(prompt, sizeof(prompt), "%s", g_ctx.prompt);
        g_ctx.task_pending = 0;
        pthread_mutex_unlock(&g_ctx.mutex);

        recognizer = image_recognizer_create();
        if (!recognizer) {
            ret = -1;
        } else {
            ret = image_recognizer_from_file(recognizer, input_path, prompt, local_result, sizeof(local_result));
            if (ret != 0) {
                MLOG_ERR("image_recognizer_from_file failed: %d (%s)", ret, image_recognizer_get_error_string(ret));
            }
            image_recognizer_destroy(recognizer);
        }

        pthread_mutex_lock(&g_ctx.mutex);
        g_ctx.error_code = ret;
        if (ret == 0) {
            snprintf(g_ctx.result_text, sizeof(g_ctx.result_text), "%s", local_result);
            g_ctx.state = IMAGE_RECOGNITION_STATE_SUCCESS;
        } else {
            g_ctx.result_text[0] = '\0';
            g_ctx.state = IMAGE_RECOGNITION_STATE_FAILED;
        }
        pthread_mutex_unlock(&g_ctx.mutex);
    }

    return NULL;
}

int image_recognition_manager_init(void)
{
    int ret;

    pthread_mutex_lock(&g_ctx.mutex);
    if (g_ctx.initialized) {
        pthread_mutex_unlock(&g_ctx.mutex);
        return 0;
    }

    g_ctx.stop_thread = 0;
    g_ctx.task_pending = 0;
    g_ctx.state = IMAGE_RECOGNITION_STATE_IDLE;
    g_ctx.error_code = 0;
    g_ctx.input_path[0] = '\0';
    g_ctx.prompt[0] = '\0';
    g_ctx.result_text[0] = '\0';
    pthread_mutex_unlock(&g_ctx.mutex);

    ret = pthread_create(&g_ctx.thread, NULL, image_recognition_worker, NULL);
    if (ret != 0) {
        MLOG_ERR("image recognition worker create failed: %d", ret);
        return -1;
    }

    pthread_mutex_lock(&g_ctx.mutex);
    g_ctx.initialized = 1;
    pthread_mutex_unlock(&g_ctx.mutex);
    return 0;
}

void image_recognition_manager_deinit(void)
{
    int need_join = 0;

    pthread_mutex_lock(&g_ctx.mutex);
    if (g_ctx.initialized) {
        g_ctx.stop_thread = 1;
        pthread_cond_signal(&g_ctx.cond);
        need_join = 1;
    }
    pthread_mutex_unlock(&g_ctx.mutex);

    if (need_join) {
        (void)pthread_join(g_ctx.thread, NULL);
    }

    pthread_mutex_lock(&g_ctx.mutex);
    g_ctx.initialized = 0;
    g_ctx.stop_thread = 0;
    g_ctx.task_pending = 0;
    g_ctx.state = IMAGE_RECOGNITION_STATE_IDLE;
    g_ctx.error_code = 0;
    g_ctx.input_path[0] = '\0';
    g_ctx.prompt[0] = '\0';
    g_ctx.result_text[0] = '\0';
    pthread_mutex_unlock(&g_ctx.mutex);
}

int image_recognition_manager_start(const char* input_real_path, const char* prompt)
{
    if (!input_real_path || !prompt || prompt[0] == '\0')
        return -1;

    pthread_mutex_lock(&g_ctx.mutex);
    if (!g_ctx.initialized) {
        pthread_mutex_unlock(&g_ctx.mutex);
        return -1;
    }

    if (g_ctx.state == IMAGE_RECOGNITION_STATE_RUNNING) {
        pthread_mutex_unlock(&g_ctx.mutex);
        return -1;
    }

    snprintf(g_ctx.input_path, sizeof(g_ctx.input_path), "%s", input_real_path);
    snprintf(g_ctx.prompt, sizeof(g_ctx.prompt), "%s", prompt);
    g_ctx.error_code = 0;
    g_ctx.result_text[0] = '\0';
    g_ctx.state = IMAGE_RECOGNITION_STATE_RUNNING;
    g_ctx.task_pending = 1;
    pthread_cond_signal(&g_ctx.cond);
    pthread_mutex_unlock(&g_ctx.mutex);

    return 0;
}

image_recognition_state_t image_recognition_manager_get_state(void)
{
    image_recognition_state_t state;

    pthread_mutex_lock(&g_ctx.mutex);
    state = g_ctx.state;
    pthread_mutex_unlock(&g_ctx.mutex);

    return state;
}

int image_recognition_manager_get_error(void)
{
    int err;

    pthread_mutex_lock(&g_ctx.mutex);
    err = g_ctx.error_code;
    pthread_mutex_unlock(&g_ctx.mutex);

    return err;
}

int image_recognition_manager_get_result_text(char* out_text, size_t out_size)
{
    int ret = -1;

    if (!out_text || out_size == 0)
        return -1;

    pthread_mutex_lock(&g_ctx.mutex);
    if (g_ctx.state == IMAGE_RECOGNITION_STATE_SUCCESS && g_ctx.result_text[0] != '\0') {
        if (snprintf(out_text, out_size, "%s", g_ctx.result_text) < (int)out_size) {
            ret = 0;
        }
    }
    pthread_mutex_unlock(&g_ctx.mutex);

    return ret;
}

void image_recognition_manager_reset(void)
{
    pthread_mutex_lock(&g_ctx.mutex);
    if (g_ctx.state != IMAGE_RECOGNITION_STATE_RUNNING) {
        g_ctx.state = IMAGE_RECOGNITION_STATE_IDLE;
        g_ctx.error_code = 0;
        g_ctx.result_text[0] = '\0';
    }
    pthread_mutex_unlock(&g_ctx.mutex);
}
