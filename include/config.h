#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* =======================
 * 资源路径配置
 * ======================= */
#ifndef PROJECT_PATH
#define PROJECT_PATH "/app"
#endif

#define RES_FONT_PATH PROJECT_PATH "/res/fonts"
#define RES_ICON_PATH PROJECT_PATH "/res/icons"
#define RES_ISP_PATH PROJECT_PATH "/res/isp"

/* =======================
 * 相册路径配置
 * ======================= */

#define VIDEO_ALBUM_VIDEO_PATH "A:/mnt/sd/DCIM/MOVIE/"
#define PHOTO_ALBUM_IMAGE_PATH "A:/mnt/sd/DCIM/PHOTO/"
#define PHOTO_ALBUM_IMAGE_THUMB_PATH "A:/mnt/sd/.thumb/photo_small/"
#define VIDEO_ALBUM_VIDEO_THUMB_PATH "A:/mnt/sd/.thumb/video_small/"
#define VIDEO_ALBUM_VIDEO_SUBPIC_PATH "A:/mnt/sd/.thumb/video_large/"
#define PHOTO_ALBUM_IMAGE_SUBPIC_PATH "A:/mnt/sd/.thumb/photo_large/"

/* =======================
 * 字体配置
 * ======================= */
#define CHINESE_FONT_PATH RES_FONT_PATH "/HarmonyOS_Sans_SC_Regular.ttf"

/* =======================
 * 分辨率配置
 * ======================= */
#define H_RES 640
#define V_RES 480

/* =======================
 * 屏幕配置
 * ======================= */
#define FB_DEV_NAME "/dev/fb0"
#define TOUCH_PANEL_EVENT_PATH "/dev/input/touchscreen"

/* =======================
 * 电源管理配置
 * ======================= */

/* 屏幕空闲超时时间（毫秒） */
#define POWER_MANAGER_SCREEN_IDLE_TIMEOUT_MS 60000

/* =======================
 * 触控手势配置
 * ======================= */

/* 滑动返回边缘阈值（部分页面可能不吃这个配置） */
#define SWIPE_BACK_EDGE_THRESHOLD_PX 40

/* 按下到手势识别的最大时间窗，超时不触发右滑返回 */
#define SWIPE_BACK_PRESS_GESTURE_MAX_MS 800

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H__ */
