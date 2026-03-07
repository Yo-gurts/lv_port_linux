// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_photo_preview.h"
#include "pages/page_album.h"
#include "config.h"
#include "core/file_manager.h"
#include "core/font_manager.h"
#include "core/page_manager.h"
#include "core/style_manager.h"
#include "ui/top_notice.h"
#include "mlog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PREVIEW_BACK_BTN_SIZE 50

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static int g_initial_photo_index = -1;

static int clamp_int(int value, int min_value, int max_value);
static int get_album_total_count(void);
static int photo_index_to_display_index(int total_photos, int photo_index);
static int display_index_to_photo_index(int total_photos, int display_index);
static void render_current_photo(page_photo_preview_data_t* data);
static void show_prev_photo(page_photo_preview_data_t* data);
static void show_next_photo(page_photo_preview_data_t* data);
static int get_current_photo_index(const page_photo_preview_data_t* data);
static void back_btn_cb(lv_event_t* e);
static void gesture_event_cb(lv_event_t* e);

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

static int clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value)
        return min_value;
    if (value > max_value)
        return max_value;
    return value;
}

static int get_album_total_count(void)
{
    if (file_manager_refresh_photo_list() != 0)
        MLOG_WARN("Album preview refresh photo list failed before counting");

    return file_manager_get_photo_count();
}

static int photo_index_to_display_index(int total_photos, int photo_index)
{
    return total_photos - 1 - photo_index;
}

static int display_index_to_photo_index(int total_photos, int display_index)
{
    return total_photos - 1 - display_index;
}

static void render_current_photo(page_photo_preview_data_t* data)
{
    char subpic_path[FILE_MANAGER_MAX_PATH_LEN];
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char text[32];
    int photo_index;

    if (!data || !data->image || !data->file_name_label)
        return;

    if (data->total_photos <= 0) {
        lv_img_set_src(data->image, NULL);
        lv_label_set_text(data->file_name_label, "");
        top_notice_show_for("0/0", TOP_NOTICE_TYPE_INFO, 3000);
        return;
    }

    data->current_display_index = clamp_int(data->current_display_index, 0, data->total_photos - 1);
    photo_index = display_index_to_photo_index(data->total_photos, data->current_display_index);

    if (file_manager_get_photo_subpic_path(photo_index, subpic_path, sizeof(subpic_path)) == 0)
        lv_img_set_src(data->image, subpic_path);
    else
        lv_img_set_src(data->image, NULL);
    if (file_manager_get_photo_name(photo_index, file_name, sizeof(file_name)) == 0)
        lv_label_set_text(data->file_name_label, file_name);
    else
        lv_label_set_text(data->file_name_label, "");

    if (snprintf(text, sizeof(text), "%d/%d", data->current_display_index + 1, data->total_photos) < (int)sizeof(text))
        top_notice_show_for(text, TOP_NOTICE_TYPE_INFO, 3000);
}

static void show_prev_photo(page_photo_preview_data_t* data)
{
    if (!data || data->total_photos <= 0)
        return;
    if (data->current_display_index <= 0)
        return;

    data->current_display_index--;
    render_current_photo(data);
}

static void show_next_photo(page_photo_preview_data_t* data)
{
    if (!data || data->total_photos <= 0)
        return;
    if (data->current_display_index >= data->total_photos - 1)
        return;

    data->current_display_index++;
    render_current_photo(data);
}

static int get_current_photo_index(const page_photo_preview_data_t* data)
{
    int display_index;

    if (!data || data->total_photos <= 0)
        return -1;

    display_index = clamp_int(data->current_display_index, 0, data->total_photos - 1);
    return data->total_photos - 1 - display_index;
}

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

static void back_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_photo_preview_data_t* data = (page_photo_preview_data_t*)page_get_private_data();
    int current_photo_index = get_current_photo_index(data);
    if (current_photo_index >= 0)
        page_album_set_focus_photo_index(current_photo_index);
    page_manager_back();
}

