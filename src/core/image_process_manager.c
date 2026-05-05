#include "core/image_process_manager.h"

#include "config.h"
#include "core/file_manager.h"
#include "filemng.h"
#include "img2img/img2img.h"
#include "jpegp.h"
#include "mlog.h"

#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define AI_OUT_IMG_WIDTH 960
#define AI_OUT_IMG_HEIGHT 720
#define SUBPIC_WIDTH 640
#define SUBPIC_HEIGHT 480
#define THUMBNAIL_WIDTH 200
#define THUMBNAIL_HEIGHT 140

typedef struct {
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int initialized;
    int stop_thread;
    int task_pending;

    image_process_state_t state;
    int error_code;

    char input_path[256];
    char prompt[1024];
    char output_path[256];
    char output_display_path[256];
} image_process_ctx_t;

static image_process_ctx_t g_ctx = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
    .state = IMAGE_PROCESS_STATE_IDLE,
};

static const char* to_real_path(const char* path)
{
    if (path && path[0] == 'A' && path[1] == ':')
        return path + 2;
    return path;
}

static int to_lv_path(const char* real_path, char* lv_path, size_t lv_size)
{
    if (!real_path || !lv_path || lv_size == 0)
        return -1;

    if (real_path[0] == '/') {
        if (snprintf(lv_path, lv_size, "A:%s", real_path) >= (int)lv_size)
            return -1;
    } else {
        if (snprintf(lv_path, lv_size, "%s", real_path) >= (int)lv_size)
            return -1;
    }
    return 0;
}

static const char* get_basename_ptr(const char* path)
{
    const char* slash;

    if (!path)
        return NULL;

    slash = strrchr(path, '/');
    return slash ? (slash + 1) : path;
}

static int find_photo_index_by_name(const char* file_name)
{
    int i;
    int count;
    char name_buf[FILE_MANAGER_MAX_NAME_LEN];

    if (!file_name)
        return -1;

    if (file_manager_refresh_photo_list() != 0)
        return -1;

    count = file_manager_get_photo_count();
    if (count <= 0)
        return -1;

    for (i = 0; i < count; i++) {
        if (file_manager_get_photo_name(i, name_buf, sizeof(name_buf)) != 0)
            continue;
        if (strcmp(name_buf, file_name) == 0)
            return i;
    }

    return -1;
}

static int ends_with_ai_suffix(const char* name_no_ext, size_t len, int* out_num)
{
    size_t i;
    int num = 0;

    if (!name_no_ext || len < 7 || !out_num)
        return 0;

    if (name_no_ext[len - 7] != '_' || name_no_ext[len - 6] != 'A' || name_no_ext[len - 5] != 'I')
        return 0;

    for (i = len - 4; i < len; i++) {
        if (!isdigit((unsigned char)name_no_ext[i]))
            return 0;
        num = num * 10 + (name_no_ext[i] - '0');
    }

    *out_num = num;
    return 1;
}

static int generate_output_path(const char* input_real_path, char* out_path, size_t out_size)
{
    const char* base_name;
    const char* ext;
    const char* out_dir_real;
    char stem[192];
    char pure_name[192];
    int start_num = 1;
    int ai_num = 0;
    int next_num;
    int n;

    if (!input_real_path || !out_path || out_size == 0)
        return -1;

    base_name = get_basename_ptr(input_real_path);
    if (!base_name)
        return -1;

    ext = strrchr(base_name, '.');
    if (!ext || ext == base_name)
        return -1;

    if ((size_t)(ext - base_name) >= sizeof(stem))
        return -1;

    snprintf(stem, sizeof(stem), "%.*s", (int)(ext - base_name), base_name);

    if (ends_with_ai_suffix(stem, strlen(stem), &ai_num)) {
        size_t pure_len = strlen(stem) - 7;
        if (pure_len >= sizeof(pure_name))
            return -1;
        snprintf(pure_name, sizeof(pure_name), "%.*s", (int)pure_len, stem);
        start_num = ai_num + 1;
    } else {
        snprintf(pure_name, sizeof(pure_name), "%s", stem);
    }

    out_dir_real = to_real_path(PHOTO_ALBUM_IMAGE_PATH);
    if (!out_dir_real)
        return -1;

    for (next_num = start_num; next_num <= 9999; next_num++) {
        n = snprintf(out_path, out_size, "%s%s_AI%04d%s", out_dir_real, pure_name, next_num, ext);
        if (n < 0 || (size_t)n >= out_size)
            return -1;
        if (access(out_path, F_OK) != 0) {
            return 0;
        }
    }

    return -1;
}

