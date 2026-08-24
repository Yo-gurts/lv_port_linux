#ifndef __MEDIA_MANAGER_H__
#define __MEDIA_MANAGER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MEDIA_OP_SWITCH_TO_PHOTO_MODE = 0, /* 切换到拍照模式；args 忽略 */
    MEDIA_OP_SWITCH_TO_BOOT_MODE, /* 切换到启动/首页模式；args 忽略 */
    MEDIA_OP_SWITCH_TO_VIDEO_MODE, /* 切换到录像模式；args 忽略 */
    MEDIA_OP_SWITCH_TO_PLAYBACK_MODE, /* 切换到回放模式；args 忽略 */
    MEDIA_OP_START_RECORD, /* 开始录像；args 忽略 */
    MEDIA_OP_STOP_RECORD, /* 停止录像；args 忽略 */
    MEDIA_OP_TAKE_PHOTO, /* 触发一次拍照；args 忽略 */
    MEDIA_OP_FOCUS_ONCE, /* 触发一次对焦；args 忽略 */
    MEDIA_OP_SET_FOCUS_ENABLE, /* 设置 AF 使能；args: 0=disable, 1=enable */
    MEDIA_OP_SET_SYSTEM_VOLUME, /* 设置系统音量；args: 0~100 */
    MEDIA_OP_ADJUST_SYSTEM_VOLUME, /* 调整系统音量增量；args: 可正可负 */
    MEDIA_OP_FORMAT_STORAGE, /* 格式化存储介质；args 忽略 */
    MEDIA_OP_FACTORY_RESET, /* 恢复出厂设置；args 忽略 */
    MEDIA_OP_SET_PHOTO_RESOLUTION, /* 设置拍照分辨率；args: photo_resolution_t */
    MEDIA_OP_SET_WHITE_BALANCE, /* 设置白平衡；args: white_balance_t */
    MEDIA_OP_SET_ISO, /* 设置感光度；args: iso_t */
    MEDIA_OP_SET_EXPOSURE, /* 设置曝光档位；args: exposure_t */
    MEDIA_OP_SET_QUALITY, /* 设置画质；args: quality_t */
    MEDIA_OP_SET_FACE_DETECTION, /* 设置人脸检测开关；args: 0/1 */
    MEDIA_OP_SET_SMILE_CAPTURE, /* 设置笑脸抓拍开关；args: 0/1 */
    MEDIA_OP_SET_VIDEO_RESOLUTION, /* 设置录像分辨率；args: video_resolution_t */
    MEDIA_OP_SET_ZOOM, /* 设置变焦倍率；args: 1/2/3/6 */
    MEDIA_OP_SET_FILTER, /* 设置滤镜下标（兼容旧接口）；args: ui_index */
    MEDIA_OP_BUTT /* 枚举边界 */
} media_operation_t;

#define MEDIA_MANAGER_OK 0
#define MEDIA_MANAGER_EINVAL (-1)
#define MEDIA_MANAGER_ERANGE (-2)
#define MEDIA_MANAGER_ESTATE (-3)
#define MEDIA_MANAGER_EUNSUP (-4)

/* 异步模式切换完成回调；result==0 成功，非0失败。运行在 UI 线程（media 层已做跨线程 hop）。 */
typedef void (*media_switch_done_cb_t)(int result);

int media_manager_execute(media_operation_t op, int32_t args);
/* 异步模式切换：仅支持 SWITCH_TO_PHOTO_MODE / SWITCH_TO_BOOT_MODE，发起后立即返回，
 * 切换完成时在 UI 线程回调 done_cb。同一时刻仅允许一个在途请求，忙时返回 EBUSY。 */
int media_manager_execute_async(media_operation_t op, media_switch_done_cb_t done_cb);
int media_manager_get_current_work_mode(void);
int media_manager_is_playback_work_mode(int work_mode);
int media_manager_restore_work_mode(int work_mode);
/* UI侧配置滤镜名称/icon，仅向底层透传 isp bin 路径。 */
int media_manager_set_filter_with_path(int ui_index, const char* isp_bin_path);

#ifdef __cplusplus
}
#endif

#endif /* __MEDIA_MANAGER_H__ */
