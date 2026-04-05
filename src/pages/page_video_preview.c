// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_video_preview.h"
#include "pages/page_video_album.h"
#include "config.h"
#include "core/file_manager.h"
#include "core/font_manager.h"
#include "core/media_manager.h"
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

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static int g_initial_video_index = -1;
static int g_return_work_mode = -1;

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
static int restore_work_mode(page_video_preview_data_t* data);
static void update_progress_from_pointer(page_video_preview_data_t* data, lv_obj_t* slider, lv_anim_enable_t anim);
static void reset_current_video_to_start(page_video_preview_data_t* data);
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

static void set_obj_visible(lv_obj_t* obj, bool visible)
{
    if (!obj)
        return;

    if (visible)
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
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
    bool show_progress;
    bool show_cover_image;

    if (!data || !data->control_layer)
        return;

    show_progress = data->total_duration_sec > 0;
    show_cover_image = visible && !data->has_played_current_video;

    /* 封面图仅在首次未播放前显示；播放过后暂停时保留底层视频帧。 */
    lv_obj_clear_flag(data->control_layer, LV_OBJ_FLAG_HIDDEN);
    set_obj_visible(data->image, show_cover_image);
    set_obj_visible(data->back_btn, visible);
    set_obj_visible(data->file_name_label, visible);
    set_obj_visible(data->center_play_pause_btn, visible);
    set_obj_visible(data->prev_btn, visible);
    set_obj_visible(data->next_btn, visible);
    set_obj_visible(data->progress_slider, show_progress);
    set_obj_visible(data->progress_time_label, show_progress);
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

static int restore_work_mode(page_video_preview_data_t* data)
{
    int ret = MEDIA_MANAGER_OK;

    if (!data || !data->switched_to_playback_mode) {
        return MEDIA_MANAGER_OK;
    }

    MLOG_INFO("video preview restore work mode begin: return_mode=%d", data->return_work_mode);

    ret = media_manager_restore_work_mode(data->return_work_mode);

    if (ret == MEDIA_MANAGER_OK) {
        data->switched_to_playback_mode = false;
        MLOG_INFO("video preview restore work mode ok: return_mode=%d", data->return_work_mode);
    } else {
        MLOG_ERR("video preview restore work mode failed: return_mode=%d ret=%d",
                 data->return_work_mode,
                 ret);
    }

    return ret;
}

static void update_progress_from_pointer(page_video_preview_data_t* data, lv_obj_t* slider, lv_anim_enable_t anim)
{
    lv_indev_t* indev;
    lv_point_t point;
    lv_area_t slider_coords;
    lv_coord_t slider_width;
    lv_coord_t local_x;
    int min_value;
    int max_value;
    int value;

    if (!data || !slider || data->total_duration_sec <= 0)
        return;

    indev = lv_indev_active();
    if (!indev)
        return;

    lv_indev_get_point(indev, &point);
    lv_obj_get_coords(slider, &slider_coords);
    slider_width = lv_area_get_width(&slider_coords);
    if (slider_width <= 0)
        return;

    local_x = point.x - slider_coords.x1;
    if (local_x < 0)
        local_x = 0;
    if (local_x > slider_width)
        local_x = slider_width;

    min_value = lv_slider_get_min_value(slider);
    max_value = lv_slider_get_max_value(slider);
    if (max_value <= min_value)
        value = min_value;
    else
        value = min_value + (int)(((int64_t)local_x * (max_value - min_value)) / slider_width);

    value = clamp_int(value, min_value, max_value);
    lv_slider_set_value(slider, value, anim);
    data->current_sec = value;
    update_progress_display(data);
}

static void reset_current_video_to_start(page_video_preview_data_t* data)
{
    if (!data)
        return;

    MLOG_INFO("video preview reset current video to start: index=%d current=%d total=%d",
              data->current_display_index,
              data->current_sec,
              data->total_duration_sec);

    (void)player_manager_stop();
    render_current_video(data, true);
    data->is_paused = true;
    set_auto_sleep_block(data, false);
    update_play_pause_symbol(data);
    set_controls_visible(data, true);
    update_progress_display(data);
}

static void set_paused_state(page_video_preview_data_t* data, bool paused)
{
    bool effective_paused;

    if (!data)
        return;

    MLOG_INFO("video preview set_paused_state request: target_paused=%d index=%d current=%d total=%d paused=%d dragging=%d",
              paused ? 1 : 0,
              data->current_display_index,
              data->current_sec,
              data->total_duration_sec,
              data->is_paused ? 1 : 0,
              data->is_dragging_progress ? 1 : 0);

    if (data->is_paused == paused) {
        set_auto_sleep_block(data, paused ? false : true);
        update_play_pause_symbol(data);
        set_controls_visible(data, paused);
        MLOG_INFO("video preview set_paused_state noop: paused=%d", data->is_paused ? 1 : 0);
        return;
    }

    effective_paused = data->is_paused;

    if (!paused) {
        if (player_manager_play() != 0) {
            MLOG_WARN("video preview play failed, keep paused");
        } else {
            data->has_played_current_video = true;
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
    MLOG_INFO("video preview set_paused_state result: paused=%d current=%d total=%d",
              data->is_paused ? 1 : 0,
              data->current_sec,
              data->total_duration_sec);
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
        MLOG_INFO("video preview render_current_video: empty album");
        (void)player_manager_stop();
        lv_img_set_src(data->image, NULL);
        lv_label_set_text(data->file_name_label, "");
        data->total_duration_sec = 0;
        data->current_sec = 0;
        data->has_played_current_video = false;
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
        duration_sec = 0;
    data->total_duration_sec = duration_sec;
    MLOG_INFO("video preview render_current_video: index=%d reset_progress=%d name=%s duration=%d",
              video_index,
              reset_progress ? 1 : 0,
              file_name,
              duration_sec);

    if (file_manager_get_video_path(video_index, video_path, sizeof(video_path), FILE_PATH_REAL) == 0) {
        MLOG_INFO("video preview prepare path: %s", video_path);
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

    MLOG_INFO("video preview switch video: from=%d to=%d delta=%d",
              data->current_display_index,
              target_index,
              delta);
    data->current_display_index = target_index;
    data->has_played_current_video = false;
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
    int previous_sec;
    bool used_fallback = false;

    if (!data)
        return;
    if (data->is_paused || data->is_dragging_progress)
        return;
    previous_sec = data->current_sec;

    if (player_manager_get_progress(&current_sec, &total_sec) == 0) {
        if (total_sec > 0)
            data->total_duration_sec = total_sec;

        current_sec = clamp_int(current_sec, 0, data->total_duration_sec > 0 ? data->total_duration_sec : current_sec);

        /*
         * 真机回放模式下 PLAYER_SERVICE_SeekTime() 有时不会随播放推进。
         * 此时退回到与旧 UI 一致的“按播放时钟+1s”策略，避免进度条停在原地。
         */
        if (current_sec <= previous_sec && data->total_duration_sec > 0 && previous_sec < data->total_duration_sec) {
            data->current_sec = clamp_int(previous_sec + (PROGRESS_TIMER_MS / 1000), 0, data->total_duration_sec);
            used_fallback = true;
        } else {
            data->current_sec = current_sec;
        }
    } else if (data->total_duration_sec > 0 && previous_sec < data->total_duration_sec) {
        data->current_sec = clamp_int(previous_sec + (PROGRESS_TIMER_MS / 1000), 0, data->total_duration_sec);
        used_fallback = true;
    } else {
        return;
    }

    if (used_fallback) {
        MLOG_DBG("video preview progress fallback tick: prev=%d current=%d total=%d",
                 previous_sec,
                 data->current_sec,
                 data->total_duration_sec);
    }

    if (data->total_duration_sec > 0 && data->current_sec >= data->total_duration_sec) {
        MLOG_INFO("video preview reached end: index=%d total=%d",
                  data->current_display_index,
                  data->total_duration_sec);
        reset_current_video_to_start(data);
        return;
    }

    update_progress_display(data);
}

static void back_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_video_preview_data_t* data = (page_video_preview_data_t*)page_get_private_data();
    int current_video_index = get_current_video_index(data);
    MLOG_INFO("video preview back clicked: index=%d current=%d", current_video_index, data ? data->current_sec : -1);
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

    MLOG_INFO("video preview center play/pause clicked: paused=%d current=%d",
              data->is_paused ? 1 : 0,
              data->current_sec);
    set_paused_state(data, !data->is_paused);
}

static void prev_btn_cb(lv_event_t* e)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)lv_event_get_user_data(e);
    if (!data)
        return;

    MLOG_INFO("video preview prev clicked: index=%d", data->current_display_index);
    try_switch_video(data, -1);
}

static void next_btn_cb(lv_event_t* e)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)lv_event_get_user_data(e);
    if (!data)
        return;

    MLOG_INFO("video preview next clicked: index=%d", data->current_display_index);
    try_switch_video(data, 1);
}

