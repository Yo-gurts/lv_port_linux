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
    lv_obj_t* lv_label_time; /* 时间标签 */
    lv_timer_t* home_update_timer; /* 时间更新定时器 */
    lv_obj_t* icon_buttons[6]; /* 6个图标按钮指针 */
    int selected_index; /* 物理按键当前选中项索引 0-5 */
} home_page_data_t;

void page_home_create(void);
void page_home_destroy(void);
void page_home_show(void);
void page_home_hide(void);
void page_home_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_HOME_H__ */
