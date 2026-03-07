#include "core/power_manager.h"
#include "core/key_manager.h"
#include "core/page_manager.h"
#include "lvgl/lvgl.h"
#include "mlog.h"
#include <string.h>

static int g_inited = 0;
static int g_screen_on = 1;
static uint8_t g_prev_touch_pressed = 0;

static int power_manager_wake_if_screen_off(void)
{
    if (!g_screen_on) {
        g_screen_on = 1;
        MLOG_INFO("输入事件唤醒背光：亮屏");
        return 1;
    }
    return 0;
}

static int power_manager_is_capture_page_active(void)
{
    const char* current = page_manager_get_current();

    if (current == NULL) {
        return 0;
    }

    return (strcmp(current, "photo") == 0) || (strcmp(current, "video") == 0);
}

static void any_key_event_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)event_type;
    (void)user_data;

    if (key == KEY_ID_POWER) {
        return;
    }
    (void)power_manager_wake_if_screen_off();
}

static void power_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)user_data;

    if (key != KEY_ID_POWER || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (power_manager_wake_if_screen_off()) {
        return;
    }

    if (power_manager_is_capture_page_active()) {
        MLOG_INFO("当前在拍照/录像页面，忽略电源键短按背光切换");
        return;
    }

    g_screen_on = !g_screen_on;
    MLOG_INFO("电源键短按切换背光：%s", g_screen_on ? "亮屏" : "熄屏");
}

int power_manager_init(void)
{
    if (g_inited) {
        return 0;
    }
    g_screen_on = 1;
    g_prev_touch_pressed = 0;
    g_inited = 1;
    if (key_manager_register_callback(KEY_ID_ANY, KEY_EVENT_ANY, any_key_event_cb, NULL) != 0) {
        MLOG_WARN("注册按键唤醒回调失败");
    }
    if (key_manager_register_callback(KEY_ID_POWER, KEY_EVENT_CLICK, power_key_click_cb, NULL) != 0) {
        MLOG_WARN("注册电源键短按回调失败");
    }
    MLOG_INFO("mock 电源管理初始化");
    return 0;
}

void power_manager_deinit(void)
{
    if (!g_inited) {
        return;
    }
    (void)key_manager_unregister_callback(KEY_ID_ANY, KEY_EVENT_ANY, any_key_event_cb, NULL);
    (void)key_manager_unregister_callback(KEY_ID_POWER, KEY_EVENT_CLICK, power_key_click_cb, NULL);
    g_inited = 0;
    MLOG_INFO("mock 电源管理反初始化");
}

void power_manager_poll(void)
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
        (void)power_manager_wake_if_screen_off();
    }
    g_prev_touch_pressed = pressed;
}

void power_manager_mark_activity(void)
{
    (void)power_manager_wake_if_screen_off();
    MLOG_INFO("mock 记录用户活动");
}

void power_manager_disable_auto_sleep(void)
{
    MLOG_INFO("mock 禁用自动休眠");
}

void power_manager_enable_auto_sleep(void)
{
    MLOG_INFO("mock 启用自动休眠");
}

void power_manager_register_shutdown_prepare_cb(power_manager_shutdown_prepare_cb_t cb, void* user_data)
{
    (void)cb;
    (void)user_data;
    MLOG_INFO("mock 注册关机前回调");
}

void power_manager_unregister_shutdown_prepare_cb(power_manager_shutdown_prepare_cb_t cb, void* user_data)
{
    (void)cb;
    (void)user_data;
    MLOG_INFO("mock 注销关机前回调");
}
