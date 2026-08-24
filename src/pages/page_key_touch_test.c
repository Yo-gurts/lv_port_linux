// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_key_touch_test.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/key_manager.h"
#include "core/page_manager.h"
#include "core/power_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <stdlib.h>
#include <string.h>

#define KTT_TOP_BAR_HEIGHT 56
#define KTT_KEY_CARD_WIDTH 138
#define KTT_KEY_CARD_HEIGHT 68
#define KTT_TOUCH_AREA_WIDTH 620
#define KTT_TOUCH_AREA_HEIGHT 360
#define KTT_TOUCH_DIAG_MIN_MOVE_X 80
#define KTT_TOUCH_DIAG_MAX_SLOPE_DIFF 80

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_key_touch_test.h)
// #############################################################################

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static const char* g_ktt_key_names[PAGE_KEY_TOUCH_TEST_KEY_COUNT] = {
    "电源键",
    "AI键",
    "音量+",
    "音量-",
    "对焦键",
    "拍照键",
};

static void clear_touch_trace(page_key_touch_test_data_t* data);
static void create_touch_guide_diagonals(lv_obj_t* parent, lv_coord_t width, lv_coord_t height);

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

static void update_key_item_visual(page_key_touch_test_data_t* data, int key_index)
{
    page_key_touch_test_key_item_t* item;

    if (data == NULL || key_index < 0 || key_index >= PAGE_KEY_TOUCH_TEST_KEY_COUNT) {
        return;
    }
    item = &data->key_items[key_index];
    if (item->card == NULL || item->state_label == NULL) {
        return;
    }

    if (item->pressing_now) {
        lv_obj_set_style_bg_color(item->card, lv_color_hex(0x2F80ED), LV_PART_MAIN);
        lv_label_set_text(item->state_label, "按下中");
        return;
    }

    if (item->pressed_once) {
        lv_obj_set_style_bg_color(item->card, lv_color_hex(0x27AE60), LV_PART_MAIN);
        lv_label_set_text(item->state_label, "已按过");
        return;
    }

    lv_obj_set_style_bg_color(item->card, lv_color_hex(0x555555), LV_PART_MAIN);
    lv_label_set_text(item->state_label, "未按下");
}

static int is_all_keys_passed(page_key_touch_test_data_t* data)
{
    int i;

    if (data == NULL) {
        return 0;
    }

    for (i = 0; i < PAGE_KEY_TOUCH_TEST_KEY_COUNT; ++i) {
        if (!data->key_items[i].pressed_once) {
            return 0;
        }
    }
    return 1;
}

static void update_overall_result(page_key_touch_test_data_t* data)
{
    int all_passed;

    if (data == NULL || data->summary_label == NULL) {
        return;
    }

    all_passed = is_all_keys_passed(data) && data->touch_passed;
    if (all_passed) {
        lv_label_set_text(data->summary_label, "结果: PASS");
        lv_obj_set_style_text_color(data->summary_label, lv_color_hex(0x27AE60), LV_PART_MAIN);
    } else {
        lv_label_set_text(data->summary_label, "结果: WAITING");
        lv_obj_set_style_text_color(data->summary_label, lv_color_hex(0xF5A623), LV_PART_MAIN);
    }
}

static void reset_test_state(page_key_touch_test_data_t* data)
{
    int i;

    if (data == NULL) {
        return;
    }

    for (i = 0; i < PAGE_KEY_TOUCH_TEST_KEY_COUNT; ++i) {
        data->key_items[i].pressed_once = 0U;
        data->key_items[i].pressing_now = 0U;
        update_key_item_visual(data, i);
    }

    data->touch_tracking = 0U;
    data->touch_passed = 0U;
    data->diag_down_passed = 0U;
    data->diag_up_passed = 0U;
    data->touch_move_accum = 0U;
    clear_touch_trace(data);

    update_overall_result(data);
}

static void clear_touch_trace(page_key_touch_test_data_t* data)
{
    uint16_t i;

    if (data == NULL) {
        return;
    }

    for (i = 0; i < data->trace_dot_count; ++i) {
        if (data->trace_dots[i] != NULL) {
            lv_obj_del(data->trace_dots[i]);
            data->trace_dots[i] = NULL;
        }
    }
    data->trace_dot_count = 0;
}

static void add_trace_dot(page_key_touch_test_data_t* data, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t* dot;

    if (data == NULL || data->touch_area == NULL || data->trace_dot_count >= PAGE_KEY_TOUCH_TEST_MAX_TRACE_DOTS) {
        return;
    }

    dot = lv_obj_create(data->touch_area);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 4, 4);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0x00D2FF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_pos(dot, x - 2, y - 2);

    data->trace_dots[data->trace_dot_count++] = dot;
}

