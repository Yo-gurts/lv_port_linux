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
    lv_obj_t* confirm_mask; /* 确认弹框遮罩 */
    lv_obj_t* confirm_panel; /* 确认弹框面板 */
    lv_obj_t* confirm_title_label; /* 确认标题 */
    lv_obj_t* confirm_msg_label; /* 确认文案 */
    lv_obj_t* confirm_cancel_btn; /* 取消按钮 */
    lv_obj_t* confirm_ok_btn; /* 确认按钮 */
    int pending_action; /* 待确认操作 */
    int action_processing; /* 是否正在处理 */
    system_setting_item_t settings[8]; /* 8个设置项 */
} page_system_settings_data_t;

void page_system_settings_create(void);
void page_system_settings_destroy(void);
void page_system_settings_show(void);
void page_system_settings_hide(void);
void page_system_settings_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_SYSTEM_SETTINGS_H__ */
