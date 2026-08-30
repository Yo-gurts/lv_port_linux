// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_key_test.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/key_manager.h"
#include "core/page_manager.h"
#include "core/power_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <stdlib.h>
#include <string.h>

#define KT_TOP_BAR_HEIGHT 56
#define KT_KEY_DIAMETER 44

/* 三态配色 */
#define KT_COLOR_IDLE 0x555555 /* 未按下 灰 */
#define KT_COLOR_PRESS 0x2F80ED /* 按下中 蓝 */
#define KT_COLOR_DONE 0x27AE60 /* 已按过 绿 */

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_key_test.h)
// #############################################################################

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

/*
 * 按键布局：以 KEY_ID 为下标（0~12），坐标为按键"圆心"在 container 内的全屏坐标
 * （原型区域偏移 left:168 top:64 已折算进来）。图标统一 key-*.png（45x45，纯白透明）。
 */
typedef struct {
    lv_coord_t cx;
    lv_coord_t cy;
    const char* icon;
} kt_key_layout_t;

static const kt_key_layout_t g_kt_layout[PAGE_KEY_TEST_KEY_COUNT] = {
    /* KEY_ID_POWER       */ { 372, 91, "A:" RES_ICON_PATH "/key-power.png" },
    /* KEY_ID_ASSISTANT   */ { 427, 91, "A:" RES_ICON_PATH "/key-ai.png" },
    /* KEY_ID_VOLUME_UP=T */ { 522, 148, "A:" RES_ICON_PATH "/key-tele.png" },
    /* KEY_ID_VOLUME_DOWN=W */ { 522, 200, "A:" RES_ICON_PATH "/key-wide.png" },
    /* KEY_ID_FOCUS       */ { 493, 91, "A:" RES_ICON_PATH "/key-focus.png" },
    /* KEY_ID_CAMERA      */ { 541, 91, "A:" RES_ICON_PATH "/key-camera.png" },
    /* KEY_ID_MODE        */ { 428, 436, "A:" RES_ICON_PATH "/key-mode.png" },
    /* KEY_ID_MENU        */ { 540, 436, "A:" RES_ICON_PATH "/key-menu.png" },
    /* KEY_ID_UP          */ { 484, 256, "A:" RES_ICON_PATH "/key-up.png" },
    /* KEY_ID_DOWN        */ { 484, 372, "A:" RES_ICON_PATH "/key-down.png" },
    /* KEY_ID_LEFT        */ { 426, 314, "A:" RES_ICON_PATH "/key-left.png" },
    /* KEY_ID_RIGHT       */ { 542, 314, "A:" RES_ICON_PATH "/key-right.png" },
    /* KEY_ID_OK          */ { 484, 314, "A:" RES_ICON_PATH "/key-ok.png" },
};

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

static void update_key_item_visual(page_key_test_data_t* data, int key_index)
{
    page_key_test_key_item_t* item;
    uint32_t color;

    if (data == NULL || key_index < 0 || key_index >= PAGE_KEY_TEST_KEY_COUNT) {
        return;
    }
    item = &data->key_items[key_index];
    if (item->btn == NULL) {
        return;
    }

    if (item->pressing_now) {
        color = KT_COLOR_PRESS;
    } else if (item->pressed_once) {
        color = KT_COLOR_DONE;
    } else {
        color = KT_COLOR_IDLE;
    }
    lv_obj_set_style_bg_color(item->btn, lv_color_hex(color), LV_PART_MAIN);
}

static int is_all_keys_passed(page_key_test_data_t* data)
{
    int i;

    if (data == NULL) {
        return 0;
    }
    for (i = 0; i < PAGE_KEY_TEST_KEY_COUNT; ++i) {
        if (!data->key_items[i].pressed_once) {
            return 0;
        }
    }
    return 1;
}

static void update_overall_result(page_key_test_data_t* data)
{
    if (data == NULL || data->summary_label == NULL) {
        return;
    }

    if (is_all_keys_passed(data)) {
        lv_label_set_text(data->summary_label, "结果: PASS");
        lv_obj_set_style_text_color(data->summary_label, lv_color_hex(KT_COLOR_DONE), LV_PART_MAIN);
    } else {
        lv_label_set_text(data->summary_label, "结果: WAITING");
        lv_obj_set_style_text_color(data->summary_label, lv_color_hex(0xF5A623), LV_PART_MAIN);
    }
}

