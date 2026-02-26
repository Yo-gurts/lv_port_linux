#ifndef __PAGE_PHOTO_H__
#define __PAGE_PHOTO_H__

#include "core/page_manager.h"

#define PHOTO_FILTER_MAX_COUNT 12

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
    lv_obj_t* filter_overlay; /* 滤镜全屏点击层（点击空白关闭） */
    lv_obj_t* filter_panel; /* 滤镜选择面板 */
    lv_obj_t* filter_list; /* 滤镜横向滚动列表 */
    lv_obj_t* filter_focus_frame; /* 中间固定选中框 */
    lv_obj_t* filter_items[PHOTO_FILTER_MAX_COUNT]; /* 滤镜条目容器 */
    lv_obj_t* filter_thumbs[PHOTO_FILTER_MAX_COUNT]; /* 滤镜缩略图容器 */
    lv_obj_t* filter_labels[PHOTO_FILTER_MAX_COUNT]; /* 滤镜名称标签 */
    int filter_count; /* 滤镜数量 */
    int selected_filter_index; /* 当前选中的滤镜下标 */
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
