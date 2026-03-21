#ifndef __PAGE_AI_RECOGNITION_PREVIEW_H__
#define __PAGE_AI_RECOGNITION_PREVIEW_H__

#include "core/file_manager.h"
#include "core/page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* container;
    lv_obj_t* back_btn;
    lv_obj_t* wifi_icon;
    lv_obj_t* title_label;

    lv_obj_t* thumb_wrap;
    lv_obj_t* thumb_img;

    lv_obj_t* text_box;
    lv_obj_t* text_label;

    lv_obj_t* read_btn;
    lv_obj_t* read_icon;
    lv_obj_t* read_label;

    char latest_thumb_path[FILE_MANAGER_MAX_PATH_LEN];
} page_ai_recognition_preview_data_t;

void page_ai_recognition_preview_create(void);
void page_ai_recognition_preview_destroy(void);
void page_ai_recognition_preview_show(void);
void page_ai_recognition_preview_hide(void);
void page_ai_recognition_preview_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_AI_RECOGNITION_PREVIEW_H__ */
