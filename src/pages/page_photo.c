// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_photo.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/media_manager.h"
#include "core/page_manager.h"
#include "core/param_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* 布局常量 */
#define TOP_BAR_HEIGHT 50
#define BOTTOM_BAR_HEIGHT 50
#define FILTER_PANEL_HEIGHT 116
#define FILTER_THUMB_WIDTH 80
#define FILTER_THUMB_HEIGHT 60
#define FILTER_ITEM_HEIGHT 100
#define FILTER_ITEM_GAP 12
#define FILTER_PANEL_TOP_PAD 8
#define FILTER_PANEL_BOTTOM_PAD 2
#define FILTER_FOCUS_Y_OFFSET -1

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_photo.h)
// #############################################################################

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

/* 分辨率选项 - 与photo_settings保持一致 */
static const char* resolution_options[] = {
    "8M", "12M", "24M", "48M", "64M"
};

typedef struct {
    const char* name;
    const char* icon_path;
} photo_filter_config_t;

static const photo_filter_config_t photo_filter_configs[] = {
    { "原图", "A:" RES_ICON_PATH "/filter.png" },
    { "美味", "A:" RES_ICON_PATH "/filter.png" },
    { "冷调", "A:" RES_ICON_PATH "/filter.png" },
    { "暖调", "A:" RES_ICON_PATH "/filter.png" },
    { "浓郁", "A:" RES_ICON_PATH "/filter.png" },
    { "高级", "A:" RES_ICON_PATH "/filter.png" },
};

#define PHOTO_FILTER_COUNT ((int)(sizeof(photo_filter_configs) / sizeof(photo_filter_configs[0])))

static void update_filter_selection_style(page_photo_data_t* data);
static void scroll_to_filter_index(page_photo_data_t* data, int index, lv_anim_enable_t anim_en);
static void snap_filter_to_center(page_photo_data_t* data, lv_anim_enable_t anim_en);
static void show_filter_overlay(page_photo_data_t* data);
static void hide_filter_overlay(page_photo_data_t* data);
static void set_selected_filter_index(page_photo_data_t* data, int index, const char* source);

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

/* 更新分辨率显示 */
static void update_resolution_display(void)
{
    page_photo_data_t* data = page_get_private_data();
    if (!data || !data->resolution_label) {
        return;
    }

    int resolution_index = param_manager_get(PARAM_ID_RESOLUTION);
    lv_label_set_text(data->resolution_label, resolution_options[resolution_index]);
}

/* 更新滤镜选中态样式。 */
static void update_filter_selection_style(page_photo_data_t* data)
{
    int i;

    if (data == NULL) {
        return;
    }

    for (i = 0; i < data->filter_count; i++) {
        lv_obj_t* item = data->filter_items[i];
        lv_obj_t* label = data->filter_labels[i];
        if (item == NULL || label == NULL) {
            continue;
        }

        if (i == data->selected_filter_index) {
            lv_obj_set_style_text_color(label, lv_color_hex(0xF09F20), LV_PART_MAIN);
            lv_obj_set_style_opa(item, LV_OPA_COVER, LV_PART_MAIN);
        } else {
            lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_opa(item, LV_OPA_80, LV_PART_MAIN);
        }
    }
}

/* 统一更新选中滤镜并打印日志，便于后续接入真实滤镜切换。 */
static void set_selected_filter_index(page_photo_data_t* data, int index, const char* source)
{
    if (data == NULL || index < 0 || index >= data->filter_count) {
        return;
    }
    if (data->selected_filter_index == index) {
        return;
    }

    data->selected_filter_index = index;
    MLOG_INFO("Filter selected [%s]: index=%d, name=%s",
        (source == NULL) ? "unknown" : source,
        index,
        photo_filter_configs[index].name);
}

/* 根据选中下标滚动列表，让条目中心对齐中间选中框。 */
static void scroll_to_filter_index(page_photo_data_t* data, int index, lv_anim_enable_t anim_en)
{
    int32_t step;
    int32_t target_x;
    int32_t max_scroll_x;

    if (data == NULL || data->filter_list == NULL || index < 0 || index >= data->filter_count) {
        return;
    }

    step = FILTER_THUMB_WIDTH + FILTER_ITEM_GAP;
    target_x = index * step;
    if (target_x < 0) {
        target_x = 0;
    }

    max_scroll_x = lv_obj_get_scroll_left(data->filter_list) + lv_obj_get_scroll_right(data->filter_list);
    if (target_x > max_scroll_x) {
        target_x = max_scroll_x;
    }

    lv_obj_scroll_to_x(data->filter_list, target_x, anim_en);
}

