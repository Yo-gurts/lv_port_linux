#include "pages/page_photo.h"
#include "config.h"
#include "core/page_manager.h"
#include "font_manager.h"
#include "mlog.h"
#include "styles/style_common.h"
#include <stdlib.h>
#include <string.h>

/* 布局常量 */
#define TOP_BAR_HEIGHT 50
#define BOTTOM_BAR_HEIGHT 50

/* 拍照/录像切换回调 */
static void mode_switch_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    MLOG_INFO("Switching to video page");
    page_manager_navigate(pm, "video");
}

/* 菜单按钮回调：返回首页 */
static void menu_back_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    MLOG_INFO("Menu clicked, navigate to home");
    page_manager_navigate(pm, "home");
}

/* 滑动手势回调：从左往右滑返回上一页 */
static void gesture_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            MLOG_INFO("Swipe right, back to home");
            page_manager_back(pm);
        }
    }
}

void page_photo_create(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

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

    /* 启用滑动手势检测 */
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    /* 添加滑动手势回调：从左往右滑返回上一页 */
    lv_obj_add_event_cb(data->container, gesture_cb, LV_EVENT_GESTURE, pm);

    /* =======================
     * 顶部状态栏：8M | 100 | [SD] | [🔋]
     * ======================= */
    data->top_bar = lv_obj_create(data->container);
    lv_obj_set_width(data->top_bar, lv_pct(100));
    lv_obj_set_height(data->top_bar, TOP_BAR_HEIGHT);
    lv_obj_set_style_bg_opa(data->top_bar, LV_OPA_TRANSP, LV_PART_MAIN); /* 全透明 */
    lv_obj_set_style_bg_color(data->top_bar, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(data->top_bar, LV_GRAD_DIR_NONE, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->top_bar, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(data->top_bar, LV_SCROLLBAR_MODE_OFF); /* 不可滚动 */
    lv_obj_set_layout(data->top_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->top_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(data->top_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(data->top_bar, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_right(data->top_bar, 20, LV_PART_MAIN);

    /* 分辨率 Label - 显示 8M */
    data->resolution_label = lv_label_create(data->top_bar);
    lv_label_set_text(data->resolution_label, "8M");
    lv_obj_add_style(data->resolution_label, &ttf_font_20, LV_PART_MAIN);

    /* 剩余照片数量 Label - 显示 100 */
    data->photo_count_label = lv_label_create(data->top_bar);
    lv_label_set_text(data->photo_count_label, "100");
    lv_obj_add_style(data->photo_count_label, &ttf_font_20, LV_PART_MAIN);

    /* SD卡图标 - 默认 offline */
    data->sd_icon = lv_img_create(data->top_bar);
    lv_img_set_src(data->sd_icon, "A:" RES_ICON_PATH "/sd_offline.png");

    /* 电池图标 - 使用 33% 电量 */
    data->battery_icon = lv_img_create(data->top_bar);
    lv_img_set_src(data->battery_icon, "A:" RES_ICON_PATH "/battery33%.png");

    /* =======================
     * 底部工具栏：[📷] | [◫] | [↻] | [≡]
     * ======================= */
    data->bottom_bar = lv_obj_create(data->container);
    lv_obj_set_width(data->bottom_bar, lv_pct(100));
    lv_obj_set_height(data->bottom_bar, BOTTOM_BAR_HEIGHT);
    lv_obj_align(data->bottom_bar, LV_ALIGN_BOTTOM_MID, 0, 0); /* 贴在底部 */
    lv_obj_set_style_bg_opa(data->bottom_bar, LV_OPA_TRANSP, LV_PART_MAIN); /* 全透明 */
    lv_obj_set_style_bg_color(data->bottom_bar, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(data->bottom_bar, LV_GRAD_DIR_NONE, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->bottom_bar, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(data->bottom_bar, LV_SCROLLBAR_MODE_OFF); /* 不可滚动 */
    lv_obj_set_layout(data->bottom_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->bottom_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(data->bottom_bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 拍照/录像切换按钮 */
    data->mode_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->mode_btn, 50, 50);
    lv_obj_add_event_cb(data->mode_btn, mode_switch_cb, LV_EVENT_CLICKED, pm);
    data->mode_img = lv_img_create(data->mode_btn);
    lv_img_set_src(data->mode_img, "A:" RES_ICON_PATH "/Photo.png");
    lv_obj_align(data->mode_img, LV_ALIGN_CENTER, 0, 0);

    /* 滤镜按钮 */
    data->filter_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->filter_btn, 50, 50);
    lv_obj_add_event_cb(data->filter_btn, NULL, LV_EVENT_CLICKED, pm);
    lv_obj_t* filter_icon = lv_img_create(data->filter_btn);
    lv_img_set_src(filter_icon, "A:" RES_ICON_PATH "/filter_default.png");
    lv_obj_align(filter_icon, LV_ALIGN_CENTER, 0, 0);

    /* 摄像头切换按钮 */
    data->switch_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->switch_btn, 50, 50);
    lv_obj_add_event_cb(data->switch_btn, NULL, LV_EVENT_CLICKED, pm);
    lv_obj_t* switch_icon = lv_img_create(data->switch_btn);
    lv_img_set_src(switch_icon, "A:" RES_ICON_PATH "/switch.png");
    lv_obj_align(switch_icon, LV_ALIGN_CENTER, 0, 0);

    /* 菜单按钮 - 点击返回首页 */
    data->menu_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->menu_btn, 50, 50);
    lv_obj_add_event_cb(data->menu_btn, menu_back_cb, LV_EVENT_CLICKED, pm);
    lv_obj_t* menu_icon = lv_img_create(data->menu_btn);
    lv_img_set_src(menu_icon, "A:" RES_ICON_PATH "/menu.png");
    lv_obj_align(menu_icon, LV_ALIGN_CENTER, 0, 0);

    /* 保存 private_data，供 show/hide/destroy 使用 */
    page_set_private_data(pm, data);
}

void page_photo_destroy(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_photo_data_t* data = page_get_private_data(pm);
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

void page_photo_show(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_photo_data_t* data = page_get_private_data(pm);
    if (!data || !data->container) {
        return;
    }

    /* 显示 UI */
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_photo_hide(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_photo_data_t* data = page_get_private_data(pm);
    if (!data || !data->container) {
        return;
    }

    /* 隐藏 UI */
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_photo_update(page_manager_t* pm)
{
    LV_UNUSED(pm);
}
