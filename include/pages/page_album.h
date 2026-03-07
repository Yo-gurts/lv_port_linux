#ifndef __PAGE_ALBUM_H__
#define __PAGE_ALBUM_H__

#include "core/page_manager.h"
#include <stdbool.h>
#include <stdint.h>

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
    int photo_index; /* 当前绑定的真实照片索引 */
    bool is_visible; /* 当前是否可见 */
    lv_obj_t* container; /* 项容器 */
    lv_obj_t* img; /* 图片对象 */
    lv_obj_t* select_box; /* 右下角选择框 */
} album_item_t;

/* 相册页面数据结构 */
typedef struct {
    lv_obj_t* container; /* 页面容器 */
    lv_obj_t* nav_bar; /* 导航栏 */
    lv_obj_t* grid_container; /* 滚动容器（viewport） */
    lv_obj_t* scroll_content; /* 滚动内容容器（用于撑开总高度） */
    lv_obj_t* fast_scrollbar; /* 可拖动的快速滚动条（slider） */
    lv_obj_t* back_btn; /* 返回按钮 */
    lv_obj_t* cancel_btn; /* 取消按钮（选择模式） */
    lv_obj_t* select_all_btn; /* 全选按钮（选择模式） */
    lv_obj_t* photo_btn; /* 拍照按钮 */
    lv_obj_t* video_btn; /* 录像按钮 */
    lv_obj_t* select_btn; /* 选择按钮（普通模式） */
    lv_obj_t* selected_count_label; /* 已选择数量文本 */
    lv_obj_t* delete_btn; /* 删除按钮 */
    lv_obj_t* op_block_mask; /* 操作阻塞遮罩 */
    album_layout_config_t layout; /* 布局参数 */
    album_item_t* item_pool; /* item复用池 */
    int pool_size; /* 复用池大小 */
    int total_photos; /* 照片总数 */
    int first_visible_row; /* 当前第一可见行 */
    int last_notice_index; /* top_notice上次显示的可见末尾索引 */
    int fast_scrollbar_range_max; /* 缓存slider range最大值，避免重复set_range */
    int fast_scrollbar_last_value; /* 缓存slider value，避免重复set_value */
    bool syncing_fast_scrollbar; /* 防止滚动与slider互相回调 */
    bool selection_mode; /* 是否处于多选模式 */
    bool* selected_flags; /* 按真实照片索引记录选中状态 */
    int selected_capacity; /* 选中数组容量 */
    int selected_count; /* 当前选中数量 */
    bool deleting_in_progress; /* 正在删除，阻塞操作 */
    bool suppress_next_item_click; /* 长按后抑制下一次点击，避免重复切换 */
    bool is_scrolling; /* 主列表是否处于滚动中 */
    uint32_t last_scroll_end_tick; /* 最近一次滚动结束时间，用于点击冷却 */
    int item_press_scroll_y; /* item按下时的滚动位置 */
    bool item_press_valid; /* 是否记录了有效的按下滚动位置 */
    uint8_t prev_input_block_mask; /* 删除前的输入屏蔽状态 */
} page_album_data_t;

/* 相册页面函数 */
void page_album_create(void);
void page_album_destroy(void);
void page_album_show(void);
void page_album_hide(void);
void page_album_update(void);
void page_album_set_focus_photo_index(int photo_index);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_ALBUM_H__ */
