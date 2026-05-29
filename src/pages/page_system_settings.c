// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_system_settings.h"
#include "config.h"
#include "core/file_manager.h"
#include "core/font_manager.h"
#include "core/key_manager.h"
#include "core/page_manager.h"
#include "core/param_manager.h"
#include "core/style_manager.h"
#include "core/wifi_manager.h"
#include "mlog.h"
#include "ui/gesture_back.h"
#include "ui/top_notice.h"
#include "ui/volume_bar.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_system_settings.h)
// #############################################################################

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static const setting_config_t settings_config[] = {
    { .icon_path = "A" RES_ICON_PATH "/language.png", .title = "语言", .value = "简体中文", .type = SETTING_TYPE_NORMAL },
    { .icon_path = "A" RES_ICON_PATH "/sys-wifi.png", .title = "WiFi设置", .value = "未连接", .type = SETTING_TYPE_NORMAL },
    { .icon_path = "A" RES_ICON_PATH "/datetime.png", .title = "时间和日期", .value = "2026-02-07 12:00", .type = SETTING_TYPE_NORMAL },
    { .icon_path = "A" RES_ICON_PATH "/sys-volume.png", .title = "音量设置", .value = "xx%", .type = SETTING_TYPE_NORMAL },
    { .icon_path = "A" RES_ICON_PATH "/switch.png", .title = "自动息屏", .value = "已开启", .type = SETTING_TYPE_TOGGLE },
    { .icon_path = "A" RES_ICON_PATH "/delete.png", .title = "格式化", .value = "请确认", .type = SETTING_TYPE_NORMAL },
    { .icon_path = "A" RES_ICON_PATH "/factory.png", .title = "出厂设置", .value = "请确认", .type = SETTING_TYPE_NORMAL },
    { .icon_path = "A" RES_ICON_PATH "/info.png", .title = "版本信息", .value = "V1.0.0", .type = SETTING_TYPE_NORMAL },
};

#define SETTINGS_COUNT (int)(sizeof(settings_config) / sizeof(settings_config[0]))
#define SETTINGS_INDEX_AUTO_SLEEP 4
#define SETTINGS_INDEX_FORMAT 5
#define SETTINGS_INDEX_FACTORY_RESET 6
#define SETTINGS_INDEX_VOLUME 3
#define SETTINGS_INDEX_WIFI 1

typedef enum {
    SYSTEM_ACTION_NONE = 0,
    SYSTEM_ACTION_FORMAT_SDCARD,
    SYSTEM_ACTION_FACTORY_RESET
} system_action_t;

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

/* 更新“音量设置”行的右侧文本。 */
static void update_volume_setting_value(page_system_settings_data_t* data)
{
    char text[16];
    int volume;

    if (data == NULL || data->settings[SETTINGS_INDEX_VOLUME].value_label == NULL) {
        return;
    }

    volume = param_manager_get(PARAM_ID_VOLUME);
    lv_snprintf(text, sizeof(text), "%d%%", volume);
    lv_label_set_text(data->settings[SETTINGS_INDEX_VOLUME].value_label, text);
}

static void update_auto_sleep_setting_value(page_system_settings_data_t* data)
{
    int enabled;
    system_setting_item_t* item;

    if (data == NULL || data->settings[SETTINGS_INDEX_AUTO_SLEEP].value_label == NULL) {
        return;
    }

    enabled = param_manager_get(PARAM_ID_AUTO_SLEEP);
    if (enabled != 0 && enabled != 1) {
        enabled = 1;
    }
    item = &data->settings[SETTINGS_INDEX_AUTO_SLEEP];
    item->current_index = enabled ? 1 : 0;
    lv_label_set_text(item->value_label, enabled ? "已开启" : "已关闭");
}

static void update_wifi_setting_value(page_system_settings_data_t* data)
{
    int connected;
    const char* ssid;

    if (data == NULL || data->settings[SETTINGS_INDEX_WIFI].value_label == NULL) {
        return;
    }

    connected = param_manager_get(PARAM_ID_WIFI_CONNECTED);
    if (connected == 1) {
        ssid = wifi_manager_get_connected_ssid();
        if (ssid != NULL && ssid[0] != '\0' && strcmp(ssid, "未连接") != 0) {
            lv_label_set_text(data->settings[SETTINGS_INDEX_WIFI].value_label, ssid);
        } else {
            lv_label_set_text(data->settings[SETTINGS_INDEX_WIFI].value_label, "已连接");
        }
    } else {
        lv_label_set_text(data->settings[SETTINGS_INDEX_WIFI].value_label, "未连接");
    }
}

