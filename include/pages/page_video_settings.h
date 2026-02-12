#ifndef __PAGE_VIDEO_SETTINGS_H__
#define __PAGE_VIDEO_SETTINGS_H__

#include "ui/common_settings.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 录像设置页面数据 */
typedef struct {
    lv_obj_t* container; /* 页面容器 */
    lv_obj_t* nav_bar; /* 顶部导航栏 */
    lv_obj_t* settings_container; /* 设置列表容器 */
    const setting_config_t* configs; /* 配置数组 */
    setting_item_t* items; /* 控件数组 */
    lv_obj_t* roller_popup; /* 滚轮弹窗 */
    lv_obj_t* roller; /* 滚轮控件 */
    int current_setting_index; /* 当前选中的设置项索引 */
} page_video_settings_data_t;

void page_video_settings_create(void);
void page_video_settings_destroy(void);
void page_video_settings_show(void);
void page_video_settings_hide(void);
void page_video_settings_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_VIDEO_SETTINGS_H__ */
