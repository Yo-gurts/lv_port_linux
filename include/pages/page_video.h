#ifndef __PAGE_VIDEO_H__
#define __PAGE_VIDEO_H__

#include "core/page_manager.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* container; /* 页面容器 */
    lv_obj_t* top_bar; /* 顶部状态栏 */
    lv_obj_t* back_btn; /* 返回按钮 */
    lv_obj_t* bottom_bar; /* 底部工具栏 */
    lv_obj_t* resolution_img; /* 分辨率图标 */
    lv_obj_t* time_label; /* 录像时长标签 */
    lv_obj_t* sd_icon; /* SD卡图标 */
    lv_obj_t* battery_icon; /* 电池图标 */
    lv_obj_t* mode_btn; /* 拍照/录像模式按钮 */
    lv_obj_t* mode_img; /* 拍照/录像模式图片 */
    lv_obj_t* menu_btn; /* 菜单按钮 */
    lv_obj_t* filter_btn; /* 滤镜按钮 */
    lv_obj_t* switch_btn; /* 摄像头切换按钮 */
    lv_obj_t* record_dot; /* 录像红点 */
    lv_obj_t* record_time_label; /* 底部录像时长标签 */
    lv_timer_t* record_ui_timer; /* 录像UI刷新定时器 */
    int is_recording; /* 是否正在录像 */
    uint32_t record_start_tick; /* 录像开始tick */
    uint8_t record_dot_visible; /* 红点当前可见状态 */
} page_video_data_t;

void page_video_create(void);
void page_video_destroy(void);
void page_video_show(void);
void page_video_hide(void);
void page_video_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_VIDEO_H__ */
