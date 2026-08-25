#include "core/style_manager.h" /* 包含样式管理头文件 */
#include "mlog.h" /* 日志输出头文件 */

/* 顶部容器样式 - 透明背景、无边框 */
lv_style_t style_common_cont_top;
/* 主背景样式 - 黑色不透明背景 */
lv_style_t style_common_main_bg;
/* 按钮背景样式 - 深灰色背景、圆角 */
lv_style_t style_common_btn_back;
/* 标签背景样式 - 白色文字、居中对齐 */
lv_style_t style_common_label_back;
/* 焦点橙色样式 - 橙色轮廓高亮 */
lv_style_t style_focus_orange;
/* 首页渐变背景样式 - 浅蓝到天蓝垂直渐变 */
lv_style_t style_home_bg;
/* 页面容器样式 - 透明背景、无边框、无内边距 */
lv_style_t style_page_container;
/* 图片按钮样式 - 全透明、无边框 */
lv_style_t style_noboarder;
/* 导航栏样式 - 纯黑背景 */
lv_style_t style_nav_bar;
/* 设置项样式 - 深色背景 */
lv_style_t style_settings_item;
/* 设置项选中样式 - 浅色背景 */
lv_style_t style_settings_item_selected;
/* 分隔线样式 - 细蓝色 */
lv_style_t style_settings_divider;
/* 选中项高亮样式 - 橙色渐变 */
lv_style_t style_settings_selected;
/* 设置项参数样式 - 黄色文字 */
lv_style_t style_settings_value;
/* 滚轮弹窗遮罩样式 - 半透明黑色 */
lv_style_t style_roller_popup;
/* 滚轮样式 - 深色背景、圆角 */
lv_style_t style_roller;
/* 通用遮罩样式 */
lv_style_t style_overlay_mask;
/* 通用弹层面板样式 */
lv_style_t style_modal_panel;
/* 通用 toast 样式 */
lv_style_t style_toast_popup;
/* 通用列表偶数行样式 */
lv_style_t style_list_row_even;
/* 通用列表奇数行样式 */
lv_style_t style_list_row_odd;
/* 拍照页滤镜面板样式 */
lv_style_t style_photo_filter_panel;
/* 拍照页滤镜选中框样式 */
lv_style_t style_photo_filter_focus_frame;
/* 拍照页滤镜条目样式 */
lv_style_t style_photo_filter_item;
/* 拍照页滤镜缩略图底样式 */
lv_style_t style_photo_filter_thumb;
/* 缩放总容器样式 */
lv_style_t style_zoom_container;
/* 缩放按钮基础样式 */
lv_style_t style_zoom_btn;
/* 缩放按钮选中样式 */
lv_style_t style_zoom_btn_active;
/* 缩放文字默认样式 */
lv_style_t style_zoom_label;
/* 缩放文字选中样式 */
lv_style_t style_zoom_label_active;
/* 缩放居中圆形选中框样式 */
lv_style_t style_zoom_focus_frame;

/**
 * @brief 初始化所有公共样式
 * @note 在系统启动时调用一次，完成所有样式的配置
 */