static void show_confirm_dialog(page_system_settings_data_t* data, system_action_t action)
{
    const char* title = "请确认操作";
    const char* message = "";

    if (data == NULL || data->confirm_mask == NULL || data->confirm_msg_label == NULL || data->confirm_title_label == NULL) {
        return;
    }

    data->pending_action = action;
    switch (action) {
    case SYSTEM_ACTION_FORMAT_SDCARD:
        message = "格式化将删除SD卡上的所有数据，是否继续？";
        break;
    case SYSTEM_ACTION_FACTORY_RESET:
        message = "恢复出厂设置将重置所有参数设置，是否继续？";
        break;
    case SYSTEM_ACTION_NONE:
    default:
        message = "未知操作";
        break;
    }

    lv_label_set_text(data->confirm_title_label, title);
    lv_label_set_text(data->confirm_msg_label, message);
    lv_obj_clear_flag(data->confirm_mask, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(data->confirm_mask);
}

static void hide_confirm_dialog(page_system_settings_data_t* data)
{
    if (data == NULL || data->confirm_mask == NULL) {
        return;
    }

    lv_obj_add_flag(data->confirm_mask, LV_OBJ_FLAG_HIDDEN);
}

/* 参数更新回调：参数变化时同步系统设置页显示。 */
static void system_settings_param_cb(param_id_t id, int value, void* user_data)
{
    page_system_settings_data_t* data = (page_system_settings_data_t*)user_data;
    LV_UNUSED(value);

    if (id == PARAM_ID_VOLUME) {
        update_volume_setting_value(data);
        return;
    }
    if (id == PARAM_ID_AUTO_SLEEP) {
        update_auto_sleep_setting_value(data);
        return;
    }
    if (id == PARAM_ID_WIFI_CONNECTED || id == PARAM_ID_WIFI_SIGNAL_DBM) {
        update_wifi_setting_value(data);
    }
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

static void menu_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)key;
    (void)event_type;
    (void)user_data;
    page_manager_back();
}

static void update_selection_highlight(page_system_settings_data_t* data, int old_index, int new_index)
{
    if (!data)
        return;

    if (old_index >= 0 && old_index < SETTINGS_COUNT && data->settings[old_index].container) {
        lv_obj_remove_style(data->settings[old_index].container, &style_settings_item_selected, LV_PART_MAIN);
    }
    if (new_index >= 0 && new_index < SETTINGS_COUNT && data->settings[new_index].container) {
        lv_obj_add_style(data->settings[new_index].container, &style_settings_item_selected, LV_PART_MAIN);
        lv_obj_scroll_to_view(data->settings[new_index].container, LV_ANIM_OFF);
    }
}

static void up_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_system_settings_data_t* data = (page_system_settings_data_t*)user_data;
    if (key != KEY_ID_UP || event_type != KEY_EVENT_CLICK) return;
    if (!data || !data->container)
        return;

    int old_index = data->selected_index;
    if (data->selected_index > 0) {
        data->selected_index--;
    } else {
        data->selected_index = SETTINGS_COUNT - 1;
    }
    update_selection_highlight(data, old_index, data->selected_index);
}

static void down_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_system_settings_data_t* data = (page_system_settings_data_t*)user_data;
    if (key != KEY_ID_DOWN || event_type != KEY_EVENT_CLICK) return;
    if (!data || !data->container)
        return;

    int old_index = data->selected_index;
    if (data->selected_index < SETTINGS_COUNT - 1) {
        data->selected_index++;
    } else {
        data->selected_index = 0;
    }
    update_selection_highlight(data, old_index, data->selected_index);
}

static void ok_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_system_settings_data_t* data = (page_system_settings_data_t*)user_data;

    if (key != KEY_ID_OK || event_type != KEY_EVENT_CLICK) return;
    if (!data || !data->container) return;
    if (data->action_processing) return;

    lv_obj_send_event(data->settings[data->selected_index].container, LV_EVENT_CLICKED, data);
}

