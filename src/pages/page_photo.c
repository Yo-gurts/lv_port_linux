#include "pages/page_photo.h"
#include "font_manager.h"
#include "mlog.h"
#include "styles/style_common.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 布局常量 */
#define TOP_BAR_HEIGHT 40
#define BOTTOM_BAR_HEIGHT 60
#define PREVIEW_TOP_MARGIN 10
#define PREVIEW_BOTTOM_MARGIN 10

/* 分辨率选项 */
static const char* resolutions[] = { "8M", "12M", "24M", "48M", "64M" };
#define RESOLUTION_COUNT (sizeof(resolutions) / sizeof(resolutions[0]))

static void resolution_btn_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    page_photo_data_t* data = (page_photo_data_t*)page_get_private_data(pm, "page_photo_data");
    if (!data) {
        return;
    }

    /* 切换分辨率 */
    data->current_resolution = (data->current_resolution + 1) % RESOLUTION_COUNT;
    lv_label_set_text(data->resolution_label, resolutions[data->current_resolution]);

    MLOG_INFO("Resolution switched to %s", resolutions[data->current_resolution]);
}

static void mode_btn_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    page_photo_data_t* data = (page_photo_data_t*)page_get_private_data(pm, "page_photo_data");
    if (!data) {
        return;
    }

    /* 切换拍照/录像模式 */
    data->is_video_mode = !data->is_video_mode;
    lv_label_set_text(data->mode_label, data->is_video_mode ? "录像" : "拍照");

    MLOG_INFO("Mode switched to %s", data->is_video_mode ? "video" : "photo");
}

static void menu_btn_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    MLOG_INFO("Menu button clicked");
    /* TODO: 打开菜单 */
}

static void filter_btn_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    MLOG_INFO("Filter button clicked");
    /* TODO: 打开滤镜选择 */
}

static void switch_btn_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    MLOG_INFO("Camera switch button clicked");
    /* TODO: 切换前后摄像头 */
}

/* 状态更新定时器回调 */
static void status_update_timer_cb(lv_timer_t* timer)
{
    page_photo_data_t* data = (page_photo_data_t*)lv_timer_get_user_data(timer);
    if (!data) {
        return;
    }

    /* TODO: 更新 SD 卡状态 */
    // static int sd_inserted = 1;
    // lv_label_set_text(data->sd_icon, sd_inserted ? LV_SYMBOL_DIRECTORY : LV_SYMBOL_CLOSE);

    /* TODO: 更新电池电量 */
    // static int battery_level = 80;
    // status_bar_set_battery_icon_by_level(data->battery_icon, battery_level);
}

