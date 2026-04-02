#include "core/media_manager.h"
#include "core/key_manager.h"
#include "core/message_manager.h"
#include "core/param_manager.h"
#include "mlog.h"
#include "mode.h"
#include "param.h"
#include <stdio.h>

#define MEDIA_MANAGER_TAKE_PHOTO_TIMEOUT_MS 2000U
#define MEDIA_MANAGER_MODE_SWITCH_TIMEOUT_MS 3000U
#define MEDIA_MANAGER_SETTING_TIMEOUT_MS 3000U

typedef int (*media_op_handler_t)(int32_t args);

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

    (void)args;

    msg.topic = EVENT_MODEMNG_START_PIV;
    blocked_prev = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(blocked_prev | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    ret = message_manager_send_sync_timeout(&msg, MEDIA_MANAGER_TAKE_PHOTO_TIMEOUT_MS);
    key_manager_set_block_non_power(blocked_prev);
    if (ret != 0) {
        MLOG_ERR("拍照失败: ret=%d", (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("已触发拍照");
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
    (void)args;
    MLOG_INFO("格式化存储(占位实现)");
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
