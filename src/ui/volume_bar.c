// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################
#include "ui/volume_bar.h"
#include "config.h"
#include "core/param_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <stdlib.h>

/* 音量控制条超时时间（毫秒） */
#define VOLUME_BAR_TIMEOUT_MS 3000
/* 淡出动画时间（毫秒） */
#define VOLUME_BAR_FADE_MS 300

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义
// #############################################################################
/* 音量控制条数据结构 */
typedef struct {
    lv_obj_t* container; /* 容器 */
    lv_obj_t* slider; /* 竖直滑块 */
    lv_obj_t* icon; /* 音量图标 */
    lv_timer_t* timer; /* 自动隐藏定时器 */
    uint8_t is_visible; /* 是否可见 */
} volume_bar_data_t;

// #endregion
// #############################################################################
// ! #region 3. 全局变量 &  函数声明
// #############################################################################
static volume_bar_data_t* g_volume_bar = NULL;

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

/* 将音量值限制在 0~100 的有效范围内。 */
static int clamp_volume(int volume)
{
    if (volume < 0) {
        return 0;
    }
    if (volume > 100) {
        return 100;
    }
    return volume;
}

/* 从 param_manager 读取音量，失败时回退默认值。 */
static int get_volume_from_param_manager_or_default(void)
{
    int volume = param_manager_get(PARAM_ID_VOLUME);
    if (volume < 0) {
        volume = param_manager_get_default(PARAM_ID_VOLUME);
    }
    if (volume < 0) {
        volume = 50;
    }
    return clamp_volume(volume);
}

/**
 * @brief 根据音量值获取对应的图标路径
 * @param volume 音量值 (0-100)
 * @return 图标路径字符串
 */
static const char* get_volume_icon_path(int volume)
{
    if (volume == 0) {
        return "A:" RES_ICON_PATH "/volume-mute.png";
    } else if (volume <= 33) {
        return "A:" RES_ICON_PATH "/volume-33.png";
    } else if (volume <= 66) {
        return "A:" RES_ICON_PATH "/volume-66.png";
    } else {
        return "A:" RES_ICON_PATH "/volume-100.png";
    }
}

/**
 * @brief 更新音量图标显示
 * @param data 音量控制条数据
 */
static void update_volume_icon(volume_bar_data_t* data, int volume)
{
    if (data == NULL || data->icon == NULL) {
        return;
    }
    lv_img_set_src(data->icon, get_volume_icon_path(clamp_volume(volume)));
}

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################
/**
 * @brief 自动隐藏定时器回调
 */
static void volume_bar_timer_cb(lv_timer_t* timer)
{
    volume_bar_data_t* data = (volume_bar_data_t*)lv_timer_get_user_data(timer);
    if (data == NULL || data->is_visible == 0) {
        return;
    }

    /* 淡出动画 */
    lv_obj_fade_out(data->container, VOLUME_BAR_FADE_MS, 0);
    data->is_visible = 0;
    MLOG_DBG("Volume bar auto hidden");
}

/**
 * @brief 滑块值改变回调
 */
static void volume_bar_slider_cb(lv_event_t* e)
{
    volume_bar_data_t* data = (volume_bar_data_t*)lv_event_get_user_data(e);
    if (data == NULL) {
        return;
    }

    /* 获取滑块值 */
    lv_obj_t* target = lv_event_get_target(e);
    int volume = clamp_volume(lv_slider_get_value(target));

    /* 更新音量图标 */
    update_volume_icon(data, volume);

    /* 同步到param_manager */
    param_manager_set(PARAM_ID_VOLUME, volume);

    /* 重置自动隐藏定时器 */
    volume_bar_reset_timer();
}

/**
 * @brief 触摸回调 - 重置定时器
 */
