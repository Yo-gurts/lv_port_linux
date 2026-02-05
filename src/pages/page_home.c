#include "pages/page_home.h"
#include "config.h"
#include "font_manager.h"
#include "mlog.h"
#include "styles/style_common.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 布局常量 - 可根据设计调整 */
#define STATUS_BAR_HEIGHT 30
#define ICON_BUTTON_PADDING 10 /* 图标按钮内边距 */
#define ICON_SIZE 64 /* 图标基础大小 */
#define LABEL_HEIGHT 24 /* 标签高度 */

static const char* get_battery_icon_path(int level)
{
    if (level < 0) {
        level = 0;
    }
    if (level >= 100) {
        return "A:" RES_ICON_PATH "/battery100%.png";
    } else if (level >= 66) {
        return "A:" RES_ICON_PATH "/battery66%.png";
    } else if (level >= 33) {
        return "A:" RES_ICON_PATH "/battery33%.png";
    } else {
        return "A:" RES_ICON_PATH "/battery33%.png";
    }
}

/* ====================== Home Page Callbacks ====================== */
static void photo_button_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    MLOG_INFO("Photo button clicked");
    page_manager_navigate(pm, "photo");
}

static void recognition_button_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    MLOG_INFO("Recognition button clicked");
    page_manager_navigate(pm, "recognition");
}

static void chat_button_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    MLOG_INFO("Chat button clicked");
    page_manager_navigate(pm, "chat");
}

static void translation_button_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    MLOG_INFO("Translation button clicked");
    page_manager_navigate(pm, "translation");
}

static void gallery_button_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    MLOG_INFO("Gallery button clicked");
    page_manager_navigate(pm, "gallery");
}

static void settings_button_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    MLOG_INFO("Settings button clicked");
    page_manager_navigate(pm, "settings");
}

/* 状态栏更新定时器回调 - 每秒更新一次 */
static void home_update_timer_cb(lv_timer_t* timer)
{
    home_page_data_t* data = (home_page_data_t*)lv_timer_get_user_data(timer);
    if (!data || !data->time_label) {
        return;
    }

    /* 更新时间 */
    time_t current_time = time(NULL);
    struct tm* tm_info = localtime(&current_time);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    lv_label_set_text(data->time_label, time_str);

    /* TODO: 更新 WiFi 信号强度 */
    // int wifi_level = get_wifi_level();
    // status_bar_set_wifi_icon(data->status_bar, wifi_level);

    /* TODO: 更新电池电量 */
    // int battery_level = get_battery_level();
    // status_bar_set_battery_icon(data->status_bar, battery_level);
}

