// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_screen_display_test.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/key_manager.h"
#include "core/page_manager.h"
#include "core/power_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SDT_TOP_BAR_HEIGHT 56
#define SDT_BTN_HEIGHT 50
#define SDT_START_BTN_WIDTH 110
#define SDT_BTN_MARGIN 10

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

/* 七色循环：红橙黄绿蓝靛紫（靛用标准 indigo）。 */
static const uint32_t g_sdt_colors[PAGE_SCREEN_DISPLAY_TEST_COLOR_COUNT] = {
    0xFF0000U, /* 红 */
    0xFFA500U, /* 橙 */
    0xFFFF00U, /* 黄 */
    0x00FF00U, /* 绿 */
    0x0000FFU, /* 蓝 */
    0x4B0082U, /* 靛 */
    0x800080U, /* 紫 */
};

/* 存活实例指针：按键回调经 e==NULL 复用按钮逻辑时取它，create 末尾置本实例、
 * destroy 开头(free 前)置 NULL，避免解引用已 free 的 data。 */
static page_screen_display_test_data_t* g_sdt_active = NULL;

static void stop_test(page_screen_display_test_data_t* data);
static void start_test(page_screen_display_test_data_t* data);
static void start_btn_cb(lv_event_t* e);
static void back_btn_cb(lv_event_t* e);

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

/* 刷新切换次数显示。 */
static void refresh_switch_count(page_screen_display_test_data_t* data)
{
    char buf[48];

    if (data == NULL || data->count_label == NULL) {
        return;
    }
    snprintf(buf, sizeof(buf), "切换次数: %llu", (unsigned long long)data->switch_count);
    lv_label_set_text(data->count_label, buf);
}

/* 定时器到期：切到下一种颜色。g_sdt_active 已销毁则空跑（timer 先于 free 删除）。 */
static void sdt_timer_cb(lv_timer_t* timer)
{
    page_screen_display_test_data_t* data = (page_screen_display_test_data_t*)lv_timer_get_user_data(timer);

    if (data == NULL || !data->running || data->color_view == NULL) {
        return;
    }
    data->color_idx = (uint8_t)((data->color_idx + 1U) % PAGE_SCREEN_DISPLAY_TEST_COLOR_COUNT);
    data->switch_count++;
    lv_obj_set_style_bg_color(data->color_view, lv_color_hex(g_sdt_colors[data->color_idx]), LV_PART_MAIN);
    refresh_switch_count(data);
}

/* 停止换色并恢复页面：删定时器/全屏色块、按钮标签回「开始」、状态栏恢复显示。
 * 幂等，可重复调用。 */
static void stop_test(page_screen_display_test_data_t* data)
{
    if (data == NULL) {
        return;
    }
    data->running = 0U;
    if (data->timer != NULL) {
        lv_timer_del(data->timer);
        data->timer = NULL;
    }
    if (data->color_view != NULL) {
        lv_obj_del(data->color_view);
        data->color_view = NULL;
    }
    if (data->start_label != NULL) {
        lv_label_set_text(data->start_label, "开始");
    }
}

/* 开始换色：纯色视图填充顶栏以下整块区域（顶栏/返回/按钮/计数标签均保留其上，
 * 与其他页面一致，状态栏不触碰），从第一种颜色起播。 */
