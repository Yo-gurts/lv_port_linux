#ifndef __PAGE_PHOTO_H__
#define __PAGE_PHOTO_H__

#include "core/page_manager.h"
#include "ui/status_bar.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* root;
    lv_obj_t* preview_container; /* 摄像头预览容器 */
    lv_obj_t* resolution_label; /* 分辨率标签 */
    lv_obj_t* sd_icon; /* SD卡图标 */
    lv_obj_t* battery_icon; /* 电池图标 */
    lv_obj_t* mode_btn; /* 拍照/录像模式按钮 */
    lv_obj_t* mode_label; /* 拍照/录像模式标签 */
    lv_obj_t* menu_btn; /* 菜单按钮 */
    lv_obj_t* filter_btn; /* 滤镜按钮 */
    lv_obj_t* switch_btn; /* 摄像头切换按钮 */
    lv_timer_t* timer; /* 状态更新定时器 */
    int current_resolution; /* 当前分辨率索引 */
    int is_video_mode; /* 是否录像模式 */
} page_photo_data_t;

void page_photo_create(page_manager_t* pm, void* user_data);
void page_photo_destroy(page_manager_t* pm, void* user_data);
void page_photo_show(page_manager_t* pm, void* user_data);
void page_photo_hide(page_manager_t* pm, void* user_data);
void page_photo_update(page_manager_t* pm, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_PHOTO_H__ */