static void reset_test_state(page_key_test_data_t* data)
{
    int i;

    if (data == NULL) {
        return;
    }
    for (i = 0; i < PAGE_KEY_TEST_KEY_COUNT; ++i) {
        data->key_items[i].pressed_once = 0U;
        data->key_items[i].pressing_now = 0U;
        update_key_item_visual(data, i);
    }
    update_overall_result(data);
}

/* 虚线胶囊分组框：给定包围两键的矩形（container 内全屏坐标），弧度取短边一半 */
static void create_dash_capsule(lv_obj_t* parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t* box = lv_obj_create(parent);
    lv_coord_t rad = (w < h ? w : h) / 2;

    lv_obj_remove_style_all(box);
    lv_obj_set_size(box, w, h);
    lv_obj_add_flag(box, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_style_radius(box, rad, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(box, lv_color_hex(0x6A6A6A), LV_PART_MAIN);
    lv_obj_set_style_border_width(box, 2, LV_PART_MAIN);
    lv_obj_set_style_border_opa(box, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_move_background(box);
}

/* 方向盘双圆框：以圆心 (cx,cy) 画一个半径为 r 的虚线圆 */
static void create_dash_ring(lv_obj_t* parent, lv_coord_t cx, lv_coord_t cy, lv_coord_t r)
{
    lv_obj_t* ring = lv_obj_create(parent);

    lv_obj_remove_style_all(ring);
    lv_obj_set_size(ring, r * 2, r * 2);
    lv_obj_add_flag(ring, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(ring, cx - r, cy - r);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(ring, lv_color_hex(0x6A6A6A), LV_PART_MAIN);
    lv_obj_set_style_border_width(ring, 2, LV_PART_MAIN);
    lv_obj_set_style_border_opa(ring, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_move_background(ring);
}

/* 图例：一行彩色圆点 + 文字 */
static lv_obj_t* create_legend_row(lv_obj_t* parent, lv_coord_t y, uint32_t color, const char* text)
{
    lv_obj_t* dot;
    lv_obj_t* label;

    dot = lv_obj_create(parent);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 22, 22);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(dot, 20, y);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);

    label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_add_style(label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_add_flag(label, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_pos(label, 52, y + 2);
    return label;
}

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

static void test_key_event_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_key_test_data_t* data = (page_key_test_data_t*)user_data;
    int key_index = (int)key;

    if (data == NULL || key_index < 0 || key_index >= PAGE_KEY_TEST_KEY_COUNT) {
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

static void register_test_key_callbacks(page_key_test_data_t* data)
{
    int ret_press;
    int ret_release;

    if (data == NULL || data->key_callbacks_registered) {
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

static void unregister_test_key_callbacks(page_key_test_data_t* data)
{
    if (data == NULL || !data->key_callbacks_registered) {
        return;
    }
    (void)key_manager_unregister_callback(KEY_ID_ANY, KEY_EVENT_PRESS, test_key_event_cb, data);
    (void)key_manager_unregister_callback(KEY_ID_ANY, KEY_EVENT_RELEASE, test_key_event_cb, data);
    data->key_callbacks_registered = 0U;
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_key_test_create(void)
{
    page_key_test_data_t* data = (page_key_test_data_t*)malloc(sizeof(page_key_test_data_t));
    int i;
    lv_obj_t* top_bar;
    lv_obj_t* back_btn;
    lv_obj_t* back_icon;
    lv_obj_t* hint;

    if (data == NULL) {
        return;
    }
    memset(data, 0, sizeof(page_key_test_data_t));

    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(data->container, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->container, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(data->container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_refr_size(data->container);

    /* 顶栏：返回 + 标题 + 结果 */
    top_bar = lv_obj_create(data->container);
    lv_obj_set_size(top_bar, LV_PCT(100), KT_TOP_BAR_HEIGHT);
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
    lv_label_set_text(data->title_label, "按键测试");
    lv_obj_add_style(data->title_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->title_label, LV_ALIGN_LEFT_MID, 70, 0);

    data->summary_label = lv_label_create(top_bar);
    lv_label_set_text(data->summary_label, "结果: WAITING");
    lv_obj_add_style(data->summary_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->summary_label, LV_ALIGN_RIGHT_MID, -16, 0);

    /* 左侧图例：三态彩色圆点 + 文字 */
    create_legend_row(data->container, 96, KT_COLOR_IDLE, "未按下");
    create_legend_row(data->container, 136, KT_COLOR_PRESS, "按下中");
    create_legend_row(data->container, 176, KT_COLOR_DONE, "已按过");
    hint = lv_label_create(data->container);
    lv_label_set_text(hint, "逐一按下每个\n实体按键\n全部变绿即通过");
    lv_obj_add_style(hint, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x9A9A9A), LV_PART_MAIN);
    lv_obj_add_flag(hint, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_pos(hint, 20, 216);

    /* 分组虚线框（先画，位于按键下层）：
     *   对焦(493,91)+拍照(541,91) 胶囊；T(522,148)+W(522,200) 胶囊；方向盘双圆 */
    {
        const lv_coord_t d = KT_KEY_DIAMETER;
        const lv_coord_t pad = 8;
        /* 对焦+拍照 横向胶囊 */
        create_dash_capsule(data->container,
            493 - d / 2 - pad, 91 - d / 2 - pad,
            (541 - 493) + d + 2 * pad, d + 2 * pad);
        /* T+W 纵向胶囊 */
        create_dash_capsule(data->container,
            522 - d / 2 - pad, 148 - d / 2 - pad,
            d + 2 * pad, (200 - 148) + d + 2 * pad);
        /* 方向盘双圆：圆心 (484,314)，外圆 88 内圆 30 */
        create_dash_ring(data->container, 484, 314, 88);
        create_dash_ring(data->container, 484, 314, 30);
    }

    /* 13 个按键：圆形按钮 + 白色 png 图标，无文字 */
    for (i = 0; i < PAGE_KEY_TEST_KEY_COUNT; ++i) {
        page_key_test_key_item_t* item = &data->key_items[i];
        lv_obj_t* btn = lv_obj_create(data->container);
        lv_obj_t* icon;

        lv_obj_remove_style_all(btn);
        lv_obj_set_size(btn, KT_KEY_DIAMETER, KT_KEY_DIAMETER);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(btn, g_kt_layout[i].cx - KT_KEY_DIAMETER / 2,
            g_kt_layout[i].cy - KT_KEY_DIAMETER / 2);
        lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, lv_color_hex(KT_COLOR_IDLE), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);

        icon = lv_img_create(btn);
        lv_img_set_src(icon, g_kt_layout[i].icon);
        lv_obj_align(icon, LV_ALIGN_CENTER, 0, 0);

        item->btn = btn;
        update_key_item_visual(data, i);
    }

    update_overall_result(data);
    page_set_private_data(data);
}

void page_key_test_destroy(void)
{
    page_key_test_data_t* data = page_get_private_data();

    if (data == NULL) {
        return;
    }
    unregister_test_key_callbacks(data);
    if (data->auto_sleep_disabled) {
        power_manager_enable_auto_sleep();
        data->auto_sleep_disabled = 0U;
    }
    if (data->container != NULL) {
        lv_obj_del(data->container);
        data->container = NULL;
    }
    free(data);
}

void page_key_test_show(void)
{
    page_key_test_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Key test page show");
    if (!data->auto_sleep_disabled) {
        power_manager_disable_auto_sleep();
        data->auto_sleep_disabled = 1U;
    }
    reset_test_state(data);
    register_test_key_callbacks(data);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_key_test_hide(void)
{
    page_key_test_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Key test page hide");
    unregister_test_key_callbacks(data);
    if (data->auto_sleep_disabled) {
        power_manager_enable_auto_sleep();
        data->auto_sleep_disabled = 0U;
    }
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_key_test_update(void)
{
    page_key_test_data_t* data = page_get_private_data();
    int i;

    if (data == NULL) {
        return;
    }
    for (i = 0; i < PAGE_KEY_TEST_KEY_COUNT; ++i) {
        update_key_item_visual(data, i);
    }
    update_overall_result(data);
}

// #endregion