static int generate_output_derived_images(char* output_real_path)
{
    JPEGP_PACKET_FILE_PARAM_S jpegp_param;
    int jpegp_ret;

    if (!output_real_path)
        return -1;

    if (access(output_real_path, F_OK) != 0) {
        MLOG_ERR("Output image not found: %s", output_real_path);
        return -1;
    }

    memset(&jpegp_param, 0, sizeof(jpegp_param));
    jpegp_param.src_file_name = (const CVI_VOID*)output_real_path;
    jpegp_param.dst_file_name = (const CVI_VOID*)output_real_path;
    jpegp_param.src_width = AI_OUT_IMG_WIDTH;
    jpegp_param.src_height = AI_OUT_IMG_HEIGHT;
    jpegp_param.subpic_width = SUBPIC_WIDTH;
    jpegp_param.subpic_height = SUBPIC_HEIGHT;
    jpegp_param.thumbnail_width = THUMBNAIL_WIDTH;
    jpegp_param.thumbnail_height = THUMBNAIL_HEIGHT;

    jpegp_ret = JPEGP_Gen_Thumbnail_And_SubPic_To_File(&jpegp_param);
    if (jpegp_ret != 0) {
        MLOG_ERR("Generate thumbnail/subpic failed: %d, file=%s", jpegp_ret, output_real_path);
        return -1;
    }

    if (FILEMNG_AddFile(0, output_real_path) != 0) {
        MLOG_WARN("FILEMNG_AddFile failed: %s", output_real_path);
    }

    return 0;
}

static int pick_display_path_via_file_manager(char* output_real_path, char* out_display_path, size_t out_size)
{
    const char* file_name;
    int photo_index;

    if (!output_real_path || !out_display_path || out_size == 0)
        return -1;

    file_name = get_basename_ptr(output_real_path);
    if (!file_name)
        return -1;

    photo_index = find_photo_index_by_name(file_name);
    if (photo_index < 0)
        return -1;

    if (file_manager_get_photo_subpic_path(photo_index, out_display_path, out_size, FILE_PATH_LV) == 0)
        return 0;
    if (file_manager_get_photo_thumbnail_path(photo_index, out_display_path, out_size, FILE_PATH_LV) == 0)
        return 0;
    if (file_manager_get_photo_path(photo_index, out_display_path, out_size, FILE_PATH_LV) == 0)
        return 0;

    return -1;
}

static int process_style(const char* input_real_path, const char* prompt, char* output_real_path, size_t output_size,
                         char* output_display_path, size_t display_size)
{
    int ret;

    if (!input_real_path || !prompt || !output_real_path || !output_display_path)
        return -1;

    if (generate_output_path(input_real_path, output_real_path, output_size) != 0)
        return -1;

    img2img_processor_t* processor = img2img_create();
    if (!processor)
        return -1;

    img2img_params_t params = img2img_default_params();
    params.prompt = prompt;
    params.width = AI_OUT_IMG_WIDTH;
    params.height = AI_OUT_IMG_HEIGHT;

    ret = img2img_process_file(processor, input_real_path, &params, output_real_path);
    if (ret != 0) {
        MLOG_ERR("img2img failed: %d (%s)", ret, img2img_get_error_string(ret));
        img2img_destroy(processor);
        return ret;
    }
    img2img_destroy(processor);

    if (generate_output_derived_images(output_real_path) != 0) {
        return -1;
    }

    if (pick_display_path_via_file_manager(output_real_path, output_display_path, display_size) != 0) {
        MLOG_WARN("Pick display path failed, fallback to original output");
        if (to_lv_path(output_real_path, output_display_path, display_size) != 0)
            return -1;
    }

    return 0;
}

static void* image_process_worker(void* arg)
{
    (void)arg;

    while (1) {
        char input_path[256];
        char prompt[1024];
        char output_path[256] = { 0 };
        char output_display_path[256] = { 0 };
        int ret;

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

        ret = process_style(input_path, prompt, output_path, sizeof(output_path), output_display_path,
                            sizeof(output_display_path));

        pthread_mutex_lock(&g_ctx.mutex);
        g_ctx.error_code = ret;
        if (ret == 0) {
            snprintf(g_ctx.output_path, sizeof(g_ctx.output_path), "%s", output_path);
            snprintf(g_ctx.output_display_path, sizeof(g_ctx.output_display_path), "%s", output_display_path);
            g_ctx.state = IMAGE_PROCESS_STATE_SUCCESS;
        } else {
            g_ctx.output_path[0] = '\0';
            g_ctx.output_display_path[0] = '\0';
            g_ctx.state = IMAGE_PROCESS_STATE_FAILED;
        }
        pthread_mutex_unlock(&g_ctx.mutex);
    }

    return NULL;
}

