// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_video_preview.h"
#include "pages/page_video_album.h"
#include "config.h"
#include "core/file_manager.h"
#include "core/font_manager.h"
#include "core/page_manager.h"
#include "core/player_manager.h"
#include "core/power_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include "ui/gesture_back.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PREVIEW_BACK_BTN_SIZE 50
#define SIDE_BTN_SIZE 66
#define PLAY_BTN_SIZE SIDE_BTN_SIZE
#define CONTROL_ROW_GAP 110
#define PROGRESS_TIMER_MS 1000
#define PROGRESS_FALLBACK_SEC 180

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static int g_initial_video_index = -1;

static int clamp_int(int value, int min_value, int max_value);
static int get_album_total_count(void);
static void format_duration_pair(int current_sec, int total_sec, char* out_text, size_t out_size);
static void update_play_pause_symbol(page_video_preview_data_t* data);
static void update_switch_buttons_state(page_video_preview_data_t* data);
static void update_progress_display(page_video_preview_data_t* data);
static void set_controls_visible(page_video_preview_data_t* data, bool visible);
static void set_auto_sleep_block(page_video_preview_data_t* data, bool blocked);
static void set_paused_state(page_video_preview_data_t* data, bool paused);
static int get_current_video_index(const page_video_preview_data_t* data);
static void render_current_video(page_video_preview_data_t* data, bool reset_progress);
static void try_switch_video(page_video_preview_data_t* data, int delta);

static void back_btn_cb(lv_event_t* e);
static void center_play_pause_btn_cb(lv_event_t* e);
static void prev_btn_cb(lv_event_t* e);
static void next_btn_cb(lv_event_t* e);
static void progress_slider_event_cb(lv_event_t* e);
static void container_click_cb(lv_event_t* e);
static void progress_timer_cb(lv_timer_t* timer);

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

static void format_duration_pair(int current_sec, int total_sec, char* out_text, size_t out_size)
{
    int c_min;
    int c_sec;
    int t_min;
    int t_sec;

    if (!out_text || out_size == 0)
        return;

    if (current_sec < 0)
        current_sec = 0;
    if (total_sec < 0)
        total_sec = 0;

    c_min = current_sec / 60;
    c_sec = current_sec % 60;
    t_min = total_sec / 60;
    t_sec = total_sec % 60;
    if (snprintf(out_text, out_size, "%02d:%02d / %02d:%02d", c_min, c_sec, t_min, t_sec) >= (int)out_size)
        out_text[0] = '\0';
}

static void update_play_pause_symbol(page_video_preview_data_t* data)
{
    if (!data || !data->center_play_pause_icon)
        return;

    lv_img_set_src(data->center_play_pause_icon,
                   data->is_paused ? "A:" RES_ICON_PATH "/video-play.png" : "A:" RES_ICON_PATH "/video-pause.png");
}

static void update_switch_buttons_state(page_video_preview_data_t* data)
{
    bool has_prev;
    bool has_next;

    if (!data || !data->prev_btn || !data->next_btn)
        return;

    has_prev = data->total_videos > 0 && data->current_display_index > 0;
    has_next = data->total_videos > 0 && data->current_display_index < data->total_videos - 1;

    if (has_prev) {
        lv_obj_clear_state(data->prev_btn, LV_STATE_DISABLED);
        lv_obj_set_style_opa(data->prev_btn, LV_OPA_100, LV_PART_MAIN);
    } else {
        lv_obj_add_state(data->prev_btn, LV_STATE_DISABLED);
        lv_obj_set_style_opa(data->prev_btn, LV_OPA_40, LV_PART_MAIN);
    }

    if (has_next) {
        lv_obj_clear_state(data->next_btn, LV_STATE_DISABLED);
        lv_obj_set_style_opa(data->next_btn, LV_OPA_100, LV_PART_MAIN);
    } else {
        lv_obj_add_state(data->next_btn, LV_STATE_DISABLED);
        lv_obj_set_style_opa(data->next_btn, LV_OPA_40, LV_PART_MAIN);
    }
}