static void gesture_event_cb(lv_event_t* e)
{
    page_photo_preview_data_t* data = (page_photo_preview_data_t*)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t* indev;
    lv_dir_t dir;

    if (!data || code != LV_EVENT_GESTURE)
        return;

    indev = lv_indev_get_act();
    if (!indev)
        return;

    dir = lv_indev_get_gesture_dir(indev);
    if (dir == LV_DIR_LEFT)
        show_next_photo(data);
    else if (dir == LV_DIR_RIGHT)
        show_prev_photo(data);

    lv_indev_wait_release(indev);
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_photo_preview_set_initial_photo_index(int photo_index)
{
    g_initial_photo_index = photo_index;
}

void page_photo_preview_create(void)
{
    page_photo_preview_data_t* data = (page_photo_preview_data_t*)malloc(sizeof(page_photo_preview_data_t));
    lv_obj_t* back_icon;
    if (!data)
        return;

    memset(data, 0, sizeof(page_photo_preview_data_t));
    data->current_display_index = 0;

    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(data->container, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_set_layout(data->container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(data->container, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(data->container, gesture_event_cb, LV_EVENT_GESTURE, data);

    data->image_area = lv_obj_create(data->container);
    lv_obj_set_size(data->image_area, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->image_area, &style_noboarder, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->image_area, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(data->image_area, 0, LV_PART_MAIN);
    lv_obj_clear_flag(data->image_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(data->image_area, LV_SCROLLBAR_MODE_OFF);

    data->image = lv_img_create(data->image_area);
    lv_obj_center(data->image);

    data->back_btn = lv_btn_create(data->container);
    lv_obj_set_size(data->back_btn, PREVIEW_BACK_BTN_SIZE, PREVIEW_BACK_BTN_SIZE);
    lv_obj_add_flag(data->back_btn, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(data->back_btn, LV_OBJ_FLAG_FLOATING);
    lv_obj_add_style(data->back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_align(data->back_btn, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_event_cb(data->back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    back_icon = lv_img_create(data->back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle-white.png");
    lv_obj_center(back_icon);

    data->file_name_label = lv_label_create(data->container);
    lv_obj_add_flag(data->file_name_label, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(data->file_name_label, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_width(data->file_name_label, H_RES - PREVIEW_BACK_BTN_SIZE - 20);
    lv_obj_set_style_text_align(data->file_name_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->file_name_label, lv_color_white(), LV_PART_MAIN);
    lv_label_set_long_mode(data->file_name_label, LV_LABEL_LONG_DOT);
    lv_obj_align(data->file_name_label, LV_ALIGN_TOP_MID, 0, 16);
    lv_label_set_text(data->file_name_label, "");

    page_set_private_data(data);
}

void page_photo_preview_destroy(void)
{
    page_photo_preview_data_t* data = (page_photo_preview_data_t*)page_get_private_data();
    if (!data)
        return;

    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    free(data);
}

void page_photo_preview_show(void)
{
    page_photo_preview_data_t* data = (page_photo_preview_data_t*)page_get_private_data();
    if (!data || !data->container)
        return;

    data->total_photos = get_album_total_count();
    if (data->total_photos <= 0) {
        data->current_display_index = 0;
        g_initial_photo_index = -1;
        render_current_photo(data);
        lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (g_initial_photo_index >= 0) {
        int clamped_photo_index = clamp_int(g_initial_photo_index, 0, data->total_photos - 1);
        data->current_display_index = photo_index_to_display_index(data->total_photos, clamped_photo_index);
        g_initial_photo_index = -1;
    } else {
        data->current_display_index = clamp_int(data->current_display_index, 0, data->total_photos - 1);
    }

    render_current_photo(data);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_photo_preview_hide(void)
{
    page_photo_preview_data_t* data = (page_photo_preview_data_t*)page_get_private_data();
    if (!data || !data->container)
        return;

    top_notice_hide();
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_photo_preview_update(void)
{
    /* no-op */
}

// #endregion
