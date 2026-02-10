#ifndef __PAGE_PHOTO_SETTINGS_H__
#define __PAGE_PHOTO_SETTINGS_H__

#include "ui/common_settings.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 设置项 - 运行时的控件 */
typedef struct {
    lv_obj_t* container; /* 行容器 */
    lv_obj_t* icon; /* 左侧图标 */
    lv_obj_t* title_label; /* 标题文字 */
    lv_obj_t* value_label; /* 参数文字 */
    int current_index; /* 当前选中的索引 */
} setting_item_t;

typedef struct {
    lv_obj_t* container; /* 页面容器 */
    lv_obj_t* nav_bar; /* 顶部导航栏 */
    lv_obj_t* settings_container; /* 设置列表容器 */
    const setting_config_t* configs; /* 配置数组 */
    setting_item_t* items; /* 控件数组 */
    lv_obj_t* roller_popup; /* 滚轮弹窗 */
    lv_obj_t* roller; /* 滚轮控件 */
    int current_setting_index; /* 当前选中的设置项索引 */
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
