#ifndef __PAGE_VIDEO_PREVIEW_H__
#define __PAGE_VIDEO_PREVIEW_H__

#include "core/page_manager.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* container;
    lv_obj_t* image_area;
    lv_obj_t* image;
    lv_obj_t* back_btn;
    lv_obj_t* control_layer;
    lv_obj_t* file_name_label;
    lv_obj_t* center_play_pause_btn;
    lv_obj_t* center_play_pause_icon;
    lv_obj_t* prev_btn;
    lv_obj_t* prev_icon;
    lv_obj_t* next_btn;
    lv_obj_t* next_icon;
    lv_obj_t* progress_slider;
    lv_obj_t* progress_time_label;
    lv_timer_t* progress_timer;
    int total_videos;
    int current_display_index;
    int total_duration_sec;
    int current_sec;
    int return_work_mode;
    bool has_played_current_video;
    bool resume_after_seek;
    bool is_paused;
    bool is_dragging_progress;
    bool auto_sleep_blocked;
    bool switched_to_playback_mode;
} page_video_preview_data_t;

void page_video_preview_set_initial_video_index(int video_index);
void page_video_preview_set_return_work_mode(int work_mode);

void page_video_preview_create(void);
void page_video_preview_destroy(void);
void page_video_preview_show(void);
void page_video_preview_hide(void);
void page_video_preview_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_VIDEO_PREVIEW_H__ */
