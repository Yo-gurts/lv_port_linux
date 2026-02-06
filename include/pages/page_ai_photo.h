#ifndef __PAGE_AI_PHOTO_H__
#define __PAGE_AI_PHOTO_H__

#include "core/page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* container; /* 页面容器，用于 hide/show */
    lv_obj_t* top_bar; /* 顶部状态栏 */
    lv_obj_t* back_btn; /* 返回按钮 */
    lv_obj_t* resolution_label; /* 分辨率标签 */
    lv_obj_t* photo_count_label; /* 照片数量标签 */
    lv_obj_t* sd_icon; /* SD卡图标 */
    lv_obj_t* wifi_icon; /* WiFi图标 */
    lv_obj_t* battery_icon; /* 电池图标 */
    lv_obj_t* bottom_bar; /* 底部工具栏 */
    lv_obj_t* filter_btn; /* 滤镜按钮 */
    lv_obj_t* menu_btn; /* 菜单按钮 */
    lv_obj_t* switch_btn; /* 摄像头切换按钮 */
} page_ai_photo_data_t;

void page_ai_photo_create(page_manager_t* pm);
void page_ai_photo_destroy(page_manager_t* pm);
void page_ai_photo_show(page_manager_t* pm);
void page_ai_photo_hide(page_manager_t* pm);
void page_ai_photo_update(page_manager_t* pm);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_AI_PHOTO_H__ */