static void create_touch_guide_diagonals(lv_obj_t* parent, lv_coord_t width, lv_coord_t height)
{
    int i;
    int segments;

    if (parent == NULL || width <= 2 || height <= 2) {
        return;
    }

    segments = (int)(width / 8);
    if (segments < 24) {
        segments = 24;
    }

    for (i = 0; i <= segments; ++i) {
        lv_obj_t* dot1;
        lv_obj_t* dot2;
        lv_coord_t x = (lv_coord_t)(((int)(width - 1) * i) / segments);
        lv_coord_t y = (lv_coord_t)(((int)(height - 1) * i) / segments);
        lv_coord_t y2 = (lv_coord_t)((height - 1) - y);

        dot1 = lv_obj_create(parent);
        lv_obj_remove_style_all(dot1);
        lv_obj_set_size(dot1, 3, 3);
        lv_obj_set_style_radius(dot1, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(dot1, lv_color_hex(0x6D6D6D), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(dot1, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_add_flag(dot1, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_clear_flag(dot1, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_pos(dot1, x - 1, y - 1);

        dot2 = lv_obj_create(parent);
        lv_obj_remove_style_all(dot2);
        lv_obj_set_size(dot2, 3, 3);
        lv_obj_set_style_radius(dot2, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(dot2, lv_color_hex(0x6D6D6D), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(dot2, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_add_flag(dot2, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_clear_flag(dot2, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_pos(dot2, x - 1, y2 - 1);
    }
}

static int get_touch_local_point(page_key_touch_test_data_t* data, lv_point_t* out_local)
{
    lv_indev_t* indev;
    lv_area_t area;
    lv_point_t p;

    if (data == NULL || out_local == NULL || data->touch_area == NULL) {
        return -1;
    }

    indev = lv_indev_get_act();
    if (indev == NULL) {
        return -1;
    }
    lv_indev_get_point(indev, &p);
    lv_obj_get_coords(data->touch_area, &area);

    if (p.x < area.x1 || p.x > area.x2 || p.y < area.y1 || p.y > area.y2) {
        return -1;
    }

    out_local->x = p.x - area.x1;
    out_local->y = p.y - area.y1;
    return 0;
}

static int max_int(int a, int b)
{
    return (a > b) ? a : b;
}

static int abs_int(int x)
{
    return (x >= 0) ? x : -x;
}

static void draw_touch_segment(page_key_touch_test_data_t* data, lv_point_t from, lv_point_t to)
{
    int dx;
    int dy;
    int step;
    int i;

    if (data == NULL) {
        return;
    }

    dx = to.x - from.x;
    dy = to.y - from.y;
    step = max_int(abs_int(dx), abs_int(dy)) / 4;
    if (step <= 0) {
        add_trace_dot(data, to.x, to.y);
        return;
    }

    for (i = 1; i <= step; ++i) {
        lv_coord_t x = from.x + (lv_coord_t)(dx * i / step);
        lv_coord_t y = from.y + (lv_coord_t)(dy * i / step);
        add_trace_dot(data, x, y);
    }
}

static void touch_mark_passed(page_key_touch_test_data_t* data)
{
    if (data == NULL || data->touch_passed) {
        return;
    }

    data->touch_passed = 1U;
    update_overall_result(data);
}

static void test_key_event_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_key_touch_test_data_t* data = (page_key_touch_test_data_t*)user_data;
    int key_index = (int)key;

    if (data == NULL || key_index < 0 || key_index >= PAGE_KEY_TOUCH_TEST_KEY_COUNT) {
        return;
    }

    if (event_type == KEY_EVENT_PRESS) {
        data->key_items[key_index].pressing_now = 1U;
        data->key_items[key_index].pressed_once = 1U;
        update_key_item_visual(data, key_index);
        update_overall_result(data);
        return;
    }

    if (event_type == KEY_EVENT_RELEASE) {
        data->key_items[key_index].pressing_now = 0U;
        update_key_item_visual(data, key_index);
        update_overall_result(data);
    }
}

static void back_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_manager_back();
}

static void touch_area_event_cb(lv_event_t* e)
{
    page_key_touch_test_data_t* data = (page_key_touch_test_data_t*)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_point_t local_point;
    int dx;
    int dy;
    int adx;
    int ady;

    if (data == NULL || data->touch_area == NULL) {
        return;
    }

    if (code == LV_EVENT_PRESSED) {
        if (get_touch_local_point(data, &local_point) != 0) {
            return;
        }
        data->touch_tracking = 1U;
        data->touch_move_accum = 0U;
        data->last_touch_local = local_point;
        if (!data->touch_passed) {
            clear_touch_trace(data);
        }
        add_trace_dot(data, local_point.x, local_point.y);
        return;
    }

    if (code == LV_EVENT_PRESSING) {
        if (!data->touch_tracking) {
            return;
        }
        if (get_touch_local_point(data, &local_point) != 0) {
            return;
        }

        dx = (int)local_point.x - (int)data->last_touch_local.x;
        dy = (int)local_point.y - (int)data->last_touch_local.y;
        adx = abs_int(dx);
        ady = abs_int(dy);
        data->touch_move_accum += (uint32_t)(abs_int(dx) + abs_int(dy));
        draw_touch_segment(data, data->last_touch_local, local_point);
        data->last_touch_local = local_point;

        if (adx >= KTT_TOUCH_DIAG_MIN_MOVE_X && abs_int(adx - ady) <= KTT_TOUCH_DIAG_MAX_SLOPE_DIFF) {
            if ((dx > 0 && dy > 0) || (dx < 0 && dy < 0)) {
                data->diag_down_passed = 1U;
            } else if ((dx > 0 && dy < 0) || (dx < 0 && dy > 0)) {
                data->diag_up_passed = 1U;
            }
        }

        if (data->diag_down_passed && data->diag_up_passed) {
            touch_mark_passed(data);
        }
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        data->touch_tracking = 0U;
    }
}

static void register_test_key_callbacks(page_key_touch_test_data_t* data)
{
    int ret_press;
    int ret_release;

    if (data == NULL) {
        return;
    }
    if (data->key_callbacks_registered) {
        return;
    }

    ret_press = key_manager_register_callback(KEY_ID_ANY, KEY_EVENT_PRESS, test_key_event_cb, data);
    if (ret_press != 0) {
        MLOG_WARN("register KEY_EVENT_PRESS(any) failed");
    }
    ret_release = key_manager_register_callback(KEY_ID_ANY, KEY_EVENT_RELEASE, test_key_event_cb, data);
    if (ret_release != 0) {
        MLOG_WARN("register KEY_EVENT_RELEASE(any) failed");
    }
    if (ret_press == 0 && ret_release == 0) {
        data->key_callbacks_registered = 1U;
    } else {
        (void)key_manager_unregister_callback(KEY_ID_ANY, KEY_EVENT_PRESS, test_key_event_cb, data);
        (void)key_manager_unregister_callback(KEY_ID_ANY, KEY_EVENT_RELEASE, test_key_event_cb, data);
    }
}

static void unregister_test_key_callbacks(page_key_touch_test_data_t* data)
{
    if (data == NULL) {
        return;
    }
    if (!data->key_callbacks_registered) {
        return;
    }

    (void)key_manager_unregister_callback(KEY_ID_ANY, KEY_EVENT_PRESS, test_key_event_cb, data);
    (void)key_manager_unregister_callback(KEY_ID_ANY, KEY_EVENT_RELEASE, test_key_event_cb, data);
    data->key_callbacks_registered = 0U;
}

// #endregion
// #############################################################################
// ! #region 5. 对外接口函数
// #############################################################################

// #endregion
// #############################################################################
// ! #region 6. 线程处理函数
// #############################################################################

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_key_touch_test_create(void)
{
    page_key_touch_test_data_t* data = (page_key_touch_test_data_t*)malloc(sizeof(page_key_touch_test_data_t));
    int i;
    lv_obj_t* top_bar;
    lv_obj_t* back_btn;
    lv_obj_t* back_icon;
    static const lv_coord_t key_positions[PAGE_KEY_TOUCH_TEST_KEY_COUNT][2] = {
        { 170, 40 }, /* 电源键 */
        { 312, 40 }, /* AI键 */
        { 60, 150 }, /* 音量+ */
        { 422, 150 }, /* 音量- */
        { 170, 264 }, /* 对焦键 */
        { 312, 264 }, /* 拍照键 */
    };

    if (data == NULL) {
        return;
    }
    memset(data, 0, sizeof(page_key_touch_test_data_t));

    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(data->container, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->container, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_refr_size(data->container);

    top_bar = lv_obj_create(data->container);
    lv_obj_set_size(top_bar, LV_PCT(100), KTT_TOP_BAR_HEIGHT);
    lv_obj_add_style(top_bar, &style_noboarder, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(top_bar, LV_SCROLLBAR_MODE_OFF);

    back_btn = lv_btn_create(top_bar);
    lv_obj_set_size(back_btn, 50, 50);
    lv_obj_add_style(back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    data->title_label = lv_label_create(top_bar);
    lv_label_set_text(data->title_label, "按键与触摸测试");
    lv_obj_add_style(data->title_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->title_label, LV_ALIGN_LEFT_MID, 70, 0);

    data->summary_label = lv_label_create(top_bar);
    lv_label_set_text(data->summary_label, "结果: WAITING");
    lv_obj_add_style(data->summary_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->summary_label, LV_ALIGN_RIGHT_MID, -16, 0);

    data->touch_area = lv_obj_create(data->container);
    lv_obj_set_size(data->touch_area, KTT_TOUCH_AREA_WIDTH, KTT_TOUCH_AREA_HEIGHT);
    lv_obj_align(data->touch_area, LV_ALIGN_TOP_MID, 0, KTT_TOP_BAR_HEIGHT + 20);
    lv_obj_set_style_bg_color(data->touch_area, lv_color_hex(0x121212), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->touch_area, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(data->touch_area, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_set_style_border_width(data->touch_area, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(data->touch_area, 8, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(data->touch_area, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(data->touch_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(data->touch_area, touch_area_event_cb, LV_EVENT_PRESSED, data);
    lv_obj_add_event_cb(data->touch_area, touch_area_event_cb, LV_EVENT_PRESSING, data);
    lv_obj_add_event_cb(data->touch_area, touch_area_event_cb, LV_EVENT_RELEASED, data);
    lv_obj_add_event_cb(data->touch_area, touch_area_event_cb, LV_EVENT_PRESS_LOST, data);
    create_touch_guide_diagonals(data->touch_area, KTT_TOUCH_AREA_WIDTH, KTT_TOUCH_AREA_HEIGHT);

    for (i = 0; i < PAGE_KEY_TOUCH_TEST_KEY_COUNT; ++i) {
        page_key_touch_test_key_item_t* item = &data->key_items[i];
        lv_obj_t* card = lv_obj_create(data->touch_area);

        lv_obj_set_size(card, KTT_KEY_CARD_WIDTH, KTT_KEY_CARD_HEIGHT);
        lv_obj_set_pos(card, key_positions[i][0], key_positions[i][1]);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(card, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_set_style_border_width(card, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(card, 12, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(card, LV_OPA_90, LV_PART_MAIN);
        lv_obj_set_style_pad_all(card, 6, LV_PART_MAIN);
        lv_obj_set_layout(card, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        item->card = card;
        item->name_label = lv_label_create(card);
        lv_label_set_text(item->name_label, g_ktt_key_names[i]);
        lv_obj_add_style(item->name_label, &SMALL_SIZE, LV_PART_MAIN);

        item->state_label = lv_label_create(card);
        lv_obj_add_style(item->state_label, &SMALL_SIZE, LV_PART_MAIN);

        update_key_item_visual(data, i);
    }

    update_overall_result(data);
    page_set_private_data(data);
}

void page_key_touch_test_destroy(void)
{
    page_key_touch_test_data_t* data = page_get_private_data();

    if (data == NULL) {
        return;
    }

    unregister_test_key_callbacks(data);
    if (data->auto_sleep_disabled) {
        power_manager_enable_auto_sleep();
        data->auto_sleep_disabled = 0U;
    }
    clear_touch_trace(data);
    if (data->container != NULL) {
        lv_obj_del(data->container);
        data->container = NULL;
    }
    free(data);
}

void page_key_touch_test_show(void)
{
    page_key_touch_test_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Key/touch test page show");
    if (!data->auto_sleep_disabled) {
        power_manager_disable_auto_sleep();
        data->auto_sleep_disabled = 1U;
    }
    reset_test_state(data);
    register_test_key_callbacks(data);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_key_touch_test_hide(void)
{
    page_key_touch_test_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Key/touch test page hide");
    unregister_test_key_callbacks(data);
    if (data->auto_sleep_disabled) {
        power_manager_enable_auto_sleep();
        data->auto_sleep_disabled = 0U;
    }
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_key_touch_test_update(void)
{
    page_key_touch_test_data_t* data = page_get_private_data();
    int i;

    if (data == NULL) {
        return;
    }
    for (i = 0; i < PAGE_KEY_TOUCH_TEST_KEY_COUNT; ++i) {
        update_key_item_visual(data, i);
    }
    update_overall_result(data);
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
