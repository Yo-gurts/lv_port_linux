#ifndef __CONFIG_H_
#define __CONFIG_H_

/************************** 屏幕相关 ******************************************/
#define FB_DEV_NAME "/dev/fb0"
#define H_RES 240 /* 水平分辨率 */
#define V_RES 1020 /* 垂直分辨率 */
#define DISP_BUF_SIZE (H_RES * V_RES)

/************************** INPUT EVENT 对应的文件路径 ************************/
#define KEY_EVENT_PATH "/dev/input/event0"
#define TOUCH_PANEL_EVENT_PATH "/dev/input/event3"
#define POWER_KEY_EVENT_PATH "/dev/input/event2"

/************************** 按键对应的 KEY CODE ******************************/
#define AUDIO_KEY 2  /* 音频按键 */
#define POWER_KEY 3  /* 电源按键 */
#define CAMERA_KEY 4 /* 摄像头按键 */

/************************** WIFI 列表展示的数量 *******************************/
#define WIFI_LIST_SHOW_NUM 20

/*****************************方正姚体加载路径**********************************/
// #define ALI_PUHUITI_FONTPATH "/mnt/sd/output/fonts/Alibaba-PuHuiTi-Light.ttf"
#define ALI_PUHUITI_FONTPATH "/usr/local/fonts/hei.TTF"
#define ALI_PUHUITI_WEIGHT 32
/* TP 最大的触摸点数 */
#define MAX_COUNT 2

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#endif