static void start_test(page_screen_display_test_data_t* data)
{
    if (data == NULL || data->container == NULL) {
        return;
    }
    stop_test(data);

    data->running = 1U;
    data->color_idx = 0U;
    data->switch_count = 0U;
    refresh_switch_count(data);

    data->color_view = lv_obj_create(data->container);
    lv_obj_set_size(data->color_view, lv_obj_get_width(data->container),
        lv_obj_get_height(data->container) - SDT_TOP_BAR_HEIGHT);
    lv_obj_set_pos(data->color_view, 0, SDT_TOP_BAR_HEIGHT);
    lv_obj_clear_flag(data->color_view, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(data->color_view, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(data->color_view, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(data->color_view, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->color_view, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->color_view, lv_color_hex(g_sdt_colors[data->color_idx]), LV_PART_MAIN);

    /* 按钮与计数标签保留在色块之上。 */
    lv_obj_move_foreground(data->start_btn);
    lv_obj_move_foreground(data->count_label);

    if (data->start_label != NULL) {
        lv_label_set_text(data->start_label, "停止");
    }

    data->timer = lv_timer_create(sdt_timer_cb, SCREEN_DISPLAY_TEST_INTERVAL_MS, data);
    MLOG_INFO("屏幕显示测试开始: %d 色 x %dms", PAGE_SCREEN_DISPLAY_TEST_COLOR_COUNT, SCREEN_DISPLAY_TEST_INTERVAL_MS);
}

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

/* 开始/停止 切换。触摸时从 event 取 data；按键调用(e==NULL)时用存活实例指针。 */
static void start_btn_cb(lv_event_t* e)
{
    page_screen_display_test_data_t* data
        = e ? (page_screen_display_test_data_t*)lv_event_get_user_data(e) : g_sdt_active;

    if (data == NULL) {
        return;
    }
    if (data->running) {
        MLOG_INFO("屏幕显示测试停止");
        stop_test(data);
        return;
    }
    start_test(data);
}

/* 返回：运行中先停止再返回上一级。 */
static void back_btn_cb(lv_event_t* e)
{
    /* 触摸时从 event 取 data；按键调用(e==NULL)时用存活实例指针。 */
    page_screen_display_test_data_t* data
        = e ? (page_screen_display_test_data_t*)lv_event_get_user_data(e) : g_sdt_active;

    if (data != NULL) {
        stop_test(data);
    }
    page_manager_back();
}

/* OK 键：开始/停止（复用开始按钮点击逻辑）。 */
static void ok_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)key;
    (void)event_type;
    (void)user_data;
    start_btn_cb(NULL);
}

/* 菜单键：返回上一级（复用返回按钮点击逻辑）。 */
static void menu_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)key;
    (void)event_type;
    (void)user_data;
    back_btn_cb(NULL);
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_screen_display_test_create(void)
{
    page_screen_display_test_data_t* data = (page_screen_display_test_data_t*)malloc(sizeof(page_screen_display_test_data_t));
    lv_obj_t* top_bar;
    lv_obj_t* back_btn;
    lv_obj_t* back_icon;

    if (data == NULL) {
        return;
    }
    memset(data, 0, sizeof(page_screen_display_test_data_t));

    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(data->container, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_refr_size(data->container);

    /* =======================
     * 1. 顶部导航栏
     * ======================= */
    top_bar = lv_obj_create(data->container);
    lv_obj_set_size(top_bar, LV_PCT(100), SDT_TOP_BAR_HEIGHT);
    lv_obj_add_style(top_bar, &style_common_cont_top, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(top_bar, LV_SCROLLBAR_MODE_OFF);

    back_btn = lv_btn_create(top_bar);
    lv_obj_set_size(back_btn, 50, 50);
    lv_obj_add_style(back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, data);
    back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle-white.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    data->title_label = lv_label_create(top_bar);
    lv_label_set_text(data->title_label, "屏幕显示测试");
    lv_obj_add_style(data->title_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->title_label, LV_ALIGN_CENTER, 0, 0);

    /* =======================
     * 2. 开始按钮（右下角）：开始/停止换色
     * ======================= */
    data->start_btn = lv_btn_create(data->container);
    lv_obj_set_size(data->start_btn, SDT_START_BTN_WIDTH, SDT_BTN_HEIGHT);
    lv_obj_set_style_radius(data->start_btn, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->start_btn, lv_color_hex(0x27AE60), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->start_btn, LV_OPA_80, LV_PART_MAIN);
    lv_obj_align(data->start_btn, LV_ALIGN_BOTTOM_RIGHT, -SDT_BTN_MARGIN, -SDT_BTN_MARGIN);
    lv_obj_add_event_cb(data->start_btn, start_btn_cb, LV_EVENT_CLICKED, data);
    data->start_label = lv_label_create(data->start_btn);
    lv_label_set_text(data->start_label, "开始");
    lv_obj_add_style(data->start_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->start_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(data->start_label);

    /* =======================
     * 3. 切换次数显示（底部居中）：半透明黑底白字，任何颜色上都可读
     * ======================= */
    data->count_label = lv_label_create(data->container);
    lv_obj_add_style(data->count_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->count_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->count_label, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->count_label, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_radius(data->count_label, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(data->count_label, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(data->count_label, 2, LV_PART_MAIN);
    lv_obj_align(data->count_label, LV_ALIGN_BOTTOM_MID, 0, -SDT_BTN_MARGIN);
    refresh_switch_count(data);

    page_set_private_data(data);
    g_sdt_active = data;
}

void page_screen_display_test_destroy(void)
{
    page_screen_display_test_data_t* data = page_get_private_data();

    if (data == NULL) {
        return;
    }

    /* 先清存活指针，再做停止清理（删定时器/色块）。 */
    g_sdt_active = NULL;
    stop_test(data);
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

void page_screen_display_test_show(void)
{
    page_screen_display_test_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Screen display test page show");
    if (!data->auto_sleep_disabled) {
        power_manager_disable_auto_sleep();
        data->auto_sleep_disabled = 1U;
    }

    key_manager_register_callback(KEY_ID_OK, KEY_EVENT_CLICK, ok_key_cb, data);
    key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_cb, NULL);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_screen_display_test_hide(void)
{
    page_screen_display_test_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Screen display test page hide");
    key_manager_unregister_callback(KEY_ID_OK, KEY_EVENT_CLICK, ok_key_cb, data);
    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_cb, NULL);
    stop_test(data);
    if (data->auto_sleep_disabled) {
        power_manager_enable_auto_sleep();
        data->auto_sleep_disabled = 0U;
    }
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_screen_display_test_update(void)
{
}

// #endregion