static void update_progress_display(page_video_preview_data_t* data)
{
    char text[40];

    if (!data || !data->progress_slider || !data->progress_time_label)
        return;

    if (data->total_duration_sec <= 0) {
        lv_slider_set_range(data->progress_slider, 0, 0);
        lv_slider_set_value(data->progress_slider, 0, LV_ANIM_OFF);
        lv_obj_add_state(data->progress_slider, LV_STATE_DISABLED);
    } else {
        lv_slider_set_range(data->progress_slider, 0, data->total_duration_sec);
        lv_slider_set_value(data->progress_slider, clamp_int(data->current_sec, 0, data->total_duration_sec), LV_ANIM_OFF);
        lv_obj_clear_state(data->progress_slider, LV_STATE_DISABLED);
    }

    format_duration_pair(data->current_sec, data->total_duration_sec, text, sizeof(text));
    lv_label_set_text(data->progress_time_label, text);
}

static void set_controls_visible(page_video_preview_data_t* data, bool visible)
{
    if (!data || !data->control_layer)
        return;

    if (visible)
        lv_obj_clear_flag(data->control_layer, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(data->control_layer, LV_OBJ_FLAG_HIDDEN);
}

static void set_auto_sleep_block(page_video_preview_data_t* data, bool blocked)
{
    if (!data)
        return;

    if (blocked) {
        if (!data->auto_sleep_blocked) {
            power_manager_disable_auto_sleep();
            data->auto_sleep_blocked = true;
        }
    } else {
        if (data->auto_sleep_blocked) {
            power_manager_enable_auto_sleep();
            data->auto_sleep_blocked = false;
        }
    }
}

static void set_paused_state(page_video_preview_data_t* data, bool paused)
{
    bool effective_paused;

    if (!data)
        return;

    effective_paused = data->is_paused;

    if (!paused) {
        if (player_manager_play() != 0) {
            MLOG_WARN("video preview play failed, keep paused");
        } else {
            effective_paused = false;
        }
    } else {
        if (player_manager_pause() != 0) {
            MLOG_WARN("video preview pause failed");
        } else {
            effective_paused = true;
        }
    }

    data->is_paused = effective_paused;
    set_auto_sleep_block(data, !effective_paused);
    update_play_pause_symbol(data);
    set_controls_visible(data, effective_paused);
}

static int get_current_video_index(const page_video_preview_data_t* data)
{
    if (!data || data->total_videos <= 0)
        return -1;

    return clamp_int(data->current_display_index, 0, data->total_videos - 1);
}

static void render_current_video(page_video_preview_data_t* data, bool reset_progress)
{
    char image_path[FILE_MANAGER_MAX_PATH_LEN];
    char video_path[FILE_MANAGER_MAX_PATH_LEN];
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    int duration_sec = 0;
    int video_index;

    if (!data || !data->image || !data->file_name_label)
        return;

    if (data->total_videos <= 0) {
        (void)player_manager_stop();
        lv_img_set_src(data->image, NULL);
        lv_label_set_text(data->file_name_label, "");
        data->total_duration_sec = 0;
        data->current_sec = 0;
        update_switch_buttons_state(data);
        update_progress_display(data);
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

    if (file_manager_get_video_duration_sec(video_index, &duration_sec) != 0 || duration_sec <= 0)
        duration_sec = PROGRESS_FALLBACK_SEC;
    data->total_duration_sec = duration_sec;

    if (file_manager_get_video_path(video_index, video_path, sizeof(video_path), FILE_PATH_REAL) == 0) {
        if (player_manager_prepare(video_path) != 0)
            MLOG_WARN("video preview prepare failed: %s", video_path);
    } else {
        MLOG_WARN("video preview get video path failed: index=%d", video_index);
    }

    if (reset_progress)
        data->current_sec = 0;
    data->current_sec = clamp_int(data->current_sec, 0, data->total_duration_sec);

    update_switch_buttons_state(data);
    update_progress_display(data);
}

static void try_switch_video(page_video_preview_data_t* data, int delta)
{
    int target_index;

    if (!data || data->total_videos <= 0)
        return;

    target_index = data->current_display_index + delta;
    if (target_index < 0 || target_index >= data->total_videos)
        return;

    data->current_display_index = target_index;
    render_current_video(data, true);
    set_paused_state(data, true);
}

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

static void progress_timer_cb(lv_timer_t* timer)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)lv_timer_get_user_data(timer);
    int current_sec;
    int total_sec;

    if (!data)
        return;
    if (data->is_paused || data->is_dragging_progress)
        return;
    if (player_manager_get_progress(&current_sec, &total_sec) != 0)
        return;

    if (total_sec > 0)
        data->total_duration_sec = total_sec;
    data->current_sec = current_sec;

    if (data->total_duration_sec > 0 && data->current_sec >= data->total_duration_sec) {
        data->current_sec = data->total_duration_sec;
        update_progress_display(data);
        set_paused_state(data, true);
        return;
    }

    update_progress_display(data);
}