void page_photo_create(page_manager_t* pm, void* user_data)
{
    if (!pm) {
        return;
    }

    LV_UNUSED(user_data);
    MLOG_INFO("Photo page created");

    page_photo_data_t* data = (page_photo_data_t*)malloc(sizeof(page_photo_data_t));
    if (!data) {
        return;
    }

    memset(data, 0, sizeof(page_photo_data_t));
    data->current_resolution = 0;
    data->is_video_mode = 0;

    data->root = lv_screen_active();

    /* 背景透明 */
    lv_obj_set_style_bg_opa(data->root, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->root, 0, LV_PART_MAIN);

    /* 顶部状态栏区域 */
    data->preview_container = lv_obj_create(data->root);
    lv_obj_set_size(data->preview_container, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(data->preview_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->preview_container, 0, LV_PART_MAIN);
    lv_obj_set_layout(data->preview_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->preview_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(data->preview_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 顶部工具栏：分辨率 | SD | 电池 */
    lv_obj_t* top_bar = lv_obj_create(data->preview_container);
    lv_obj_set_width(top_bar, lv_pct(100));
    lv_obj_set_height(top_bar, TOP_BAR_HEIGHT);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(top_bar, 0, LV_PART_MAIN);
    lv_obj_set_layout(top_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(top_bar, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_right(top_bar, 20, LV_PART_MAIN);

    /* 分辨率按钮 */
    lv_obj_t* resolution_btn = lv_btn_create(top_bar);
    lv_obj_set_height(resolution_btn, 28);
    lv_obj_add_style(resolution_btn, &style_common_btn_back, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(resolution_btn, resolution_btn_cb, LV_EVENT_CLICKED, pm);
    lv_obj_set_style_pad_hor(resolution_btn, 10, LV_PART_MAIN);

    data->resolution_label = lv_label_create(resolution_btn);
    lv_label_set_text(data->resolution_label, resolutions[0]);
    lv_obj_add_style(data->resolution_label, &ttf_font_16, LV_PART_MAIN);

    /* SD卡图标 */
    data->sd_icon = lv_label_create(top_bar);
    lv_label_set_text(data->sd_icon, LV_SYMBOL_DIRECTORY);
    lv_obj_set_style_text_font(data->sd_icon, &lv_font_montserrat_20, LV_PART_MAIN);

    /* 电池图标 */
    data->battery_icon = lv_label_create(top_bar);
    lv_label_set_text(data->battery_icon, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_font(data->battery_icon, &lv_font_montserrat_20, LV_PART_MAIN);

    /* 预览区域（摄像头画面） */
    lv_obj_t* preview_area = lv_obj_create(data->preview_container);
    lv_obj_set_width(preview_area, lv_pct(100));
    lv_obj_set_flex_grow(preview_area, 1);
    lv_obj_set_style_bg_color(preview_area, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(preview_area, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_border_width(preview_area, 0, LV_PART_MAIN);

    /* 预览区域提示文字 */
    lv_obj_t* preview_hint = lv_label_create(preview_area);
    lv_label_set_text(preview_hint, "摄像头预览区域");
    lv_obj_set_style_text_color(preview_hint, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_add_style(preview_hint, &ttf_font_20, LV_PART_MAIN);
    lv_obj_align(preview_hint, LV_ALIGN_CENTER, 0, 0);

    /* 底部工具栏：菜单 | 拍照/录像 | 滤镜 | 切换 */
    lv_obj_t* bottom_bar = lv_obj_create(data->preview_container);
    lv_obj_set_width(bottom_bar, lv_pct(100));
    lv_obj_set_height(bottom_bar, BOTTOM_BAR_HEIGHT);
    lv_obj_set_style_bg_opa(bottom_bar, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(bottom_bar, 0, LV_PART_MAIN);
    lv_obj_set_layout(bottom_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bottom_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 菜单按钮 */
    data->menu_btn = lv_btn_create(bottom_bar);
    lv_obj_set_size(data->menu_btn, 50, 50);
    lv_obj_add_style(data->menu_btn, &style_common_btn_back, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(data->menu_btn, menu_btn_cb, LV_EVENT_CLICKED, pm);
    lv_obj_t* menu_icon = lv_label_create(data->menu_btn);
    lv_label_set_text(menu_icon, LV_SYMBOL_LIST);
    lv_obj_set_style_text_font(menu_icon, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(menu_icon, LV_ALIGN_CENTER, 0, 0);

    /* 拍照/录像模式按钮（中间大按钮） */
    data->mode_btn = lv_btn_create(bottom_bar);
    lv_obj_set_size(data->mode_btn, 80, 80);
    lv_obj_add_style(data->mode_btn, &style_common_btn_back, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(data->mode_btn, mode_btn_cb, LV_EVENT_CLICKED, pm);
    lv_obj_set_style_radius(data->mode_btn, 40, LV_PART_MAIN); /* 圆形 */
    data->mode_label = lv_label_create(data->mode_btn);
    lv_label_set_text(data->mode_label, "拍照");
    lv_obj_add_style(data->mode_label, &ttf_font_20, LV_PART_MAIN);
    lv_obj_align(data->mode_label, LV_ALIGN_CENTER, 0, 0);

    /* 滤镜按钮 */
    data->filter_btn = lv_btn_create(bottom_bar);
    lv_obj_set_size(data->filter_btn, 50, 50);
    lv_obj_add_style(data->filter_btn, &style_common_btn_back, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(data->filter_btn, filter_btn_cb, LV_EVENT_CLICKED, pm);
    lv_obj_t* filter_icon = lv_label_create(data->filter_btn);
    lv_label_set_text(filter_icon, LV_SYMBOL_IMAGE);
    lv_obj_set_style_text_font(filter_icon, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(filter_icon, LV_ALIGN_CENTER, 0, 0);

    /* 摄像头切换按钮 */
    data->switch_btn = lv_btn_create(bottom_bar);
    lv_obj_set_size(data->switch_btn, 50, 50);
    lv_obj_add_style(data->switch_btn, &style_common_btn_back, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(data->switch_btn, switch_btn_cb, LV_EVENT_CLICKED, pm);
    lv_obj_t* switch_icon = lv_label_create(data->switch_btn);
    lv_label_set_text(switch_icon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(switch_icon, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(switch_icon, LV_ALIGN_CENTER, 0, 0);

    /* 创建状态更新定时器，每秒更新一次 */
    data->timer = lv_timer_create(status_update_timer_cb, 1000, data);

    page_set_private_data(pm, "page_photo_data", data);
}

void page_photo_destroy(page_manager_t* pm, void* user_data)
{
    if (!pm) {
        return;
    }

    LV_UNUSED(user_data);
    MLOG_INFO("Photo page destroyed");

    page_photo_data_t* data = (page_photo_data_t*)page_get_private_data(pm, "page_photo_data");
    if (!data) {
        return;
    }

    /* 删除定时器 */
    if (data->timer) {
        lv_timer_delete(data->timer);
        data->timer = NULL;
    }

    /* 删除根对象（子对象会自动删除） */
    if (data->root) {
        lv_obj_del(data->root);
        data->root = NULL;
    }

    free(data);
}

void page_photo_show(page_manager_t* pm, void* user_data)
{
    if (!pm) {
        return;
    }

    LV_UNUSED(user_data);
    MLOG_INFO("Photo page shown");
}

void page_photo_hide(page_manager_t* pm, void* user_data)
{
    if (!pm) {
        return;
    }

    LV_UNUSED(user_data);
    MLOG_INFO("Photo page hidden");
}

void page_photo_update(page_manager_t* pm, void* user_data)
{
    LV_UNUSED(pm);
    LV_UNUSED(user_data);
    /* 由定时器自动更新 */
}
