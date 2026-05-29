#ifndef __PAGE_VIDEO_ALBUM_H__
#define __PAGE_VIDEO_ALBUM_H__

#include "core/page_manager.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int item_width;
    int item_height;
    int gap_x;
    int gap_y;
    int cols;
    int start_x;
    int row_height;
    int visible_rows;
    int pool_rows;
} video_album_layout_config_t;

typedef struct {
    int index;
    int video_index;
    bool is_visible;
    lv_obj_t* container;
    lv_obj_t* img;
    lv_obj_t* select_box;
    lv_obj_t* duration_bg;
    lv_obj_t* duration_label;
} video_album_item_t;

typedef struct {
    lv_obj_t* container;
    lv_obj_t* nav_bar;
    lv_obj_t* grid_container;
    lv_obj_t* scroll_content;
    lv_obj_t* fast_scrollbar;
    lv_obj_t* back_btn;
    lv_obj_t* cancel_btn;
    lv_obj_t* select_all_btn;
    lv_obj_t* photo_btn;
    lv_obj_t* video_btn;
    lv_obj_t* photo_icon;
    lv_obj_t* video_icon;
    lv_obj_t* select_btn;
    lv_obj_t* selected_count_label;
    lv_obj_t* delete_btn;
    lv_obj_t* op_block_mask;
    video_album_layout_config_t layout;
    video_album_item_t* item_pool;
    int pool_size;
    int total_videos;
    int first_visible_row;
    int last_notice_index;
    int fast_scrollbar_range_max;
    int fast_scrollbar_last_value;
    bool syncing_fast_scrollbar;
    bool selection_mode;
    bool* selected_flags;
    int selected_capacity;
    int selected_count;
    bool deleting_in_progress;
    bool suppress_next_item_click;
    bool is_scrolling;
    uint32_t last_scroll_end_tick;
    int item_press_scroll_y;
    bool item_press_valid;
    uint8_t prev_input_block_mask;
    bool fast_scrollbar_pressed;
    lv_point_t fast_scrollbar_press_point;
    int cursor_row; /* 物理按键当前游标行 */
    int cursor_col; /* 物理按键当前游标列 */
} page_video_album_data_t;

void page_video_album_create(void);
void page_video_album_destroy(void);
void page_video_album_show(void);
void page_video_album_hide(void);
void page_video_album_update(void);
void page_video_album_set_focus_video_index(int video_index);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_VIDEO_ALBUM_H__ */
