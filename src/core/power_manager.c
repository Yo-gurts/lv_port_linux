// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "core/power_manager.h"
#include "config.h"
#include "core/key_manager.h"
#include "core/page_manager.h"
#include "hal_backlight.h"
#include "lvgl/lvgl.h"
#include "mlog.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/reboot.h>
#include <time.h>
#include <unistd.h>


// #endregion
// #############################################################################
// ! #region 2. 数据结构定义
// #############################################################################

typedef struct {
    power_manager_shutdown_prepare_cb_t cb;
    void* user_data;
    uint8_t valid;
} shutdown_prepare_entry_t;

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static uint8_t g_inited = 0;
static uint64_t g_last_activity_ms = 0;
static uint8_t g_screen_on = 1;
static uint8_t g_prev_touch_pressed = 0;
static uint32_t g_disable_auto_sleep_depth = 0;
static uint8_t g_wait_power_release = 0;
static shutdown_prepare_entry_t g_shutdown_prepare = { 0 };

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

static uint64_t power_manager_now_ms(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

static int power_manager_set_screen_on(int on)
{
    BACKLIGHT_STATE_E state;
    int32_t ret;

    if ((uint8_t)(on != 0) == g_screen_on) {
        return 0;
    }

    state = on ? BACKLIGHT_STATE_ON : BACKLIGHT_STATE_OFF;
    ret = HAL_BACKLIGHT_SetBackLightState(state);
    if (ret != 0) {
        MLOG_WARN("设置背光状态失败: state=%d ret=%d", (int)state, (int)ret);
        return -1;
    }

    g_screen_on = (uint8_t)(on != 0);
    MLOG_INFO("屏幕背光已%s", g_screen_on ? "开启" : "关闭");
    return 0;
}

static void power_manager_do_shutdown(void)
{
    sync();
    (void)reboot(RB_POWER_OFF);
}

static void power_manager_on_user_activity(void)
{
    g_last_activity_ms = power_manager_now_ms();

    if (!g_screen_on && !g_wait_power_release) {
        (void)power_manager_set_screen_on(1);
    }
}

static int power_manager_wake_if_screen_off(void)
{
    if (!g_screen_on && !g_wait_power_release) {
        power_manager_on_user_activity();
        return 1;
    }
    return 0;
}

static void power_manager_on_key_activity(key_id_t key, key_event_type_t event_type, void* user_data)
{
    LV_UNUSED(user_data);
    LV_UNUSED(event_type);

    if (key == KEY_ID_POWER) {
        return;
    }
    if (power_manager_wake_if_screen_off()) {
        return;
    }

    power_manager_on_user_activity();
}

static int power_manager_is_capture_page_active(void)
{
    const char* current = page_manager_get_current();

    if (current == NULL) {
        return 0;
    }

    return (strcmp(current, "photo") == 0) || (strcmp(current, "video") == 0);
}

static void power_manager_on_power_key_click(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)user_data;

    if (key != KEY_ID_POWER || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (power_manager_wake_if_screen_off()) {
        return;
    }
    if (g_wait_power_release) {
        return;
    }
    if (power_manager_is_capture_page_active()) {
        MLOG_INFO("当前在拍照/录像页面，忽略电源键短按背光切换");
        return;
    }

    (void)power_manager_set_screen_on(!g_screen_on);
    g_last_activity_ms = power_manager_now_ms();
}

static void power_manager_on_power_key_event(key_id_t key, key_event_type_t event_type, void* user_data)
{
    LV_UNUSED(user_data);

    if (key != KEY_ID_POWER) {
        return;
    }

    if (event_type == KEY_EVENT_LONG_PRESS_3S) {
        if (g_wait_power_release) {
            return;
        }

        g_wait_power_release = 1;
        if (g_shutdown_prepare.valid && g_shutdown_prepare.cb != NULL) {
            (void)g_shutdown_prepare.cb(g_shutdown_prepare.user_data);
        }
        (void)power_manager_set_screen_on(0);
        MLOG_INFO("进入假关机状态，等待电源键释放");
        return;
    }

    if (event_type == KEY_EVENT_LONG_PRESS_3S_RELEASE && g_wait_power_release) {
        MLOG_INFO("电源键已释放，执行关机");
        power_manager_do_shutdown();
    }
}

static void power_manager_poll_touch_activity(void)
{
    lv_indev_t* indev = NULL;
    uint8_t pressed = 0;

    while ((indev = lv_indev_get_next(indev)) != NULL) {
        lv_indev_type_t indev_type = lv_indev_get_type(indev);
        lv_indev_state_t indev_state;

        if (indev_type != LV_INDEV_TYPE_POINTER && indev_type != LV_INDEV_TYPE_BUTTON) {
            continue;
        }

        indev_state = lv_indev_get_state(indev);
        if (indev_state == LV_INDEV_STATE_PRESSED) {
            pressed = 1;
            break;
        }
    }

    if (pressed && !g_prev_touch_pressed) {
        if (!power_manager_wake_if_screen_off()) {
            power_manager_on_user_activity();
        }
    }
    g_prev_touch_pressed = pressed;
}

// #endregion
// #############################################################################
// ! #region 5. 对外接口函数
// #############################################################################

int power_manager_init(void)
{
    if (g_inited) {
        return 0;
    }

    g_last_activity_ms = power_manager_now_ms();
    g_screen_on = 1;
    g_prev_touch_pressed = 0;
    g_disable_auto_sleep_depth = 0;
    g_wait_power_release = 0;
    memset(&g_shutdown_prepare, 0, sizeof(g_shutdown_prepare));

    if (key_manager_register_callback(KEY_ID_ANY, KEY_EVENT_ANY, power_manager_on_key_activity, NULL) != 0) {
        MLOG_WARN("注册按键活动回调失败");
    }
    if (key_manager_register_callback(KEY_ID_POWER, KEY_EVENT_LONG_PRESS_3S, power_manager_on_power_key_event, NULL) != 0) {
        MLOG_WARN("注册电源键长按回调失败");
    }
    if (key_manager_register_callback(KEY_ID_POWER, KEY_EVENT_LONG_PRESS_3S_RELEASE, power_manager_on_power_key_event, NULL) != 0) {
        MLOG_WARN("注册电源键长按释放回调失败");
    }
    if (key_manager_register_callback(KEY_ID_POWER, KEY_EVENT_CLICK, power_manager_on_power_key_click, NULL) != 0) {
        MLOG_WARN("注册电源键短按回调失败");
    }

    g_inited = 1;
    return 0;
}

void power_manager_deinit(void)
{
    if (!g_inited) {
        return;
    }

    (void)key_manager_unregister_callback(KEY_ID_ANY, KEY_EVENT_ANY, power_manager_on_key_activity, NULL);
    (void)key_manager_unregister_callback(KEY_ID_POWER, KEY_EVENT_LONG_PRESS_3S, power_manager_on_power_key_event, NULL);
    (void)key_manager_unregister_callback(KEY_ID_POWER, KEY_EVENT_LONG_PRESS_3S_RELEASE, power_manager_on_power_key_event, NULL);
    (void)key_manager_unregister_callback(KEY_ID_POWER, KEY_EVENT_CLICK, power_manager_on_power_key_click, NULL);
    g_inited = 0;
}

void power_manager_poll(void)
{
    uint64_t now_ms;

    if (!g_inited) {
        return;
    }

    power_manager_poll_touch_activity();

    if (g_disable_auto_sleep_depth > 0 || !g_screen_on) {
        return;
    }

    now_ms = power_manager_now_ms();
    if (now_ms >= g_last_activity_ms + (uint64_t)POWER_MANAGER_SCREEN_IDLE_TIMEOUT_MS) {
        (void)power_manager_set_screen_on(0);
    }
}

void power_manager_mark_activity(void)
{
    if (!g_inited) {
        return;
    }

    power_manager_on_user_activity();
}

void power_manager_disable_auto_sleep(void)
{
    if (!g_inited) {
        return;
    }

    g_disable_auto_sleep_depth++;
    power_manager_on_user_activity();
}

void power_manager_enable_auto_sleep(void)
{
    if (!g_inited) {
        return;
    }

    if (g_disable_auto_sleep_depth == 0) {
        return;
    }

    g_disable_auto_sleep_depth--;
    g_last_activity_ms = power_manager_now_ms();
}

void power_manager_register_shutdown_prepare_cb(power_manager_shutdown_prepare_cb_t cb, void* user_data)
{
    if (!g_inited) {
        return;
    }

    g_shutdown_prepare.cb = cb;
    g_shutdown_prepare.user_data = user_data;
    g_shutdown_prepare.valid = (cb != NULL);
}

void power_manager_unregister_shutdown_prepare_cb(power_manager_shutdown_prepare_cb_t cb, void* user_data)
{
    if (!g_inited) {
        return;
    }

    if (!g_shutdown_prepare.valid) {
        return;
    }
    if (g_shutdown_prepare.cb != cb || g_shutdown_prepare.user_data != user_data) {
        return;
    }

    memset(&g_shutdown_prepare, 0, sizeof(g_shutdown_prepare));
}

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

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