static void create_icon_button(page_manager_t* pm, lv_obj_t* parent,
    const char* symbol, const char* name,
    lv_align_t align, int x_ofs, int y_ofs,
    lv_event_cb_t cb, lv_obj_t** out_btn)
{
    /* 容器：图标 + 文字 */
    lv_obj_t* container = lv_btn_create(parent);
    lv_obj_align(container, align, x_ofs, y_ofs);
    lv_obj_set_width(container, lv_pct(30)); /* 占父容器30%宽度，自适应 */
    lv_obj_set_height(container, lv_pct(40)); /* 占剩余高度约40% */
    lv_obj_add_style(container, &style_common_btn_back, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(container, cb, LV_EVENT_CLICKED, pm);

    /* 使用 flex 布局垂直排列图标和文字 */
    lv_obj_set_layout(container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(container, ICON_BUTTON_PADDING, LV_PART_MAIN);

    /* 图标 */
    lv_obj_t* icon = lv_label_create(container);
    lv_label_set_text(icon, symbol);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_add_style(icon, &style_common_label_back, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 文字 - 使用中文字体样式 */
    lv_obj_t* label = lv_label_create(container);
    lv_label_set_text(label, name);
    lv_obj_add_style(label, &ttf_font_24, LV_PART_MAIN);

    if (out_btn) {
        *out_btn = container;
    }
}

void page_home_create(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    home_page_data_t* data = (home_page_data_t*)malloc(sizeof(home_page_data_t));
    if (!data) {
        return;
    }

    memset(data, 0, sizeof(home_page_data_t));

    /* 创建页面容器 */
    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_width(data->container, LV_PCT(100));
    lv_obj_set_height(data->container, LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_add_style(data->container, &style_home_bg, LV_PART_MAIN);
    lv_obj_refr_size(data->container);

    /* Time label - center */
    data->time_label = lv_label_create(data->container);
    lv_label_set_text(data->time_label, "0000-00-00 00:00:00");
    lv_obj_add_style(data->time_label, &ttf_font_26, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->time_label, lv_color_black(), LV_PART_MAIN);
    lv_obj_align(data->time_label, LV_ALIGN_TOP_MID, 0, 10);

    /* Wifi icon - right */
    data->wifi_icon = lv_img_create(data->container);
    lv_img_set_src(data->wifi_icon, "A:" RES_ICON_PATH "/wifi.png");
    lv_obj_align(data->wifi_icon, LV_ALIGN_TOP_RIGHT, -70, 0);

    /* Battery icon - right */
    data->battery_icon = lv_img_create(data->container);
    lv_img_set_src(data->battery_icon, get_battery_icon_path(80));
    lv_obj_align(data->battery_icon, LV_ALIGN_TOP_RIGHT, -10, 5);

    /* 创建状态栏更新定时器，每秒更新一次 */
    data->timer = lv_timer_create(home_update_timer_cb, 1000, data);

    /* 2行3列图标容器 - 使用容器和flex布局实现自适应 */
    data->grid_container = lv_obj_create(data->container);
    lv_obj_set_width(data->grid_container, lv_pct(100));
    lv_obj_set_height(data->grid_container, lv_pct(100));
    lv_obj_set_style_bg_opa(data->grid_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->grid_container, 0, LV_PART_MAIN);
    lv_obj_set_layout(data->grid_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->grid_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(data->grid_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 第1行 */
    create_icon_button(pm, data->grid_container, LV_SYMBOL_IMAGE, "AI拍照",
        LV_ALIGN_TOP_LEFT, 0, 0,
        photo_button_cb, NULL);
    create_icon_button(pm, data->grid_container, LV_SYMBOL_FILE, "识万物",
        LV_ALIGN_TOP_LEFT, 0, 0,
        recognition_button_cb, NULL);
    create_icon_button(pm, data->grid_container, LV_SYMBOL_DIRECTORY, "相册",
        LV_ALIGN_TOP_LEFT, 0, 0,
        gallery_button_cb, NULL);

    /* 第2行 */
    create_icon_button(pm, data->grid_container, LV_SYMBOL_CALL, "AI对话",
        LV_ALIGN_TOP_LEFT, 0, 0,
        chat_button_cb, NULL);
    create_icon_button(pm, data->grid_container, LV_SYMBOL_EDIT, "拍照翻译",
        LV_ALIGN_TOP_LEFT, 0, 0,
        translation_button_cb, NULL);
    create_icon_button(pm, data->grid_container, LV_SYMBOL_SETTINGS, "设置",
        LV_ALIGN_TOP_LEFT, 0, 0,
        settings_button_cb, NULL);

    /* 保存 private_data，供 show/hide/destroy 使用 */
    page_set_private_data(pm, data);
}

void page_home_destroy(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    home_page_data_t* data = page_get_private_data(pm);
    if (!data) {
        return;
    }

    /* 先删除时间更新定时器，防止回调访问已释放的 data */
    if (data->timer) {
        lv_timer_delete(data->timer);
        data->timer = NULL;
    }

    /* 删除容器（子元素会自动删除） */
    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    free(data);
}

void page_home_show(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    home_page_data_t* data = page_get_private_data(pm);
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("Home page show");
    /* 显示 UI */
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);

    /* 恢复定时器 */
    if (data->timer) {
        lv_timer_resume(data->timer);
    }

    /* 更新电池图标 */
    if (data->battery_icon) {
        lv_img_set_src(data->battery_icon, get_battery_icon_path(80));
    }

    /* 立即更新时间 */
    if (data->timer) {
        home_update_timer_cb(data->timer);
    }
}

void page_home_hide(page_manager_t* pm)
{
    home_page_data_t* data = page_get_private_data(pm);
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("Home page hide");
    /* 隐藏 UI */
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);

    /* 暂停定时器，节省资源 */
    if (data->timer) {
        lv_timer_pause(data->timer);
    }
}

void page_home_update(page_manager_t* pm)
{
    LV_UNUSED(pm);
    /* 时间由 timer 自动更新，此处无需处理 */
}