int image_process_manager_init(void)
{
    int ret;

    pthread_mutex_lock(&g_ctx.mutex);
    if (g_ctx.initialized) {
        pthread_mutex_unlock(&g_ctx.mutex);
        return 0;
    }

    g_ctx.stop_thread = 0;
    g_ctx.task_pending = 0;
    g_ctx.state = IMAGE_PROCESS_STATE_IDLE;
    g_ctx.error_code = 0;
    g_ctx.input_path[0] = '\0';
    g_ctx.prompt[0] = '\0';
    g_ctx.output_path[0] = '\0';
    g_ctx.output_display_path[0] = '\0';
    pthread_mutex_unlock(&g_ctx.mutex);

    ret = pthread_create(&g_ctx.thread, NULL, image_process_worker, NULL);
    if (ret != 0) {
        MLOG_ERR("image process worker create failed: %d", ret);
        return -1;
    }

    pthread_mutex_lock(&g_ctx.mutex);
    g_ctx.initialized = 1;
    pthread_mutex_unlock(&g_ctx.mutex);
    return 0;
}

void image_process_manager_deinit(void)
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
    g_ctx.state = IMAGE_PROCESS_STATE_IDLE;
    g_ctx.error_code = 0;
    g_ctx.input_path[0] = '\0';
    g_ctx.prompt[0] = '\0';
    g_ctx.output_path[0] = '\0';
    g_ctx.output_display_path[0] = '\0';
    pthread_mutex_unlock(&g_ctx.mutex);
}

int image_process_manager_start_style(const char* input_real_path, const char* prompt)
{
    if (!input_real_path || !prompt || prompt[0] == '\0')
        return -1;

    pthread_mutex_lock(&g_ctx.mutex);
    if (!g_ctx.initialized) {
        pthread_mutex_unlock(&g_ctx.mutex);
        return -1;
    }

    if (g_ctx.state == IMAGE_PROCESS_STATE_RUNNING) {
        pthread_mutex_unlock(&g_ctx.mutex);
        return -1;
    }

    snprintf(g_ctx.input_path, sizeof(g_ctx.input_path), "%s", input_real_path);
    snprintf(g_ctx.prompt, sizeof(g_ctx.prompt), "%s", prompt);
    g_ctx.error_code = 0;
    g_ctx.output_path[0] = '\0';
    g_ctx.output_display_path[0] = '\0';
    g_ctx.state = IMAGE_PROCESS_STATE_RUNNING;
    g_ctx.task_pending = 1;
    pthread_cond_signal(&g_ctx.cond);
    pthread_mutex_unlock(&g_ctx.mutex);

    return 0;
}

image_process_state_t image_process_manager_get_state(void)
{
    image_process_state_t state;

    pthread_mutex_lock(&g_ctx.mutex);
    state = g_ctx.state;
    pthread_mutex_unlock(&g_ctx.mutex);

    return state;
}

int image_process_manager_get_error(void)
{
    int err;

    pthread_mutex_lock(&g_ctx.mutex);
    err = g_ctx.error_code;
    pthread_mutex_unlock(&g_ctx.mutex);

    return err;
}

int image_process_manager_get_result_path(char* out_path, size_t out_size, int lv_path)
{
    int ret = -1;

    if (!out_path || out_size == 0)
        return -1;

    pthread_mutex_lock(&g_ctx.mutex);
    if (g_ctx.state == IMAGE_PROCESS_STATE_SUCCESS) {
        const char* src = lv_path ? g_ctx.output_display_path : g_ctx.output_path;
        if (src[0] != '\0' && snprintf(out_path, out_size, "%s", src) < (int)out_size) {
            ret = 0;
        }
    }
    pthread_mutex_unlock(&g_ctx.mutex);

    return ret;
}

void image_process_manager_reset(void)
{
    pthread_mutex_lock(&g_ctx.mutex);
    if (g_ctx.state != IMAGE_PROCESS_STATE_RUNNING) {
        g_ctx.state = IMAGE_PROCESS_STATE_IDLE;
        g_ctx.error_code = 0;
        g_ctx.output_path[0] = '\0';
        g_ctx.output_display_path[0] = '\0';
    }
    pthread_mutex_unlock(&g_ctx.mutex);
}
