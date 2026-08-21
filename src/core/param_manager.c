#include "core/param_manager.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mlog.h"
#include "param.h"

#define PARAM_MANAGER_TAG "param_manager"

/* 最大回调数量 */
#define MAX_CALLBACKS 8

typedef struct {
    int min_value;
    int max_value;
    uint8_t validate_enabled;
} param_rule_t;

/* 参数默认值 - 与configs数组的value字段对应 */
static const int default_values[PARAM_ID_BUTT] = {
    [PARAM_ID_RESOLUTION] = PHOTO_RESOLUTION_12M,
    [PARAM_ID_WHITE_BALANCE] = WHITE_BALANCE_AUTO,
    [PARAM_ID_ISO] = ISO_AUTO,
    [PARAM_ID_EXPOSURE] = EXPOSURE_EV_0,
    [PARAM_ID_QUALITY] = QUALITY_SUPER,
    [PARAM_ID_FACE_DETECTION] = 0, /* 关闭 */
    [PARAM_ID_SMILE_CAPTURE] = 0, /* 关闭 */
    [PARAM_ID_VIDEO_RESOLUTION] = VIDEO_RESOLUTION_4K,
    [PARAM_ID_AI_MODE] = AI_MODE_STYLE_TRANSFER,
    [PARAM_ID_FILTER_INDEX] = 0, /* 默认滤镜下标=0（原图） */
    [PARAM_ID_FILTER_RESET_ON_MODE_SWITCH] = 0, /* 默认切模式不重置滤镜 */
    [PARAM_ID_ZOOM] = ZOOM_LEVEL_1X, /* 默认1x */
    [PARAM_ID_VOLUME] = 50, /* 默认音量50% */
    [PARAM_ID_AUTO_SLEEP] = 1, /* 默认开启自动息屏 */
    [PARAM_ID_FOCUS_FRAME_STATE] = FOCUS_FRAME_STATE_HIDDEN, /* 默认隐藏 */
    [PARAM_ID_WIFI_CONNECTED] = 0, /* 默认未连接 */
    [PARAM_ID_WIFI_SIGNAL_DBM] = -1, /* 默认无信号 */
    [PARAM_ID_WIFI_ENABLED] = 1, /* 默认开启 */
    [PARAM_ID_BATTERY_VAL] = 0,
    [PARAM_ID_SD_READY] = SD_READY_FALSE /* 默认未就绪 */
};

/* 参数合法性规则：set 时按规则校验；get 不做合法性修正。
 * ❗❗❗ 注意：部分默认值要和 profiles/config_menu.ini 中定义的一致 */
