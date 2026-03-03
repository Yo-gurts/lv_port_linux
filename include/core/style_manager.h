#ifndef __STYLE_MANAGER_H__
#define __STYLE_MANAGER_H__

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
extern lv_style_t style_overlay_mask; /* 通用遮罩样式 */
extern lv_style_t style_modal_panel; /* 通用弹层面板样式 */
extern lv_style_t style_toast_popup; /* 通用 toast 样式 */
extern lv_style_t style_list_row_even; /* 通用列表偶数行样式 */
extern lv_style_t style_list_row_odd; /* 通用列表奇数行样式 */
extern lv_style_t style_photo_filter_panel; /* 拍照页滤镜面板样式 */
extern lv_style_t style_photo_filter_focus_frame; /* 拍照页滤镜选中框样式 */
extern lv_style_t style_photo_filter_item; /* 拍照页滤镜条目样式 */
extern lv_style_t style_photo_filter_thumb; /* 拍照页滤镜缩略图底样式 */
extern lv_style_t style_zoom_container; /* 缩放总容器样式 */
extern lv_style_t style_zoom_btn; /* 缩放按钮基础样式 */
extern lv_style_t style_zoom_btn_active; /* 缩放按钮选中样式 */
extern lv_style_t style_zoom_label; /* 缩放文字默认样式 */
extern lv_style_t style_zoom_label_active; /* 缩放文字选中样式 */

void style_common_init(void);

#ifdef __cplusplus
}
#endif

#endif
