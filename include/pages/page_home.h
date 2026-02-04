#ifndef __PAGE_HOME_H__
#define __PAGE_HOME_H__

#include "core/page_manager.h"
#include "ui/status_bar.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* root;
    status_bar_t* status_bar;
    lv_obj_t* grid_container; /* 图标网格容器 */
} home_page_data_t;

void page_home_create(page_manager_t* pm, void* user_data);
void page_home_destroy(page_manager_t *pm, void *user_data);
void page_home_show(page_manager_t *pm, void *user_data);
void page_home_hide(page_manager_t *pm, void *user_data);
void page_home_update(page_manager_t *pm, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_HOME_H__ */
