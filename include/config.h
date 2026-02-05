#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* =======================
 * 资源路径配置
 * ======================= */
#define RES_PATH "/home/yogurt/Documents/sophgo/v6.x.x/sophcam_v20260203/applications/dashcam/ui/aicamera/sm3_81/res"

#define RES_FONT_PATH RES_PATH "/fonts"
#define RES_ICON_PATH RES_PATH "/icons"

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

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H__ */