/* 滚动结束后吸附到最近滤镜，并刷新选中态。 */
static void snap_filter_to_center(page_photo_data_t* data, lv_anim_enable_t anim_en)
{
    int32_t scroll_x;
    int32_t step;
    int nearest_index;

    if (data == NULL || data->filter_list == NULL || data->filter_count <= 0) {
        return;
    }

    step = FILTER_THUMB_WIDTH + FILTER_ITEM_GAP;
    scroll_x = lv_obj_get_scroll_left(data->filter_list);
    nearest_index = (scroll_x + step / 2) / step;
    if (nearest_index < 0) {
        nearest_index = 0;
    }
    if (nearest_index >= data->filter_count) {
        nearest_index = data->filter_count - 1;
    }

    set_selected_filter_index(data, nearest_index, "scroll_snap");
    update_filter_selection_style(data);
    scroll_to_filter_index(data, nearest_index, anim_en);
}

/* 让中间选中框和任意缩略图容器严格上下对齐。 */
static void align_filter_focus_frame(page_photo_data_t* data)
{
    lv_obj_t* ref_thumb;
    lv_area_t thumb_coords;
    lv_area_t panel_coords;
    int32_t y_in_panel;

    if (data == NULL || data->filter_focus_frame == NULL || data->filter_panel == NULL || data->filter_list == NULL) {
        return;
    }
    if (data->filter_count <= 0) {
        return;
    }
    ref_thumb = data->filter_thumbs[0];
    if (ref_thumb == NULL) {
        return;
    }

    lv_obj_update_layout(data->filter_list);
    lv_obj_get_coords(ref_thumb, &thumb_coords);
    lv_obj_get_coords(data->filter_panel, &panel_coords);
    y_in_panel = thumb_coords.y1 - panel_coords.y1;
    lv_obj_align(data->filter_focus_frame, LV_ALIGN_TOP_MID, 0, y_in_panel + FILTER_FOCUS_Y_OFFSET);
}

/* 显示滤镜全屏层。 */
static void show_filter_overlay(page_photo_data_t* data)
{
    if (data == NULL || data->filter_overlay == NULL) {
        return;
    }
    lv_obj_clear_flag(data->filter_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(data->filter_overlay);
    align_filter_focus_frame(data);
    snap_filter_to_center(data, LV_ANIM_OFF);
}

/* 隐藏滤镜全屏层。 */
static void hide_filter_overlay(page_photo_data_t* data)
{
    if (data == NULL || data->filter_overlay == NULL) {
        return;
    }
    lv_obj_add_flag(data->filter_overlay, LV_OBJ_FLAG_HIDDEN);
}

// #endregion
// #############################################################################
// ! #region 5. 对外接口函数
// #############################################################################

// #endregion
// #############################################################################
// ! #region 6. 线程处理函数
// #############################################################################

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

/* 拍照/录像切换回调 */
static void mode_switch_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    MLOG_INFO("Switching to video page");
    (void)media_manager_execute(MEDIA_OP_SWITCH_TO_VIDEO_MODE, 0);
    page_manager_navigate("video");
}

/* 菜单按钮回调：跳转拍照设置页面 */
static void menu_back_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    MLOG_INFO("Menu clicked, navigate to photo_settings");
    page_manager_navigate("photo_settings");
}

/* 顶部返回按钮回调：返回时切换到 boot mode */
static void back_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    (void)media_manager_execute(MEDIA_OP_SWITCH_TO_BOOT_MODE, 0);
    page_manager_back();
}

/* 当检测到滑动时，调用 lv_indev_set_wait_until_release() 避免释放时触发点击 */
static void swipe_right_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            MLOG_INFO("Swipe right, back to previous page");
            back_btn_cb(NULL);
        }

        /* 检测到滑动，忽略后续的点击事件 */
        lv_indev_t* indev = lv_indev_get_act();
        if (indev) {
            lv_indev_wait_release(indev);
        }
    }
}

