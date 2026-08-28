#ifndef __PAGE_AI_STYLE_PREVIEW_H__
#define __PAGE_AI_STYLE_PREVIEW_H__

#include "core/ai_style_config.h"
#include "core/file_manager.h"
#include "core/page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 风格数量在运行时由配置文件决定；UI item 按上限一次性建好，show 时按实际数量刷新可见。 */
#define AI_STYLE_PREVIEW_STYLE_MAX AI_STYLE_MAX_COUNT

typedef struct {
    lv_obj_t* container;
    lv_obj_t* image_area;
    lv_obj_t* image;
    lv_obj_t* back_btn;

    lv_obj_t* style_panel;
    lv_obj_t* style_list;
    lv_obj_t* style_focus_frame;
    lv_obj_t* style_items[AI_STYLE_PREVIEW_STYLE_MAX];
    lv_obj_t* style_labels[AI_STYLE_PREVIEW_STYLE_MAX];
    lv_obj_t* style_thumb_imgs[AI_STYLE_PREVIEW_STYLE_MAX];
    int style_count; /* 当前生效的风格数量（含原图） */
    lv_obj_t* loading_overlay;
    lv_obj_t* loading_spinner;
    lv_obj_t* loading_label;
    lv_timer_t* process_poll_timer;

    int selected_style_index;
    uint8_t panel_visible;
    uint8_t ai_key_registered;
    uint8_t processing;
    char latest_photo_display_path[FILE_MANAGER_MAX_PATH_LEN];
    char latest_photo_real_path[FILE_MANAGER_MAX_PATH_LEN];
    char processed_photo_display_path[FILE_MANAGER_MAX_PATH_LEN];
} page_ai_style_preview_data_t;

void page_ai_style_preview_create(void);
void page_ai_style_preview_destroy(void);
void page_ai_style_preview_show(void);
void page_ai_style_preview_hide(void);
void page_ai_style_preview_update(void);
void page_ai_style_preview_set_photo_index(int photo_index);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_AI_STYLE_PREVIEW_H__ */