static void progress_slider_event_cb(lv_event_t* e)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    bool need_resume;

    if (!data || !data->progress_slider)
        return;

    if (code == LV_EVENT_PRESSED) {
        MLOG_INFO("video preview progress pressed: current=%d", data->current_sec);
        data->resume_after_seek = data->is_paused;
        data->is_dragging_progress = true;
        update_progress_from_pointer(data, data->progress_slider, LV_ANIM_OFF);
        return;
    }

    if (code == LV_EVENT_PRESSING) {
        update_progress_from_pointer(data, data->progress_slider, LV_ANIM_OFF);
        return;
    }

    if (code == LV_EVENT_VALUE_CHANGED) {
        data->current_sec = lv_slider_get_value(data->progress_slider);
        update_progress_display(data);
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        need_resume = data->resume_after_seek;
        data->resume_after_seek = false;
        data->is_dragging_progress = false;
        update_progress_from_pointer(data, data->progress_slider, LV_ANIM_OFF);
        MLOG_INFO("video preview progress released: seek_to=%d need_resume=%d",
                  data->current_sec,
                  need_resume ? 1 : 0);
        if (player_manager_seek_sec(data->current_sec) != 0)
            MLOG_WARN("video preview seek failed: sec=%d", data->current_sec);
        if (need_resume)
            set_paused_state(data, false);
    }
}

