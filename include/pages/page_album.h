#ifndef __PAGE_ALBUM_H__
#define __PAGE_ALBUM_H__

#include "core/page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 相册页面数据结构 */
typedef struct {
    lv_obj_t* container; /* 页面容器 */
    lv_obj_t* nav_bar; /* 导航栏 */
    lv_obj_t* grid_container; /* 9宫格容器 */
    lv_obj_t* back_btn; /* 返回按钮 */
    lv_obj_t* photo_btn; /* 拍照按钮 */
    lv_obj_t* video_btn; /* 录像按钮 */
    lv_obj_t* delete_all_btn; /* 删除全部按钮 */
    lv_obj_t* delete_btn; /* 删除按钮 */
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
