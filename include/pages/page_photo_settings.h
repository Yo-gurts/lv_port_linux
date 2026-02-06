#ifndef __PAGE_PHOTO_SETTINGS_H__
#define __PAGE_PHOTO_SETTINGS_H__

#include "core/page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 设置项类型 */
typedef enum {
    SETTING_TYPE_NORMAL, /* 普通设置，点击仅日志 */
    SETTING_TYPE_TOGGLE, /* 开关设置，点击切换状态 */
} setting_type_t;

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
} setting_item_t;

typedef struct {
    lv_obj_t* container; /* 页面容器 */
    lv_obj_t* nav_bar; /* 顶部导航栏 */
    lv_obj_t* settings_container; /* 设置列表容器 */
    setting_item_t settings[8]; /* 8个设置项 */
} page_photo_settings_data_t;

void page_photo_settings_create(page_manager_t* pm);
void page_photo_settings_destroy(page_manager_t* pm);
void page_photo_settings_show(page_manager_t* pm);
void page_photo_settings_hide(page_manager_t* pm);
void page_photo_settings_update(page_manager_t* pm);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_PHOTO_SETTINGS_H__ */
