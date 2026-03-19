// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_wifi_list.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/page_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include "ui/gesture_back.h"
#include "ui/top_notice.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_wifi_list.h)
// #############################################################################

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static void rebuild_wifi_list(page_wifi_list_data_t* data);
static void wifi_item_click_cb(lv_event_t* e);
static void refresh_btn_click_cb(lv_event_t* e);
static void wifi_switch_change_cb(lv_event_t* e);
static void password_cancel_btn_cb(lv_event_t* e);
static void password_confirm_btn_cb(lv_event_t* e);
static void password_keyboard_event_cb(lv_event_t* e);
static void show_password_modal(page_wifi_list_data_t* data, const char* ssid);
static void hide_password_modal(page_wifi_list_data_t* data);
static void connect_wifi_and_refresh(page_wifi_list_data_t* data, const char* ssid, const char* password);
static void start_wifi_scan(page_wifi_list_data_t* data);
static void scan_timer_cb(lv_timer_t* timer);

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

/* 根据是否加密选择 WiFi 列表图标。 */
static const char* get_wifi_icon_path(const wifi_ap_info_t* ap)
{
    if (ap != NULL && ap->is_secured) {
        return "A:" RES_ICON_PATH "/sys-wifi-lock.png";
    }
    return "A:" RES_ICON_PATH "/sys-wifi.png";
}

