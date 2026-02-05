#ifndef __PAGE_HOME_H__
#define __PAGE_HOME_H__

#include "core/page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Home page data */
typedef struct {
    lv_obj_t* container; /* 页面容器 */
    lv_obj_t* grid_container; /* 图标网格容器 */
    /* Status bar widgets */
    lv_obj_t* time_label;
    lv_obj_t* wifi_icon;
    lv_obj_t* battery_icon;
    lv_timer_t* timer; /* 时间更新定时器 */
} home_page_data_t;

void page_home_create(page_manager_t* pm);
void page_home_destroy(page_manager_t* pm);
void page_home_show(page_manager_t* pm);
void page_home_hide(page_manager_t* pm);
void page_home_update(page_manager_t* pm);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_HOME_H__ */