static void back_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_video_preview_data_t* data = (page_video_preview_data_t*)page_get_private_data();
    int current_video_index = get_current_video_index(data);
    (void)player_manager_stop();
    if (current_video_index >= 0)
        page_video_album_set_focus_video_index(current_video_index);
    page_manager_back();
}

static void center_play_pause_btn_cb(lv_event_t* e)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)lv_event_get_user_data(e);
    if (!data)
        return;

    set_paused_state(data, !data->is_paused);
}

static void prev_btn_cb(lv_event_t* e)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)lv_event_get_user_data(e);
    if (!data)
        return;

    try_switch_video(data, -1);
}

static void next_btn_cb(lv_event_t* e)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)lv_event_get_user_data(e);
    if (!data)
        return;

    try_switch_video(data, 1);
}

static void progress_slider_event_cb(lv_event_t* e)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (!data || !data->progress_slider)
        return;

    if (code == LV_EVENT_PRESSED) {
        data->is_dragging_progress = true;
        set_paused_state(data, true);
        return;
    }

    if (code == LV_EVENT_VALUE_CHANGED) {
        data->current_sec = lv_slider_get_value(data->progress_slider);
        update_progress_display(data);
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        data->is_dragging_progress = false;
        if (player_manager_seek_sec(data->current_sec) != 0)
            MLOG_WARN("video preview seek failed: sec=%d", data->current_sec);
    }
}

