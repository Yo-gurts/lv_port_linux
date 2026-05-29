#ifndef __PAGE_AI_PHOTO_SETTINGS_H__
#define __PAGE_AI_PHOTO_SETTINGS_H__

#include "core/page_manager.h"
#include "pages/page_photo_settings.h"

#ifdef __cplusplus
extern "C" {
#endif

/* AI设置项类型 */
typedef enum {
    AI_SETTING_STYLE_TRANSFORM, /* 风格变换 */
    AI_SETTING_RECOGNITION, /* AI识万物 */
    AI_SETTING_TRANSLATION, /* 拍照翻译 */
} ai_setting_type_t;

/* AI设置项结构 - 仅包含静态配置 */
typedef struct {
    const char* icon_path; /* 图标路径 */
    const char* title; /* 标题 */
    ai_setting_type_t type; /* 设置类型 */
} ai_setting_config_t;

/* AI设置项运行时数据结构 */
typedef struct {
    lv_obj_t* container; /* 行容器 */
    lv_obj_t* icon; /* 左侧图标 */
    lv_obj_t* title_label; /* 标题文字 */
    lv_obj_t* check_icon; /* 选中图标 */
} ai_setting_item_t;

typedef struct {
    lv_obj_t* container; /* 页面容器 */
    lv_obj_t* nav_bar; /* 顶部导航栏 */
    lv_obj_t* settings_container; /* 设置列表容器 */
    const ai_setting_config_t* configs; /* 配置数组 */
    ai_setting_item_t* items; /* 控件数组 */
    int settings_count; /* 设置项数量 */
    int selected_index; /* 物理按键当前选中项索引 */
} page_ai_photo_settings_data_t;

void page_ai_photo_settings_create(void);
void page_ai_photo_settings_destroy(void);
void page_ai_photo_settings_show(void);
void page_ai_photo_settings_hide(void);
void page_ai_photo_settings_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_AI_PHOTO_SETTINGS_H__ */
