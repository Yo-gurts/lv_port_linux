#include "core/media_manager.h"
#include "core/param_manager.h"
#include "mlog.h"

typedef int (*media_op_handler_t)(int32_t args);

enum {
    MOCK_WORK_MODE_BOOT = 0,
    MOCK_WORK_MODE_PHOTO = 1,
    MOCK_WORK_MODE_VIDEO = 2,
    MOCK_WORK_MODE_PLAYBACK = 3,
};

static int g_mock_work_mode = MOCK_WORK_MODE_BOOT;

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
    g_mock_work_mode = MOCK_WORK_MODE_PHOTO;
    MLOG_INFO("media_manager 切换拍照模式(占位实现)");
    return MEDIA_MANAGER_OK;
}

static int handle_switch_to_boot_mode(int32_t args)
{
    (void)args;
    g_mock_work_mode = MOCK_WORK_MODE_BOOT;
    MLOG_INFO("media_manager 切换boot模式(占位实现)");
    return MEDIA_MANAGER_OK;
}

static int handle_switch_to_video_mode(int32_t args)
{
    (void)args;
    g_mock_work_mode = MOCK_WORK_MODE_VIDEO;
    MLOG_INFO("media_manager 切换录像模式(占位实现)");
    return MEDIA_MANAGER_OK;
}

static int handle_switch_to_playback_mode(int32_t args)
{
    (void)args;
    g_mock_work_mode = MOCK_WORK_MODE_PLAYBACK;
    MLOG_INFO("media_manager 切换回放模式(占位实现)");
    return MEDIA_MANAGER_OK;
}

static int handle_start_record(int32_t args)
{
    (void)args;
    MLOG_INFO("media_manager 开始录像(占位实现)");
    return MEDIA_MANAGER_OK;
}

static int handle_stop_record(int32_t args)
{
    (void)args;
    MLOG_INFO("media_manager 停止录像(占位实现)");
    return MEDIA_MANAGER_OK;
}

static int handle_take_photo(int32_t args)
{
    (void)args;
    MLOG_INFO("media_manager 拍照(占位实现)");
    return MEDIA_MANAGER_OK;
}

static int handle_focus_once(int32_t args)
{
    (void)args;
    MLOG_INFO("media_manager 对焦(占位实现)");
    return MEDIA_MANAGER_OK;
}

static int handle_set_focus_enable(int32_t args)
{
    MLOG_INFO("media_manager 设置对焦使能(占位实现): enable=%d", (int)args);
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
    int ret = media_manager_set_param_checked(PARAM_ID_EXPOSURE, (int)args, "设置曝光");
    if (ret != MEDIA_MANAGER_OK) {
        return ret;
    }
    MLOG_INFO("media_manager 设置曝光: level=%d", (int)args);
    return MEDIA_MANAGER_OK;
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

static int handle_set_zoom(int32_t args)
{
    MLOG_INFO("media_manager 设置变焦(占位实现): zoom=%d", (int)args);
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

int media_manager_get_current_work_mode(void)
{
    return g_mock_work_mode;
}

int media_manager_is_playback_work_mode(int work_mode)
{
    return work_mode == MOCK_WORK_MODE_PLAYBACK ? 1 : 0;
}

int media_manager_restore_work_mode(int work_mode)
{
    g_mock_work_mode = work_mode;
    MLOG_INFO("media_manager 恢复模式(占位实现): mode=%d", work_mode);
    return MEDIA_MANAGER_OK;
}