static void volume_bar_press_cb(lv_event_t* e)
{
    (void)e;
    volume_bar_reset_timer();
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################
/**
 * @brief 创建音量控制条
 */
static void volume_bar_create(void)
{
    if (g_volume_bar != NULL) {
        return;
    }

    MLOG_INFO("Creating vertical volume bar");

    /* 分配内存 */
    g_volume_bar = (volume_bar_data_t*)malloc(sizeof(volume_bar_data_t));
    if (g_volume_bar == NULL) {
        MLOG_ERR("Failed to allocate memory for volume bar");
        return;
    }
    memset(g_volume_bar, 0, sizeof(volume_bar_data_t));

    /* 屏幕尺寸 */
    int32_t screen_width = lv_obj_get_width(lv_screen_active());
    int32_t screen_height = lv_obj_get_height(lv_screen_active());

    /* 音量条位置：屏幕右侧 */
    int bar_x = screen_width - 70;
    int bar_y = (screen_height - 320) / 2;

    /* 创建容器 */
    g_volume_bar->container = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(g_volume_bar->container, bar_x, bar_y);
    lv_obj_set_size(g_volume_bar->container, 60, 320);
    lv_obj_set_scrollbar_mode(g_volume_bar->container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(g_volume_bar->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(g_volume_bar->container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_volume_bar->container, LV_OBJ_FLAG_CLICKABLE);

    /* 设置容器背景透明 */
    static lv_style_t style_container;
    lv_style_init(&style_container);
    lv_style_set_bg_opa(&style_container, LV_OPA_60);
    lv_style_set_bg_color(&style_container, lv_color_black());
    lv_style_set_border_width(&style_container, 0);
    lv_style_set_radius(&style_container, 30);
    lv_obj_add_style(g_volume_bar->container, &style_container, LV_PART_MAIN);

    /* 音量图标（顶部） */
    int initial_volume = get_volume_from_param_manager_or_default();
    g_volume_bar->icon = lv_img_create(g_volume_bar->container);
    lv_img_set_src(g_volume_bar->icon, get_volume_icon_path(initial_volume));
    lv_obj_align(g_volume_bar->icon, LV_ALIGN_TOP_MID, 0, 10);

    /* 创建竖直滑块 */
    g_volume_bar->slider = lv_slider_create(g_volume_bar->container);
    lv_obj_set_size(g_volume_bar->slider, 16, 200);
    lv_obj_clear_flag(g_volume_bar->slider, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(g_volume_bar->slider, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_slider_set_range(g_volume_bar->slider, 0, 100);
    lv_slider_set_value(g_volume_bar->slider, initial_volume, LV_ANIM_OFF);

    /* 滑块样式 - 轨道背景 */
    static lv_style_t style_slider_bg;
    lv_style_init(&style_slider_bg);
    lv_style_set_bg_opa(&style_slider_bg, LV_OPA_40);
    lv_style_set_bg_color(&style_slider_bg, lv_color_white());
    lv_style_set_radius(&style_slider_bg, 8);
    lv_style_set_border_width(&style_slider_bg, 0);
    lv_obj_add_style(g_volume_bar->slider, &style_slider_bg, LV_PART_MAIN);

    /* 滑块样式 - 进度指示 */
    static lv_style_t style_slider_indic;
    lv_style_init(&style_slider_indic);
    lv_style_set_bg_opa(&style_slider_indic, LV_OPA_COVER);
    lv_style_set_bg_color(&style_slider_indic, lv_color_hex(0x4A90D9));
    lv_style_set_radius(&style_slider_indic, 8);
    lv_obj_add_style(g_volume_bar->slider, &style_slider_indic, LV_PART_INDICATOR);

    /* 滑块样式 - 旋钮 */
    static lv_style_t style_slider_knob;
    lv_style_init(&style_slider_knob);
    lv_style_set_bg_opa(&style_slider_knob, LV_OPA_COVER);
    lv_style_set_bg_color(&style_slider_knob, lv_color_white());
    lv_style_set_radius(&style_slider_knob, LV_RADIUS_CIRCLE);
    lv_style_set_width(&style_slider_knob, 24);
    lv_style_set_height(&style_slider_knob, 24);
    lv_obj_add_style(g_volume_bar->slider, &style_slider_knob, LV_PART_KNOB);

    g_volume_bar->is_visible = 0;

    /* 添加事件回调 */
    lv_obj_add_event_cb(g_volume_bar->slider, volume_bar_slider_cb, LV_EVENT_VALUE_CHANGED, g_volume_bar);
    lv_obj_add_event_cb(g_volume_bar->slider, volume_bar_press_cb, LV_EVENT_PRESSED, g_volume_bar);
    lv_obj_add_event_cb(g_volume_bar->container, volume_bar_press_cb, LV_EVENT_CLICKED, g_volume_bar);

    MLOG_INFO("Vertical volume bar created successfully");
}

// #endregion
// #############################################################################
// ! #region 5. 对外接口函数
// #############################################################################
/**
 * @brief 初始化音量控制条
 */
void volume_bar_init(void)
{
    volume_bar_create();
}

/**
 * @brief 显示音量控制条
 */
void volume_bar_show(void)
{
    int volume = get_volume_from_param_manager_or_default();

    if (g_volume_bar == NULL) {
        volume_bar_create();
    }

    if (g_volume_bar == NULL) {
        return;
    }

    /* 确保音量值在有效范围内 */
    volume = clamp_volume(volume);

    /* 设置音量值 */
    lv_slider_set_value(g_volume_bar->slider, volume, LV_ANIM_ON);

    /* 更新音量图标 */
    update_volume_icon(g_volume_bar, volume);

    /* 同步到param_manager */
    param_manager_set(PARAM_ID_VOLUME, volume);

    /* 显示容器 */
    lv_obj_clear_flag(g_volume_bar->container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_fade_in(g_volume_bar->container, VOLUME_BAR_FADE_MS, 0);
    g_volume_bar->is_visible = 1;

    /* 删除旧定时器 */
    if (g_volume_bar->timer != NULL) {
        lv_timer_del(g_volume_bar->timer);
        g_volume_bar->timer = NULL;
    }

    /* 创建新定时器 */
    g_volume_bar->timer = lv_timer_create(volume_bar_timer_cb, VOLUME_BAR_TIMEOUT_MS, g_volume_bar);
    MLOG_DBG("Volume bar shown, volume: %d", volume);
}

/**
 * @brief 隐藏音量控制条
 */
void volume_bar_hide(void)
{
    if (g_volume_bar == NULL || g_volume_bar->is_visible == 0) {
        return;
    }

    /* 删除定时器 */
    if (g_volume_bar->timer != NULL) {
        lv_timer_del(g_volume_bar->timer);
        g_volume_bar->timer = NULL;
    }

    /* 淡出动画 */
    lv_obj_fade_out(g_volume_bar->container, VOLUME_BAR_FADE_MS, 0);
    g_volume_bar->is_visible = 0;
    MLOG_DBG("Volume bar hidden");
}

/**
 * @brief 设置音量值
 */
void volume_bar_set_value(int volume)
{
    if (g_volume_bar == NULL) {
        volume_bar_create();
    }

    if (g_volume_bar == NULL) {
        return;
    }

    volume = clamp_volume(volume);
    param_manager_set(PARAM_ID_VOLUME, volume);

    if (g_volume_bar->is_visible == 0) {
        volume_bar_show();
        return;
    }

    lv_slider_set_value(g_volume_bar->slider, volume, LV_ANIM_ON);
    update_volume_icon(g_volume_bar, volume);
    volume_bar_reset_timer();
    MLOG_DBG("Volume set to: %d", volume);
}

/**
 * @brief 获取当前音量值
 */
int volume_bar_get_value(void)
{
    return get_volume_from_param_manager_or_default();
}

/**
 * @brief 重置自动隐藏计时器
 */
void volume_bar_reset_timer(void)
{
    if (g_volume_bar == NULL || g_volume_bar->is_visible == 0) {
        return;
    }

    /* 删除旧定时器 */
    if (g_volume_bar->timer != NULL) {
        lv_timer_del(g_volume_bar->timer);
        g_volume_bar->timer = NULL;
    }

    /* 创建新定时器 */
    g_volume_bar->timer = lv_timer_create(volume_bar_timer_cb, VOLUME_BAR_TIMEOUT_MS, g_volume_bar);
}

// #endregion
// #############################################################################
// ! #region 6. 线程处理函数
// #############################################################################
// volume_bar 模块当前不涉及线程处理逻辑。

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################
// volume_bar 模块当前无独立调试与测试入口。

// #endregion
