// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "ui/top_notice.h"
#include "core/font_manager.h"
#include "core/style_manager.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TOP_NOTICE_DEFAULT_MS 1800

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义
// #############################################################################

typedef struct {
    lv_obj_t* container;
    lv_obj_t* label;
    lv_timer_t* hide_timer;
} top_notice_data_t;

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static top_notice_data_t* g_top_notice = NULL;

static lv_color_t get_notice_text_color(top_notice_type_t type);
static lv_color_t get_notice_bg_color(top_notice_type_t type);
static void top_notice_create(void);
static void top_notice_timer_cb(lv_timer_t* timer);

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

static lv_color_t get_notice_text_color(top_notice_type_t type)
{
    switch (type) {
    case TOP_NOTICE_TYPE_SUCCESS:
        return lv_color_hex(0x7CFC00);
    case TOP_NOTICE_TYPE_WARNING:
        return lv_color_hex(0xFFD700);
    case TOP_NOTICE_TYPE_ERROR:
        return lv_palette_main(LV_PALETTE_RED);
    case TOP_NOTICE_TYPE_BLOCKING:
        return lv_color_hex(0x66D9EF);
    case TOP_NOTICE_TYPE_INFO:
    default:
        return lv_color_white();
    }
}

static lv_color_t get_notice_bg_color(top_notice_type_t type)
{
    switch (type) {
    case TOP_NOTICE_TYPE_SUCCESS:
        return lv_color_hex(0x143322);
    case TOP_NOTICE_TYPE_WARNING:
        return lv_color_hex(0x3A2D06);
    case TOP_NOTICE_TYPE_ERROR:
        return lv_color_hex(0x3B1515);
    case TOP_NOTICE_TYPE_BLOCKING:
        return lv_color_hex(0x102733);
    case TOP_NOTICE_TYPE_INFO:
    default:
        return lv_color_hex(0x111111);
    }
}

// #endregion
// #############################################################################
// ! #region 5. 对外接口函数
// #############################################################################

void top_notice_init(void)
{
    top_notice_create();
}

void top_notice_show_for(const char* text, top_notice_type_t type, uint32_t duration_ms)
{
    if (g_top_notice == NULL) {
        top_notice_create();
    }
    if (g_top_notice == NULL || g_top_notice->container == NULL || g_top_notice->label == NULL) {
        return;
    }

    if (text == NULL) {
        text = "";
    }

    lv_label_set_text(g_top_notice->label, text);
    lv_obj_set_style_text_color(g_top_notice->label, get_notice_text_color(type), LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_top_notice->container, get_notice_bg_color(type), LV_PART_MAIN);
    lv_obj_clear_flag(g_top_notice->container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(g_top_notice->container);
    lv_obj_align(g_top_notice->container, LV_ALIGN_BOTTOM_MID, 0, -24);

    if (duration_ms == 0U) {
        duration_ms = TOP_NOTICE_DEFAULT_MS;
    }

    if (g_top_notice->hide_timer != NULL) {
        lv_timer_set_period(g_top_notice->hide_timer, duration_ms);
        lv_timer_reset(g_top_notice->hide_timer);
        lv_timer_resume(g_top_notice->hide_timer);
    }
}

void top_notice_show(const char* text, top_notice_type_t type)
{
    top_notice_show_for(text, type, TOP_NOTICE_DEFAULT_MS);
}

void top_notice_update(const char* text, top_notice_type_t type)
{
    top_notice_show(text, type);
}

void top_notice_hide(void)
{
    if (g_top_notice == NULL || g_top_notice->container == NULL) {
        return;
    }

    if (g_top_notice->hide_timer != NULL) {
        lv_timer_pause(g_top_notice->hide_timer);
    }
    lv_obj_add_flag(g_top_notice->container, LV_OBJ_FLAG_HIDDEN);
}

// #endregion
// #############################################################################
// ! #region 6. 线程处理函数
// #############################################################################

// top_notice 模块当前不涉及线程处理逻辑。

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

static void top_notice_timer_cb(lv_timer_t* timer)
{
    top_notice_data_t* data;

    if (timer == NULL) {
        return;
    }

    data = (top_notice_data_t*)lv_timer_get_user_data(timer);
    if (data == NULL || data->container == NULL) {
        return;
    }

    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
    lv_timer_pause(timer);
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

static void top_notice_create(void)
{
    int32_t screen_width;
    int32_t max_label_width;

    if (g_top_notice != NULL) {
        return;
    }

    g_top_notice = (top_notice_data_t*)malloc(sizeof(top_notice_data_t));
    if (g_top_notice == NULL) {
        return;
    }
    memset(g_top_notice, 0, sizeof(top_notice_data_t));

    g_top_notice->container = lv_obj_create(lv_layer_top());
    lv_obj_set_size(g_top_notice->container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(g_top_notice->container, &style_toast_popup, LV_PART_MAIN);
    lv_obj_add_flag(g_top_notice->container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_top_notice->container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(g_top_notice->container, LV_ALIGN_BOTTOM_MID, 0, -24);

    g_top_notice->label = lv_label_create(g_top_notice->container);
    lv_label_set_long_mode(g_top_notice->label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(g_top_notice->label, "");
    screen_width = lv_obj_get_width(lv_screen_active());
    max_label_width = screen_width - 40;
    if (max_label_width < 120) {
        max_label_width = 120;
    }
    lv_obj_set_style_max_width(g_top_notice->label, max_label_width, LV_PART_MAIN);
    lv_obj_add_style(g_top_notice->label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_center(g_top_notice->label);

    g_top_notice->hide_timer = lv_timer_create(top_notice_timer_cb, TOP_NOTICE_DEFAULT_MS, g_top_notice);
    if (g_top_notice->hide_timer != NULL) {
        lv_timer_pause(g_top_notice->hide_timer);
    }
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// top_notice 模块当前无独立调试与测试入口。

// #endregion
