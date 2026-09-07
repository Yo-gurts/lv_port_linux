// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################
#include "ui/brightness_bar.h"
#include "config.h"
#include "core/param_manager.h"
#include "core/power_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <stdlib.h>
#include <string.h>

/* 亮度控制条超时时间（毫秒） */
#define BRIGHTNESS_BAR_TIMEOUT_MS 3000
/* 淡出动画时间（毫秒） */
#define BRIGHTNESS_BAR_FADE_MS 300

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义
// #############################################################################
/* 亮度控制条数据结构 */
typedef struct {
    lv_obj_t* container; /* 容器 */
    lv_obj_t* icon; /* 亮度图标（顶部） */
    lv_obj_t* slider; /* 竖直滑块 */
    lv_timer_t* timer; /* 自动隐藏定时器 */
    uint8_t is_visible; /* 是否可见 */
} brightness_bar_data_t;

// #endregion
// #############################################################################
// ! #region 3. 全局变量 &  函数声明
// #############################################################################
static brightness_bar_data_t* g_brightness_bar = NULL;

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

/* 将亮度值限制在 10~100 的有效范围内。 */
static int clamp_brightness(int value)
{
    if (value < POWER_MANAGER_LUMA_MIN) {
        return POWER_MANAGER_LUMA_MIN;
    }
    if (value > POWER_MANAGER_LUMA_MAX) {
        return POWER_MANAGER_LUMA_MAX;
    }
    return value;
}

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################
/**
 * @brief 自动隐藏定时器回调
 */
static void brightness_bar_timer_cb(lv_timer_t* timer)
{
    brightness_bar_data_t* data = (brightness_bar_data_t*)lv_timer_get_user_data(timer);
    if (data == NULL || data->is_visible == 0) {
        return;
    }

    /* 隐藏后必须不可触摸 */
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
    data->is_visible = 0;
    MLOG_DBG("Brightness bar auto hidden");
}

/**
 * @brief 滑块值改变回调
 */
static void brightness_bar_slider_cb(lv_event_t* e)
{
    brightness_bar_data_t* data = (brightness_bar_data_t*)lv_event_get_user_data(e);
    if (data == NULL) {
        return;
    }

    /* 获取滑块值 */
    lv_obj_t* target = lv_event_get_target(e);
    int brightness = clamp_brightness(lv_slider_get_value(target));

    /* 单一真相源：只写 param_manager，硬件下发与各处回显统一由 poll 派发的
     * param 回调完成（brightness_bar_param_cb 下发背光并同步本滑块，
     * system_settings_param_cb 刷新设置行 xx%）。 */
    param_manager_set(PARAM_ID_BRIGHTNESS, brightness);

    /* 重置自动隐藏定时器 */
    brightness_bar_reset_timer();
}

/**
 * @brief 亮度参数变化回调：下发背光并同步滑块。
 */
static void brightness_bar_param_cb(param_id_t id, int value, void* user_data)
{
    int clamped = 0;

    (void)user_data;
    if (id != PARAM_ID_BRIGHTNESS || g_brightness_bar == NULL || g_brightness_bar->slider == NULL) {
        return;
    }

    clamped = clamp_brightness(value);

    /* 下发到背光硬件 */
    power_manager_set_luma(clamped);

    /* 同步滑块显示（拖动本身已到位，此处兼容外部 set） */
    lv_slider_set_value(g_brightness_bar->slider, clamped, LV_ANIM_OFF);
    MLOG_DBG("Brightness set to: %d", clamped);
}

/**
 * @brief 触摸回调 - 重置定时器
 */
