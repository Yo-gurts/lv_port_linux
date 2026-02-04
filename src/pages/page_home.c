#include "pages/page_home.h"
#include "font_manager.h"
#include "styles/style_common.h"
#include "ui/status_bar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 布局常量 - 可根据设计调整 */
#define STATUS_BAR_HEIGHT 30
#define ICON_BUTTON_PADDING 10  /* 图标按钮内边距 */
#define ICON_SIZE 64            /* 图标基础大小 */
#define LABEL_HEIGHT 24         /* 标签高度 */

static void photo_button_cb(lv_event_t *e)
{
    page_manager_t *pm = (page_manager_t *)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    printf("Photo button clicked\n");
    page_manager_navigate(pm, "photo");
}

static void video_button_cb(lv_event_t *e)
{
    page_manager_t *pm = (page_manager_t *)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    printf("Video button clicked\n");
    page_manager_navigate(pm, "video");
}

static void gallery_button_cb(lv_event_t *e)
{
    page_manager_t *pm = (page_manager_t *)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    printf("Gallery button clicked\n");
    page_manager_navigate(pm, "gallery");
}

static void settings_button_cb(lv_event_t *e)
{
    page_manager_t *pm = (page_manager_t *)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    printf("Settings button clicked\n");
    page_manager_navigate(pm, "settings");
}

static void create_icon_button(page_manager_t *pm, lv_obj_t *parent,
                               const char *symbol, const char *name,
                               lv_align_t align, int x_ofs, int y_ofs,
                               lv_event_cb_t cb, lv_obj_t **out_btn)
{
    /* 容器：图标 + 文字 */
    lv_obj_t *container = lv_btn_create(parent);
    lv_obj_align(container, align, x_ofs, y_ofs);
    lv_obj_set_width(container, lv_pct(30));  /* 占父容器30%宽度，自适应 */
    lv_obj_set_height(container, lv_pct(40));  /* 占剩余高度约40% */
    lv_obj_add_style(container, &style_common_btn_back, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(container, cb, LV_EVENT_CLICKED, pm);

    /* 使用 flex 布局垂直排列图标和文字 */
    lv_obj_set_layout(container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(container, ICON_BUTTON_PADDING, LV_PART_MAIN);

    /* 图标 */
    lv_obj_t *icon = lv_label_create(container);
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

void page_home_create(page_manager_t *pm, void *user_data)
{
    if (!pm) {
        return;
    }

    printf("Home page created\n");

    home_page_data_t *data = (home_page_data_t *)malloc(sizeof(home_page_data_t));
    if (!data) {
        return;
    }

    memset(data, 0, sizeof(home_page_data_t));

    data->root = lv_screen_active();

    /* Create status bar */
    data->status_bar = status_bar_create(data->root);

    status_bar_item_t time_item = {
        .type = STATUS_BAR_ITEM_TYPE_TEXT,
        .text = "12:30",
        .visible = true
    };
    status_bar_add_item(data->status_bar, &time_item);

    /* 2行3列图标容器 - 使用容器和flex布局实现自适应 */
    data->grid_container = lv_obj_create(data->root);
    lv_obj_set_width(data->grid_container, lv_pct(100));
    lv_obj_set_height(data->grid_container, lv_pct(100));
    lv_obj_set_style_bg_opa(data->grid_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->grid_container, 0, LV_PART_MAIN);
    lv_obj_set_layout(data->grid_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->grid_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(data->grid_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 第1行 */
    create_icon_button(pm, data->grid_container, LV_SYMBOL_IMAGE, "拍照",
                       LV_ALIGN_TOP_LEFT, 0, 0,
                       photo_button_cb, &data->photo_button);
    create_icon_button(pm, data->grid_container, LV_SYMBOL_VIDEO, "录像",
                       LV_ALIGN_TOP_LEFT, 0, 0,
                       video_button_cb, &data->video_button);
    create_icon_button(pm, data->grid_container, LV_SYMBOL_DIRECTORY, "相册",
                       LV_ALIGN_TOP_LEFT, 0, 0,
                       gallery_button_cb, &data->gallery_button);

    /* 第2行 */
    create_icon_button(pm, data->grid_container, LV_SYMBOL_SETTINGS, "设置",
                       LV_ALIGN_TOP_LEFT, 0, 0,
                       settings_button_cb, &data->settings_button);
    /* 预留2个位置 */
    create_icon_button(pm, data->grid_container, LV_SYMBOL_PLAY, "预览",
        LV_ALIGN_TOP_LEFT, 0, 0,
        NULL, NULL);
    create_icon_button(pm, data->grid_container, LV_SYMBOL_REFRESH, "More",
        LV_ALIGN_TOP_LEFT, 0, 0,
        NULL, NULL);

    page_set_private_data(pm, "home_page_data", data);
}

void page_home_destroy(page_manager_t *pm, void *user_data)
{
    if (!pm) {
        return;
    }

    printf("Home page destroyed\n");

    home_page_data_t *data = (home_page_data_t *)page_get_private_data(pm, "home_page_data");
    if (!data) {
        return;
    }

    /* 先删除网格容器（子元素会自动删除） */
    if (data->grid_container) {
        lv_obj_del(data->grid_container);
        data->grid_container = NULL;
    }

    /* 按钮已随容器删除，无需单独处理 */
    data->photo_button = NULL;
    data->video_button = NULL;
    data->gallery_button = NULL;
    data->settings_button = NULL;

    if (data->status_bar) {
        status_bar_destroy(data->status_bar);
        data->status_bar = NULL;
    }

    free(data);
}

void page_home_show(page_manager_t *pm, void *user_data)
{
    if (!pm) {
        return;
    }

    printf("Home page shown\n");

    home_page_data_t *data = (home_page_data_t *)page_get_private_data(pm, "home_page_data");
    if (!data) {
        return;
    }

    if (data->status_bar) {
        status_bar_refresh(data->status_bar);
    }
}

void page_home_hide(page_manager_t *pm, void *user_data)
{
    if (!pm) {
        return;
    }

    printf("Home page hidden\n");
}

void page_home_update(page_manager_t *pm, void *user_data)
{
    if (!pm) {
        return;
    }

    home_page_data_t *data = (home_page_data_t *)page_get_private_data(pm, "home_page_data");
    if (!data) {
        return;
    }

    if (data->status_bar) {
        time_t current_time = time(NULL);
        struct tm *tm_info = localtime(&current_time);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%H:%M", tm_info);
        status_bar_set_item_text(data->status_bar, 0, time_str);
    }
}
