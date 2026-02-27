#ifndef __PAGE_PHOTO_H__
#define __PAGE_PHOTO_H__

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
    lv_obj_t* battery_icon; /* 电池图标 */
    lv_obj_t* bottom_bar; /* 底部工具栏 */
    lv_obj_t* mode_btn; /* 拍照/录像模式按钮 */
    lv_obj_t* mode_img; /* 拍照/录像模式图片 */
    lv_obj_t* menu_btn; /* 菜单按钮 */
    lv_obj_t* filter_btn; /* 滤镜按钮 */
    lv_obj_t* switch_btn; /* 摄像头切换按钮 */
    lv_obj_t* focus_box; /* 对焦框 */
    lv_obj_t* focus_corners[4]; /* 对焦框四角 */
} page_photo_data_t;

void page_photo_create(void);
void page_photo_destroy(void);
void page_photo_show(void);
void page_photo_hide(void);
void page_photo_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_PHOTO_H__ */
