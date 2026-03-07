#include "ui/gesture_back.h"
#include "config.h"
#include "mlog.h"

/* 单实例策略：
 * 当前仅维护“正在显示页面”的一个手势回调，进入页面(show)时重绑。 */
static lv_point_t g_swipe_start_point;
static uint32_t g_swipe_start_tick;
static lv_obj_t* g_swipe_pressed_obj;
static int g_swipe_start_valid;
static lv_obj_t* g_active_container;
static lv_event_cb_t g_active_action;

static void gesture_back_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* container = lv_event_get_current_target(e);
    lv_indev_t* indev = lv_indev_get_act();

    /* 只处理当前活跃页面容器的事件。 */
    if (container != g_active_container || g_active_action == NULL) {
        return;
    }

    if (code == LV_EVENT_PRESSED) {
        if (indev != NULL) {
            lv_indev_get_point(indev, &g_swipe_start_point);
            g_swipe_start_tick = lv_tick_get();
            g_swipe_pressed_obj = container;
            g_swipe_start_valid = 1;
        }
        return;
    }

    if (code == LV_EVENT_GESTURE) {
        uint32_t elapsed;
        lv_dir_t dir = indev ? lv_indev_get_gesture_dir(indev) : LV_DIR_NONE;
        int same_target = g_swipe_start_valid && (g_swipe_pressed_obj == container);
        int from_left_edge = same_target && (g_swipe_start_point.x <= SWIPE_BACK_EDGE_THRESHOLD_PX);

        if (same_target == 0) {
            return;
        }

        /* 使用 PRESSED->GESTURE 时间窗过滤慢速拖拽误触。 */
        elapsed = lv_tick_elaps(g_swipe_start_tick);
        if (dir == LV_DIR_RIGHT
            && from_left_edge
            && elapsed <= SWIPE_BACK_PRESS_GESTURE_MAX_MS
            && g_active_action != NULL) {
            g_active_action(NULL);
        }

        /* 检测到滑动后忽略后续点击，避免误触。 */
        if (indev != NULL) {
            lv_indev_wait_release(indev);
        }
        g_swipe_start_valid = 0;
        g_swipe_pressed_obj = NULL;
    }
}

static void gesture_back_enable_event_bubble_children_recursive(lv_obj_t* parent)
{
    uint32_t child_count;
    uint32_t i;

    if (parent == NULL) {
        return;
    }

    child_count = lv_obj_get_child_count(parent);
    for (i = 0; i < child_count; i++) {
        lv_obj_t* child = lv_obj_get_child(parent, i);
        lv_obj_add_flag(child, LV_OBJ_FLAG_EVENT_BUBBLE);
        gesture_back_enable_event_bubble_children_recursive(child);
    }
}

void gesture_back_enable_event_bubble_recursive(lv_obj_t* obj)
{
    if (obj == NULL) {
        return;
    }

    /* 页面容器自身不向上冒泡，避免继续传到屏幕或更上层。 */
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    gesture_back_enable_event_bubble_children_recursive(obj);
}

void gesture_back_register_events(lv_obj_t* container)
{
    if (container == NULL) {
        return;
    }

    /* 允许手势识别，但不向父对象冒泡 GESTURE 事件。 */
    lv_obj_clear_flag(container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_obj_add_event_cb(container, gesture_back_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(container, gesture_back_event_cb, LV_EVENT_GESTURE, NULL);
}

void gesture_back_set_left_edge_swipe_cb(lv_obj_t* container, lv_event_cb_t action)
{
    if (container == NULL || action == NULL) {
        return;
    }

    g_active_container = container;
    g_active_action = action;
}
