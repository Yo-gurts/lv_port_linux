#ifndef __PAGE_PHOTO_PREVIEW_H__
#define __PAGE_PHOTO_PREVIEW_H__

#include "core/page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* container;
    lv_obj_t* back_btn;
    lv_obj_t* file_name_label;
    lv_obj_t* image_area;
    lv_obj_t* current_slide;
    lv_obj_t* current_image;
    lv_obj_t* target_slide;
    lv_obj_t* target_image;
    int total_photos;
    int current_display_index; /* 0-based, 与相册页显示顺序一致（新到旧） */
    int drag_active;
    int drag_ignore;
    int drag_start_x;
    int drag_last_x;
    int drag_offset_x;
    int drag_direction;
    int drag_last_move_dir;
    int drag_target_index;
    int swipe_anim_running;
    int swipe_commit_on_anim_end;
} page_photo_preview_data_t;

void page_photo_preview_set_initial_photo_index(int photo_index);

void page_photo_preview_create(void);
void page_photo_preview_destroy(void);
void page_photo_preview_show(void);
void page_photo_preview_hide(void);
void page_photo_preview_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_PHOTO_PREVIEW_H__ */