static void brightness_bar_press_cb(lv_event_t* e)
{
    (void)e;
    brightness_bar_reset_timer();
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################
/**
 * @brief 创建亮度控制条
 */
static void brightness_bar_create(void)
{
    if (g_brightness_bar != NULL) {
        return;
    }

    MLOG_INFO("Creating vertical brightness bar");

    /* 分配内存 */
    g_brightness_bar = (brightness_bar_data_t*)malloc(sizeof(brightness_bar_data_t));
    if (g_brightness_bar == NULL) {
        MLOG_ERR("Failed to allocate memory for brightness bar");
        return;
    }
    memset(g_brightness_bar, 0, sizeof(brightness_bar_data_t));

    /* 屏幕尺寸 */
    int32_t screen_width = lv_obj_get_width(lv_screen_active());
    int32_t screen_height = lv_obj_get_height(lv_screen_active());

    /* 亮度条位置：紧挨音量条左侧（音量条 x=screen_width-70、宽 60，二者宽度一致，
     * 留 10px 间隙）。 */
    int bar_x = screen_width - 70 - 60 - 10;
    int bar_y = (screen_height - 320) / 2;

    /* 创建容器 */
    g_brightness_bar->container = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(g_brightness_bar->container, bar_x, bar_y);
    lv_obj_set_size(g_brightness_bar->container, 60, 320);
    lv_obj_set_scrollbar_mode(g_brightness_bar->container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(g_brightness_bar->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(g_brightness_bar->container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_brightness_bar->container, LV_OBJ_FLAG_CLICKABLE);

    /* 设置容器背景半透明 */
    static lv_style_t style_container;
    lv_style_init(&style_container);
    lv_style_set_bg_opa(&style_container, LV_OPA_60);
    lv_style_set_bg_color(&style_container, lv_color_black());
    lv_style_set_border_width(&style_container, 0);
    lv_style_set_radius(&style_container, 30);
    lv_obj_add_style(g_brightness_bar->container, &style_container, LV_PART_MAIN);

    /* 亮度图标（顶部，与音量条对齐） */
    int initial_brightness = clamp_brightness(param_manager_get(PARAM_ID_BRIGHTNESS));
    g_brightness_bar->icon = lv_img_create(g_brightness_bar->container);
    lv_img_set_src(g_brightness_bar->icon, "A:" RES_ICON_PATH "/sys-brightness.png");
    lv_obj_align(g_brightness_bar->icon, LV_ALIGN_TOP_MID, 0, 10);

    /* 创建竖直滑块（初值取 param 单一真相源） */
    g_brightness_bar->slider = lv_slider_create(g_brightness_bar->container);
    lv_obj_set_size(g_brightness_bar->slider, 16, 200);
    lv_obj_clear_flag(g_brightness_bar->slider, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(g_brightness_bar->slider, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_slider_set_range(g_brightness_bar->slider, POWER_MANAGER_LUMA_MIN, POWER_MANAGER_LUMA_MAX);
    lv_slider_set_value(g_brightness_bar->slider, initial_brightness, LV_ANIM_OFF);

    /* 滑块样式 - 轨道背景 */
    static lv_style_t style_slider_bg;
    lv_style_init(&style_slider_bg);
    lv_style_set_bg_opa(&style_slider_bg, LV_OPA_40);
    lv_style_set_bg_color(&style_slider_bg, lv_color_white());
    lv_style_set_radius(&style_slider_bg, 8);
    lv_style_set_border_width(&style_slider_bg, 0);
    lv_obj_add_style(g_brightness_bar->slider, &style_slider_bg, LV_PART_MAIN);

    /* 滑块样式 - 进度指示 */
    static lv_style_t style_slider_indic;
    lv_style_init(&style_slider_indic);
    lv_style_set_bg_opa(&style_slider_indic, LV_OPA_COVER);
    lv_style_set_bg_color(&style_slider_indic, lv_color_hex(0x4A90D9));
    lv_style_set_radius(&style_slider_indic, 8);
    lv_obj_add_style(g_brightness_bar->slider, &style_slider_indic, LV_PART_INDICATOR);

    /* 滑块样式 - 旋钮 */
    static lv_style_t style_slider_knob;
    lv_style_init(&style_slider_knob);
    lv_style_set_bg_opa(&style_slider_knob, LV_OPA_COVER);
    lv_style_set_bg_color(&style_slider_knob, lv_color_white());
    lv_style_set_radius(&style_slider_knob, LV_RADIUS_CIRCLE);
    lv_style_set_width(&style_slider_knob, 24);
    lv_style_set_height(&style_slider_knob, 24);
    lv_obj_add_style(g_brightness_bar->slider, &style_slider_knob, LV_PART_KNOB);

    g_brightness_bar->is_visible = 0;

    /* 添加事件回调 */
    lv_obj_add_event_cb(g_brightness_bar->slider, brightness_bar_slider_cb, LV_EVENT_VALUE_CHANGED, g_brightness_bar);
    lv_obj_add_event_cb(g_brightness_bar->slider, brightness_bar_press_cb, LV_EVENT_PRESSED, g_brightness_bar);
    lv_obj_add_event_cb(g_brightness_bar->container, brightness_bar_press_cb, LV_EVENT_CLICKED, g_brightness_bar);

    /* 注册 param 回调：亮度变化时下发背光并同步滑块 */
    param_manager_register_callback(brightness_bar_param_cb, NULL);

    MLOG_INFO("Vertical brightness bar created successfully");
}

// #endregion
// #############################################################################
// ! #region 5. 对外接口函数
// #############################################################################
/**
 * @brief 初始化亮度控制条
 */
void brightness_bar_init(void)
{
    brightness_bar_create();
}

/**
 * @brief 显示亮度控制条
 */
void brightness_bar_show(void)
{
    if (g_brightness_bar == NULL) {
        brightness_bar_create();
    }

    if (g_brightness_bar == NULL) {
        return;
    }

    /* 显示前同步当前亮度到滑块（取 param 单一真相源） */
    if (g_brightness_bar->slider != NULL) {
        lv_slider_set_value(g_brightness_bar->slider, clamp_brightness(param_manager_get(PARAM_ID_BRIGHTNESS)), LV_ANIM_OFF);
    }

    /* 显示容器 */
    lv_obj_clear_flag(g_brightness_bar->container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_fade_in(g_brightness_bar->container, BRIGHTNESS_BAR_FADE_MS, 0);
    g_brightness_bar->is_visible = 1;

    /* 删除旧定时器 */
    if (g_brightness_bar->timer != NULL) {
        lv_timer_del(g_brightness_bar->timer);
        g_brightness_bar->timer = NULL;
    }

    /* 创建新定时器 */
    g_brightness_bar->timer = lv_timer_create(brightness_bar_timer_cb, BRIGHTNESS_BAR_TIMEOUT_MS, g_brightness_bar);
    MLOG_DBG("Brightness bar shown");
}

/**
 * @brief 隐藏亮度控制条
 */
void brightness_bar_hide(void)
{
    if (g_brightness_bar == NULL || g_brightness_bar->is_visible == 0) {
        return;
    }

    /* 删除定时器 */
    if (g_brightness_bar->timer != NULL) {
        lv_timer_del(g_brightness_bar->timer);
        g_brightness_bar->timer = NULL;
    }

    /* 隐藏后必须不可触摸 */
    lv_obj_add_flag(g_brightness_bar->container, LV_OBJ_FLAG_HIDDEN);
    g_brightness_bar->is_visible = 0;
    MLOG_DBG("Brightness bar hidden");
}

/**
 * @brief 重置自动隐藏计时器
 */
void brightness_bar_reset_timer(void)
{
    if (g_brightness_bar == NULL || g_brightness_bar->is_visible == 0) {
        return;
    }

    /* 删除旧定时器 */
    if (g_brightness_bar->timer != NULL) {
        lv_timer_del(g_brightness_bar->timer);
        g_brightness_bar->timer = NULL;
    }

    /* 创建新定时器 */
    g_brightness_bar->timer = lv_timer_create(brightness_bar_timer_cb, BRIGHTNESS_BAR_TIMEOUT_MS, g_brightness_bar);
}

// #endregion
// #############################################################################
// ! #region 6. 线程处理函数
// #############################################################################
// brightness_bar 模块当前不涉及线程处理逻辑。

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################
// brightness_bar 模块当前无独立调试与测试入口。

// #endregion
