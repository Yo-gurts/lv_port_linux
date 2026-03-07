#ifndef __GESTURE_BACK_H__
#define __GESTURE_BACK_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 递归开启对象树事件冒泡，确保子对象按压事件可传递到父容器。 */
void gesture_back_enable_event_bubble_recursive(lv_obj_t* obj);

/* 在页面 create 阶段注册手势事件回调（PRESSED/GESTURE）。 */
void gesture_back_register_events(lv_obj_t* container);

/* 设置当前页面边缘滑动返回回调（左边缘右滑 / 右边缘左滑）。 */
void gesture_back_set_left_edge_swipe_cb(lv_obj_t* container, lv_event_cb_t action);

#ifdef __cplusplus
}
#endif

#endif /* __GESTURE_BACK_H__ */
