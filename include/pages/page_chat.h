#ifndef __PAGE_CHAT_H__
#define __PAGE_CHAT_H__

#include "core/page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* container; /* 页面容器 */
    lv_obj_t* back_btn; /* 返回按钮 */
    lv_obj_t* msg_container; /* 消息列表容器 */
    lv_obj_t* bottom_bar; /* 底部工具栏 */
    lv_obj_t* volume_btn; /* 音量按钮 */
    lv_obj_t* voice_btn; /* 按住说话按钮 */
    lv_obj_t* voice_label; /* 按住说话文字 */
    lv_obj_t* voice_icon; /* 按住说话图标 */
    lv_obj_t* timbre_btn; /* 音色按钮 */
} page_chat_data_t;

void page_chat_create(page_manager_t* pm);
void page_chat_destroy(page_manager_t* pm);
void page_chat_show(page_manager_t* pm);
void page_chat_hide(page_manager_t* pm);
void page_chat_update(page_manager_t* pm);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_CHAT_H__ */
