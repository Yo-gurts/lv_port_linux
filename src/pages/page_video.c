// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_video.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/key_manager.h"
#include "core/media_manager.h"
#include "core/page_manager.h"
#include "core/param_manager.h"
#include "core/power_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include "ui/filter_panel.h"
#include "ui/zoom_bar.h"
#include "ui/gesture_back.h"
#include <stdlib.h>
#include <string.h>

/* 布局常量 */
#define TOP_BAR_HEIGHT 50
#define BOTTOM_BAR_HEIGHT 50
#define RECORD_UI_UPDATE_PERIOD_MS 500

/* 视频分辨率图标 - 与video_settings保持一致 */
static const char* video_resolution_icons[] = {
    "A" RES_ICON_PATH "/4k.png",
    "A" RES_ICON_PATH "/2.7k.png",
    "A" RES_ICON_PATH "/1080p.png",
    "A" RES_ICON_PATH "/720p.png",
};

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_video.h)
// #############################################################################

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

/* 更新分辨率显示 */
static void update_resolution_display(void)
{
    page_video_data_t* data = page_get_private_data();
    int resolution_index;

    if (data == NULL || data->resolution_img == NULL) {
        return;
    }

    resolution_index = param_manager_get(PARAM_ID_VIDEO_RESOLUTION);
    lv_img_set_src(data->resolution_img, video_resolution_icons[resolution_index]);
}

static void update_zoom_buttons_display(page_video_data_t* data)
{
    if (data == NULL) {
        return;
    }

    zoom_bar_set_value(param_manager_get(PARAM_ID_ZOOM));
}

static void update_recording_input_lock(page_video_data_t* data)
{
    uint8_t mask;

    if (data == NULL) {
        return;
    }

    if (data->is_recording) {
        if (data->record_input_locked) {
            return;
        }
        mask = key_manager_get_block_non_power();
        data->record_input_prev_mask = mask;
        key_manager_set_block_non_power(
            (uint8_t)(mask | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_NON_CAMERA_KEYS));
        data->record_input_locked = 1U;
        return;
    }

    if (!data->record_input_locked) {
        return;
    }
    key_manager_set_block_non_power(data->record_input_prev_mask);
    data->record_input_locked = 0U;
}

