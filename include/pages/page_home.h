#ifndef __PAGE_HOME_H__
#define __PAGE_HOME_H__

#include "core/page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t *root;
    lv_obj_t *status_bar;
    lv_obj_t *grid_container;      /* 图标网格容器 */
    lv_obj_t *photo_button;
    lv_obj_t *video_button;
    lv_obj_t *gallery_button;
    lv_obj_t *settings_button;
} home_page_data_t;

void page_home_create(page_manager_t *pm, void *user_data);
void page_home_destroy(page_manager_t *pm, void *user_data);
void page_home_show(page_manager_t *pm, void *user_data);
void page_home_hide(page_manager_t *pm, void *user_data);
void page_home_update(page_manager_t *pm, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_HOME_H__ */