static void system_action_timer_cb(lv_timer_t* timer)
{
    page_system_settings_data_t* data;
    int ret = -1;

    if (timer == NULL) {
        return;
    }

    data = (page_system_settings_data_t*)lv_timer_get_user_data(timer);
    if (data == NULL) {
        lv_timer_del(timer);
        return;
    }

    if (data->pending_action == SYSTEM_ACTION_FORMAT_SDCARD) {
        ret = file_manager_format_sdcard();
        if (ret == 0) {
            top_notice_show("SD卡格式化完成", TOP_NOTICE_TYPE_SUCCESS);
        } else {
            top_notice_show("SD卡格式化失败", TOP_NOTICE_TYPE_ERROR);
        }
    } else if (data->pending_action == SYSTEM_ACTION_FACTORY_RESET) {
        ret = param_manager_factory_reset();
        if (ret == 0) {
            top_notice_show("恢复出厂设置完成", TOP_NOTICE_TYPE_SUCCESS);
        } else {
            top_notice_show("恢复出厂设置失败", TOP_NOTICE_TYPE_ERROR);
        }
    }

    update_volume_setting_value(data);
    update_auto_sleep_setting_value(data);
    data->pending_action = SYSTEM_ACTION_NONE;
    data->action_processing = 0;
    data->action_timer = NULL;
    lv_timer_del(timer);
}

static void confirm_cancel_btn_cb(lv_event_t* e)
{
    page_system_settings_data_t* data = (page_system_settings_data_t*)lv_event_get_user_data(e);
    if (data == NULL) {
        return;
    }

    data->pending_action = SYSTEM_ACTION_NONE;
    hide_confirm_dialog(data);
}

static void confirm_ok_btn_cb(lv_event_t* e)
{
    page_system_settings_data_t* data = (page_system_settings_data_t*)lv_event_get_user_data(e);

    if (data == NULL) {
        return;
    }
    if (data->action_processing) {
        top_notice_show("正在处理，请稍候...", TOP_NOTICE_TYPE_WARNING);
        return;
    }
    if (data->pending_action == SYSTEM_ACTION_NONE) {
        hide_confirm_dialog(data);
        return;
    }

    if (data->pending_action == SYSTEM_ACTION_FORMAT_SDCARD) {
        top_notice_update("正在格式化SD卡...", TOP_NOTICE_TYPE_BLOCKING);
    } else if (data->pending_action == SYSTEM_ACTION_FACTORY_RESET) {
        top_notice_update("正在恢复出厂设置...", TOP_NOTICE_TYPE_BLOCKING);
    }

    data->action_processing = 1;
    hide_confirm_dialog(data);

    if (data->action_timer != NULL) {
        lv_timer_del(data->action_timer);
        data->action_timer = NULL;
    }
    data->action_timer = lv_timer_create(system_action_timer_cb, 10, data);
    if (data->action_timer != NULL) {
        lv_timer_set_repeat_count(data->action_timer, 1);
    }
}