/* 滤镜按钮回调：显示/隐藏滤镜面板。 */
static void filter_btn_cb(lv_event_t* e)
{
    page_photo_data_t* data = (page_photo_data_t*)lv_event_get_user_data(e);

    if (data == NULL || data->filter_overlay == NULL) {
        return;
    }

    if (lv_obj_has_flag(data->filter_overlay, LV_OBJ_FLAG_HIDDEN)) {
        show_filter_overlay(data);
    } else {
        hide_filter_overlay(data);
    }
}

/* 滤镜条目点击回调：切换选中项（滤镜实际效果后续接入）。 */
static void filter_item_cb(lv_event_t* e)
{
    page_photo_data_t* data = (page_photo_data_t*)lv_event_get_user_data(e);
    lv_obj_t* target = lv_event_get_current_target(e);
    int index;

    if (data == NULL || target == NULL) {
        return;
    }

    index = (int)(intptr_t)lv_obj_get_user_data(target);
    if (index < 0 || index >= data->filter_count) {
        return;
    }

    set_selected_filter_index(data, index, "item_click");
    update_filter_selection_style(data);
    scroll_to_filter_index(data, index, LV_ANIM_ON);
}

/* 滤镜列表滚动结束回调：自动吸附到中间框。 */
static void filter_list_scroll_end_cb(lv_event_t* e)
{
    page_photo_data_t* data = (page_photo_data_t*)lv_event_get_user_data(e);
    if (data == NULL) {
        return;
    }
    snap_filter_to_center(data, LV_ANIM_ON);
}

