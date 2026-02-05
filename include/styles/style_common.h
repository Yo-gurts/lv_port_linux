#ifndef __STYLE_COMMON_H__
#define __STYLE_COMMON_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern lv_style_t style_common_cont_top;
extern lv_style_t style_common_main_bg;
extern lv_style_t style_common_btn_back;
extern lv_style_t style_common_label_back;
extern lv_style_t style_focus_orange;
extern lv_style_t style_home_bg; /* 首页渐变背景 */

void style_common_init(void);

#ifdef __cplusplus
}
#endif

#endif