/* 设置项点击回调 */
static void setting_item_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_current_target(e);
    page_system_settings_data_t* data = (page_system_settings_data_t*)lv_event_get_user_data(e);
    if (!data) {
        return;
    }
    uintptr_t user_data = (uintptr_t)lv_obj_get_user_data(obj);
    int index = (int)user_data;

    if (index < 0 || index >= SETTINGS_COUNT) {
        return;
    }

    system_setting_item_t* item = &data->settings[index];
    const setting_config_t* config = &settings_config[index];

    if (config->type == SETTING_TYPE_TOGGLE) {
        if (index == SETTINGS_INDEX_AUTO_SLEEP) {
            item->current_index = !item->current_index;
            (void)param_manager_set(PARAM_ID_AUTO_SLEEP, item->current_index ? 1 : 0);
            lv_label_set_text(item->value_label, item->current_index ? "已开启" : "已关闭");
            MLOG_INFO("自动息屏已%s", item->current_index ? "开启" : "关闭");
        }
    } else {
        /* WiFi设置跳转 */
        if (index == 1) {
            page_manager_navigate("wifi_list");
        }
        /* 版本信息跳转 */
        else if (index == SETTINGS_COUNT - 1) {
            page_manager_navigate("version_info");
        } else if (index == SETTINGS_INDEX_VOLUME) {
            volume_bar_show();
        } else if (index == SETTINGS_INDEX_FORMAT) {
            show_confirm_dialog(data, SYSTEM_ACTION_FORMAT_SDCARD);
        } else if (index == SETTINGS_INDEX_FACTORY_RESET) {
            show_confirm_dialog(data, SYSTEM_ACTION_FACTORY_RESET);
        } else {
            MLOG_INFO("Setting '%s' clicked, value: %s", config->title, config->value);
        }
    }
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_system_settings_create(void)
{
    page_system_settings_data_t* data = (page_system_settings_data_t*)malloc(sizeof(page_system_settings_data_t));
    if (!data) {
        return;
    }

    memset(data, 0, sizeof(page_system_settings_data_t));

    /* 创建页面容器 */
    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_width(data->container, LV_PCT(100));
    lv_obj_set_height(data->container, LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_set_layout(data->container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->container, LV_FLEX_FLOW_COLUMN);
    lv_obj_refr_size(data->container);

    /* 启用滑动手势检测，不将 GESTURE 事件传递给父控件 */
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    /* 添加滑动手势回调：从左往右滑返回上一页 */
    gesture_back_register_events(data->container);
    gesture_back_set_left_edge_swipe_cb(data->container, page_manager_back_cb);

    /* =======================
     * 1. 顶部导航栏
     * ======================= */
    data->nav_bar = lv_obj_create(data->container);
    lv_obj_set_width(data->nav_bar, lv_pct(100));
    lv_obj_set_height(data->nav_bar, 50); /* 50高度 */
    lv_obj_clear_flag(data->nav_bar, LV_OBJ_FLAG_SCROLLABLE); /* 禁用滚动 */
    lv_obj_set_scrollbar_mode(data->nav_bar, LV_SCROLLBAR_MODE_OFF); /* 隐藏滚动条 */
    lv_obj_add_style(data->nav_bar, &style_common_cont_top, LV_PART_MAIN);

    /* 返回按钮 - 左上角 */
    lv_obj_t* back_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(back_btn, 50, 50);
    lv_obj_add_style(back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(back_btn, page_manager_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_t* back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle-white.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    /* 标题文字 - 居中 */
    lv_obj_t* title_label = lv_label_create(data->nav_bar);
    lv_label_set_text(title_label, "系统设置");
    lv_obj_add_style(title_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    /* =======================
     * 2. 设置列表区域 - 占满剩余空间
     * ======================= */
    data->settings_container = lv_obj_create(data->container);
    lv_obj_set_width(data->settings_container, lv_pct(100));
    lv_obj_set_flex_grow(data->settings_container, 1); /* 填满剩余空间 */
    lv_obj_set_style_bg_opa(data->settings_container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->settings_container, lv_color_hex(0x121212), LV_PART_MAIN);
    lv_obj_set_layout(data->settings_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->settings_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(data->settings_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 创建8个设置项 */
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        system_setting_item_t* item = &data->settings[i];

        /* 设置项容器 */
        item->container = lv_obj_create(data->settings_container);
        lv_obj_set_width(item->container, lv_pct(100));
        lv_obj_set_height(item->container, 55);
        lv_obj_clear_flag(item->container, LV_OBJ_FLAG_SCROLLABLE); /* 禁用滚动 */
        lv_obj_set_scrollbar_mode(item->container, LV_SCROLLBAR_MODE_OFF); /* 隐藏滚动条 */
        lv_obj_add_style(item->container, &style_settings_item, LV_PART_MAIN);

        /* 左侧图标 */
        item->icon = lv_img_create(item->container);
        lv_img_set_src(item->icon, settings_config[i].icon_path);
        lv_obj_align(item->icon, LV_ALIGN_LEFT_MID, 0, 0);

        /* 标题文字 */
        item->title_label = lv_label_create(item->container);
        lv_label_set_text(item->title_label, settings_config[i].title);
        lv_obj_add_style(item->title_label, &NORMAL_SIZE, LV_PART_MAIN);
        lv_obj_align(item->title_label, LV_ALIGN_LEFT_MID, 55, 0);

        /* 参数文字 */
        item->value_label = lv_label_create(item->container);
        lv_label_set_text(item->value_label, settings_config[i].value);
        lv_obj_add_style(item->value_label, &NORMAL_SIZE, LV_PART_MAIN);
        lv_obj_set_style_text_color(item->value_label, lv_color_hex(0xFFD700), LV_PART_MAIN);
        lv_obj_align(item->value_label, LV_ALIGN_RIGHT_MID, 0, 0);

        item->current_index = 0;

        /* 点击事件 */
        lv_obj_add_event_cb(item->container, setting_item_cb, LV_EVENT_CLICKED, data);
        lv_obj_set_user_data(item->container, (void*)(intptr_t)i);
    }

    /* 确认弹框全屏遮罩 */
    data->confirm_mask = lv_obj_create(data->container);
    lv_obj_add_flag(data->confirm_mask, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(data->confirm_mask, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->confirm_mask, &style_overlay_mask, LV_PART_MAIN);
    lv_obj_add_flag(data->confirm_mask, LV_OBJ_FLAG_HIDDEN);

    /* 确认弹框面板 */
    data->confirm_panel = lv_obj_create(data->confirm_mask);
    lv_obj_set_width(data->confirm_panel, LV_PCT(88));
    lv_obj_set_height(data->confirm_panel, 190);
    lv_obj_add_style(data->confirm_panel, &style_modal_panel, LV_PART_MAIN);
    lv_obj_clear_flag(data->confirm_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(data->confirm_panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(data->confirm_panel, LV_ALIGN_CENTER, 0, 0);

    data->confirm_title_label = lv_label_create(data->confirm_panel);
    lv_label_set_text(data->confirm_title_label, "请确认操作");
    lv_obj_add_style(data->confirm_title_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->confirm_title_label, LV_ALIGN_TOP_MID, 0, 14);

    data->confirm_msg_label = lv_label_create(data->confirm_panel);
    lv_label_set_text(data->confirm_msg_label, "");
    lv_label_set_long_mode(data->confirm_msg_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(data->confirm_msg_label, LV_PCT(90));
    lv_obj_add_style(data->confirm_msg_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_align(data->confirm_msg_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(data->confirm_msg_label, LV_ALIGN_TOP_MID, 0, 56);

    data->confirm_cancel_btn = lv_btn_create(data->confirm_panel);
    lv_obj_set_size(data->confirm_cancel_btn, 112, 40);
    lv_obj_align(data->confirm_cancel_btn, LV_ALIGN_BOTTOM_LEFT, 12, -10);
    lv_obj_add_event_cb(data->confirm_cancel_btn, confirm_cancel_btn_cb, LV_EVENT_CLICKED, data);
    {
        lv_obj_t* cancel_label = lv_label_create(data->confirm_cancel_btn);
        lv_label_set_text(cancel_label, "取消");
        lv_obj_add_style(cancel_label, &SMALL_SIZE, LV_PART_MAIN);
        lv_obj_center(cancel_label);
    }

    data->confirm_ok_btn = lv_btn_create(data->confirm_panel);
    lv_obj_set_size(data->confirm_ok_btn, 112, 40);
    lv_obj_align(data->confirm_ok_btn, LV_ALIGN_BOTTOM_RIGHT, -12, -10);
    lv_obj_add_event_cb(data->confirm_ok_btn, confirm_ok_btn_cb, LV_EVENT_CLICKED, data);
    {
        lv_obj_t* ok_label = lv_label_create(data->confirm_ok_btn);
        lv_label_set_text(ok_label, "确认");
        lv_obj_add_style(ok_label, &SMALL_SIZE, LV_PART_MAIN);
        lv_obj_center(ok_label);
    }

    /* 启用整页事件冒泡，确保子对象按压事件传递到父容器。 */
    gesture_back_enable_event_bubble_recursive(data->container);

    /* 保存 private_data */
    page_set_private_data(data);

    /* 监听参数变化，保持“音量设置”与 param_manager 同步。 */
    param_manager_register_callback(system_settings_param_cb, data);
    update_volume_setting_value(data);
    update_auto_sleep_setting_value(data);
    update_wifi_setting_value(data);
}

void page_system_settings_destroy(void)
{
    page_system_settings_data_t* data = page_get_private_data();
    if (!data) {
        return;
    }

    if (data->action_timer != NULL) {
        lv_timer_del(data->action_timer);
        data->action_timer = NULL;
    }

    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    param_manager_unregister_callback(system_settings_param_cb);
    free(data);
}

void page_system_settings_show(void)
{
    page_system_settings_data_t* data = page_get_private_data();
    if (!data || !data->container) {
        return;
    }

    gesture_back_set_left_edge_swipe_cb(data->container, page_manager_back_cb);
    key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    key_manager_register_callback(KEY_ID_UP, KEY_EVENT_CLICK, up_key_click_cb, data);
    key_manager_register_callback(KEY_ID_DOWN, KEY_EVENT_CLICK, down_key_click_cb, data);
    key_manager_register_callback(KEY_ID_OK, KEY_EVENT_CLICK, ok_key_click_cb, data);

    MLOG_INFO("System settings page show");
    update_volume_setting_value(data);
    update_auto_sleep_setting_value(data);
    update_wifi_setting_value(data);
    update_selection_highlight(data, -1, data->selected_index);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_system_settings_hide(void)
{
    page_system_settings_data_t* data = page_get_private_data();
    if (!data || !data->container) {
        return;
    }

    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    key_manager_unregister_callback(KEY_ID_UP, KEY_EVENT_CLICK, up_key_click_cb, data);
    key_manager_unregister_callback(KEY_ID_DOWN, KEY_EVENT_CLICK, down_key_click_cb, data);
    key_manager_unregister_callback(KEY_ID_OK, KEY_EVENT_CLICK, ok_key_click_cb, data);

    MLOG_INFO("System settings page hide");
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_system_settings_update(void)
{
    /* no-op */
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
