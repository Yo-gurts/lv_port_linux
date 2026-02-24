#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* =======================
 * 资源路径配置
 * ======================= */
#ifndef PROJECT_PATH
#define PROJECT_PATH "/mnt/data/bin"
#endif

#define RES_FONT_PATH PROJECT_PATH "/res/fonts"
#define RES_ICON_PATH PROJECT_PATH "/res/icons"

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