static void container_click_cb(lv_event_t* e)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)lv_event_get_user_data(e);

    if (!data)
        return;

    /* 播放中点击画面 -> 暂停并显示控制层 */
    if (!data->is_paused)
        set_paused_state(data, true);
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
    data->is_paused = true;
    if (player_manager_init() != 0)
        MLOG_WARN("video preview player manager init failed");

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
    lv_obj_add_event_cb(data->container, container_click_cb, LV_EVENT_CLICKED, data);

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

    data->control_layer = lv_obj_create(data->container);
    lv_obj_add_flag(data->control_layer, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(data->control_layer, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_size(data->control_layer, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->control_layer, &style_noboarder, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->control_layer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_move_foreground(data->back_btn);

    data->file_name_label = lv_label_create(data->control_layer);
    lv_obj_set_width(data->file_name_label, H_RES - PREVIEW_BACK_BTN_SIZE - 80);
    lv_obj_set_style_text_align(data->file_name_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->file_name_label, lv_color_white(), LV_PART_MAIN);
    lv_label_set_long_mode(data->file_name_label, LV_LABEL_LONG_DOT);
    lv_obj_add_style(data->file_name_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->file_name_label, LV_ALIGN_TOP_MID, 0, 20);

    data->center_play_pause_btn = lv_btn_create(data->control_layer);
    lv_obj_set_size(data->center_play_pause_btn, PLAY_BTN_SIZE, PLAY_BTN_SIZE);
    lv_obj_set_style_bg_opa(data->center_play_pause_btn, LV_OPA_60, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->center_play_pause_btn, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(data->center_play_pause_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(data->center_play_pause_btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_align(data->center_play_pause_btn, LV_ALIGN_CENTER, 0, 6);
    lv_obj_add_event_cb(data->center_play_pause_btn, center_play_pause_btn_cb, LV_EVENT_CLICKED, data);

    data->center_play_pause_icon = lv_img_create(data->center_play_pause_btn);
    lv_obj_center(data->center_play_pause_icon);

    data->prev_btn = lv_btn_create(data->control_layer);
    lv_obj_set_size(data->prev_btn, SIDE_BTN_SIZE, SIDE_BTN_SIZE);
    lv_obj_set_style_bg_opa(data->prev_btn, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->prev_btn, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(data->prev_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(data->prev_btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_align_to(data->prev_btn, data->center_play_pause_btn, LV_ALIGN_OUT_LEFT_MID, -CONTROL_ROW_GAP, 0);
    lv_obj_add_event_cb(data->prev_btn, prev_btn_cb, LV_EVENT_CLICKED, data);

    data->prev_icon = lv_img_create(data->prev_btn);
    lv_img_set_src(data->prev_icon, "A:" RES_ICON_PATH "/video-prev.png");
    lv_obj_center(data->prev_icon);

    data->next_btn = lv_btn_create(data->control_layer);
    lv_obj_set_size(data->next_btn, SIDE_BTN_SIZE, SIDE_BTN_SIZE);
    lv_obj_set_style_bg_opa(data->next_btn, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->next_btn, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(data->next_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(data->next_btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_align_to(data->next_btn, data->center_play_pause_btn, LV_ALIGN_OUT_RIGHT_MID, CONTROL_ROW_GAP, 0);
    lv_obj_add_event_cb(data->next_btn, next_btn_cb, LV_EVENT_CLICKED, data);

    data->next_icon = lv_img_create(data->next_btn);
    lv_img_set_src(data->next_icon, "A:" RES_ICON_PATH "/video-next.png");
    lv_obj_center(data->next_icon);

    data->progress_slider = lv_slider_create(data->control_layer);
    lv_obj_set_size(data->progress_slider, LV_PCT(86), 8);
    lv_obj_align(data->progress_slider, LV_ALIGN_BOTTOM_MID, 0, -46);
    lv_obj_set_style_radius(data->progress_slider, 4, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->progress_slider, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->progress_slider, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->progress_slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(data->progress_slider, lv_color_hex(0xFF2D2D), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(data->progress_slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_bg_color(data->progress_slider, lv_color_white(), LV_PART_KNOB);
    lv_obj_add_event_cb(data->progress_slider, progress_slider_event_cb, LV_EVENT_PRESSED, data);
    lv_obj_add_event_cb(data->progress_slider, progress_slider_event_cb, LV_EVENT_VALUE_CHANGED, data);
    lv_obj_add_event_cb(data->progress_slider, progress_slider_event_cb, LV_EVENT_RELEASED, data);
    lv_obj_add_event_cb(data->progress_slider, progress_slider_event_cb, LV_EVENT_PRESS_LOST, data);

    data->progress_time_label = lv_label_create(data->control_layer);
    lv_obj_add_style(data->progress_time_label, &TINY_SIZE, LV_PART_MAIN);
    lv_obj_set_width(data->progress_time_label, LV_PCT(86));
    lv_obj_set_style_text_align(data->progress_time_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->progress_time_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(data->progress_time_label, LV_ALIGN_BOTTOM_MID, 0, -14);
    lv_label_set_text(data->progress_time_label, "00:00 / 00:00");

    data->progress_timer = lv_timer_create(progress_timer_cb, PROGRESS_TIMER_MS, data);

    update_play_pause_symbol(data);
    gesture_back_enable_event_bubble_recursive(data->container);
    page_set_private_data(data);
}

void page_video_preview_destroy(void)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)page_get_private_data();
    if (!data)
        return;

    (void)player_manager_stop();
    player_manager_deinit();

    if (data->progress_timer) {
        lv_timer_del(data->progress_timer);
        data->progress_timer = NULL;
    }

    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    set_auto_sleep_block(data, false);
    free(data);
}

void page_video_preview_show(void)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)page_get_private_data();
    if (!data || !data->container)
        return;

    gesture_back_set_left_edge_swipe_cb(data->container, back_btn_cb);

    if (!file_manager_is_storage_ready()) {
        (void)player_manager_stop();
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

    data->is_dragging_progress = false;
    render_current_video(data, true);
    set_paused_state(data, true);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(data->back_btn);
}

void page_video_preview_hide(void)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)page_get_private_data();
    if (!data || !data->container)
        return;

    set_paused_state(data, true);
    (void)player_manager_stop();
    data->current_sec = 0;
    update_progress_display(data);
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_video_preview_update(void)
{
    /* no-op */
}
