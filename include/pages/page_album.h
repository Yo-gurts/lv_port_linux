#ifndef __PAGE_ALBUM_H__
#define __PAGE_ALBUM_H__

#include "core/page_manager.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 相册布局配置 */
typedef struct {
    int item_width; /* 单个图片宽度 */
    int item_height; /* 单个图片高度 */
    int gap_x; /* 水平间距 */
    int gap_y; /* 垂直间距 */
    int cols; /* 自动计算列数 */
    int start_x; /* 网格起始x（居中） */
    int row_height; /* 一行总高度 = item_height + gap_y */
    int visible_rows; /* 可见行数 */
    int pool_rows; /* 复用池行数（可见 + 缓冲） */
} album_layout_config_t;

/* 复用池中的图片项 */
typedef struct {
    int index; /* 当前绑定的照片索引 */
    bool is_visible; /* 当前是否可见 */
    lv_obj_t* container; /* 项容器 */
    lv_obj_t* img; /* 图片对象 */
    lv_obj_t* label; /* 索引标签 */
} album_item_t;

/* 相册页面数据结构 */
typedef struct {
    lv_obj_t* container; /* 页面容器 */
    lv_obj_t* nav_bar; /* 导航栏 */
    lv_obj_t* grid_container; /* 滚动容器（viewport） */
    lv_obj_t* scroll_content; /* 滚动内容容器（用于撑开总高度） */
    lv_obj_t* fast_scrollbar; /* 可拖动的快速滚动条（slider） */
    lv_timer_t* fast_scrollbar_hide_timer; /* 快速滚动条延时隐藏定时器 */
    lv_obj_t* back_btn; /* 返回按钮 */
    lv_obj_t* photo_btn; /* 拍照按钮 */
    lv_obj_t* video_btn; /* 录像按钮 */
    lv_obj_t* delete_all_btn; /* 删除全部按钮 */
    lv_obj_t* delete_btn; /* 删除按钮 */
    album_layout_config_t layout; /* 布局参数 */
    album_item_t* item_pool; /* item复用池 */
    int pool_size; /* 复用池大小 */
    int total_photos; /* 照片总数 */
    int first_visible_row; /* 当前第一可见行 */
    int fast_scrollbar_range_max; /* 缓存slider range最大值，避免重复set_range */
    int fast_scrollbar_last_value; /* 缓存slider value，避免重复set_value */
    bool syncing_fast_scrollbar; /* 防止滚动与slider互相回调 */
} page_album_data_t;

/* 相册页面函数 */
void page_album_create(void);
void page_album_destroy(void);
void page_album_show(void);
void page_album_hide(void);
void page_album_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_ALBUM_H__ */
