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
extern lv_style_t style_page_container; /* 页面容器样式 */
extern lv_style_t style_noboarder; /* 图片按钮样式 - 全透明 */
extern lv_style_t style_nav_bar; /* 导航栏样式 */
extern lv_style_t style_settings_item; /* 设置项样式 */
extern lv_style_t style_settings_item_selected; /* 设置项选中样式 */
extern lv_style_t style_settings_divider; /* 分隔线样式 */
extern lv_style_t style_settings_selected; /* 选中项高亮样式 */
extern lv_style_t style_settings_value; /* 设置项参数样式 */
extern lv_style_t style_roller_popup; /* 滚轮弹窗遮罩样式 */
extern lv_style_t style_roller; /* 滚轮样式 */

void style_common_init(void);

#ifdef __cplusplus
}
#endif

#endif