/* 点击滤镜面板外区域时关闭滤镜层。 */
static void filter_overlay_click_cb(lv_event_t* e)
{
    page_photo_data_t* data = (page_photo_data_t*)lv_event_get_user_data(e);
    if (data == NULL) {
        return;
    }
    hide_filter_overlay(data);
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_photo_create(void)
{
    page_photo_data_t* data = (page_photo_data_t*)malloc(sizeof(page_photo_data_t));
    if (!data) {
        return;
    }

    memset(data, 0, sizeof(page_photo_data_t));

    /* 创建页面容器 */
    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_width(data->container, LV_PCT(100));
    lv_obj_set_height(data->container, LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_refr_size(data->container);

    /* 启用滑动手势检测，不将 GESTURE 事件传递给父控件 */
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    /* 添加滑动手势回调：从左往右滑返回上一页 */
    lv_obj_add_event_cb(data->container, swipe_right_cb, LV_EVENT_GESTURE, NULL);

    /* =======================
     * 顶部状态栏：[back][8M] 在左边，剩余拍照数 [SD][battery] 在右边
     * ======================= */
    data->top_bar = lv_obj_create(data->container);
    lv_obj_set_width(data->top_bar, lv_pct(100));
    lv_obj_set_height(data->top_bar, TOP_BAR_HEIGHT);
    lv_obj_add_style(data->top_bar, &style_noboarder, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(data->top_bar, LV_SCROLLBAR_MODE_OFF);

    /* 返回按钮 - 左上角 */
    data->back_btn = lv_btn_create(data->top_bar);
    lv_obj_set_size(data->back_btn, 50, 50);
    lv_obj_add_style(data->back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->back_btn, LV_ALIGN_TOP_LEFT, 10, 0);
    lv_obj_t* back_icon = lv_img_create(data->back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    /* 分辨率 Label - 跟在返回按钮后面 */
    data->resolution_label = lv_label_create(data->top_bar);
    int resolution_index = param_manager_get(PARAM_ID_RESOLUTION);
    lv_label_set_text(data->resolution_label, resolution_options[resolution_index]);
    lv_obj_add_style(data->resolution_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->resolution_label, LV_ALIGN_LEFT_MID, 70, 0);

    /* 剩余照片数量 Label - 右上角 */
    data->photo_count_label = lv_label_create(data->top_bar);
    lv_label_set_text(data->photo_count_label, "100");
    lv_obj_add_style(data->photo_count_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->photo_count_label, LV_ALIGN_RIGHT_MID, -110, 0);

    /* SD卡图标 - 右上角 */
    data->sd_icon = lv_img_create(data->top_bar);
    lv_img_set_src(data->sd_icon, "A:" RES_ICON_PATH "/sd_online.png");
    lv_obj_align(data->sd_icon, LV_ALIGN_RIGHT_MID, -60, 0);

    /* 电池图标 - 最右上角 */
    data->battery_icon = lv_img_create(data->top_bar);
    lv_img_set_src(data->battery_icon, "A:" RES_ICON_PATH "/battery-charging.png");
    lv_obj_align(data->battery_icon, LV_ALIGN_RIGHT_MID, -10, 0);

    /* =======================
     * 底部工具栏：[photo][filter] ... [switch][menu]
     * ======================= */
    data->bottom_bar = lv_obj_create(data->container);
    lv_obj_set_width(data->bottom_bar, lv_pct(100));
    lv_obj_set_height(data->bottom_bar, BOTTOM_BAR_HEIGHT);
    lv_obj_add_style(data->bottom_bar, &style_noboarder, LV_PART_MAIN);
    lv_obj_align(data->bottom_bar, LV_ALIGN_BOTTOM_MID, 0, -5); /* 底部向上5像素 */
    lv_obj_set_scrollbar_mode(data->bottom_bar, LV_SCROLLBAR_MODE_OFF); /* 不可滚动 */

    /* 拍照/录像切换按钮 - 左对齐 */
    data->mode_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->mode_btn, 50, 50);
    lv_obj_add_style(data->mode_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->mode_btn, mode_switch_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->mode_btn, LV_ALIGN_LEFT_MID, 10, 0);
    data->mode_img = lv_img_create(data->mode_btn);
    lv_img_set_src(data->mode_img, "A:" RES_ICON_PATH "/camera.png");
    lv_obj_align(data->mode_img, LV_ALIGN_CENTER, 0, 0);

    /* 滤镜按钮 - 紧随拍照按钮 */
    data->filter_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->filter_btn, 50, 50);
    lv_obj_add_style(data->filter_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->filter_btn, filter_btn_cb, LV_EVENT_CLICKED, data);
    lv_obj_align(data->filter_btn, LV_ALIGN_LEFT_MID, 70, 0);
    lv_obj_t* filter_icon = lv_img_create(data->filter_btn);
    lv_img_set_src(filter_icon, "A:" RES_ICON_PATH "/filter_default.png");
    lv_obj_align(filter_icon, LV_ALIGN_CENTER, 0, 0);

    /* 摄像头切换按钮 - 右对齐 */
    data->switch_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->switch_btn, 50, 50);
    lv_obj_add_style(data->switch_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->switch_btn, NULL, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->switch_btn, LV_ALIGN_RIGHT_MID, -70, 0);
    lv_obj_t* switch_icon = lv_img_create(data->switch_btn);
    lv_img_set_src(switch_icon, "A:" RES_ICON_PATH "/switch.png");
    lv_obj_align(switch_icon, LV_ALIGN_CENTER, 0, 0);

    /* 菜单按钮 - 最右侧 */
    data->menu_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->menu_btn, 50, 50);
    lv_obj_add_style(data->menu_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->menu_btn, menu_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->menu_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_t* menu_icon = lv_img_create(data->menu_btn);
    lv_img_set_src(menu_icon, "A:" RES_ICON_PATH "/menu.png");
    lv_obj_align(menu_icon, LV_ALIGN_CENTER, 0, 0);

    /* 滤镜全屏点击层：点击空白处关闭。 */
    data->filter_overlay = lv_obj_create(data->container);
    lv_obj_set_size(data->filter_overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(data->filter_overlay, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->filter_overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(data->filter_overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(data->filter_overlay, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(data->filter_overlay, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(data->filter_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(data->filter_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(data->filter_overlay, filter_overlay_click_cb, LV_EVENT_CLICKED, data);

    /* 滤镜横向选择面板（作为 overlay 子容器）。 */
    data->filter_panel = lv_obj_create(data->filter_overlay);
    lv_obj_set_width(data->filter_panel, lv_pct(100));
    lv_obj_set_height(data->filter_panel, FILTER_PANEL_HEIGHT);
    lv_obj_align(data->filter_panel, LV_ALIGN_BOTTOM_MID, 0, -63);
    lv_obj_set_scrollbar_mode(data->filter_panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(data->filter_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(data->filter_panel, &style_photo_filter_panel, LV_PART_MAIN);
    lv_obj_add_flag(data->filter_panel, LV_OBJ_FLAG_CLICKABLE);

    data->filter_list = lv_obj_create(data->filter_panel);
    lv_obj_set_size(data->filter_list, lv_pct(100), lv_pct(100));
    lv_obj_set_scroll_dir(data->filter_list, LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(data->filter_list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_layout(data->filter_list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->filter_list, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(data->filter_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(data->filter_list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->filter_list, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(data->filter_list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(data->filter_list, (H_RES - FILTER_THUMB_WIDTH) / 2, LV_PART_MAIN);
    lv_obj_set_style_pad_right(data->filter_list, (H_RES - FILTER_THUMB_WIDTH) / 2, LV_PART_MAIN);
    lv_obj_set_style_pad_top(data->filter_list, FILTER_PANEL_TOP_PAD, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(data->filter_list, FILTER_PANEL_BOTTOM_PAD, LV_PART_MAIN);
    lv_obj_set_style_pad_column(data->filter_list, FILTER_ITEM_GAP, LV_PART_MAIN);
    lv_obj_set_style_pad_row(data->filter_list, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(data->filter_list, filter_list_scroll_end_cb, LV_EVENT_SCROLL_END, data);

    data->filter_focus_frame = lv_obj_create(data->filter_panel);
    lv_obj_add_flag(data->filter_focus_frame, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(data->filter_focus_frame, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(data->filter_focus_frame, FILTER_THUMB_WIDTH, FILTER_THUMB_HEIGHT);
    lv_obj_align(data->filter_focus_frame, LV_ALIGN_TOP_MID, 0, FILTER_PANEL_TOP_PAD);
    lv_obj_add_style(data->filter_focus_frame, &style_photo_filter_focus_frame, LV_PART_MAIN);

    data->filter_count = (PHOTO_FILTER_COUNT > PHOTO_FILTER_MAX_COUNT) ? PHOTO_FILTER_MAX_COUNT : PHOTO_FILTER_COUNT;
    data->selected_filter_index = 0;

    for (int i = 0; i < data->filter_count; i++) {
        lv_obj_t* item = lv_obj_create(data->filter_list);
        lv_obj_set_size(item, FILTER_THUMB_WIDTH, FILTER_ITEM_HEIGHT);
        lv_obj_set_scrollbar_mode(item, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_style(item, &style_photo_filter_item, LV_PART_MAIN);
        lv_obj_set_layout(item, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(item, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(item, 8, LV_PART_MAIN);
        lv_obj_add_event_cb(item, filter_item_cb, LV_EVENT_CLICKED, data);
        lv_obj_set_user_data(item, (void*)(intptr_t)i);

        lv_obj_t* thumb = lv_obj_create(item);
        lv_obj_set_size(thumb, FILTER_THUMB_WIDTH, FILTER_THUMB_HEIGHT);
        lv_obj_clear_flag(thumb, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(thumb, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_style(thumb, &style_photo_filter_thumb, LV_PART_MAIN);

        lv_obj_t* thumb_img = lv_img_create(thumb);
        lv_img_set_src(thumb_img, photo_filter_configs[i].icon_path);
        lv_obj_add_flag(thumb_img, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_align(thumb_img, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t* label = lv_label_create(item);
        lv_label_set_text(label, photo_filter_configs[i].name);
        lv_obj_add_style(label, &SMALL_SIZE, LV_PART_MAIN);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_add_flag(label, LV_OBJ_FLAG_EVENT_BUBBLE);

        data->filter_items[i] = item;
        data->filter_thumbs[i] = thumb;
        data->filter_labels[i] = label;
    }
    update_filter_selection_style(data);
    align_filter_focus_frame(data);
    scroll_to_filter_index(data, data->selected_filter_index, LV_ANIM_OFF);

    /* 保存 private_data，供 show/hide/destroy 使用 */
    page_set_private_data(data);
}

void page_photo_destroy(void)
{
    page_photo_data_t* data = page_get_private_data();
    if (!data) {
        return;
    }

    /* 删除容器（子元素会自动删除） */
    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    free(data);
}

void page_photo_show(void)
{
    page_photo_data_t* data = page_get_private_data();
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("Photo page show");

    hide_filter_overlay(data);

    /* 显示 UI */
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_photo_hide(void)
{
    page_photo_data_t* data = page_get_private_data();
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("Photo page hide");
    hide_filter_overlay(data);
    /* 隐藏 UI */
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_photo_update(void)
{
    update_resolution_display();
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
