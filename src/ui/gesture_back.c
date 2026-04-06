#include "ui/gesture_back.h"
#include "config.h"

/* 单实例策略：
 * 当前仅维护“正在显示页面”的一个手势回调，进入页面(show)时重绑。 */
static lv_point_t g_swipe_start_point;
static lv_obj_t* g_swipe_pressed_obj;
static int g_swipe_start_valid;
static int g_swipe_edge_dir;
static int g_swipe_trigger_ready;
static lv_obj_t* g_active_container;
static lv_event_cb_t g_active_action;
static lv_obj_t* g_swipe_hint_icon;

#define SWIPE_BACK_TRIGGER_DISTANCE_PX 90
#define SWIPE_HINT_EDGE_OFFSET_PX 18
#define SWIPE_HINT_MAX_SLIDE_PX 28
#define SWIPE_HINT_VISUAL_MARGIN_PX 12

static int clamp_int(int value, int min, int max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

static int gesture_back_detect_edge_dir(lv_coord_t start_x, lv_coord_t container_width)
{
    if (start_x <= SWIPE_BACK_EDGE_THRESHOLD_PX) {
        return 1;
    }
    if (start_x >= container_width - SWIPE_BACK_EDGE_THRESHOLD_PX) {
        return -1;
    }
    return 0;
}

static void gesture_back_hide_hint(void)
{
    if (g_swipe_hint_icon != NULL && lv_obj_is_valid(g_swipe_hint_icon)) {
        lv_obj_add_flag(g_swipe_hint_icon, LV_OBJ_FLAG_HIDDEN);
    } else {
        g_swipe_hint_icon = NULL;
    }
    g_swipe_trigger_ready = 0;
}

static void gesture_back_apply_hint_active_style(int active)
{
    if (g_swipe_hint_icon == NULL) {
        return;
    }

    if (active) {
        lv_obj_set_style_img_opa(g_swipe_hint_icon, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_img_opa(g_swipe_hint_icon, LV_OPA_50, 0);
    }
}

static void gesture_back_layout_hint_icon(int edge_dir)
{
    lv_coord_t icon_w;
    lv_coord_t icon_h;

    if (g_swipe_hint_icon == NULL) {
        return;
    }

    lv_obj_update_layout(g_swipe_hint_icon);
    icon_w = lv_obj_get_width(g_swipe_hint_icon);
    icon_h = lv_obj_get_height(g_swipe_hint_icon);

    /* 右滑时复用同一图标并做反向显示。 */
    lv_img_set_pivot(g_swipe_hint_icon, icon_w / 2, icon_h / 2);
    lv_img_set_angle(g_swipe_hint_icon, (edge_dir > 0) ? 0 : 1800);
}

static void gesture_back_ensure_hint(void)
{
    lv_obj_t* top = lv_layer_top();

    if (g_swipe_hint_icon != NULL && !lv_obj_is_valid(g_swipe_hint_icon)) {
        g_swipe_hint_icon = NULL;
    }

    if (g_swipe_hint_icon == NULL || lv_obj_get_parent(g_swipe_hint_icon) != top) {
        if (g_swipe_hint_icon != NULL) {
            lv_obj_del(g_swipe_hint_icon);
        }

        g_swipe_hint_icon = lv_img_create(top);
        lv_img_set_src(g_swipe_hint_icon, "A:" RES_ICON_PATH "/back.png");
        lv_obj_add_flag(g_swipe_hint_icon, LV_OBJ_FLAG_HIDDEN);
        gesture_back_layout_hint_icon(0);
    }
}

static void gesture_back_update_hint(int edge_dir, int drag_distance)
{
    int clamped_drag;
    int hint_offset;
    int min_x;
    int max_x;
    lv_coord_t y;
    lv_coord_t x;
    lv_coord_t top_width;
    lv_coord_t top_height;
    lv_coord_t icon_w;
    lv_coord_t icon_h;
    lv_obj_t* top = lv_layer_top();

    gesture_back_ensure_hint();
    if (g_swipe_hint_icon == NULL) {
        return;
    }

    lv_obj_update_layout(g_swipe_hint_icon);
    icon_w = lv_obj_get_width(g_swipe_hint_icon);
    icon_h = lv_obj_get_height(g_swipe_hint_icon);
    clamped_drag = clamp_int(drag_distance, 0, SWIPE_BACK_TRIGGER_DISTANCE_PX);
    hint_offset = (clamped_drag * SWIPE_HINT_MAX_SLIDE_PX) / SWIPE_BACK_TRIGGER_DISTANCE_PX;
    top_width = lv_obj_get_width(top);
    top_height = lv_obj_get_height(top);
    y = (top_height - icon_h) / 2;

    if (edge_dir > 0) {
        x = SWIPE_HINT_EDGE_OFFSET_PX + hint_offset;
    } else {
        x = top_width - icon_w - SWIPE_HINT_EDGE_OFFSET_PX - hint_offset;
    }
    min_x = SWIPE_HINT_VISUAL_MARGIN_PX;
    max_x = top_width - icon_w - SWIPE_HINT_VISUAL_MARGIN_PX;
    if (max_x < min_x) {
        max_x = min_x;
    }
    x = clamp_int(x, min_x, max_x);
    gesture_back_layout_hint_icon(edge_dir);
    gesture_back_apply_hint_active_style(drag_distance >= SWIPE_BACK_TRIGGER_DISTANCE_PX);
    lv_obj_set_align(g_swipe_hint_icon, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(g_swipe_hint_icon, x, y);
    lv_obj_clear_flag(g_swipe_hint_icon, LV_OBJ_FLAG_HIDDEN);
    g_swipe_trigger_ready = (drag_distance >= SWIPE_BACK_TRIGGER_DISTANCE_PX) ? 1 : 0;
}

static void gesture_back_reset_swipe_state(void)
{
    g_swipe_start_valid = 0;
    g_swipe_pressed_obj = NULL;
    g_swipe_edge_dir = 0;
    g_swipe_trigger_ready = 0;
    gesture_back_hide_hint();
}

static void gesture_back_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* container = lv_event_get_current_target(e);
    lv_indev_t* indev = lv_indev_get_act();

    /* 只处理当前活跃页面容器的事件。 */
    if (container != g_active_container || g_active_action == NULL) {
        return;
    }

    switch (code) {
    case LV_EVENT_PRESSED: {
        lv_coord_t container_width = lv_obj_get_width(container);

        gesture_back_hide_hint();
        if (indev == NULL) {
            return;
        }

        lv_indev_get_point(indev, &g_swipe_start_point);
        g_swipe_pressed_obj = container;
        g_swipe_start_valid = 1;
        /* 仅从左右边缘起点进入返回手势判定。 */
        g_swipe_edge_dir = gesture_back_detect_edge_dir(g_swipe_start_point.x, container_width);
        return;
    }

    case LV_EVENT_PRESSING: {
        lv_point_t cur_point;
        int same_target = g_swipe_start_valid && (g_swipe_pressed_obj == container);
        int drag_distance = 0;

        if (same_target == 0) {
            return;
        }
        if (indev == NULL || g_swipe_edge_dir == 0) {
            return;
        }

        lv_indev_get_point(indev, &cur_point);
        if (g_swipe_edge_dir > 0) {
            drag_distance = cur_point.x - g_swipe_start_point.x;
        } else {
            drag_distance = g_swipe_start_point.x - cur_point.x;
        }

        if (drag_distance > 0) {
            gesture_back_update_hint(g_swipe_edge_dir, drag_distance);
        } else {
            gesture_back_hide_hint();
        }
        return;
    }

    case LV_EVENT_RELEASED: {
        int same_target = g_swipe_start_valid && (g_swipe_pressed_obj == container);
        int should_trigger = same_target && g_swipe_edge_dir != 0 && g_swipe_trigger_ready;

        if (same_target == 0) {
            gesture_back_reset_swipe_state();
            return;
        }

        /* 抬手后再决定是否触发返回，避免按压过程中提前返回。 */
        gesture_back_reset_swipe_state();
        if (should_trigger && g_active_action != NULL) {
            g_active_action(NULL);
        }
        return;
    }

    case LV_EVENT_PRESS_LOST:
        /* 触点丢失仅清理状态，不触发返回。 */
        gesture_back_reset_swipe_state();
        return;

    default:
        return;
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

    /* 页面根容器不应参与拖动滚动，避免边缘返回时出现“整页跟手位移”。 */
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    /* 允许手势识别，但不向父对象冒泡 GESTURE 事件。 */
    lv_obj_clear_flag(container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_obj_add_event_cb(container, gesture_back_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(container, gesture_back_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(container, gesture_back_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(container, gesture_back_event_cb, LV_EVENT_PRESS_LOST, NULL);
}

void gesture_back_set_left_edge_swipe_cb(lv_obj_t* container, lv_event_cb_t action)
{
    if (container == NULL || action == NULL) {
        return;
    }

    g_active_container = container;
    g_active_action = action;
    gesture_back_hide_hint();
}

void gesture_back_clear_active_swipe_cb(lv_obj_t* container)
{
    if (container == NULL) {
        return;
    }

    if (g_active_container != container) {
        return;
    }

    g_active_container = NULL;
    g_active_action = NULL;
    gesture_back_hide_hint();
}
