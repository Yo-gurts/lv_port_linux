#ifndef __PAGE_SYSTEM_SETTINGS_H__
#define __PAGE_SYSTEM_SETTINGS_H__

#include "ui/common_settings.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 设置项结构 */
typedef struct {
    lv_obj_t* container; /* 行容器 */
    lv_obj_t* icon; /* 左侧图标 */
    lv_obj_t* title_label; /* 标题文字 */
    lv_obj_t* value_label; /* 参数文字 */
    int current_index; /* 当前选中的索引（toggle时0=关闭，1=开启） */
} system_setting_item_t;

typedef struct {
    lv_obj_t* container; /* 页面容器 */
    lv_obj_t* nav_bar; /* 顶部导航栏 */
    lv_obj_t* settings_container; /* 设置列表容器 */
    system_setting_item_t settings[7]; /* 7个设置项 */
} page_system_settings_data_t;

void page_system_settings_create(page_manager_t* pm);
void page_system_settings_destroy(page_manager_t* pm);
void page_system_settings_show(page_manager_t* pm);
void page_system_settings_hide(page_manager_t* pm);
void page_system_settings_update(page_manager_t* pm);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_SYSTEM_SETTINGS_H__ */
