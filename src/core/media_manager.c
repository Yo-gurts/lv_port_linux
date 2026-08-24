#include "core/media_manager.h"
#include "core/file_manager.h"
#include "core/key_manager.h"
#include "core/message_manager.h"
#include "core/param_manager.h"
#include "lvgl.h"
#include "mlog.h"
#include "mode.h"
#include "param.h"
#include "ui/top_notice.h"
#include <stdio.h>

#define MEDIA_MANAGER_TAKE_PHOTO_TIMEOUT_MS 5000U
#define MEDIA_MANAGER_MODE_SWITCH_TIMEOUT_MS 3000U
#define MEDIA_MANAGER_SETTING_TIMEOUT_MS 3000U
#define MEDIA_MANAGER_FORMAT_TIMEOUT_MS 20000U

typedef int (*media_op_handler_t)(int32_t args);
static int media_manager_set_filter_impl(int ui_index, const char* isp_bin_path);

static int media_manager_clamp_volume(int value)
{
    if (value < 0) {
        return 0;
    }
    if (value > 100) {
        return 100;
    }
    return value;
}

static int media_manager_set_param_checked(param_id_t id, int value, const char* what)
{
    int ret = param_manager_set(id, value);
    if (ret != 0) {
        MLOG_ERR("%s失败: value=%d ret=%d", what, value, ret);
        return MEDIA_MANAGER_ESTATE;
    }
    return MEDIA_MANAGER_OK;
}

static int media_manager_can_format_storage(void)
{
    switch (MODEMNG_GetCardState()) {
    case CARD_STATE_AVAILABLE:
    case CARD_STATE_FORMATED:
    case CARD_STATE_FULL_SPACE:
        return 1;
    default:
        return 0;
    }
}

