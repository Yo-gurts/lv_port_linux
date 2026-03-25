// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_video_preview.h"
#include "pages/page_video_album.h"
#include "config.h"
#include "core/file_manager.h"
#include "core/font_manager.h"
#include "core/page_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include "ui/gesture_back.h"
#include "ui/top_notice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PREVIEW_BACK_BTN_SIZE 50

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static int g_initial_video_index = -1;

static int clamp_int(int value, int min_value, int max_value);
static int get_album_total_count(void);
static void format_duration(int duration_sec, char* out_text, size_t out_size);
static void render_current_video(page_video_preview_data_t* data);
static void show_prev_video(page_video_preview_data_t* data);
static void show_next_video(page_video_preview_data_t* data);
static int get_current_video_index(const page_video_preview_data_t* data);
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
    if (file_manager_refresh_video_list() != 0)
        MLOG_WARN("Video preview refresh list failed before counting");

    return file_manager_get_video_count();
}

static void format_duration(int duration_sec, char* out_text, size_t out_size)
{
    int minute;
    int second;

    if (!out_text || out_size == 0)
        return;

    if (duration_sec < 0)
        duration_sec = 0;
    minute = duration_sec / 60;
    second = duration_sec % 60;
    if (snprintf(out_text, out_size, "%02d:%02d", minute, second) >= (int)out_size)
        out_text[0] = '\0';
}

static void render_current_video(page_video_preview_data_t* data)
{
    char image_path[FILE_MANAGER_MAX_PATH_LEN];
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char text[32];
    int duration_sec = 0;
    int video_index;

    if (!data || !data->image || !data->file_name_label || !data->duration_label)
        return;

    if (data->total_videos <= 0) {
        lv_img_set_src(data->image, NULL);
        lv_label_set_text(data->file_name_label, "");
        lv_label_set_text(data->duration_label, "00:00");
        top_notice_show_for("0/0", TOP_NOTICE_TYPE_INFO, 3000);
        return;
    }

    data->current_display_index = clamp_int(data->current_display_index, 0, data->total_videos - 1);
    video_index = data->current_display_index;

    if (file_manager_get_video_subpic_path(video_index, image_path, sizeof(image_path), FILE_PATH_LV) == 0)
        lv_img_set_src(data->image, image_path);
    else if (file_manager_get_video_thumbnail_path(video_index, image_path, sizeof(image_path), FILE_PATH_LV) == 0)
        lv_img_set_src(data->image, image_path);
    else
        lv_img_set_src(data->image, "A:" RES_ICON_PATH "/video.png");

    if (file_manager_get_video_name(video_index, file_name, sizeof(file_name)) == 0)
        lv_label_set_text(data->file_name_label, file_name);
    else
        lv_label_set_text(data->file_name_label, "");

    if (file_manager_get_video_duration_sec(video_index, &duration_sec) != 0)
        duration_sec = 0;
    format_duration(duration_sec, text, sizeof(text));
    lv_label_set_text(data->duration_label, text);

    if (snprintf(text, sizeof(text), "%d/%d", data->current_display_index + 1, data->total_videos) < (int)sizeof(text))
        top_notice_show_for(text, TOP_NOTICE_TYPE_INFO, 3000);
}

static void show_prev_video(page_video_preview_data_t* data)
{
    if (!data || data->total_videos <= 0)
        return;
    if (data->current_display_index <= 0)
        data->current_display_index = data->total_videos - 1;
    else
        data->current_display_index--;
    render_current_video(data);
}

static void show_next_video(page_video_preview_data_t* data)
{
    if (!data || data->total_videos <= 0)
        return;
    if (data->current_display_index >= data->total_videos - 1)
        data->current_display_index = 0;
    else
        data->current_display_index++;
    render_current_video(data);
}

static int get_current_video_index(const page_video_preview_data_t* data)
{
    if (!data || data->total_videos <= 0)
        return -1;

    return clamp_int(data->current_display_index, 0, data->total_videos - 1);
}

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

static void back_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_video_preview_data_t* data = (page_video_preview_data_t*)page_get_private_data();
    int current_video_index = get_current_video_index(data);
    if (current_video_index >= 0)
        page_video_album_set_focus_video_index(current_video_index);
    page_manager_back();
}

static void gesture_event_cb(lv_event_t* e)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)lv_event_get_user_data(e);
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
        show_next_video(data);
    else if (dir == LV_DIR_RIGHT)
        show_prev_video(data);

    lv_indev_wait_release(indev);
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_video_preview_set_initial_video_index(int video_index)
{
    g_initial_video_index = video_index;
}

void page_video_preview_create(void)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)malloc(sizeof(page_video_preview_data_t));
    lv_obj_t* back_icon;
    if (!data)
        return;

    memset(data, 0, sizeof(page_video_preview_data_t));
    data->current_display_index = 0;

    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(data->container, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_set_layout(data->container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(data->container, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_GESTURE_BUBBLE);
    gesture_back_register_events(data->container);
    gesture_back_set_left_edge_swipe_cb(data->container, back_btn_cb);
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
    lv_obj_set_width(data->file_name_label, H_RES - PREVIEW_BACK_BTN_SIZE - 80);
    lv_obj_set_style_text_align(data->file_name_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->file_name_label, lv_color_white(), LV_PART_MAIN);
    lv_label_set_long_mode(data->file_name_label, LV_LABEL_LONG_DOT);
    lv_obj_add_style(data->file_name_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->file_name_label, LV_ALIGN_TOP_MID, 0, 15);

    data->duration_label = lv_label_create(data->container);
    lv_obj_add_flag(data->duration_label, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(data->duration_label, LV_OBJ_FLAG_FLOATING);
    lv_obj_add_style(data->duration_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->duration_label, lv_color_hex(0xD0D0D0), LV_PART_MAIN);
    lv_obj_align(data->duration_label, LV_ALIGN_TOP_RIGHT, -14, 17);
    lv_label_set_text(data->duration_label, "00:00");

    data->play_hint = lv_label_create(data->container);
    lv_obj_add_flag(data->play_hint, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(data->play_hint, LV_OBJ_FLAG_FLOATING);
    lv_obj_add_style(data->play_hint, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->play_hint, lv_color_hex(0xDADADA), LV_PART_MAIN);
    lv_label_set_text(data->play_hint, "视频播放页（播放器接入点）");
    lv_obj_align(data->play_hint, LV_ALIGN_BOTTOM_MID, 0, -14);

    gesture_back_enable_event_bubble_recursive(data->container);
    page_set_private_data(data);
}

void page_video_preview_destroy(void)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)page_get_private_data();
    if (!data)
        return;

    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    free(data);
}

void page_video_preview_show(void)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)page_get_private_data();
    if (!data || !data->container)
        return;

    gesture_back_set_left_edge_swipe_cb(data->container, back_btn_cb);

    if (!file_manager_is_storage_ready()) {
        top_notice_show("SD卡未插入", TOP_NOTICE_TYPE_WARNING);
        page_manager_back();
        return;
    }

    data->total_videos = get_album_total_count();
    if (data->total_videos <= 0)
        data->current_display_index = 0;
    else if (g_initial_video_index >= 0)
        data->current_display_index = clamp_int(g_initial_video_index, 0, data->total_videos - 1);
    else
        data->current_display_index = clamp_int(data->current_display_index, 0, data->total_videos - 1);
    g_initial_video_index = -1;

    render_current_video(data);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_video_preview_hide(void)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)page_get_private_data();
    if (!data || !data->container)
        return;

    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_video_preview_update(void)
{
    /* no-op */
}
