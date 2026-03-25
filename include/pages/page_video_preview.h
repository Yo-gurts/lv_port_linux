#ifndef __PAGE_VIDEO_PREVIEW_H__
#define __PAGE_VIDEO_PREVIEW_H__

#include "core/page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* container;
    lv_obj_t* back_btn;
    lv_obj_t* file_name_label;
    lv_obj_t* duration_label;
    lv_obj_t* image_area;
    lv_obj_t* image;
    lv_obj_t* play_hint;
    int total_videos;
    int current_display_index;
} page_video_preview_data_t;

void page_video_preview_set_initial_video_index(int video_index);

void page_video_preview_create(void);
void page_video_preview_destroy(void);
void page_video_preview_show(void);
void page_video_preview_hide(void);
void page_video_preview_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_VIDEO_PREVIEW_H__ */
