#ifndef __PAGE_VIDEO_H__
#define __PAGE_VIDEO_H__

#include "core/page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* container; /* 页面容器 */
    lv_obj_t* top_bar; /* 顶部状态栏 */
    lv_obj_t* bottom_bar; /* 底部工具栏 */
    lv_obj_t* resolution_label; /* 分辨率标签 */
    lv_obj_t* time_label; /* 录像时长标签 */
    lv_obj_t* sd_icon; /* SD卡图标 */
    lv_obj_t* battery_icon; /* 电池图标 */
    lv_obj_t* mode_btn; /* 拍照/录像模式按钮 */
    lv_obj_t* mode_img; /* 拍照/录像模式图片 */
    lv_obj_t* menu_btn; /* 菜单按钮 */
    lv_obj_t* filter_btn; /* 滤镜按钮 */
    lv_obj_t* switch_btn; /* 摄像头切换按钮 */
    int current_resolution; /* 当前分辨率索引 */
    int is_recording; /* 是否正在录像 */
} page_video_data_t;

/* 录像分辨率选项 */
typedef enum {
    VIDEO_RES_FULL = 0,
    VIDEO_RES_HD,
    VIDEO_RES_2K7,
    VIDEO_RES_4K,
    VIDEO_RES_COUNT
} video_resolution_t;

void page_video_create(page_manager_t* pm);
void page_video_destroy(page_manager_t* pm);
void page_video_show(page_manager_t* pm);
void page_video_hide(page_manager_t* pm);
void page_video_update(page_manager_t* pm);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_VIDEO_H__ */
