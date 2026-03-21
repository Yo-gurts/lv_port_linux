#ifndef __PAGE_AI_STYLE_PREVIEW_H__
#define __PAGE_AI_STYLE_PREVIEW_H__

#include "core/file_manager.h"
#include "core/page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AI_STYLE_PREVIEW_STYLE_COUNT 6

typedef struct {
    lv_obj_t* container;
    lv_obj_t* image_area;
    lv_obj_t* image;
    lv_obj_t* back_btn;
    lv_obj_t* wifi_icon;

    lv_obj_t* style_panel;
    lv_obj_t* style_list;
    lv_obj_t* style_focus_frame;
    lv_obj_t* style_items[AI_STYLE_PREVIEW_STYLE_COUNT];
    lv_obj_t* style_labels[AI_STYLE_PREVIEW_STYLE_COUNT];

    int selected_style_index;
    uint8_t panel_visible;
    char latest_photo_path[FILE_MANAGER_MAX_PATH_LEN];
} page_ai_style_preview_data_t;

void page_ai_style_preview_create(void);
void page_ai_style_preview_destroy(void);
void page_ai_style_preview_show(void);
void page_ai_style_preview_hide(void);
void page_ai_style_preview_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_AI_STYLE_PREVIEW_H__ */
