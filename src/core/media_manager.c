#include "core/media_manager.h"
#include "core/key_manager.h"
#include "core/message_manager.h"
#include "core/param_manager.h"
#include "mlog.h"
#include "mode.h"
#include "param.h"

#define MEDIA_MANAGER_TAKE_PHOTO_TIMEOUT_MS 2000U

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
        MLOG_ERR("media_manager %s失败: value=%d ret=%d", what, value, ret);
        return MEDIA_MANAGER_ESTATE;
    }
    return MEDIA_MANAGER_OK;
}

static int handle_switch_to_photo_mode(int32_t args)
{
    (void)args;
    MESSAGE_S msg = { 0 };
    int32_t ret = 0;

    msg.topic = EVENT_MODEMNG_MODESWITCH;
    msg.arg1 = WORK_MODE_PHOTO;
    ret = message_manager_send_async(&msg, NULL);
    if (ret != 0) {
        MLOG_ERR("media_manager 切换到拍照模式失败: ret=%d", (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("media_manager 已请求切换到拍照模式");
    return MEDIA_MANAGER_OK;
}

static int handle_switch_to_boot_mode(int32_t args)
{
    (void)args;
    MESSAGE_S msg = { 0 };
    int32_t ret = 0;

    msg.topic = EVENT_MODEMNG_MODESWITCH;
    msg.arg1 = WORK_MODE_BOOT;
    ret = message_manager_send_async(&msg, NULL);
    if (ret != 0) {
        MLOG_ERR("media_manager 切换到boot模式失败: ret=%d", (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("media_manager 已请求切换到boot模式");
    return MEDIA_MANAGER_OK;
}

static int handle_switch_to_video_mode(int32_t args)
{
    (void)args;
    MESSAGE_S msg = { 0 };
    int32_t ret = 0;

    msg.topic = EVENT_MODEMNG_MODESWITCH;
    msg.arg1 = WORK_MODE_MOVIE;
    ret = message_manager_send_async(&msg, NULL);
    if (ret != 0) {
        MLOG_ERR("media_manager 切换到录像模式失败: ret=%d", (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("media_manager 已请求切换到录像模式");
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
        MLOG_ERR("media_manager 拍照失败: ret=%d", (int)ret);
        return MEDIA_MANAGER_ESTATE;
    }

    MLOG_INFO("media_manager 已触发拍照");
    return MEDIA_MANAGER_OK;
}

static int handle_focus(int32_t args)
{
    (void)args;
    MLOG_INFO("media_manager 对焦(占位实现)");
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
        MLOG_ERR("media_manager 获取当前音量失败: ret=%d", current_volume);
        return MEDIA_MANAGER_ESTATE;
    }
    target_volume = media_manager_clamp_volume(current_volume + (int)args);
    ret = media_manager_set_param_checked(PARAM_ID_VOLUME, target_volume, "调整系统音量");
    if (ret != MEDIA_MANAGER_OK) {
        MLOG_ERR("media_manager 调整音量失败: current=%d delta=%d target=%d",
            current_volume, (int)args, target_volume);
    }
    return ret;
}

static int handle_format_storage(int32_t args)
{
    (void)args;
    MLOG_INFO("media_manager 格式化存储(占位实现)");
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

    ret = media_manager_set_param_checked(PARAM_ID_RESOLUTION, value, "设置拍照分辨率");
    if (ret != MEDIA_MANAGER_OK) {
        return ret;
    }

    msg.topic = EVENT_MODEMNG_SETTING;
    msg.arg1 = PARAM_MENU_PHOTO_SIZE;
    msg.arg2 = (uint32_t)value;
    ret = message_manager_send_async(&msg, NULL);
    if (ret != MEDIA_MANAGER_OK) {
        MLOG_ERR("media_manager 设置拍照分辨率消息发送失败: value=%d ret=%d", value, ret);
        return ret;
    }

    MLOG_INFO("media_manager 已设置拍照分辨率: index=%d", value);
    return MEDIA_MANAGER_OK;
}

static int handle_set_white_balance(int32_t args)
{
    return media_manager_set_param_checked(PARAM_ID_WHITE_BALANCE, (int)args, "设置白平衡");
}

static int handle_set_iso(int32_t args)
{
    return media_manager_set_param_checked(PARAM_ID_ISO, (int)args, "设置感光度");
}

static int handle_set_exposure(int32_t args)
{
    return media_manager_set_param_checked(PARAM_ID_EXPOSURE, (int)args, "设置曝光");
}

static int handle_set_quality(int32_t args)
{
    return media_manager_set_param_checked(PARAM_ID_QUALITY, (int)args, "设置画质");
}

static int handle_set_face_detection(int32_t args)
{
    return media_manager_set_param_checked(PARAM_ID_FACE_DETECTION, args ? 1 : 0, "设置人脸检测");
}

static int handle_set_smile_capture(int32_t args)
{
    return media_manager_set_param_checked(PARAM_ID_SMILE_CAPTURE, args ? 1 : 0, "设置笑脸抓拍");
}

static int handle_set_video_resolution(int32_t args)
{
    int value = (int)args;
    int ret = MEDIA_MANAGER_OK;
    MESSAGE_S msg = { 0 };

    ret = media_manager_set_param_checked(PARAM_ID_VIDEO_RESOLUTION, value, "设置录像分辨率");
    if (ret != MEDIA_MANAGER_OK) {
        return ret;
    }

    msg.topic = EVENT_MODEMNG_SETTING;
    msg.arg1 = PARAM_MENU_VIDEO_SIZE;
    msg.arg2 = (uint32_t)value;
    ret = message_manager_send_async(&msg, NULL);
    if (ret != 0) {
        MLOG_ERR("media_manager 设置录像分辨率消息发送失败: value=%d ret=%d", value, ret);
        return MEDIA_MANAGER_ESTATE;
    }
    MLOG_INFO("media_manager 已设置录像分辨率: index=%d", value);
    return MEDIA_MANAGER_OK;
}

static const media_op_handler_t g_media_handlers[MEDIA_OP_BUTT] = {
    [MEDIA_OP_SWITCH_TO_PHOTO_MODE] = handle_switch_to_photo_mode,
    [MEDIA_OP_SWITCH_TO_BOOT_MODE] = handle_switch_to_boot_mode,
    [MEDIA_OP_SWITCH_TO_VIDEO_MODE] = handle_switch_to_video_mode,
    [MEDIA_OP_TAKE_PHOTO] = handle_take_photo,
    [MEDIA_OP_FOCUS] = handle_focus,
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
};

int media_manager_execute(media_operation_t op, int32_t args)
{
    media_op_handler_t handler = NULL;

    if (op < 0 || op >= MEDIA_OP_BUTT) {
        MLOG_WARN("media_manager 非法操作: %d", op);
        return MEDIA_MANAGER_EINVAL;
    }

    handler = g_media_handlers[op];
    if (handler == NULL) {
        MLOG_WARN("media_manager 不支持的操作: %d", op);
        return MEDIA_MANAGER_EUNSUP;
    }

    return handler(args);
}