void style_common_init(void)
{
    MLOG_INFO("Initializing common styles"); /* 输出初始化日志 */

    /* 初始化顶部容器样式 */
    lv_style_init(&style_common_cont_top); /* 创建样式对象 */
    lv_style_set_border_width(&style_common_cont_top, 0); /* 无边框 */
    lv_style_set_radius(&style_common_cont_top, 0); /* 无圆角 */
    lv_style_set_bg_opa(&style_common_cont_top, LV_OPA_COVER); /* 背景完全不透明 */
    lv_style_set_bg_color(&style_common_cont_top, lv_color_black()); /* 背景色为黑色 */
    lv_style_set_bg_grad_dir(&style_common_cont_top, LV_GRAD_DIR_NONE); /* 无渐变 */
    lv_style_set_pad_top(&style_common_cont_top, 0); /* 无内边距 */
    lv_style_set_pad_bottom(&style_common_cont_top, 0);
    lv_style_set_pad_left(&style_common_cont_top, 0);
    lv_style_set_pad_right(&style_common_cont_top, 0);
    lv_style_set_shadow_width(&style_common_cont_top, 0); /* 无阴影 */
    lv_style_set_margin_top(&style_common_cont_top, 0); /* 无外边距 */
    lv_style_set_margin_bottom(&style_common_cont_top, 0);
    lv_style_set_margin_left(&style_common_cont_top, 0);
    lv_style_set_margin_right(&style_common_cont_top, 0);

    /* 初始化主背景样式 */
    lv_style_init(&style_common_main_bg); /* 创建样式对象 */
    lv_style_set_bg_opa(&style_common_main_bg, LV_OPA_COVER); /* 背景完全不透明 */
    lv_style_set_bg_color(&style_common_main_bg, lv_color_hex(0x000000)); /* 背景色为黑色 */

    /* 初始化按钮背景样式 */
    lv_style_init(&style_common_btn_back); /* 创建样式对象 */
    lv_style_set_bg_opa(&style_common_btn_back, LV_OPA_COVER); /* 背景完全不透明 */
    lv_style_set_bg_color(&style_common_btn_back, lv_color_hex(0x171717)); /* 背景色为深灰 */
    lv_style_set_border_width(&style_common_btn_back, 0); /* 无边框 */
    lv_style_set_radius(&style_common_btn_back, 20); /* 圆角半径20px */
    lv_style_set_shadow_width(&style_common_btn_back, 0); /* 无阴影 */
    lv_style_set_text_color(&style_common_btn_back, lv_color_white()); /* 文字颜色为深灰 */
    lv_style_set_text_align(&style_common_btn_back, LV_TEXT_ALIGN_CENTER); /* 文字居中对齐 */
    lv_style_set_pad_top(&style_common_btn_back, 0); /* 上内边距为0 */
    lv_style_set_pad_bottom(&style_common_btn_back, 0); /* 下内边距为0 */
    lv_style_set_pad_left(&style_common_btn_back, 0); /* 左内边距为0 */
    lv_style_set_pad_right(&style_common_btn_back, 0); /* 右内边距为0 */

    /* 初始化标签背景样式 */
    lv_style_init(&style_common_label_back); /* 创建样式对象 */
    lv_style_set_text_color(&style_common_label_back, lv_color_white()); /* 文字颜色为白色 */
    lv_style_set_text_align(&style_common_label_back, LV_TEXT_ALIGN_CENTER); /* 文字居中对齐 */

    /* 初始化焦点橙色样式 */
    lv_style_init(&style_focus_orange); /* 创建样式对象 */
    lv_style_set_outline_color(&style_focus_orange, lv_color_hex(0xF09F20)); /* 轮廓颜色为橙色 */
    lv_style_set_outline_opa(&style_focus_orange, LV_OPA_COVER); /* 轮廓完全不透明 */

    /* 初始化首页渐变背景样式 */
    lv_style_init(&style_home_bg); /* 创建样式对象 */
    lv_style_set_bg_color(&style_home_bg, lv_color_hex(0x4a90d9)); /* 渐变起始色：浅蓝 */
    lv_style_set_bg_grad_color(&style_home_bg, lv_color_hex(0x87ceeb)); /* 渐变结束色：天蓝 */
    lv_style_set_bg_grad_dir(&style_home_bg, LV_GRAD_DIR_VER); /* 渐变方向：垂直 */
    lv_style_set_bg_opa(&style_home_bg, LV_OPA_COVER); /* 背景完全不透明 */

    /* 初始化页面容器样式 */
    lv_style_init(&style_page_container); /* 创建样式对象 */
    lv_style_set_border_width(&style_page_container, 0); /* 无边框 */
    lv_style_set_radius(&style_page_container, 0); /* 无圆角 */
    lv_style_set_bg_opa(&style_page_container, LV_OPA_TRANSP); /* 背景透明 */
    lv_style_set_pad_top(&style_page_container, 0); /* 无内边距 */
    lv_style_set_pad_bottom(&style_page_container, 0);
    lv_style_set_pad_left(&style_page_container, 0);
    lv_style_set_pad_right(&style_page_container, 0);
    lv_style_set_pad_row(&style_page_container, 0); /* FLEX 布局无行间距 */
    lv_style_set_margin_top(&style_page_container, 0); /* 无外边距 */
    lv_style_set_margin_bottom(&style_page_container, 0);
    lv_style_set_margin_left(&style_page_container, 0);
    lv_style_set_margin_right(&style_page_container, 0);

    /* 初始化无边框-全透明样式 */
    lv_style_init(&style_noboarder); /* 创建样式对象 */
    lv_style_set_bg_opa(&style_noboarder, LV_OPA_TRANSP); /* 背景完全透明 */
    lv_style_set_border_width(&style_noboarder, 0); /* 无边框 */
    lv_style_set_radius(&style_noboarder, 0); /* 无圆角 */
    lv_style_set_shadow_width(&style_noboarder, 0); /* 无阴影 */
    lv_style_set_pad_top(&style_noboarder, 0); /* 无内边距 */
    lv_style_set_pad_bottom(&style_noboarder, 0);
    lv_style_set_pad_left(&style_noboarder, 0);
    lv_style_set_pad_right(&style_noboarder, 0);

    /* 初始化导航栏样式 */
    lv_style_init(&style_nav_bar);
    lv_style_set_bg_opa(&style_nav_bar, LV_OPA_COVER);
    lv_style_set_bg_color(&style_nav_bar, lv_color_hex(0x000000));
    lv_style_set_border_width(&style_nav_bar, 0);
    lv_style_set_radius(&style_nav_bar, 0);

    /* 初始化设置项样式 */
    lv_style_init(&style_settings_item);
    lv_style_set_bg_opa(&style_settings_item, LV_OPA_COVER);
    lv_style_set_bg_color(&style_settings_item, lv_color_hex(0x1A1A1A)); /* 深色背景 */
    lv_style_set_border_width(&style_settings_item, 0);
    lv_style_set_pad_top(&style_settings_item, 12);
    lv_style_set_pad_bottom(&style_settings_item, 12);
    lv_style_set_pad_left(&style_settings_item, 15);
    lv_style_set_pad_right(&style_settings_item, 15);

    /* 初始化设置项选中样式 */
    lv_style_init(&style_settings_item_selected);
    lv_style_set_bg_opa(&style_settings_item_selected, LV_OPA_COVER);
    lv_style_set_bg_color(&style_settings_item_selected, lv_color_hex(0x2A2A2A)); /* 浅色背景表示选中 */
    lv_style_set_border_width(&style_settings_item_selected, 0);
    lv_style_set_pad_top(&style_settings_item_selected, 12);
    lv_style_set_pad_bottom(&style_settings_item_selected, 12);
    lv_style_set_pad_left(&style_settings_item_selected, 15);
    lv_style_set_pad_right(&style_settings_item_selected, 15);

    /* 初始化分隔线样式 */
    lv_style_init(&style_settings_divider);
    lv_style_set_bg_opa(&style_settings_divider, LV_OPA_COVER);
    lv_style_set_bg_color(&style_settings_divider, lv_color_hex(0x4A90D9)); /* 蓝色分隔线 */
    lv_style_set_height(&style_settings_divider, 1);

    /* 初始化选中项高亮样式 */
    lv_style_init(&style_settings_selected);
    lv_style_set_bg_color(&style_settings_selected, lv_color_hex(0xF09F20)); /* 橙色 */
    lv_style_set_bg_grad_color(&style_settings_selected, lv_color_hex(0xE08510)); /* 渐变 */
    lv_style_set_bg_grad_dir(&style_settings_selected, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_settings_selected, LV_OPA_COVER);

    /* 初始化设置项参数样式 */
    lv_style_init(&style_settings_value);
    lv_style_set_text_color(&style_settings_value, lv_color_hex(0xFFD700)); /* 黄色 */

    /* 初始化滚轮弹窗遮罩样式 */
    lv_style_init(&style_roller_popup);
    lv_style_set_bg_opa(&style_roller_popup, LV_OPA_50);
    lv_style_set_bg_color(&style_roller_popup, lv_color_black());
    lv_style_set_border_width(&style_roller_popup, 0);
    lv_style_set_radius(&style_roller_popup, 0);

    /* 初始化滚轮样式 */
    lv_style_init(&style_roller);
    lv_style_set_bg_opa(&style_roller, LV_OPA_COVER);
    lv_style_set_bg_color(&style_roller, lv_color_hex(0x2A2A2A));
    lv_style_set_radius(&style_roller, 20);

    /* 初始化通用遮罩样式 */
    lv_style_init(&style_overlay_mask);
    lv_style_set_bg_opa(&style_overlay_mask, LV_OPA_60);
    lv_style_set_bg_color(&style_overlay_mask, lv_color_black());
    lv_style_set_border_width(&style_overlay_mask, 0);
    lv_style_set_radius(&style_overlay_mask, 0);

    /* 初始化通用弹层面板样式 */
    lv_style_init(&style_modal_panel);
    lv_style_set_bg_opa(&style_modal_panel, LV_OPA_COVER);
    lv_style_set_bg_color(&style_modal_panel, lv_color_hex(0x1E1E1E));
    lv_style_set_border_width(&style_modal_panel, 0);
    lv_style_set_radius(&style_modal_panel, 12);

    /* 初始化通用 toast 样式 */
    lv_style_init(&style_toast_popup);
    lv_style_set_bg_opa(&style_toast_popup, LV_OPA_60);
    lv_style_set_bg_color(&style_toast_popup, lv_color_black());
    lv_style_set_border_width(&style_toast_popup, 0);
    lv_style_set_radius(&style_toast_popup, 22);
    lv_style_set_pad_hor(&style_toast_popup, 20);
    lv_style_set_pad_ver(&style_toast_popup, 10);

    /* 初始化通用列表行样式（偶数） */
    lv_style_init(&style_list_row_even);
    lv_style_set_radius(&style_list_row_even, 0);
    lv_style_set_bg_opa(&style_list_row_even, LV_OPA_COVER);
    lv_style_set_bg_color(&style_list_row_even, lv_color_hex(0x151515));
    lv_style_set_border_side(&style_list_row_even, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_width(&style_list_row_even, 1);
    lv_style_set_border_color(&style_list_row_even, lv_color_hex(0x2E2E2E));

    /* 初始化通用列表行样式（奇数） */
    lv_style_init(&style_list_row_odd);
    lv_style_set_radius(&style_list_row_odd, 0);
    lv_style_set_bg_opa(&style_list_row_odd, LV_OPA_COVER);
    lv_style_set_bg_color(&style_list_row_odd, lv_color_hex(0x191919));
    lv_style_set_border_side(&style_list_row_odd, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_width(&style_list_row_odd, 1);
    lv_style_set_border_color(&style_list_row_odd, lv_color_hex(0x2E2E2E));

    /* 初始化拍照页滤镜面板样式 */
    lv_style_init(&style_photo_filter_panel);
    lv_style_set_bg_opa(&style_photo_filter_panel, LV_OPA_40);
    lv_style_set_bg_color(&style_photo_filter_panel, lv_color_hex(0xFFFFFF));
    lv_style_set_bg_grad_color(&style_photo_filter_panel, lv_color_hex(0xEDEDED));
    lv_style_set_bg_grad_dir(&style_photo_filter_panel, LV_GRAD_DIR_VER);
    lv_style_set_border_width(&style_photo_filter_panel, 1);
    lv_style_set_border_color(&style_photo_filter_panel, lv_color_hex(0xF6F6F6));
    lv_style_set_border_side(&style_photo_filter_panel, LV_BORDER_SIDE_TOP);
    lv_style_set_radius(&style_photo_filter_panel, 10);
    lv_style_set_pad_all(&style_photo_filter_panel, 0);

    /* 初始化拍照页滤镜选中框样式 */
    lv_style_init(&style_photo_filter_focus_frame);
    lv_style_set_bg_opa(&style_photo_filter_focus_frame, LV_OPA_TRANSP);
    lv_style_set_border_width(&style_photo_filter_focus_frame, 4);
    lv_style_set_border_color(&style_photo_filter_focus_frame, lv_color_hex(0xF09F20));
    lv_style_set_radius(&style_photo_filter_focus_frame, 12);
    lv_style_set_shadow_width(&style_photo_filter_focus_frame, 10);
    lv_style_set_shadow_color(&style_photo_filter_focus_frame, lv_color_hex(0xF09F20));
    lv_style_set_shadow_opa(&style_photo_filter_focus_frame, LV_OPA_30);

    /* 初始化拍照页滤镜条目样式 */
    lv_style_init(&style_photo_filter_item);
    lv_style_set_bg_opa(&style_photo_filter_item, LV_OPA_TRANSP);
    lv_style_set_border_width(&style_photo_filter_item, 0);
    lv_style_set_pad_all(&style_photo_filter_item, 0);
    lv_style_set_radius(&style_photo_filter_item, 14);

    /* 初始化拍照页滤镜缩略图底样式 */
    lv_style_init(&style_photo_filter_thumb);
    lv_style_set_bg_opa(&style_photo_filter_thumb, LV_OPA_COVER);
    lv_style_set_bg_color(&style_photo_filter_thumb, lv_color_hex(0x1A1A1A));
    lv_style_set_border_width(&style_photo_filter_thumb, 0);
    lv_style_set_radius(&style_photo_filter_thumb, 12);
    lv_style_set_pad_all(&style_photo_filter_thumb, 0);

    /* 初始化缩放总容器样式 */
    lv_style_init(&style_zoom_container);
    lv_style_set_bg_color(&style_zoom_container, lv_color_hex(0xFFFFFF));
    lv_style_set_bg_opa(&style_zoom_container, LV_OPA_20);
    lv_style_set_border_width(&style_zoom_container, 0);
    lv_style_set_radius(&style_zoom_container, LV_RADIUS_CIRCLE);
    lv_style_set_pad_all(&style_zoom_container, 0);

    /* 初始化缩放按钮基础样式 */
    lv_style_init(&style_zoom_btn);
    lv_style_set_bg_color(&style_zoom_btn, lv_color_hex(0xFFFFFF));
    lv_style_set_bg_opa(&style_zoom_btn, LV_OPA_10);
    lv_style_set_border_width(&style_zoom_btn, 0);
    lv_style_set_radius(&style_zoom_btn, LV_RADIUS_CIRCLE);
    lv_style_set_shadow_width(&style_zoom_btn, 0);
    lv_style_set_pad_all(&style_zoom_btn, 0);

    /* 初始化缩放按钮选中样式 */
    lv_style_init(&style_zoom_btn_active);
    lv_style_set_bg_color(&style_zoom_btn_active, lv_color_hex(0xFFFFFF));
    lv_style_set_bg_opa(&style_zoom_btn_active, LV_OPA_30);

    /* 初始化缩放文字默认样式 */
    lv_style_init(&style_zoom_label);
    lv_style_set_text_color(&style_zoom_label, lv_color_hex(0xFFFFFF));
    lv_style_set_text_align(&style_zoom_label, LV_TEXT_ALIGN_CENTER);

    /* 初始化缩放文字选中样式 */
    lv_style_init(&style_zoom_label_active);
    lv_style_set_text_color(&style_zoom_label_active, lv_color_hex(0xF05A28));
    lv_style_set_text_align(&style_zoom_label_active, LV_TEXT_ALIGN_CENTER);

    /* 初始化缩放居中圆形选中框样式 */
    lv_style_init(&style_zoom_focus_frame);
    lv_style_set_bg_opa(&style_zoom_focus_frame, LV_OPA_TRANSP);
    lv_style_set_border_width(&style_zoom_focus_frame, 3);
    lv_style_set_border_color(&style_zoom_focus_frame, lv_color_hex(0xF05A28));
    lv_style_set_radius(&style_zoom_focus_frame, LV_RADIUS_CIRCLE);
    lv_style_set_shadow_width(&style_zoom_focus_frame, 10);
    lv_style_set_shadow_color(&style_zoom_focus_frame, lv_color_hex(0xF05A28));
    lv_style_set_shadow_opa(&style_zoom_focus_frame, LV_OPA_30);
}