static int handle_switch_to_photo_mode(int32_t args)
{
    (void)args;
    MESSAGE_S msg = { 0 };
    int32_t ret = 0;
    uint8_t blocked_prev = 0;

    msg.topic = EVENT_MODEMNG_MODESWITCH;
    msg.arg1 = WORK_MODE_PHOTO;
    blocked_prev = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(blocked_prev | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    ret = message_manager_send_sync_timeout(&msg, MEDIA_MANAGER_MODE_SWITCH_TIMEOUT_MS);
    key_manager_set_block_non_power(blocked_prev);
    if (ret != 0) {
        MLOG_ERR("切换到拍照模式失败: ret=%d", (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已请求切换到拍照模式");
    return MEDIA_MANAGER_OK;
}

static int handle_switch_to_boot_mode(int32_t args)
{
    (void)args;
    MESSAGE_S msg = { 0 };
    int32_t ret = 0;
    uint8_t blocked_prev = 0;

    msg.topic = EVENT_MODEMNG_MODESWITCH;
    msg.arg1 = WORK_MODE_BOOT;
    blocked_prev = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(blocked_prev | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    ret = message_manager_send_sync_timeout(&msg, MEDIA_MANAGER_MODE_SWITCH_TIMEOUT_MS);
    key_manager_set_block_non_power(blocked_prev);
    if (ret != 0) {
        MLOG_ERR("切换到boot模式失败: ret=%d", (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已请求切换到boot模式");
    return MEDIA_MANAGER_OK;
}

static int handle_switch_to_video_mode(int32_t args)
{
    (void)args;
    MESSAGE_S msg = { 0 };
    int32_t ret = 0;
    uint8_t blocked_prev = 0;

    msg.topic = EVENT_MODEMNG_MODESWITCH;
    msg.arg1 = WORK_MODE_MOVIE;
    blocked_prev = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(blocked_prev | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    ret = message_manager_send_sync_timeout(&msg, MEDIA_MANAGER_MODE_SWITCH_TIMEOUT_MS);
    key_manager_set_block_non_power(blocked_prev);
    if (ret != 0) {
        MLOG_ERR("切换到录像模式失败: ret=%d", (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已请求切换到录像模式");
    return MEDIA_MANAGER_OK;
}

static int handle_switch_to_playback_mode(int32_t args)
{
    (void)args;
    MESSAGE_S msg = { 0 };
    int32_t ret = 0;
    uint8_t blocked_prev = 0;

    msg.topic = EVENT_MODEMNG_MODESWITCH;
    msg.arg1 = WORK_MODE_PLAYBACK;
    blocked_prev = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(blocked_prev | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    ret = message_manager_send_sync_timeout(&msg, MEDIA_MANAGER_MODE_SWITCH_TIMEOUT_MS);
    key_manager_set_block_non_power(blocked_prev);
    if (ret != 0) {
        MLOG_ERR("切换到回放模式失败: ret=%d", (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已请求切换到回放模式");
    return MEDIA_MANAGER_OK;
}

static int handle_start_record(int32_t args)
{
    MESSAGE_S msg = { 0 };
    int32_t ret = 0;

    (void)args;
    msg.topic = EVENT_MODEMNG_START_REC;
    ret = MODEMNG_SendMessage(&msg);
    if (ret != 0) {
        MLOG_ERR("开始录像消息发送失败: ret=%d", (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已请求开始录像");
    return MEDIA_MANAGER_OK;
}

static int handle_stop_record(int32_t args)
{
    MESSAGE_S msg = { 0 };
    int32_t ret = 0;

    (void)args;
    msg.topic = EVENT_MODEMNG_STOP_REC;
    ret = MODEMNG_SendMessage(&msg);
    if (ret != 0) {
        MLOG_ERR("停止录像消息发送失败: ret=%d", (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已请求停止录像");
    return MEDIA_MANAGER_OK;
}

static int handle_take_photo(int32_t args)
{
    MESSAGE_S msg = { 0 };
    int32_t ret = 0;
    uint8_t blocked_prev = 0;
    char latest_name[FILE_MANAGER_MAX_NAME_LEN] = { 0 };
    char notice_text[96] = { 0 };

    (void)args;

    if (param_manager_get(PARAM_ID_SD_READY) != SD_READY_TRUE) {
        top_notice_show_for("SD卡未就绪", TOP_NOTICE_TYPE_WARNING, 2000);
        MLOG_WARN("拍照失败: SD卡未就绪");
        return MEDIA_MANAGER_ESTATE;
    }

    msg.topic = EVENT_MODEMNG_START_PIV;
    blocked_prev = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(blocked_prev | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    ret = message_manager_send_sync_topics_timeout(
        &msg, EVENT_MODEMNG_PHOTO_INDEXED, EVENT_MODEMNG_PHOTO_INDEX_FAILED, MEDIA_MANAGER_TAKE_PHOTO_TIMEOUT_MS);
    key_manager_set_block_non_power(blocked_prev);
    if (ret != 0) {
        MLOG_ERR("拍照失败: ret=%d", (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }

    if (file_manager_get_photo_name(0, latest_name, sizeof(latest_name)) == 0 && latest_name[0] != '\0') {
        snprintf(notice_text, sizeof(notice_text), "已拍摄：%s", latest_name);
        top_notice_show_for(notice_text, TOP_NOTICE_TYPE_SUCCESS, 2000);
        MLOG_INFO("拍照成功，最新文件：%s", latest_name);
    }

    return MEDIA_MANAGER_OK;
}

static int handle_focus_once(int32_t args)
{
    MESSAGE_S msg = { 0 };
    int32_t ret = 0;

    (void)args;

    /* 对齐 dc309：对焦键触发时发送 EVENT_MODEMNG_FOCUS。 */
    msg.topic = EVENT_MODEMNG_FOCUS;
    msg.arg1 = MODEMNG_FOCUS_CMD_ONCE;
    ret = MODEMNG_SendMessage(&msg);
    if (ret != 0) {
        MLOG_ERR("对焦消息发送失败: ret=%d", (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }
    MLOG_INFO("已触发对焦");
    return MEDIA_MANAGER_OK;
}

static int handle_set_focus_enable(int32_t args)
{
    int enable = (int)args;
    MESSAGE_S msg = { 0 };
    int32_t ret = 0;

    if (enable != 0 && enable != 1) {
        MLOG_ERR("设置对焦使能参数非法: args=%d", enable);
        return MEDIA_MANAGER_EINVAL;
    }

    msg.topic = EVENT_MODEMNG_FOCUS;
    msg.arg1 = MODEMNG_FOCUS_CMD_SET_ENABLE;
    msg.arg2 = (uint32_t)enable;
    ret = MODEMNG_SendMessage(&msg);
    if (ret != 0) {
        MLOG_ERR("设置对焦使能消息发送失败: enable=%d ret=%d", enable, (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已请求设置对焦使能: enable=%d", enable);
    return MEDIA_MANAGER_OK;
}

static int handle_set_system_volume(int32_t args)
{
    int target_volume = media_manager_clamp_volume((int)args);
    return media_manager_set_param_checked(PARAM_ID_VOLUME, target_volume, "设置系统音量");
}

static int handle_adjust_system_volume(int32_t args)
{
    int current_volume = 0;
    int target_volume = 0;
    int ret = MEDIA_MANAGER_OK;

    current_volume = param_manager_get(PARAM_ID_VOLUME);
    if (current_volume < 0) {
        MLOG_ERR("获取当前音量失败: ret=%d", current_volume);
        return MEDIA_MANAGER_ESTATE;
    }
    target_volume = media_manager_clamp_volume(current_volume + (int)args);
    ret = media_manager_set_param_checked(PARAM_ID_VOLUME, target_volume, "调整系统音量");
    if (ret != MEDIA_MANAGER_OK) {
        MLOG_ERR("调整音量失败: current=%d delta=%d target=%d",
            current_volume, (int)args, target_volume);
    }
    return ret;
}

static int handle_format_storage(int32_t args)
{
    MESSAGE_S msg = { 0 };
    int32_t ret = 0;
    uint8_t blocked_prev = 0;

    (void)args;

    if (!media_manager_can_format_storage()) {
        MLOG_ERR("格式化存储失败: 当前存储状态不允许格式化 card_state=%u", MODEMNG_GetCardState());
        return MEDIA_MANAGER_ESTATE;
    }

    msg.topic = EVENT_MODEMNG_CARD_FORMAT;
    blocked_prev = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(blocked_prev | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    ret = message_manager_send_sync_topics_timeout(
        &msg, EVENT_MODEMNG_CARD_FORMAT_SUCCESSED, EVENT_MODEMNG_CARD_FORMAT_FAILED, MEDIA_MANAGER_FORMAT_TIMEOUT_MS);
    key_manager_set_block_non_power(blocked_prev);
    if (ret != 0) {
        MLOG_ERR("格式化存储失败: ret=%d", (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已完成存储格式化");
    return MEDIA_MANAGER_OK;
}

static int handle_factory_reset(int32_t args)
{
    (void)args;
    param_manager_reset_all();
    return MEDIA_MANAGER_OK;
}

static int handle_set_photo_resolution(int32_t args)
{
    int value = (int)args;
    int ret = MEDIA_MANAGER_OK;
    MESSAGE_S msg = { 0 };
    uint8_t blocked_prev = 0;

    ret = media_manager_set_param_checked(PARAM_ID_RESOLUTION, value, "设置拍照分辨率");
    if (ret != MEDIA_MANAGER_OK) {
        return ret;
    }

    msg.topic = EVENT_MODEMNG_SETTING;
    msg.arg1 = PARAM_MENU_PHOTO_SIZE;
    msg.arg2 = (uint32_t)value;
    blocked_prev = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(blocked_prev | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    ret = message_manager_send_sync_timeout(&msg, MEDIA_MANAGER_SETTING_TIMEOUT_MS);
    key_manager_set_block_non_power(blocked_prev);
    if (ret != 0) {
        MLOG_ERR("设置拍照分辨率消息发送失败: value=%d ret=%d", value, ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已设置拍照分辨率: index=%d", value);
    return MEDIA_MANAGER_OK;
}

static int handle_set_white_balance(int32_t args)
{
    int value = (int)args;
    int ret = MEDIA_MANAGER_OK;
    MESSAGE_S msg = { 0 };
    uint8_t blocked_prev = 0;

    ret = media_manager_set_param_checked(PARAM_ID_WHITE_BALANCE, value, "设置白平衡");
    if (ret != MEDIA_MANAGER_OK) {
        return ret;
    }

    msg.topic = EVENT_MODEMNG_SET_WHITE_BALANCE;
    msg.arg1 = (uint32_t)value;
    blocked_prev = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(blocked_prev | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    ret = message_manager_send_sync_timeout(&msg, MEDIA_MANAGER_SETTING_TIMEOUT_MS);
    key_manager_set_block_non_power(blocked_prev);
    if (ret != 0) {
        MLOG_ERR("设置白平衡消息发送失败: value=%d ret=%d", value, ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已设置白平衡: index=%d", value);
    return MEDIA_MANAGER_OK;
}

static int handle_set_iso(int32_t args)
{
    int value = (int)args;
    int ret = MEDIA_MANAGER_OK;
    MESSAGE_S msg = { 0 };
    uint8_t blocked_prev = 0;

    ret = media_manager_set_param_checked(PARAM_ID_ISO, value, "设置感光度");
    if (ret != MEDIA_MANAGER_OK) {
        return ret;
    }

    msg.topic = EVENT_MODEMNG_SET_ISO;
    msg.arg1 = (uint32_t)value;
    blocked_prev = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(blocked_prev | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    ret = message_manager_send_sync_timeout(&msg, MEDIA_MANAGER_SETTING_TIMEOUT_MS);
    key_manager_set_block_non_power(blocked_prev);
    if (ret != 0) {
        MLOG_ERR("设置感光度消息发送失败: value=%d ret=%d", value, ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已设置感光度: index=%d", value);
    return MEDIA_MANAGER_OK;
}

static int handle_set_exposure(int32_t args)
{
    int value = (int)args;
    int ret = MEDIA_MANAGER_OK;
    MESSAGE_S msg = { 0 };
    uint8_t blocked_prev = 0;

    ret = media_manager_set_param_checked(PARAM_ID_EXPOSURE, value, "设置曝光");
    if (ret != MEDIA_MANAGER_OK) {
        return ret;
    }

    msg.topic = EVENT_MODEMNG_SET_EXPOSURE;
    msg.arg1 = (uint32_t)value;
    blocked_prev = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(blocked_prev | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    ret = message_manager_send_sync_timeout(&msg, MEDIA_MANAGER_SETTING_TIMEOUT_MS);
    key_manager_set_block_non_power(blocked_prev);
    if (ret != 0) {
        MLOG_ERR("设置曝光消息发送失败: value=%d ret=%d", value, ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已设置曝光: index=%d", value);
    return MEDIA_MANAGER_OK;
}

static int handle_set_quality(int32_t args)
{
    int value = (int)args;
    int ret = MEDIA_MANAGER_OK;
    MESSAGE_S msg = { 0 };
    uint8_t blocked_prev = 0;

    ret = media_manager_set_param_checked(PARAM_ID_QUALITY, value, "设置画质");
    if (ret != MEDIA_MANAGER_OK) {
        return ret;
    }

    msg.topic = EVENT_MODEMNG_SETTING;
    msg.arg1 = PARAM_MENU_PHOTO_QUALITY;
    msg.arg2 = (uint32_t)value;
    blocked_prev = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(blocked_prev | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    ret = message_manager_send_sync_timeout(&msg, MEDIA_MANAGER_SETTING_TIMEOUT_MS);
    key_manager_set_block_non_power(blocked_prev);
    if (ret != 0) {
        MLOG_ERR("设置画质消息发送失败: value=%d ret=%d", value, ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已设置画质: index=%d", value);
    return MEDIA_MANAGER_OK;
}

static int handle_set_face_detection(int32_t args)
{
    int enable = args ? 1 : 0;
    int mode_value = enable; /* 统一语义: 1=开, 0=关 */
    int ret = MEDIA_MANAGER_OK;
    MESSAGE_S msg = { 0 };
    uint8_t blocked_prev = 0;

    ret = media_manager_set_param_checked(PARAM_ID_FACE_DETECTION, enable, "设置人脸检测");
    if (ret != MEDIA_MANAGER_OK) {
        return ret;
    }

    msg.topic = EVENT_MODEMNG_SETTING;
    msg.arg1 = PARAM_MENU_FACE_DET;
    msg.arg2 = (uint32_t)mode_value;
    blocked_prev = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(blocked_prev | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    ret = message_manager_send_sync_timeout(&msg, MEDIA_MANAGER_SETTING_TIMEOUT_MS);
    key_manager_set_block_non_power(blocked_prev);
    if (ret != 0) {
        MLOG_ERR("设置人脸检测消息发送失败: enable=%d ret=%d", enable, ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已设置人脸检测: enable=%d mode_value=%d", enable, mode_value);
    return MEDIA_MANAGER_OK;
}

static int handle_set_smile_capture(int32_t args)
{
    int enable = args ? 1 : 0;
    int mode_value = enable; /* 统一语义: 1=开, 0=关 */
    int ret = MEDIA_MANAGER_OK;
    MESSAGE_S msg = { 0 };
    uint8_t blocked_prev = 0;

    ret = media_manager_set_param_checked(PARAM_ID_SMILE_CAPTURE, enable, "设置笑脸抓拍");
    if (ret != MEDIA_MANAGER_OK) {
        return ret;
    }

    msg.topic = EVENT_MODEMNG_SETTING;
    msg.arg1 = PARAM_MENU_FACE_SMILE;
    msg.arg2 = (uint32_t)mode_value;
    blocked_prev = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(blocked_prev | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    ret = message_manager_send_sync_timeout(&msg, MEDIA_MANAGER_SETTING_TIMEOUT_MS);
    key_manager_set_block_non_power(blocked_prev);
    if (ret != 0) {
        MLOG_ERR("设置笑脸抓拍消息发送失败: enable=%d ret=%d", enable, ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已设置笑脸抓拍: enable=%d mode_value=%d", enable, mode_value);
    return MEDIA_MANAGER_OK;
}

static int handle_set_video_resolution(int32_t args)
{
    int value = (int)args;
    int ret = MEDIA_MANAGER_OK;
    MESSAGE_S msg = { 0 };
    uint8_t blocked_prev = 0;

    ret = media_manager_set_param_checked(PARAM_ID_VIDEO_RESOLUTION, value, "设置录像分辨率");
    if (ret != MEDIA_MANAGER_OK) {
        return ret;
    }

    msg.topic = EVENT_MODEMNG_SETTING;
    msg.arg1 = PARAM_MENU_VIDEO_SIZE;
    msg.arg2 = (uint32_t)value;
    blocked_prev = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(blocked_prev | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    ret = message_manager_send_sync_timeout(&msg, MEDIA_MANAGER_SETTING_TIMEOUT_MS);
    key_manager_set_block_non_power(blocked_prev);
    if (ret != 0) {
        MLOG_ERR("设置录像分辨率消息发送失败: value=%d ret=%d", value, ret);
        return MEDIA_MANAGER_ESTATE;
    }
    MLOG_INFO("已设置录像分辨率: index=%d", value);
    return MEDIA_MANAGER_OK;
}

static int handle_set_zoom(int32_t args)
{
    int zoom = (int)args;
    int ret = MEDIA_MANAGER_OK;
    MESSAGE_S msg = { 0 };

    ret = media_manager_set_param_checked(PARAM_ID_ZOOM, zoom, "设置变焦倍率");
    if (ret != MEDIA_MANAGER_OK) {
        return ret;
    }

    msg.topic = EVENT_MODEMNG_LIVEVIEW_ADJUSTFOCUS;
    msg.arg1 = 0;
    snprintf((char*)msg.aszPayload, sizeof(msg.aszPayload), "%d", zoom);
    ret = MODEMNG_SendMessage(&msg);
    if (ret != 0) {
        MLOG_ERR("设置变焦消息发送失败: zoom=%d ret=%d", zoom, (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已请求设置变焦: zoom=%d", zoom);
    return MEDIA_MANAGER_OK;
}

static int handle_set_filter(int32_t args)
{
    return media_manager_set_filter_impl((int)args, NULL);
}

static const media_op_handler_t g_media_handlers[MEDIA_OP_BUTT] = {
    [MEDIA_OP_SWITCH_TO_PHOTO_MODE] = handle_switch_to_photo_mode,
    [MEDIA_OP_SWITCH_TO_BOOT_MODE] = handle_switch_to_boot_mode,
    [MEDIA_OP_SWITCH_TO_VIDEO_MODE] = handle_switch_to_video_mode,
    [MEDIA_OP_SWITCH_TO_PLAYBACK_MODE] = handle_switch_to_playback_mode,
    [MEDIA_OP_START_RECORD] = handle_start_record,
    [MEDIA_OP_STOP_RECORD] = handle_stop_record,
    [MEDIA_OP_TAKE_PHOTO] = handle_take_photo,
    [MEDIA_OP_FOCUS_ONCE] = handle_focus_once,
    [MEDIA_OP_SET_FOCUS_ENABLE] = handle_set_focus_enable,
    [MEDIA_OP_SET_SYSTEM_VOLUME] = handle_set_system_volume,
    [MEDIA_OP_ADJUST_SYSTEM_VOLUME] = handle_adjust_system_volume,
    [MEDIA_OP_FORMAT_STORAGE] = handle_format_storage,
    [MEDIA_OP_FACTORY_RESET] = handle_factory_reset,
    [MEDIA_OP_SET_PHOTO_RESOLUTION] = handle_set_photo_resolution,
    [MEDIA_OP_SET_WHITE_BALANCE] = handle_set_white_balance,
    [MEDIA_OP_SET_ISO] = handle_set_iso,
    [MEDIA_OP_SET_EXPOSURE] = handle_set_exposure,
    [MEDIA_OP_SET_QUALITY] = handle_set_quality,
    [MEDIA_OP_SET_FACE_DETECTION] = handle_set_face_detection,
    [MEDIA_OP_SET_SMILE_CAPTURE] = handle_set_smile_capture,
    [MEDIA_OP_SET_VIDEO_RESOLUTION] = handle_set_video_resolution,
    [MEDIA_OP_SET_ZOOM] = handle_set_zoom,
    [MEDIA_OP_SET_FILTER] = handle_set_filter,
};

int media_manager_execute(media_operation_t op, int32_t args)
{
    media_op_handler_t handler = NULL;

    if (op < 0 || op >= MEDIA_OP_BUTT) {
        MLOG_WARN("非法操作: %d", op);
        return MEDIA_MANAGER_EINVAL;
    }

    handler = g_media_handlers[op];
    if (handler == NULL) {
        MLOG_WARN("不支持的操作: %d", op);
        return MEDIA_MANAGER_EUNSUP;
    }

    return handler(args);
}

/* 异步模式切换：把「EventHub 线程回包 -> UI 线程回调」的跨线程 hop 收在本层，
 * 对上层只暴露模式无关的 media_switch_done_cb_t。msg_processed 门保证同一时刻仅一个
 * 在途请求，故单槽 static 存 done_cb/result 足够。 */
static media_switch_done_cb_t g_switch_done_cb = NULL;
static volatile int g_switch_result = 0;

/* UI 线程：由 lv_async 调度，把结果交给上层回调。 */
static void mm_switch_done_on_ui(void* param)
{
    media_switch_done_cb_t cb = g_switch_done_cb;

    (void)param;
    g_switch_done_cb = NULL;
    if (cb != NULL) {
        cb((int)g_switch_result);
    }
}

/* EventHub 线程：回包到达，记录结果并 hop 到 UI 线程（仿 message_manager 既有做法）。 */
static int32_t mm_switch_result_cb(EVENT_S* evt)
{
    g_switch_result = (evt != NULL) ? evt->s32Result : MEDIA_MANAGER_ESTATE;
    (void)lv_async_call(mm_switch_done_on_ui, NULL);
    return g_switch_result;
}

/* 发起一次异步模式切换：组 msg 后 send_async，回包走 mm_switch_result_cb。
 * 不屏蔽 TP（异步切换正是为了 UI 全程可响应）。 */
static int media_manager_send_switch_async(int32_t work_mode)
{
    MESSAGE_S msg = { 0 };
    int32_t ret = 0;

    msg.topic = EVENT_MODEMNG_MODESWITCH;
    msg.arg1 = work_mode;
    ret = message_manager_send_async(&msg, mm_switch_result_cb);
    if (ret != 0) {
        MLOG_ERR("异步切换发送失败: work_mode=%d ret=%d", (int)work_mode, (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }
    return MEDIA_MANAGER_OK;
}

/* 异步 handler：与同步 handle_switch_to_*_mode 并行，op->WORK_MODE 映射同样收在各自 handler 里。 */
typedef int (*media_async_handler_t)(void);

static int handle_switch_to_photo_mode_async(void)
{
    return media_manager_send_switch_async(WORK_MODE_PHOTO);
}

static int handle_switch_to_boot_mode_async(void)
{
    return media_manager_send_switch_async(WORK_MODE_BOOT);
}

static const media_async_handler_t g_media_async_handlers[MEDIA_OP_BUTT] = {
    [MEDIA_OP_SWITCH_TO_PHOTO_MODE] = handle_switch_to_photo_mode_async,
    [MEDIA_OP_SWITCH_TO_BOOT_MODE] = handle_switch_to_boot_mode_async,
};

int media_manager_execute_async(media_operation_t op, media_switch_done_cb_t done_cb)
{
    media_async_handler_t handler = NULL;
    int ret = 0;

    if (op < 0 || op >= MEDIA_OP_BUTT) {
        MLOG_WARN("异步切换非法操作: %d", op);
        return MEDIA_MANAGER_EINVAL;
    }

    handler = g_media_async_handlers[op];
    if (handler == NULL) {
        MLOG_WARN("异步切换不支持的操作: %d", op);
        return MEDIA_MANAGER_EUNSUP;
    }

    g_switch_done_cb = done_cb;
    ret = handler();
    if (ret != MEDIA_MANAGER_OK) {
        g_switch_done_cb = NULL; /* 发起失败：清回单槽，避免残留 cb 被下次误触发 */
        return ret;
    }

    MLOG_INFO("已异步请求切换: op=%d", op);
    return MEDIA_MANAGER_OK;
}

int media_manager_get_current_work_mode(void)
{
    return MODEMNG_GetCurWorkMode();
}

int media_manager_is_playback_work_mode(int work_mode)
{
    return work_mode == WORK_MODE_PLAYBACK ? 1 : 0;
}

int media_manager_restore_work_mode(int work_mode)
{
    switch (work_mode) {
    case WORK_MODE_BOOT:
        return media_manager_execute(MEDIA_OP_SWITCH_TO_BOOT_MODE, 0);
    case WORK_MODE_PHOTO:
        return media_manager_execute(MEDIA_OP_SWITCH_TO_PHOTO_MODE, 0);
    case WORK_MODE_MOVIE:
        return media_manager_execute(MEDIA_OP_SWITCH_TO_VIDEO_MODE, 0);
    case WORK_MODE_PLAYBACK:
    case WORK_MODE_BUTT:
        return MEDIA_MANAGER_OK;
    default:
        MLOG_WARN("不支持恢复到该模式: %d", work_mode);
        return MEDIA_MANAGER_EUNSUP;
    }
}

static int media_manager_set_filter_impl(int ui_index, const char* isp_bin_path)
{
    int ret = MEDIA_MANAGER_OK;
    MESSAGE_S msg = { 0 };
    uint8_t blocked_prev = 0;

    ret = media_manager_set_param_checked(PARAM_ID_FILTER_INDEX, ui_index, "设置滤镜索引");
    if (ret != MEDIA_MANAGER_OK) {
        return ret;
    }

    msg.topic = EVENT_MODEMNG_SETTING;
    msg.arg1 = (MODEMNG_GetCurWorkMode() == WORK_MODE_MOVIE) ? PARAM_MENU_VIDEO_EFFECT : PARAM_MENU_PHOTO_EFFECT;
    msg.arg2 = (uint32_t)ui_index;
    if (isp_bin_path != NULL) {
        snprintf((char*)msg.aszPayload, sizeof(msg.aszPayload), "%s", isp_bin_path);
    }

    blocked_prev = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(blocked_prev | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    ret = message_manager_send_sync_timeout(&msg, MEDIA_MANAGER_SETTING_TIMEOUT_MS);
    key_manager_set_block_non_power(blocked_prev);
    if (ret != 0) {
        MLOG_ERR("设置滤镜消息发送失败: index=%d path=%s ret=%d",
            ui_index, (isp_bin_path == NULL) ? "(null)" : isp_bin_path, ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已设置滤镜: index=%d path=%s",
        ui_index, (isp_bin_path == NULL) ? "(null)" : isp_bin_path);
    return MEDIA_MANAGER_OK;
}

int media_manager_set_filter_with_path(int ui_index, const char* isp_bin_path)
{
    return media_manager_set_filter_impl(ui_index, isp_bin_path);
}