/* 显示密码输入弹层并绑定目标 SSID。 */
static void show_password_modal(page_wifi_list_data_t* data, const char* ssid)
{
    char title[128];

    if (data == NULL || data->password_modal_mask == NULL || data->password_ta == NULL) {
        return;
    }

    if (ssid == NULL) {
        ssid = "";
    }

    lv_snprintf(data->pending_ssid, sizeof(data->pending_ssid), "%s", ssid);
    lv_snprintf(title, sizeof(title), "输入密码: %s", data->pending_ssid);
    lv_label_set_text(data->password_ssid_label, title);
    lv_textarea_set_text(data->password_ta, "");
    lv_keyboard_set_textarea(data->password_kb, data->password_ta);
    lv_obj_clear_flag(data->password_modal_mask, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(data->password_modal_mask);
    lv_obj_add_state(data->password_ta, LV_STATE_FOCUSED);
}

/* 隐藏密码输入弹层。 */
static void hide_password_modal(page_wifi_list_data_t* data)
{
    if (data == NULL || data->password_modal_mask == NULL) {
        return;
    }

    lv_obj_add_flag(data->password_modal_mask, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_state(data->password_ta, LV_STATE_FOCUSED);
}

/* 封装连接流程，统一提示与列表刷新。 */
static void connect_wifi_and_refresh(page_wifi_list_data_t* data, const char* ssid, const char* password)
{
    int ret;

    if (data == NULL || ssid == NULL || ssid[0] == '\0') {
        return;
    }

    top_notice_show_for("正在连接...", TOP_NOTICE_TYPE_WARNING, 1200);
    ret = wifi_manager_connect(ssid, password);
    if (ret == 0) {
        top_notice_show("连接成功", TOP_NOTICE_TYPE_SUCCESS);
    } else {
        top_notice_show("连接失败", TOP_NOTICE_TYPE_ERROR);
    }

    rebuild_wifi_list(data);
}

/* 扫描并重建 WiFi 列表。 */
static void rebuild_wifi_list(page_wifi_list_data_t* data)
{
    int i;

    if (data == NULL || data->wifi_list == NULL) {
        return;
    }

    if (data->wifi_enabled == 0) {
        lv_obj_clean(data->wifi_list);
        return;
    }

    lv_obj_clean(data->wifi_list);

    for (i = 0; i < data->scan_count; i++) {
        lv_obj_t* btn;
        const char* icon_path;

        icon_path = get_wifi_icon_path(&data->scan_results[i]);
        btn = lv_list_add_btn(data->wifi_list, icon_path, data->scan_results[i].ssid);
        lv_obj_add_style(btn, &style_settings_item, LV_PART_MAIN);
        lv_obj_add_style(btn, &NORMAL_SIZE, LV_PART_MAIN);
        lv_obj_add_style(btn, (i % 2 == 0) ? &style_list_row_even : &style_list_row_odd, LV_PART_MAIN);
        {
            lv_obj_t* label = lv_obj_get_child(btn, 1);
            if (label != NULL) {
                lv_obj_add_style(label, &NORMAL_SIZE, LV_PART_MAIN);
                lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
                lv_obj_set_width(label, lv_pct(70));
                lv_obj_align(label, LV_ALIGN_LEFT_MID, 44, 0);
            }
        }
        if (data->scan_results[i].is_connected) {
            lv_obj_t* connected_icon = lv_img_create(btn);
            lv_img_set_src(connected_icon, "A:" RES_ICON_PATH "/check.png");
            lv_obj_align(connected_icon, LV_ALIGN_RIGHT_MID, -10, 0);
        } else if (data->scan_results[i].is_saved) {
            lv_obj_t* saved_icon = lv_img_create(btn);
            lv_img_set_src(saved_icon, "A:" RES_ICON_PATH "/wifi-saved-unlock.png");
            lv_obj_align(saved_icon, LV_ALIGN_RIGHT_MID, -10, 0);
        }
        lv_obj_set_user_data(btn, &data->scan_results[i]);
        lv_obj_add_event_cb(btn, wifi_item_click_cb, LV_EVENT_CLICKED, data);
    }
}

/* 开始 WiFi 扫描 */
static void start_wifi_scan(page_wifi_list_data_t* data)
{
    if (data == NULL) {
        return;
    }

    /* 停止之前的定时器 */
    if (data->scan_timer != NULL) {
        lv_timer_del(data->scan_timer);
        data->scan_timer = NULL;
    }

    /* 启动扫描 */
    data->pending_scan_id = wifi_manager_start_scan();
    if (data->pending_scan_id < 0) {
        top_notice_show_for("扫描启动失败", TOP_NOTICE_TYPE_ERROR, 2000);
        return;
    }

    top_notice_show_for("正在扫描 WiFi...", TOP_NOTICE_TYPE_INFO, 5000);

    /* 创建 1 秒定时器轮询结果 */
    data->scan_timer = lv_timer_create(scan_timer_cb, 1000, data);
}

/* 扫描定时器回调 */
static void scan_timer_cb(lv_timer_t* timer)
{
    page_wifi_list_data_t* data;
    int count;
    int i;

    if (timer == NULL) {
        return;
    }

    data = (page_wifi_list_data_t*)lv_timer_get_user_data(timer);
    if (data == NULL || data->wifi_list == NULL) {
        lv_timer_del(timer);
        data->scan_timer = NULL;
        return;
    }

    if (data->wifi_enabled == 0) {
        lv_timer_del(timer);
        data->scan_timer = NULL;
        return;
    }

    /* 获取扫描结果 */
    count = wifi_manager_get_scan_results(data->scan_results, WIFI_LIST_MAX_AP_COUNT, data->pending_scan_id);
    if (count < 0) {
        /* 还在扫描中，继续等待 */
        return;
    }

    /* 扫描完成 */
    data->scan_count = count;
    lv_timer_del(timer);
    data->scan_timer = NULL;

    /* 重建列表 */
    lv_obj_clean(data->wifi_list);
    for (i = 0; i < count; i++) {
        lv_obj_t* btn;
        const char* icon_path;

        icon_path = get_wifi_icon_path(&data->scan_results[i]);
        btn = lv_list_add_btn(data->wifi_list, icon_path, data->scan_results[i].ssid);
        lv_obj_add_style(btn, &style_settings_item, LV_PART_MAIN);
        lv_obj_add_style(btn, &NORMAL_SIZE, LV_PART_MAIN);
        lv_obj_add_style(btn, (i % 2 == 0) ? &style_list_row_even : &style_list_row_odd, LV_PART_MAIN);
        {
            lv_obj_t* label = lv_obj_get_child(btn, 1);
            if (label != NULL) {
                lv_obj_add_style(label, &NORMAL_SIZE, LV_PART_MAIN);
                lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
                lv_obj_set_width(label, lv_pct(70));
                lv_obj_align(label, LV_ALIGN_LEFT_MID, 44, 0);
            }
        }
        if (data->scan_results[i].is_connected) {
            lv_obj_t* connected_icon = lv_img_create(btn);
            lv_img_set_src(connected_icon, "A:" RES_ICON_PATH "/check.png");
            lv_obj_align(connected_icon, LV_ALIGN_RIGHT_MID, -10, 0);
        } else if (data->scan_results[i].is_saved) {
            lv_obj_t* saved_icon = lv_img_create(btn);
            lv_img_set_src(saved_icon, "A:" RES_ICON_PATH "/wifi-saved-unlock.png");
            lv_obj_align(saved_icon, LV_ALIGN_RIGHT_MID, -10, 0);
        }
        lv_obj_set_user_data(btn, &data->scan_results[i]);
        lv_obj_add_event_cb(btn, wifi_item_click_cb, LV_EVENT_CLICKED, data);
    }

    char notice_text[64];
    snprintf(notice_text, sizeof(notice_text), "扫描到 %d 个 WiFi", count);
    top_notice_show(notice_text, TOP_NOTICE_TYPE_INFO);
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

static void wifi_item_click_cb(lv_event_t* e)
{
    page_wifi_list_data_t* data = (page_wifi_list_data_t*)lv_event_get_user_data(e);
    lv_obj_t* target = lv_event_get_target(e);
    wifi_ap_info_t* ap = NULL;

    if (data == NULL || target == NULL) {
        return;
    }

    ap = (wifi_ap_info_t*)lv_obj_get_user_data(target);
    if (ap == NULL) {
        return;
    }
    if (ap->is_connected) {
        top_notice_show("该网络已连接", TOP_NOTICE_TYPE_SUCCESS);
        return;
    }

    if (ap->is_secured && !ap->is_saved) {
        show_password_modal(data, ap->ssid);
        return;
    }
    connect_wifi_and_refresh(data, ap->ssid, "");
}

static void password_cancel_btn_cb(lv_event_t* e)
{
    page_wifi_list_data_t* data = (page_wifi_list_data_t*)lv_event_get_user_data(e);
    if (data == NULL) {
        return;
    }

    hide_password_modal(data);
}

static void password_confirm_btn_cb(lv_event_t* e)
{
    page_wifi_list_data_t* data = (page_wifi_list_data_t*)lv_event_get_user_data(e);
    const char* password;

    if (data == NULL || data->password_ta == NULL) {
        return;
    }

    password = lv_textarea_get_text(data->password_ta);
    if (password == NULL || password[0] == '\0') {
        top_notice_show("请输入密码", TOP_NOTICE_TYPE_ERROR);
        return;
    }

    connect_wifi_and_refresh(data, data->pending_ssid, password);
    hide_password_modal(data);
}

static void password_keyboard_event_cb(lv_event_t* e)
{
    page_wifi_list_data_t* data = (page_wifi_list_data_t*)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (data == NULL) {
        return;
    }

    if (code == LV_EVENT_READY) {
        password_confirm_btn_cb(e);
    } else if (code == LV_EVENT_CANCEL) {
        hide_password_modal(data);
    }
}

static void refresh_btn_click_cb(lv_event_t* e)
{
    page_wifi_list_data_t* data = (page_wifi_list_data_t*)lv_event_get_user_data(e);
    if (data == NULL) {
        return;
    }

    if (data->wifi_enabled == 0) {
        top_notice_show_for("WiFi已关闭", TOP_NOTICE_TYPE_ERROR, 2000);
        return;
    }

    start_wifi_scan(data);
}

static void wifi_switch_change_cb(lv_event_t* e)
{
    page_wifi_list_data_t* data = (page_wifi_list_data_t*)lv_event_get_user_data(e);
    int enabled;

    if (data == NULL || data->wifi_switch == NULL) {
        return;
    }

    enabled = lv_obj_has_state(data->wifi_switch, LV_STATE_CHECKED) ? 1 : 0;
    if (wifi_manager_set_enabled(enabled) != 0) {
        top_notice_show_for(enabled ? "WiFi开启失败" : "WiFi关闭失败", TOP_NOTICE_TYPE_ERROR, 2000);
        return;
    }

    data->wifi_enabled = enabled;
    if (enabled) {
        start_wifi_scan(data);
        top_notice_show_for("WiFi已开启", TOP_NOTICE_TYPE_SUCCESS, 2000);
    } else {
        /* 停止扫描定时器 */
        if (data->scan_timer != NULL) {
            lv_timer_del(data->scan_timer);
            data->scan_timer = NULL;
        }
        lv_obj_clean(data->wifi_list);
        top_notice_show_for("WiFi已关闭", TOP_NOTICE_TYPE_ERROR, 2000);
    }
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_wifi_list_create(void)
{
    page_wifi_list_data_t* data;
    lv_obj_t* back_btn;
    lv_obj_t* back_icon;
    lv_obj_t* title_label;
    lv_obj_t* refresh_icon;
    lv_obj_t* cancel_label;
    lv_obj_t* confirm_label;

    data = (page_wifi_list_data_t*)malloc(sizeof(page_wifi_list_data_t));
    if (data == NULL) {
        return;
    }
    memset(data, 0, sizeof(*data));

    /* 页面根容器。 */
    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_width(data->container, LV_PCT(100));
    lv_obj_set_height(data->container, LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_set_layout(data->container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(data->container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_GESTURE_BUBBLE);
    gesture_back_register_events(data->container);
    gesture_back_set_left_edge_swipe_cb(data->container, page_manager_back_cb);

    /* 顶部导航栏容器。 */
    data->nav_bar = lv_obj_create(data->container);
    lv_obj_set_width(data->nav_bar, lv_pct(100));
    lv_obj_set_height(data->nav_bar, 50);
    lv_obj_set_scrollbar_mode(data->nav_bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(data->nav_bar, &style_common_cont_top, LV_PART_MAIN);

    /* 左上角返回按钮。 */
    back_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(back_btn, 50, 50);
    lv_obj_add_style(back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(back_btn, page_manager_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    /* 返回按钮图标。 */
    back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle-white.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    /* 导航栏标题。 */
    title_label = lv_label_create(data->nav_bar);
    lv_label_set_text(title_label, "WiFi列表");
    lv_obj_add_style(title_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    /* 右上角刷新按钮。 */
    data->refresh_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(data->refresh_btn, 40, 34);
    lv_obj_add_style(data->refresh_btn, &style_common_btn_back, LV_PART_MAIN);
    lv_obj_set_style_radius(data->refresh_btn, 8, LV_PART_MAIN);
    lv_obj_align(data->refresh_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_event_cb(data->refresh_btn, refresh_btn_click_cb, LV_EVENT_CLICKED, data);

    /* 刷新按钮图标。 */
    refresh_icon = lv_img_create(data->refresh_btn);
    lv_img_set_src(refresh_icon, "A:" RES_ICON_PATH "/refresh.png");
    lv_obj_align(refresh_icon, LV_ALIGN_CENTER, 0, 0);

    /* 右上角 WiFi 开关。 */
    data->wifi_switch = lv_switch_create(data->nav_bar);
    lv_obj_set_size(data->wifi_switch, 52, 30);
    lv_obj_align_to(data->wifi_switch, data->refresh_btn, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    /* 获取 WiFi 状态并设置开关 */
    data->wifi_enabled = wifi_manager_get_status() == 1 ? 1 : 0;
    if (data->wifi_enabled) {
        lv_obj_add_state(data->wifi_switch, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(data->wifi_switch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(data->wifi_switch, wifi_switch_change_cb, LV_EVENT_VALUE_CHANGED, data);

    /* 密码输入流程的全屏遮罩。 */
    data->password_modal_mask = lv_obj_create(data->container);
    lv_obj_add_flag(data->password_modal_mask, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(data->password_modal_mask, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->password_modal_mask, &style_overlay_mask, LV_PART_MAIN);
    lv_obj_add_flag(data->password_modal_mask, LV_OBJ_FLAG_HIDDEN);

    /* 密码输入面板。 */
    data->password_panel = lv_obj_create(data->password_modal_mask);
    lv_obj_set_width(data->password_panel, LV_PCT(92));
    lv_obj_set_height(data->password_panel, 170);
    lv_obj_add_style(data->password_panel, &style_modal_panel, LV_PART_MAIN);
    lv_obj_clear_flag(data->password_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(data->password_panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(data->password_panel, LV_ALIGN_TOP_MID, 0, 50);

    /* 面板标题（显示当前输入的 SSID）。 */
    data->password_ssid_label = lv_label_create(data->password_panel);
    lv_label_set_text(data->password_ssid_label, "输入密码");
    lv_obj_add_style(data->password_ssid_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->password_ssid_label, LV_ALIGN_TOP_LEFT, 12, 10);

    /* 密码输入框。 */
    data->password_ta = lv_textarea_create(data->password_panel);
    lv_obj_set_width(data->password_ta, LV_PCT(94));
    lv_obj_set_height(data->password_ta, 46);
    lv_obj_align(data->password_ta, LV_ALIGN_TOP_MID, 0, 44);
    lv_obj_clear_flag(data->password_ta, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(data->password_ta, LV_SCROLLBAR_MODE_OFF);
    lv_textarea_set_one_line(data->password_ta, true);
    lv_textarea_set_password_mode(data->password_ta, false);
    lv_textarea_set_placeholder_text(data->password_ta, "请输入WiFi密码");
    lv_obj_add_style(data->password_ta, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->password_ta, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->password_ta, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_text_color(data->password_ta, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_color(data->password_ta, lv_color_hex(0x808080), LV_PART_CURSOR | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(data->password_ta, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_text_color(data->password_ta, lv_color_hex(0xA0A0A0), LV_PART_TEXTAREA_PLACEHOLDER);

    /* 取消按钮。 */
    data->password_cancel_btn = lv_btn_create(data->password_panel);
    lv_obj_set_size(data->password_cancel_btn, 112, 38);
    lv_obj_align(data->password_cancel_btn, LV_ALIGN_BOTTOM_LEFT, 12, 2);
    lv_obj_add_event_cb(data->password_cancel_btn, password_cancel_btn_cb, LV_EVENT_CLICKED, data);
    /* 取消按钮文字。 */
    cancel_label = lv_label_create(data->password_cancel_btn);
    lv_label_set_text(cancel_label, "取消");
    lv_obj_add_style(cancel_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_align(cancel_label, LV_ALIGN_CENTER, 0, 0);

    /* 确认按钮。 */
    data->password_confirm_btn = lv_btn_create(data->password_panel);
    lv_obj_set_size(data->password_confirm_btn, 112, 38);
    lv_obj_align(data->password_confirm_btn, LV_ALIGN_BOTTOM_RIGHT, -12, 2);
    lv_obj_add_event_cb(data->password_confirm_btn, password_confirm_btn_cb, LV_EVENT_CLICKED, data);
    /* 确认按钮文字。 */
    confirm_label = lv_label_create(data->password_confirm_btn);
    lv_label_set_text(confirm_label, "确认");
    lv_obj_add_style(confirm_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_align(confirm_label, LV_ALIGN_CENTER, 0, 0);

    /* 屏幕底部密码键盘。 */
    data->password_kb = lv_keyboard_create(data->password_modal_mask);
    lv_obj_set_width(data->password_kb, LV_PCT(100));
    lv_obj_set_height(data->password_kb, 220);
    lv_obj_align(data->password_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(data->password_kb, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->password_kb, lv_color_hex(0x121212), LV_PART_MAIN);
    lv_obj_set_style_border_width(data->password_kb, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_top(data->password_kb, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(data->password_kb, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_left(data->password_kb, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_right(data->password_kb, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_row(data->password_kb, 3, LV_PART_MAIN);
    lv_obj_set_style_pad_column(data->password_kb, 3, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->password_kb, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(data->password_kb, lv_color_hex(0x2A2A2A), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(data->password_kb, lv_color_hex(0x3A3A3A), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(data->password_kb, lv_color_hex(0x3A3A3A), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(data->password_kb, lv_color_hex(0x464646), LV_PART_ITEMS | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(data->password_kb, 0, LV_PART_ITEMS);
    lv_obj_set_style_radius(data->password_kb, 8, LV_PART_ITEMS);
    lv_obj_set_style_text_color(data->password_kb, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(data->password_kb, lv_color_white(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_font(data->password_kb, &lv_font_montserrat_28, LV_PART_ITEMS);
    lv_keyboard_set_mode(data->password_kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(data->password_kb, data->password_ta);
    lv_obj_add_event_cb(data->password_kb, password_keyboard_event_cb, LV_EVENT_ALL, data);

    /* WiFi 列表容器。 */
    data->wifi_list = lv_list_create(data->container);
    lv_obj_set_width(data->wifi_list, lv_pct(100));
    lv_obj_set_flex_grow(data->wifi_list, 1);
    lv_obj_set_style_margin_all(data->wifi_list, 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->wifi_list, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->wifi_list, lv_color_hex(0x121212), LV_PART_MAIN);
    lv_obj_add_style(data->wifi_list, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_set_scroll_dir(data->wifi_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(data->wifi_list, LV_SCROLLBAR_MODE_AUTO);

    /* 初始化定时器为 NULL */
    data->scan_timer = NULL;
    data->scan_count = 0;
    data->pending_scan_id = -1;

    /* 启用整页事件冒泡，确保子对象按压事件传递到父容器。 */
    gesture_back_enable_event_bubble_recursive(data->container);

    /* 保存 private_data */
    page_set_private_data(data);
}

void page_wifi_list_destroy(void)
{
    page_wifi_list_data_t* data = (page_wifi_list_data_t*)page_get_private_data();
    if (data == NULL) {
        return;
    }

    /* 停止扫描定时器 */
    if (data->scan_timer != NULL) {
        lv_timer_del(data->scan_timer);
        data->scan_timer = NULL;
    }

    if (data->container != NULL) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    free(data);
}

void page_wifi_list_show(void)
{
    page_wifi_list_data_t* data = (page_wifi_list_data_t*)page_get_private_data();
    if (data == NULL || data->container == NULL) {
        return;
    }

    gesture_back_set_left_edge_swipe_cb(data->container, page_manager_back_cb);

    /* WiFi 已启用则开始扫描 */
    if (data->wifi_enabled) {
        start_wifi_scan(data);
    }

    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_wifi_list_hide(void)
{
    page_wifi_list_data_t* data = (page_wifi_list_data_t*)page_get_private_data();
    if (data == NULL || data->container == NULL) {
        return;
    }

    /* 停止扫描定时器 */
    if (data->scan_timer != NULL) {
        lv_timer_del(data->scan_timer);
        data->scan_timer = NULL;
    }

    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_wifi_list_update(void)
{
    /* no-op */
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
