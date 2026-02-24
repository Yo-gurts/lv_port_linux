#include "core/param_manager.h"

#include <stdlib.h>
#include <string.h>

#include "mlog.h"

#define PARAM_MANAGER_TAG "param_manager"

/* 最大回调数量 */
#define MAX_CALLBACKS 4

/* 参数默认值 - 与configs数组的value字段对应 */
static const int default_values[PARAM_ID_BUTT] = {
    [PARAM_ID_RESOLUTION] = 0, /* 8M(3840x2160) */
    [PARAM_ID_WHITE_BALANCE] = 0, /* 自动 */
    [PARAM_ID_ISO] = 0, /* 自动 */
    [PARAM_ID_EXPOSURE] = 4, /* EV0 (中间值) */
    [PARAM_ID_QUALITY] = 0, /* 超高画质 */
    [PARAM_ID_FACE_DETECTION] = 0, /* 关闭 */
    [PARAM_ID_SMILE_CAPTURE] = 0, /* 关闭 */
    [PARAM_ID_VIDEO_RESOLUTION] = 0, /* 4K(3840x2160) */
    [PARAM_ID_AI_MODE] = 0, /* 风格变换 */
    [PARAM_ID_VOLUME] = 50, /* 默认音量50% */
};

/* 参数当前值 */
static int current_values[PARAM_ID_BUTT];

/* 回调结构体 */
typedef struct {
    param_change_callback_t callback;
    void* user_data;
    int valid;
} callback_entry_t;

/* 回调列表 */
static callback_entry_t callbacks[MAX_CALLBACKS];

/* 是否已初始化 */
static int g_initialized = 0;

int param_manager_init(void)
{
    if (g_initialized) {
        MLOG_WARN("param_manager already initialized\n");
        return 0;
    }

    memset(current_values, 0, sizeof(current_values));
    memset(callbacks, 0, sizeof(callbacks));

    /* 加载默认值 */
    for (int i = 0; i < PARAM_ID_BUTT; i++) {
        current_values[i] = default_values[i];
    }

    g_initialized = 1;
    MLOG_INFO("param_manager initialized\n");
    return 0;
}

void param_manager_deinit(void)
{
    if (!g_initialized) {
        return;
    }

    memset(callbacks, 0, sizeof(callbacks));
    g_initialized = 0;
    MLOG_INFO("param_manager deinitialized\n");
}

int param_manager_get(param_id_t id)
{
    if (!g_initialized || id >= PARAM_ID_BUTT) {
        MLOG_ERR("param_manager not initialized or invalid id: %d\n", id);
        return -1;
    }

    return current_values[id];
}

int param_manager_set(param_id_t id, int value)
{
    if (!g_initialized || id >= PARAM_ID_BUTT) {
        MLOG_ERR("param_manager not initialized or invalid id: %d\n", id);
        return -1;
    }

    int old_value = current_values[id];
    if (old_value == value) {
        return 0;
    }

    current_values[id] = value;
    MLOG_DBG("param[%d]: %d -> %d\n", id, old_value, value);

    /* 通知所有回调 */
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (callbacks[i].valid && callbacks[i].callback) {
            callbacks[i].callback(id, value, callbacks[i].user_data);
        }
    }

    return 0;
}

int param_manager_get_default(param_id_t id)
{
    if (id >= PARAM_ID_BUTT) {
        MLOG_ERR("invalid param id: %d\n", id);
        return -1;
    }

    return default_values[id];
}

void param_manager_reset_all(void)
{
    if (!g_initialized) {
        return;
    }

    for (int i = 0; i < PARAM_ID_BUTT; i++) {
        int old_value = current_values[i];
        current_values[i] = default_values[i];

        if (old_value != default_values[i]) {
            /* 通知回调 */
            for (int j = 0; j < MAX_CALLBACKS; j++) {
                if (callbacks[j].valid && callbacks[j].callback) {
                    callbacks[j].callback(i, default_values[i], callbacks[j].user_data);
                }
            }
        }
    }

    MLOG_INFO("param_manager reset to defaults\n");
}

int param_manager_register_callback(param_change_callback_t callback, void* user_data)
{
    if (!callback) {
        MLOG_ERR("callback is NULL\n");
        return -1;
    }

    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (!callbacks[i].valid) {
            callbacks[i].callback = callback;
            callbacks[i].user_data = user_data;
            callbacks[i].valid = 1;
            MLOG_DBG("registered callback at slot %d\n", i);
            return 0;
        }
    }

    MLOG_ERR("no free callback slot\n");
    return -1;
}

void param_manager_unregister_callback(param_change_callback_t callback)
{
    if (!callback) {
        return;
    }

    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (callbacks[i].valid && callbacks[i].callback == callback) {
            callbacks[i].valid = 0;
            callbacks[i].callback = NULL;
            callbacks[i].user_data = NULL;
            MLOG_DBG("unregistered callback at slot %d\n", i);
            return;
        }
    }
}
