#ifndef __FONT_MANAGER_H__
#define __FONT_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/* 中文字体样式 */
extern lv_style_t ttf_font_12;
extern lv_style_t ttf_font_14;
extern lv_style_t ttf_font_16;
extern lv_style_t ttf_font_18;
extern lv_style_t ttf_font_20;
extern lv_style_t ttf_font_22;
extern lv_style_t ttf_font_24;
extern lv_style_t ttf_font_28;
extern lv_style_t ttf_font_30;
extern lv_style_t ttf_font_34;

/* 初始化字体管理器 */
int font_manager_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __FONT_MANAGER_H__ */