static void update_recording_indicator(page_video_data_t* data)
{
    char time_text[16];
    uint32_t elapsed_ms;
    uint32_t elapsed_sec;
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;
    uint8_t dot_visible;

    if (data == NULL || data->record_dot == NULL || data->record_time_label == NULL) {
        return;
    }

    if (!data->is_recording) {
        lv_obj_add_flag(data->record_dot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(data->record_time_label, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_clear_flag(data->record_dot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(data->record_time_label, LV_OBJ_FLAG_HIDDEN);

    elapsed_ms = lv_tick_elaps(data->record_start_tick);
    elapsed_sec = elapsed_ms / 1000U;
    hours = elapsed_sec / 3600U;
    minutes = (elapsed_sec % 3600U) / 60U;
    seconds = elapsed_sec % 60U;
    lv_snprintf(time_text, sizeof(time_text), "%02u:%02u:%02u", (unsigned int)hours, (unsigned int)minutes, (unsigned int)seconds);
    lv_label_set_text(data->record_time_label, time_text);

    dot_visible = (uint8_t)(((elapsed_ms / 500U) % 2U) == 0U);
    if (dot_visible == data->record_dot_visible) {
        return;
    }
    data->record_dot_visible = dot_visible;
    if (dot_visible) {
        lv_obj_clear_flag(data->record_dot, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_add_flag(data->record_dot, LV_OBJ_FLAG_HIDDEN);
}

static void record_ui_timer_cb(lv_timer_t* timer)
{
    page_video_data_t* data;

    if (timer == NULL) {
        return;
    }

    data = (page_video_data_t*)lv_timer_get_user_data(timer);
    if (data == NULL || data->container == NULL) {
        return;
    }
    if (lv_obj_has_flag(data->container, LV_OBJ_FLAG_HIDDEN)) {
        return;
    }

    update_recording_indicator(data);
}

static int page_video_stop_recording_if_needed(void* user_data)
{
    page_video_data_t* data = (page_video_data_t*)user_data;
    int ret;

    if (data == NULL || data->container == NULL) {
        return 0;
    }
    if (lv_obj_has_flag(data->container, LV_OBJ_FLAG_HIDDEN)) {
        return 0;
    }
    if (!data->is_recording) {
        return 0;
    }

    ret = media_manager_execute(MEDIA_OP_STOP_RECORD, 0);
    if (ret != 0) {
        MLOG_WARN("stop record before shutdown failed: ret=%d", ret);
        return -1;
    }

    data->is_recording = 0;
    update_recording_input_lock(data);
    update_recording_indicator(data);
    return 0;
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

/* 录像/拍照切换回调。 */
static void mode_switch_cb(lv_event_t* e)
{
    int reset_filter;

    LV_UNUSED(e);
    reset_filter = param_manager_get(PARAM_ID_FILTER_RESET_ON_MODE_SWITCH);
    if (reset_filter == 1) {
        (void)param_manager_set(PARAM_ID_FILTER_INDEX, 0);
    }
    (void)param_manager_set(PARAM_ID_ZOOM, param_manager_get_default(PARAM_ID_ZOOM));

    filter_panel_hide();
    zoom_bar_hide();
    MLOG_INFO("Back to photo page");
    (void)media_manager_execute(MEDIA_OP_SWITCH_TO_PHOTO_MODE, 0);
    page_manager_back();
}

/* 顶部返回按钮回调：页面专用，避免使用通用 back_cb 丢失 intent */
static void back_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    (void)param_manager_set(PARAM_ID_ZOOM, param_manager_get_default(PARAM_ID_ZOOM));
    filter_panel_hide();
    zoom_bar_hide();
    (void)media_manager_execute(MEDIA_OP_SWITCH_TO_PHOTO_MODE, 0);
    page_manager_back();
}

/* 菜单按钮回调：跳转录像设置页面 */
static void menu_back_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    filter_panel_hide();
    MLOG_INFO("Menu clicked, navigate to video_settings");
    page_manager_navigate("video_settings");
}

/* 滤镜按钮回调：显示/隐藏全局滤镜面板。 */
static void filter_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);

    if (filter_panel_is_visible()) {
        filter_panel_hide();
        return;
    }

    filter_panel_show();
}

static void record_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_video_data_t* data = (page_video_data_t*)user_data;
    int ret;

    if (key != KEY_ID_CAMERA || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (data == NULL || data->container == NULL) {
        return;
    }
    if (lv_obj_has_flag(data->container, LV_OBJ_FLAG_HIDDEN)) {
        return;
    }

    if (!data->is_recording) {
        ret = media_manager_execute(MEDIA_OP_START_RECORD, 0);
        if (ret != 0) {
            MLOG_ERR("start record failed: ret=%d", ret);
            return;
        }
        data->is_recording = 1;
        data->record_start_tick = lv_tick_get();
        data->record_dot_visible = 1U;
        lv_label_set_text(data->record_time_label, "00:00:00");
        update_recording_input_lock(data);
        update_recording_indicator(data);
        return;
    }

    ret = media_manager_execute(MEDIA_OP_STOP_RECORD, 0);
    if (ret != 0) {
        MLOG_ERR("stop record failed: ret=%d", ret);
        return;
    }
    data->is_recording = 0;
    update_recording_input_lock(data);
    update_recording_indicator(data);
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_video_create(void)
{
    page_video_data_t* data = (page_video_data_t*)malloc(sizeof(page_video_data_t));
    if (data == NULL) {
        return;
    }

    memset(data, 0, sizeof(page_video_data_t));
    data->is_recording = 0; /* 默认未录像 */

    /* 初始化全局滤镜面板（单例）。 */
    filter_panel_init();
    zoom_bar_init();

    /* 创建页面容器 */
    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_width(data->container, LV_PCT(100));
    lv_obj_set_height(data->container, LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_refr_size(data->container);

    /* 启用滑动手势检测，不将 GESTURE 事件传递给父控件 */
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    /* 添加滑动手势回调：从左往右滑返回拍照页 */
    gesture_back_register_events(data->container);
    gesture_back_set_left_edge_swipe_cb(data->container, back_btn_cb);

    zoom_bar_hide();
    update_zoom_buttons_display(data);

    /* =======================
     * 顶部状态栏：[back][4K] 在左边，录像时长 [SD][battery] 在右边
     * ======================= */
    data->top_bar = lv_obj_create(data->container);
    lv_obj_set_width(data->top_bar, lv_pct(100));
    lv_obj_set_height(data->top_bar, TOP_BAR_HEIGHT);
    lv_obj_add_style(data->top_bar, &style_noboarder, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(data->top_bar, LV_SCROLLBAR_MODE_OFF);

    /* 返回按钮 - 左上角 */
    data->back_btn = lv_btn_create(data->top_bar);
    lv_obj_set_size(data->back_btn, 50, 50);
    lv_obj_add_style(data->back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->back_btn, LV_ALIGN_TOP_LEFT, 10, 0);
    lv_obj_t* back_icon = lv_img_create(data->back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    /* 分辨率图标 - 跟在返回按钮后面 */
    data->resolution_img = lv_img_create(data->top_bar);
    lv_obj_align(data->resolution_img, LV_ALIGN_LEFT_MID, 65, 0);
    update_resolution_display();

    /* 录像时长 Label - 右上角 */
    data->time_label = lv_label_create(data->top_bar);
    lv_label_set_text(data->time_label, "00:00:00");
    lv_obj_add_style(data->time_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->time_label, LV_ALIGN_RIGHT_MID, -110, 0);

    /* SD卡图标 - 右上角 */
    data->sd_icon = lv_img_create(data->top_bar);
    lv_img_set_src(data->sd_icon, "A:" RES_ICON_PATH "/sd_online.png");
    lv_obj_align(data->sd_icon, LV_ALIGN_RIGHT_MID, -60, 0);

    /* 电池图标 - 最右上角 */
    data->battery_icon = lv_img_create(data->top_bar);
    lv_img_set_src(data->battery_icon, "A:" RES_ICON_PATH "/battery-full.png");
    lv_obj_align(data->battery_icon, LV_ALIGN_RIGHT_MID, -10, 0);

    /* =======================
     * 底部工具栏：[photo][filter] ... [switch][menu]
     * ======================= */
    data->bottom_bar = lv_obj_create(data->container);
    lv_obj_set_width(data->bottom_bar, lv_pct(100));
    lv_obj_set_height(data->bottom_bar, BOTTOM_BAR_HEIGHT);
    lv_obj_add_style(data->bottom_bar, &style_noboarder, LV_PART_MAIN);
    lv_obj_align(data->bottom_bar, LV_ALIGN_BOTTOM_MID, 0, -5); /* 底部向上5像素 */
    lv_obj_set_scrollbar_mode(data->bottom_bar, LV_SCROLLBAR_MODE_OFF);

    /* 拍照/录像切换按钮 - 左对齐 */
    data->mode_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->mode_btn, 50, 50);
    lv_obj_add_style(data->mode_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->mode_btn, mode_switch_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->mode_btn, LV_ALIGN_LEFT_MID, 10, 0);
    data->mode_img = lv_img_create(data->mode_btn);
    lv_img_set_src(data->mode_img, "A:" RES_ICON_PATH "/video.png");
    lv_obj_align(data->mode_img, LV_ALIGN_CENTER, 0, 0);

    /* 滤镜按钮 - 紧随拍照按钮 */
    data->filter_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->filter_btn, 50, 50);
    lv_obj_add_style(data->filter_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->filter_btn, filter_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->filter_btn, LV_ALIGN_LEFT_MID, 70, 0);
    lv_obj_t* filter_icon = lv_img_create(data->filter_btn);
    lv_img_set_src(filter_icon, "A:" RES_ICON_PATH "/filter_default.png");
    lv_obj_align(filter_icon, LV_ALIGN_CENTER, 0, 0);

    /* 摄像头切换按钮 - 右对齐 */
    data->switch_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->switch_btn, 50, 50);
    lv_obj_add_style(data->switch_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->switch_btn, NULL, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->switch_btn, LV_ALIGN_RIGHT_MID, -70, 0);
    lv_obj_t* switch_icon = lv_img_create(data->switch_btn);
    lv_img_set_src(switch_icon, "A:" RES_ICON_PATH "/switch.png");
    lv_obj_align(switch_icon, LV_ALIGN_CENTER, 0, 0);

    /* 菜单按钮 - 最右侧 */
    data->menu_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->menu_btn, 50, 50);
    lv_obj_add_style(data->menu_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->menu_btn, menu_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->menu_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_t* menu_icon = lv_img_create(data->menu_btn);
    lv_img_set_src(menu_icon, "A:" RES_ICON_PATH "/menu.png");
    lv_obj_align(menu_icon, LV_ALIGN_CENTER, 0, 0);

    /* 底部录制状态：[红点][00:00:00] */
    data->record_dot = lv_obj_create(data->bottom_bar);
    lv_obj_remove_style_all(data->record_dot);
    lv_obj_set_size(data->record_dot, 18, 18);
    lv_obj_set_style_radius(data->record_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->record_dot, lv_color_hex(0xFF2A2A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->record_dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->record_dot, 0, LV_PART_MAIN);
    lv_obj_align(data->record_dot, LV_ALIGN_CENTER, -75, 0);

    data->record_time_label = lv_label_create(data->bottom_bar);
    lv_label_set_text(data->record_time_label, "00:00:00");
    lv_obj_add_style(data->record_time_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->record_time_label, LV_ALIGN_CENTER, 6, 0);

    lv_obj_add_flag(data->record_dot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(data->record_time_label, LV_OBJ_FLAG_HIDDEN);

    /* 启用整页事件冒泡，确保子对象按压事件传递到父容器。 */
    gesture_back_enable_event_bubble_recursive(data->container);

    /* 保存 private_data，供 show/hide/destroy 使用 */
    page_set_private_data(data);
}

void page_video_destroy(void)
{
    page_video_data_t* data = page_get_private_data();
    if (data == NULL) {
        return;
    }

    (void)key_manager_unregister_callback(KEY_ID_CAMERA, KEY_EVENT_CLICK, record_key_cb, data);
    power_manager_unregister_shutdown_prepare_cb(page_video_stop_recording_if_needed, data);
    if (data->record_ui_timer != NULL) {
        lv_timer_del(data->record_ui_timer);
        data->record_ui_timer = NULL;
    }
    if (data->record_input_locked) {
        key_manager_set_block_non_power(data->record_input_prev_mask);
        data->record_input_locked = 0U;
    }
    power_manager_enable_auto_sleep();
    filter_panel_hide();
    zoom_bar_hide();
    /* 删除容器（子元素会自动删除） */
    if (data->container != NULL) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    free(data);
}

void page_video_show(void)
{
    page_video_data_t* data = page_get_private_data();
    if (data == NULL || data->container == NULL) {
        return;
    }

    gesture_back_set_left_edge_swipe_cb(data->container, back_btn_cb);

    MLOG_INFO("Video page show");
    power_manager_disable_auto_sleep();
    power_manager_register_shutdown_prepare_cb(page_video_stop_recording_if_needed, data);
    filter_panel_hide();
    zoom_bar_show();
    update_resolution_display();
    update_zoom_buttons_display(data);
    update_recording_input_lock(data);
    update_recording_indicator(data);
    if (data->record_ui_timer == NULL) {
        data->record_ui_timer = lv_timer_create(record_ui_timer_cb, RECORD_UI_UPDATE_PERIOD_MS, data);
    }
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);

    if (key_manager_register_callback(KEY_ID_CAMERA, KEY_EVENT_CLICK, record_key_cb, data) != 0) {
        MLOG_WARN("register video record key callback failed");
    }
}

void page_video_hide(void)
{
    page_video_data_t* data = page_get_private_data();
    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Video page hide");
    power_manager_enable_auto_sleep();
    power_manager_unregister_shutdown_prepare_cb(page_video_stop_recording_if_needed, data);
    (void)key_manager_unregister_callback(KEY_ID_CAMERA, KEY_EVENT_CLICK, record_key_cb, data);
    if (data->record_ui_timer != NULL) {
        lv_timer_del(data->record_ui_timer);
        data->record_ui_timer = NULL;
    }
    if (data->record_input_locked) {
        key_manager_set_block_non_power(data->record_input_prev_mask);
        data->record_input_locked = 0U;
    }
    filter_panel_hide();
    zoom_bar_hide();
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_video_update(void)
{
    page_video_data_t* data = page_get_private_data();

    /* 更新分辨率显示 */
    update_resolution_display();
    update_zoom_buttons_display(data);
    update_recording_input_lock(data);
    update_recording_indicator(data);
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
