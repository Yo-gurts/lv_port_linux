#ifndef __COMMON_SETTINGS_H__
#define __COMMON_SETTINGS_H__

#include "core/page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 设置项类型 */
typedef enum {
    SETTING_TYPE_NORMAL, /* 普通设置，点击弹出滚轮 */
    SETTING_TYPE_TOGGLE, /* 开关设置，点击切换状态 */
} setting_type_t;

/* 设置项配置 - 静态数据 */
typedef struct {
    const char* title; /* 标题 */
    const char* icon_path; /* 图标路径 */
    const char* value; /* 默认值（用于显示） */
    const char** roller_options; /* 滚轮选项数组 */
    int roller_count; /* 滚轮选项数量 */
    setting_type_t type; /* 设置类型 */
} setting_config_t;

/* 设置项 - 运行时的控件 */
typedef struct {
    lv_obj_t* container; /* 行容器 */
    lv_obj_t* icon; /* 左侧图标 */
    lv_obj_t* title_label; /* 标题文字 */
    lv_obj_t* value_label; /* 参数文字 */
    int current_index; /* 当前选中的索引 */
} setting_item_t;

#ifdef __cplusplus
}
#endif

#endif /* __COMMON_SETTINGS_H__ */