static void container_click_cb(lv_event_t* e)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)lv_event_get_user_data(e);
    lv_obj_t* target = lv_event_get_target(e);

    if (!data)
        return;

    /* 仅在点击视频画面区域时执行“点屏暂停”，避免播放按钮事件冒泡后被立即暂停。 */
    if (target != data->container && target != data->image_area && target != data->image && target != data->control_layer)
        return;

    /* 播放中点击画面 -> 暂停并显示控制层 */
    if (!data->is_paused) {
        MLOG_INFO("video preview container clicked while playing: current=%d", data->current_sec);
        set_paused_state(data, true);
    }
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_video_preview_set_initial_video_index(int video_index)
{
    g_initial_video_index = video_index;
    MLOG_INFO("video preview set initial index: %d", video_index);
}

void page_video_preview_set_return_work_mode(int work_mode)
{
    g_return_work_mode = work_mode;
    MLOG_INFO("video preview set return work mode: %d", work_mode);
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
    data->return_work_mode = -1;
    data->switched_to_playback_mode = false;
    MLOG_INFO("video preview create begin");
    if (player_manager_init() != 0)
        MLOG_WARN("video preview player manager init failed");

    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(data->container, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_set_layout(data->container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(data->container, LV_OPA_TRANSP, LV_PART_MAIN);
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
    lv_obj_add_event_cb(data->progress_slider, progress_slider_event_cb, LV_EVENT_PRESSING, data);
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
    lv_obj_clear_flag(data->progress_slider, LV_OBJ_FLAG_EVENT_BUBBLE);
    page_set_private_data(data);
    MLOG_INFO("video preview create ok");
}

void page_video_preview_destroy(void)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)page_get_private_data();
    if (!data)
        return;

    MLOG_INFO("video preview destroy begin: index=%d current=%d",
              data->current_display_index,
              data->current_sec);
    (void)player_manager_stop();
    (void)restore_work_mode(data);
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
    MLOG_INFO("video preview destroy ok");
}

void page_video_preview_show(void)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)page_get_private_data();
    if (!data || !data->container)
        return;

    MLOG_INFO("video preview show begin: initial_index=%d", g_initial_video_index);
    gesture_back_set_left_edge_swipe_cb(data->container, back_btn_cb);
    data->return_work_mode = g_return_work_mode;
    data->switched_to_playback_mode = (g_return_work_mode >= 0 && !media_manager_is_playback_work_mode(g_return_work_mode));
    g_return_work_mode = -1;

    if (!file_manager_is_storage_ready()) {
        MLOG_WARN("video preview show aborted: storage not ready");
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
    data->resume_after_seek = false;
    data->has_played_current_video = false;
    render_current_video(data, true);
    set_paused_state(data, true);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(data->back_btn);
    MLOG_INFO("video preview show ok: total_videos=%d index=%d",
              data->total_videos,
              data->current_display_index);
}

void page_video_preview_hide(void)
{
    page_video_preview_data_t* data = (page_video_preview_data_t*)page_get_private_data();
    if (!data || !data->container)
        return;

    MLOG_INFO("video preview hide begin: index=%d current=%d paused=%d",
              data->current_display_index,
              data->current_sec,
              data->is_paused ? 1 : 0);
    set_paused_state(data, true);
    (void)player_manager_stop();
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
    (void)restore_work_mode(data);
    data->current_sec = 0;
    update_progress_display(data);
    MLOG_INFO("video preview hide ok");
}

void page_video_preview_update(void)
{
    /* no-op */
}