static const param_rule_t param_rules[PARAM_ID_BUTT] = {
    [PARAM_ID_RESOLUTION] = { .min_value = PHOTO_RESOLUTION_8M, .max_value = PHOTO_RESOLUTION_BUTT - 1, .validate_enabled = 1 },
    [PARAM_ID_WHITE_BALANCE] = { .min_value = WHITE_BALANCE_AUTO, .max_value = WHITE_BALANCE_BUTT - 1, .validate_enabled = 1 },
    [PARAM_ID_ISO] = { .min_value = ISO_AUTO, .max_value = ISO_BUTT - 1, .validate_enabled = 1 },
    [PARAM_ID_EXPOSURE] = { .min_value = EXPOSURE_EV_NEG_2_0, .max_value = EXPOSURE_BUTT - 1, .validate_enabled = 1 },
    [PARAM_ID_QUALITY] = { .min_value = QUALITY_SUPER, .max_value = QUALITY_BUTT - 1, .validate_enabled = 1 },
    [PARAM_ID_FACE_DETECTION] = { .min_value = 0, .max_value = 1, .validate_enabled = 1 },
    [PARAM_ID_SMILE_CAPTURE] = { .min_value = 0, .max_value = 1, .validate_enabled = 1 },
    [PARAM_ID_VIDEO_RESOLUTION] = { .min_value = VIDEO_RESOLUTION_4K, .max_value = VIDEO_RESOLUTION_BUTT - 1, .validate_enabled = 1 },
    [PARAM_ID_AI_MODE] = { .min_value = AI_MODE_STYLE_TRANSFER, .max_value = AI_MODE_BUTT - 1, .validate_enabled = 1 },
    [PARAM_ID_FILTER_INDEX] = { .min_value = 0, .max_value = 255, .validate_enabled = 1 },
    [PARAM_ID_FILTER_RESET_ON_MODE_SWITCH] = { .min_value = 0, .max_value = 1, .validate_enabled = 1 },
    [PARAM_ID_ZOOM] = { .min_value = ZOOM_LEVEL_1X, .max_value = ZOOM_LEVEL_6X, .validate_enabled = 1 },
    [PARAM_ID_VOLUME] = { .min_value = 0, .max_value = 100, .validate_enabled = 1 },
    [PARAM_ID_AUTO_SLEEP] = { .min_value = 0, .max_value = 1, .validate_enabled = 1 },
    [PARAM_ID_FOCUS_FRAME_STATE] = { .min_value = FOCUS_FRAME_STATE_HIDDEN, .max_value = FOCUS_FRAME_STATE_LOCKING, .validate_enabled = 1 },
    [PARAM_ID_WIFI_CONNECTED] = { .min_value = 0, .max_value = 1, .validate_enabled = 1 },
    [PARAM_ID_WIFI_SIGNAL_DBM] = { .min_value = -150, .max_value = 0, .validate_enabled = 1 },
    [PARAM_ID_WIFI_ENABLED] = { .min_value = 0, .max_value = 1, .validate_enabled = 1 },
    [PARAM_ID_BATTERY_VAL] = { .min_value = -1, .max_value = 100, .validate_enabled = 1 },
    [PARAM_ID_SD_READY] = { .min_value = SD_READY_FALSE, .max_value = SD_READY_TRUE, .validate_enabled = 1 }
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
static pthread_mutex_t g_param_mutex = PTHREAD_MUTEX_INITIALIZER;
static int pending_values[PARAM_ID_BUTT];
static int pending_flags[PARAM_ID_BUTT];

static int is_valid_param_id(param_id_t id)
{
    return id >= PARAM_ID_RESOLUTION && id < PARAM_ID_BUTT;
}

static int validate_param_value(param_id_t id, int value)
{
    const param_rule_t* rule = NULL;

    if (!is_valid_param_id(id)) {
        return -1;
    }

    if (id == PARAM_ID_ZOOM) {
        if (value != ZOOM_LEVEL_1X && value != ZOOM_LEVEL_2X && value != ZOOM_LEVEL_3X && value != ZOOM_LEVEL_6X) {
            MLOG_WARN("param[%d] value %d is not a supported zoom level", id, value);
            return -1;
        }
        return 0;
    }

    rule = &param_rules[id];
    if (!rule->validate_enabled) {
        return 0;
    }

    if (value < rule->min_value || value > rule->max_value) {
        MLOG_WARN("param[%d] value %d out of range [%d, %d]\n", id, value, rule->min_value, rule->max_value);
        return -1;
    }

    return 0;
}

/* 从底层持久化参数(app_cfg.bin)同步启动值，避免 UI 每次开机用硬编码默认值与底层不对齐。
 * 仅同步底层会持久化且有 UI 对应项的参数；值非法时保留 UI 默认。 */
static void sync_from_bottom(param_id_t id, int value)
{
    if (validate_param_value(id, value) != 0) {
        MLOG_WARN("param[%d] bottom value %d invalid, keep default %d\n", id, value, current_values[id]);
        return;
    }
    current_values[id] = value;
}

int param_manager_init(void)
{
    int i = 0;

    if (g_initialized) {
        MLOG_WARN("param_manager already initialized\n");
        return 0;
    }

    pthread_mutex_lock(&g_param_mutex);
    memset(current_values, 0, sizeof(current_values));
    memset(callbacks, 0, sizeof(callbacks));
    memset(pending_values, 0, sizeof(pending_values));
    memset(pending_flags, 0, sizeof(pending_flags));

    /* 加载默认值 */
    for (i = 0; i < PARAM_ID_BUTT; i++) {
        int value = default_values[i];
        if (validate_param_value((param_id_t)i, value) != 0) {
            MLOG_WARN("invalid default value for param[%d]: %d, fallback to min=%d\n", i, value, param_rules[i].min_value);
            value = param_rules[i].min_value;
        }
        current_values[i] = value;
    }

    /* 从底层持久化参数(app_cfg.bin)同步有对应项的配置。
     * 直接取共享内存参数指针(PARAM_GetCtx)而非深拷贝，避免 308KB 拷贝；
     * 注意：PARAM_CFG_S 约 308KB，禁止在 UI 线程(栈 256KB)栈上声明。 */
    PARAM_CONTEXT_S* pstParamCtx = PARAM_GetCtx();
    if (pstParamCtx != NULL && pstParamCtx->bInit == true && pstParamCtx->pstCfg != NULL) {
        PARAM_CFG_S* cfg = pstParamCtx->pstCfg;
        sync_from_bottom(PARAM_ID_RESOLUTION, (int)cfg->Menu.PhotoSize.Current);
        sync_from_bottom(PARAM_ID_QUALITY, (int)cfg->Menu.PhotoQuality.Current);
        sync_from_bottom(PARAM_ID_VIDEO_RESOLUTION, (int)cfg->Menu.VideoSize.Current);
        sync_from_bottom(PARAM_ID_FACE_DETECTION, (int)cfg->Menu.FaceDet.Current);
        sync_from_bottom(PARAM_ID_SMILE_CAPTURE, (int)cfg->Menu.FaceSmile.Current);
    } else {
        MLOG_WARN("PARAM ctx not ready, keep UI defaults\n");
    }

    g_initialized = 1;
    pthread_mutex_unlock(&g_param_mutex);
    MLOG_INFO("param_manager initialized\n");
    return 0;
}

void param_manager_deinit(void)
{
    pthread_mutex_lock(&g_param_mutex);
    if (!g_initialized) {
        pthread_mutex_unlock(&g_param_mutex);
        return;
    }

    memset(callbacks, 0, sizeof(callbacks));
    memset(pending_flags, 0, sizeof(pending_flags));
    g_initialized = 0;
    pthread_mutex_unlock(&g_param_mutex);
    MLOG_INFO("param_manager deinitialized\n");
}

int param_manager_get(param_id_t id)
{
    int value = -1;

    pthread_mutex_lock(&g_param_mutex);
    if (!g_initialized || !is_valid_param_id(id)) {
        pthread_mutex_unlock(&g_param_mutex);
        MLOG_ERR("param_manager not initialized or invalid id: %d\n", id);
        return -1;
    }

    value = current_values[id];
    pthread_mutex_unlock(&g_param_mutex);
    return value;
}

int param_manager_set(param_id_t id, int value)
{
    int old_value = 0;

    pthread_mutex_lock(&g_param_mutex);
    if (!g_initialized || !is_valid_param_id(id)) {
        pthread_mutex_unlock(&g_param_mutex);
        MLOG_ERR("param_manager not initialized or invalid id: %d\n", id);
        return -1;
    }
    if (validate_param_value(id, value) != 0) {
        pthread_mutex_unlock(&g_param_mutex);
        MLOG_ERR("invalid value for param[%d]: %d\n", id, value);
        return -1;
    }

    old_value = current_values[id];
    if (old_value == value) {
        pthread_mutex_unlock(&g_param_mutex);
        return 0;
    }

    current_values[id] = value;
    pending_values[id] = value;
    pending_flags[id] = 1;
    pthread_mutex_unlock(&g_param_mutex);
    MLOG_DBG("param[%d]: %d -> %d\n", id, old_value, value);

    return 0;
}

int param_manager_get_default(param_id_t id)
{
    if (!is_valid_param_id(id)) {
        MLOG_ERR("invalid param id: %d\n", id);
        return -1;
    }

    return default_values[id];
}

void param_manager_reset_all(void)
{
    int i = 0;

    pthread_mutex_lock(&g_param_mutex);
    if (!g_initialized) {
        pthread_mutex_unlock(&g_param_mutex);
        return;
    }

    for (i = 0; i < PARAM_ID_BUTT; i++) {
        int old_value = current_values[i];
        current_values[i] = default_values[i];

        if (old_value != default_values[i]) {
            pending_values[i] = default_values[i];
            pending_flags[i] = 1;
        }
    }
    pthread_mutex_unlock(&g_param_mutex);

    MLOG_INFO("param_manager reset to defaults\n");
}

int param_manager_factory_reset(void)
{
    MLOG_INFO("param_manager factory reset start (mock)\n");
    usleep(1000 * 1000);
    param_manager_reset_all();
    MLOG_INFO("param_manager factory reset done (mock)\n");
    return 0;
}

int param_manager_register_callback(param_change_callback_t callback, void* user_data)
{
    int i = 0;

    if (!callback) {
        MLOG_ERR("callback is NULL\n");
        return -1;
    }

    pthread_mutex_lock(&g_param_mutex);
    for (i = 0; i < MAX_CALLBACKS; i++) {
        if (!callbacks[i].valid) {
            callbacks[i].callback = callback;
            callbacks[i].user_data = user_data;
            callbacks[i].valid = 1;
            pthread_mutex_unlock(&g_param_mutex);
            MLOG_DBG("registered callback at slot %d\n", i);
            return 0;
        }
    }
    pthread_mutex_unlock(&g_param_mutex);

    MLOG_ERR("no free callback slot\n");
    return -1;
}

void param_manager_unregister_callback(param_change_callback_t callback)
{
    int i = 0;

    if (!callback) {
        return;
    }

    pthread_mutex_lock(&g_param_mutex);
    for (i = 0; i < MAX_CALLBACKS; i++) {
        if (callbacks[i].valid && callbacks[i].callback == callback) {
            callbacks[i].valid = 0;
            callbacks[i].callback = NULL;
            callbacks[i].user_data = NULL;
            pthread_mutex_unlock(&g_param_mutex);
            MLOG_DBG("unregistered callback at slot %d\n", i);
            return;
        }
    }
    pthread_mutex_unlock(&g_param_mutex);
}

void param_manager_poll(void)
{
    callback_entry_t cb_snapshot[MAX_CALLBACKS];
    int changed_ids[PARAM_ID_BUTT];
    int changed_values[PARAM_ID_BUTT];
    int changed_cnt = 0;
    int i = 0;
    int j = 0;

    pthread_mutex_lock(&g_param_mutex);
    if (!g_initialized) {
        pthread_mutex_unlock(&g_param_mutex);
        return;
    }

    memcpy(cb_snapshot, callbacks, sizeof(cb_snapshot));
    for (i = 0; i < PARAM_ID_BUTT; i++) {
        if (pending_flags[i]) {
            changed_ids[changed_cnt] = i;
            changed_values[changed_cnt] = pending_values[i];
            changed_cnt++;
            pending_flags[i] = 0;
        }
    }
    pthread_mutex_unlock(&g_param_mutex);

    for (i = 0; i < changed_cnt; i++) {
        for (j = 0; j < MAX_CALLBACKS; j++) {
            if (cb_snapshot[j].valid && cb_snapshot[j].callback != NULL) {
                cb_snapshot[j].callback((param_id_t)changed_ids[i], changed_values[i], cb_snapshot[j].user_data);
            }
        }
    }
}
