#ifndef __PAGE_HOME_H__
#define __PAGE_HOME_H__

#include "core/page_manager.h"
#include "ui/status_bar.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* container; /* 页面容器，用于 hide/show */
    status_bar_t* status_bar;
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
