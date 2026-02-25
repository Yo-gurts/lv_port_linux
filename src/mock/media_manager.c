#include "core/media_manager.h"
#include "core/param_manager.h"
#include "mlog.h"

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
    MLOG_INFO("media_manager 切换拍照模式(占位实现)");
    return MEDIA_MANAGER_OK;
}

static int handle_switch_to_boot_mode(int32_t args)
{
    (void)args;
    MLOG_INFO("media_manager 切换boot模式(占位实现)");
    return MEDIA_MANAGER_OK;
}

static int handle_switch_to_video_mode(int32_t args)
{
    (void)args;
    MLOG_INFO("media_manager 切换录像模式(占位实现)");
    return MEDIA_MANAGER_OK;
}

static int handle_take_photo(int32_t args)
{
    (void)args;
    MLOG_INFO("media_manager 拍照(占位实现)");
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
    return media_manager_set_param_checked(PARAM_ID_RESOLUTION, (int)args, "设置拍照分辨率");
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
    return media_manager_set_param_checked(PARAM_ID_VIDEO_RESOLUTION, (int)args, "设置录像分辨率");
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
