#ifndef __PAGE_SYSTEM_SETTINGS_H__
#define __PAGE_SYSTEM_SETTINGS_H__

#include "core/page_manager.h"
#include "pages/page_photo_settings.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 设置项结构 */
typedef struct {
    lv_obj_t* container; /* 行容器 */
    lv_obj_t* icon; /* 左侧图标 */
    lv_obj_t* title_label; /* 标题文字 */
    lv_obj_t* value_label; /* 参数文字 */
    const char* icon_path; /* 图标路径 */
    const char* title; /* 标题 */
    const char* value; /* 当前值 */
    const char* toggle_on; /* 开启时显示的值 */
    const char* toggle_off; /* 关闭时显示的值 */
    setting_type_t type; /* 设置类型 */
    int is_on; /* 开关状态 */
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
