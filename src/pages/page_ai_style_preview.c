// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_ai_style_preview.h"
#include "config.h"
#include "core/file_manager.h"
#include "core/font_manager.h"
#include "core/image_process_manager.h"
#include "core/key_manager.h"
#include "core/page_manager.h"
#include "core/param_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include "ui/gesture_back.h"
#include "ui/status_bar.h"
#include "ui/top_notice.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义
// #############################################################################

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

#define PREVIEW_BACK_BTN_SIZE 50
#define STYLE_PANEL_HEIGHT 116
#define STYLE_PANEL_BOTTOM_OFFSET 10
#define STYLE_THUMB_WIDTH 80
#define STYLE_THUMB_HEIGHT 60
#define STYLE_ITEM_HEIGHT 100
#define STYLE_ITEM_GAP 12
#define STYLE_PANEL_TOP_PAD 8
#define STYLE_PANEL_BOTTOM_PAD 2
#define STYLE_FOCUS_Y_OFFSET -1

#define STYLE_PROCESS_POLL_MS 100

static const char* g_style_names[AI_STYLE_PREVIEW_STYLE_COUNT] = {
    "原图", "3D风格", "写实风", "天使风", "动漫风",
    "日漫风", "公主风", "梦幻风", "水墨风", "新莫奈花园风",
    "水彩风", "莫奈花园风", "精致美漫", "赛博机械", "精致韩漫",
    "国风-水墨", "浪漫光影", "瓷娃娃", "中国红", "丑萌粘土",
    "可爱玩偶", "3D游戏Z世代风", "动画电影", "玩偶", "青年15岁",
    "中年35岁", "老年80岁"
};

static const char* g_style_prompts[AI_STYLE_PREVIEW_STYLE_COUNT - 1] = {
    "将这张照片重绘成 3D 风格",
    "将这张照片重绘成 写实风 风格",
    "将这张照片重绘成 天使风 风格",
    "将这张照片重绘成 动漫风 风格",
    "将这张照片重绘成 日漫风 风格",
    "将这张照片重绘成 公主风 风格",
    "将这张照片重绘成 梦幻风 风格",
    "将这张照片重绘成 水墨风 风格",
    "将这张照片重绘成 新莫奈花园风 风格",
    "将这张照片重绘成 水彩风 风格",
    "将这张照片重绘成 莫奈花园风 风格",
    "将这张照片重绘成 精致美漫 风格",
    "将这张照片重绘成 赛博机械 风格",
    "将这张照片重绘成 精致韩漫 风格",
    "将这张照片重绘成 国风-水墨 风格",
    "将这张照片重绘成 浪漫光影 风格",
    "将这张照片重绘成 瓷娃娃 风格",
    "将这张照片重绘成 中国红 风格",
    "将这张照片重绘成 丑萌粘土 风格",
    "将这张照片重绘成 可爱玩偶 风格",
    "将这张照片重绘成 3D游戏Z世代风 风格",
    "将这张照片重绘成 动画电影 风格",
    "将这张照片重绘成 玩偶 风格",
    "将这张照片重绘成 青年15岁 风格，年轻，稚嫩",
    "将这张照片重绘成 中年35岁 风格，成熟，稳重",
    "将这张照片重绘成 老年80岁 风格，白发，皱纹",
};

static void refresh_latest_photo(page_ai_style_preview_data_t* data);
static void update_style_selection(page_ai_style_preview_data_t* data);
static void scroll_to_style(page_ai_style_preview_data_t* data, int index, lv_anim_enable_t anim_en);
static void set_style_panel_visible(page_ai_style_preview_data_t* data, bool visible, lv_anim_enable_t anim_en);
static void align_focus_frame(page_ai_style_preview_data_t* data);
static void set_loading_visible(page_ai_style_preview_data_t* data, bool visible);
static void stop_process_poll_timer(page_ai_style_preview_data_t* data);
static void start_style_process(page_ai_style_preview_data_t* data);

static void style_item_click_cb(lv_event_t* e);
static void style_list_scroll_end_cb(lv_event_t* e);
static void back_btn_cb(lv_event_t* e);
static void gesture_event_cb(lv_event_t* e);
static void process_poll_timer_cb(lv_timer_t* timer);
static void ai_key_cb(key_id_t key, key_event_type_t event_type, void* user_data);

static int g_target_photo_index = -1;

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数
// #############################################################################

static void refresh_latest_photo(page_ai_style_preview_data_t* data)
{
    int total_photos;
    int photo_index;

    if (!data || !data->image) {
        return;
    }

    if (file_manager_refresh_photo_list() != 0) {
        MLOG_WARN("Style preview refresh photo list failed");
    }

    total_photos = file_manager_get_photo_count();
    if (total_photos <= 0) {
        data->latest_photo_display_path[0] = '\0';
        data->latest_photo_real_path[0] = '\0';
        lv_img_set_src(data->image, NULL);
        return;
    }

    photo_index = (g_target_photo_index >= 0 && g_target_photo_index < total_photos)
        ? g_target_photo_index
        : 0;
    g_target_photo_index = -1;

    if (file_manager_get_photo_subpic_path(photo_index, data->latest_photo_display_path, sizeof(data->latest_photo_display_path),
            FILE_PATH_LV)
        != 0) {
        if (file_manager_get_photo_path(photo_index, data->latest_photo_display_path, sizeof(data->latest_photo_display_path),
                FILE_PATH_LV)
            != 0) {
            data->latest_photo_display_path[0] = '\0';
            data->latest_photo_real_path[0] = '\0';
            lv_img_set_src(data->image, NULL);
            return;
        }
    }

    if (file_manager_get_photo_path(photo_index, data->latest_photo_real_path, sizeof(data->latest_photo_real_path),
            FILE_PATH_REAL)
        != 0) {
        data->latest_photo_real_path[0] = '\0';
    }

    lv_img_set_src(data->image, data->latest_photo_display_path);
    lv_obj_center(data->image);
    MLOG_INFO("Style preview showing photo[%d]: %s", photo_index, data->latest_photo_display_path);
}

static void update_style_selection(page_ai_style_preview_data_t* data)
{
    int i;

    if (!data) {
        return;
    }

    for (i = 0; i < AI_STYLE_PREVIEW_STYLE_COUNT; i++) {
        if (data->style_labels[i] == NULL || data->style_items[i] == NULL) {
            continue;
        }

        if (i == data->selected_style_index) {
            lv_obj_set_style_text_color(data->style_labels[i], lv_color_hex(0xF09F20), LV_PART_MAIN);
            lv_obj_set_style_opa(data->style_items[i], LV_OPA_COVER, LV_PART_MAIN);
        } else {
            lv_obj_set_style_text_color(data->style_labels[i], lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_opa(data->style_items[i], LV_OPA_80, LV_PART_MAIN);
        }
    }
}

static void scroll_to_style(page_ai_style_preview_data_t* data, int index, lv_anim_enable_t anim_en)
{
    int32_t step;
    int32_t target_x;
    int32_t max_scroll_x;

    if (!data || !data->style_list || index < 0 || index >= AI_STYLE_PREVIEW_STYLE_COUNT) {
        return;
    }

    step = STYLE_THUMB_WIDTH + STYLE_ITEM_GAP;
    target_x = index * step;
    if (target_x < 0) {
        target_x = 0;
    }

    max_scroll_x = lv_obj_get_scroll_left(data->style_list) + lv_obj_get_scroll_right(data->style_list);
    if (target_x > max_scroll_x) {
        target_x = max_scroll_x;
    }
    lv_obj_scroll_to_x(data->style_list, target_x, anim_en);
}

static void set_style_panel_visible(page_ai_style_preview_data_t* data, bool visible, lv_anim_enable_t anim_en)
{
    lv_coord_t target_y;
    lv_coord_t shown_y;

    if (!data || !data->style_panel) {
        return;
    }

    shown_y = V_RES - STYLE_PANEL_HEIGHT - STYLE_PANEL_BOTTOM_OFFSET;
    target_y = visible ? shown_y : V_RES;

    if (anim_en == LV_ANIM_ON) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, data->style_panel);
        lv_anim_set_values(&a, lv_obj_get_y(data->style_panel), target_y);
        lv_anim_set_time(&a, 180);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
        lv_anim_start(&a);
    } else {
        lv_obj_set_y(data->style_panel, target_y);
    }

    data->panel_visible = visible ? 1 : 0;
}

static void align_focus_frame(page_ai_style_preview_data_t* data)
{
    lv_obj_t* ref_item;
    lv_obj_t* ref_thumb;
    lv_area_t thumb_coords;
    lv_area_t panel_coords;
    int32_t y_in_panel;

    if (!data || !data->style_focus_frame || !data->style_panel || !data->style_list) {
        return;
    }

    ref_item = data->style_items[0];
    if (!ref_item) {
        return;
    }

    ref_thumb = lv_obj_get_child(ref_item, 0);
    if (!ref_thumb) {
        return;
    }

    lv_obj_update_layout(data->style_list);
    lv_obj_get_coords(ref_thumb, &thumb_coords);
    lv_obj_get_coords(data->style_panel, &panel_coords);
    y_in_panel = thumb_coords.y1 - panel_coords.y1;
    lv_obj_align(data->style_focus_frame, LV_ALIGN_TOP_MID, 0, y_in_panel + STYLE_FOCUS_Y_OFFSET);
}

static void set_loading_visible(page_ai_style_preview_data_t* data, bool visible)
{
    if (!data || !data->loading_overlay)
        return;

    if (visible) {
        lv_label_set_text(data->loading_label, "正在处理中，请稍后。");
        lv_obj_clear_flag(data->loading_overlay, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(data->loading_overlay, LV_OBJ_FLAG_HIDDEN);
    }
}

static void stop_process_poll_timer(page_ai_style_preview_data_t* data)
{
    if (!data)
        return;

    if (data->process_poll_timer) {
        lv_timer_del(data->process_poll_timer);
        data->process_poll_timer = NULL;
    }
}

static void start_style_process(page_ai_style_preview_data_t* data)
{
    int ret;
    int prompt_index;

    if (!data)
        return;

    if (data->selected_style_index <= 0) {
        if (data->latest_photo_display_path[0] != '\0') {
            lv_img_set_src(data->image, data->latest_photo_display_path);
            lv_obj_center(data->image);
        }
        return;
    }

    if (data->processing) {
        return;
    }

    if (data->latest_photo_real_path[0] == '\0') {
        refresh_latest_photo(data);
    }
    if (data->latest_photo_real_path[0] == '\0') {
        top_notice_show("暂无可处理图片", TOP_NOTICE_TYPE_WARNING);
        return;
    }

    prompt_index = data->selected_style_index - 1;
    if (prompt_index < 0 || prompt_index >= AI_STYLE_PREVIEW_STYLE_COUNT - 1) {
        return;
    }

    ret = image_process_manager_start_style(data->latest_photo_real_path, g_style_prompts[prompt_index]);
    if (ret != 0) {
        top_notice_show("启动处理失败，请稍后重试", TOP_NOTICE_TYPE_WARNING);
        return;
    }

    data->processing = 1;
    set_loading_visible(data, true);

    stop_process_poll_timer(data);
    data->process_poll_timer = lv_timer_create(process_poll_timer_cb, STYLE_PROCESS_POLL_MS, data);
    if (data->process_poll_timer) {
        lv_timer_ready(data->process_poll_timer);
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

static void style_item_click_cb(lv_event_t* e)
{
    page_ai_style_preview_data_t* data = (page_ai_style_preview_data_t*)lv_event_get_user_data(e);
    int index = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_current_target(e));

    if (!data || index < 0 || index >= AI_STYLE_PREVIEW_STYLE_COUNT) {
        return;
    }

    data->selected_style_index = index;
    update_style_selection(data);
    scroll_to_style(data, index, LV_ANIM_ON);
}

static void style_list_scroll_end_cb(lv_event_t* e)
{
    page_ai_style_preview_data_t* data = (page_ai_style_preview_data_t*)lv_event_get_user_data(e);
    int32_t step;
    int32_t scroll_x;
    int nearest_index;

    if (!data || !data->style_list) {
        return;
    }

    step = STYLE_THUMB_WIDTH + STYLE_ITEM_GAP;
    scroll_x = lv_obj_get_scroll_left(data->style_list);
    nearest_index = (scroll_x + step / 2) / step;
    if (nearest_index < 0) {
        nearest_index = 0;
    }
    if (nearest_index >= AI_STYLE_PREVIEW_STYLE_COUNT) {
        nearest_index = AI_STYLE_PREVIEW_STYLE_COUNT - 1;
    }

    data->selected_style_index = nearest_index;
    update_style_selection(data);
    scroll_to_style(data, nearest_index, LV_ANIM_ON);
}

static void back_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_manager_back();
}

static void menu_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)key;
    (void)event_type;
    (void)user_data;
    page_manager_back();
}

static void menu_key_long_press_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)key;
    (void)event_type;
    (void)user_data;
    page_manager_back();
}

static void ok_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_ai_style_preview_data_t* data = (page_ai_style_preview_data_t*)user_data;

    if (key != KEY_ID_OK || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (!data || data->processing) {
        return;
    }

    {
        int wifi_enabled = param_manager_get(PARAM_ID_WIFI_ENABLED);
        if (wifi_enabled != 1) {
            top_notice_show("WiFi未使能", TOP_NOTICE_TYPE_WARNING);
            return;
        }
        int wifi_connected = param_manager_get(PARAM_ID_WIFI_CONNECTED);
        if (wifi_connected != 1) {
            top_notice_show("WiFi未连接", TOP_NOTICE_TYPE_WARNING);
            return;
        }
    }

    start_style_process(data);
}

static void left_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_ai_style_preview_data_t* data = (page_ai_style_preview_data_t*)user_data;

    if (key != KEY_ID_LEFT || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (!data || !data->panel_visible) {
        return;
    }

    if (data->selected_style_index > 0) {
        data->selected_style_index--;
        update_style_selection(data);
        scroll_to_style(data, data->selected_style_index, LV_ANIM_ON);
        align_focus_frame(data);
    }
}

static void right_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_ai_style_preview_data_t* data = (page_ai_style_preview_data_t*)user_data;

    if (key != KEY_ID_RIGHT || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (!data || !data->panel_visible) {
        return;
    }

    if (data->selected_style_index < AI_STYLE_PREVIEW_STYLE_COUNT - 1) {
        data->selected_style_index++;
        update_style_selection(data);
        scroll_to_style(data, data->selected_style_index, LV_ANIM_ON);
        align_focus_frame(data);
    }
}

static void gesture_event_cb(lv_event_t* e)
{
    page_ai_style_preview_data_t* data = (page_ai_style_preview_data_t*)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t* indev;
    lv_dir_t dir;

    if (!data || code != LV_EVENT_GESTURE) {
        return;
    }

    indev = lv_indev_get_act();
    if (!indev) {
        return;
    }

    dir = lv_indev_get_gesture_dir(indev);
    if (dir == LV_DIR_BOTTOM) {
        set_style_panel_visible(data, false, LV_ANIM_ON);
        lv_indev_wait_release(indev);
    } else if (dir == LV_DIR_TOP) {
        set_style_panel_visible(data, true, LV_ANIM_ON);
        lv_indev_wait_release(indev);
    }
}

static void process_poll_timer_cb(lv_timer_t* timer)
{
    page_ai_style_preview_data_t* data;
    image_process_state_t state;
    char result_path[FILE_MANAGER_MAX_PATH_LEN];

    if (!timer)
        return;

    data = (page_ai_style_preview_data_t*)lv_timer_get_user_data(timer);
    if (!data || !data->container)
        return;

    state = image_process_manager_get_state();
    if (state == IMAGE_PROCESS_STATE_RUNNING) {
        return;
    }

    if (state == IMAGE_PROCESS_STATE_SUCCESS) {
        if (image_process_manager_get_result_path(result_path, sizeof(result_path), 1) == 0) {
            snprintf(data->processed_photo_display_path, sizeof(data->processed_photo_display_path), "%s", result_path);
            lv_img_set_src(data->image, data->processed_photo_display_path);
            lv_obj_center(data->image);
            MLOG_INFO("Style process success, result: %s", data->processed_photo_display_path);
        }
    } else if (state == IMAGE_PROCESS_STATE_FAILED) {
        int err = image_process_manager_get_error();
        char notice[64];
        snprintf(notice, sizeof(notice), "处理失败(%d)，请重试", err);
        top_notice_show(notice, TOP_NOTICE_TYPE_WARNING);
        MLOG_ERR("Style process failed: %d", err);
    }

    data->processing = 0;
    set_loading_visible(data, false);
    stop_process_poll_timer(data);
}

static void ai_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_ai_style_preview_data_t* data = (page_ai_style_preview_data_t*)user_data;
    int wifi_enabled;
    int wifi_connected;

    if (key != KEY_ID_ASSISTANT || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (!data || !data->container) {
        return;
    }
    if (lv_obj_has_flag(data->container, LV_OBJ_FLAG_HIDDEN)) {
        return;
    }

    wifi_enabled = param_manager_get(PARAM_ID_WIFI_ENABLED);
    if (wifi_enabled != 1) {
        top_notice_show("WiFi未使能", TOP_NOTICE_TYPE_WARNING);
        return;
    }

    wifi_connected = param_manager_get(PARAM_ID_WIFI_CONNECTED);
    if (wifi_connected != 1) {
        top_notice_show("WiFi未连接", TOP_NOTICE_TYPE_WARNING);
        return;
    }

    start_style_process(data);
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_ai_style_preview_create(void)
{
    int i;
    page_ai_style_preview_data_t* data = (page_ai_style_preview_data_t*)malloc(sizeof(page_ai_style_preview_data_t));

    if (!data) {
        return;
    }

    memset(data, 0, sizeof(page_ai_style_preview_data_t));

    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(data->container, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->container, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(data->container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    gesture_back_register_events(data->container);
    gesture_back_set_left_edge_swipe_cb(data->container, back_btn_cb);
    lv_obj_add_event_cb(data->container, gesture_event_cb, LV_EVENT_GESTURE, data);

    data->image_area = lv_obj_create(data->container);
    lv_obj_set_size(data->image_area, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->image_area, &style_noboarder, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->image_area, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(data->image_area, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(data->image_area, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(data->image_area, LV_OBJ_FLAG_SCROLLABLE);

    data->image = lv_img_create(data->image_area);
    lv_obj_center(data->image);

    // 返回按钮
    data->back_btn = lv_btn_create(data->container);
    lv_obj_set_size(data->back_btn, PREVIEW_BACK_BTN_SIZE, PREVIEW_BACK_BTN_SIZE);
    lv_obj_add_flag(data->back_btn, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(data->back_btn, LV_OBJ_FLAG_FLOATING);
    lv_obj_add_style(data->back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_align(data->back_btn, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_event_cb(data->back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* back_icon = lv_img_create(data->back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle-white.png");
    lv_obj_center(back_icon);

    // 样式选择面板
    data->style_panel = lv_obj_create(data->container);
    lv_obj_set_size(data->style_panel, H_RES, STYLE_PANEL_HEIGHT);
    lv_obj_add_flag(data->style_panel, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_style(data->style_panel, &style_photo_filter_panel, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(data->style_panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(data->style_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(data->style_panel, 0, V_RES - STYLE_PANEL_HEIGHT - STYLE_PANEL_BOTTOM_OFFSET);

    // 样式列表
    data->style_list = lv_obj_create(data->style_panel);
    lv_obj_set_size(data->style_list, lv_pct(100), lv_pct(100));
    lv_obj_set_scroll_dir(data->style_list, LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(data->style_list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_layout(data->style_list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->style_list, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(data->style_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(data->style_list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->style_list, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(data->style_list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(data->style_list, (H_RES - STYLE_THUMB_WIDTH) / 2, LV_PART_MAIN);
    lv_obj_set_style_pad_right(data->style_list, (H_RES - STYLE_THUMB_WIDTH) / 2, LV_PART_MAIN);
    lv_obj_set_style_pad_top(data->style_list, STYLE_PANEL_TOP_PAD, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(data->style_list, STYLE_PANEL_BOTTOM_PAD, LV_PART_MAIN);
    lv_obj_set_style_pad_column(data->style_list, STYLE_ITEM_GAP, LV_PART_MAIN);
    lv_obj_add_event_cb(data->style_list, style_list_scroll_end_cb, LV_EVENT_SCROLL_END, data);

    // 样式项
    for (i = 0; i < AI_STYLE_PREVIEW_STYLE_COUNT; i++) {
        lv_obj_t* item = lv_obj_create(data->style_list);
        lv_obj_t* thumb = lv_obj_create(item);
        lv_obj_t* thumb_img = lv_img_create(thumb);
        lv_obj_t* label = lv_label_create(item);

        lv_obj_set_size(item, STYLE_THUMB_WIDTH, STYLE_ITEM_HEIGHT);
        lv_obj_set_scrollbar_mode(item, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_style(item, &style_photo_filter_item, LV_PART_MAIN);
        lv_obj_set_layout(item, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(item, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(item, 8, LV_PART_MAIN);
        lv_obj_add_event_cb(item, style_item_click_cb, LV_EVENT_CLICKED, data);
        lv_obj_set_user_data(item, (void*)(intptr_t)i);

        lv_obj_set_size(thumb, STYLE_THUMB_WIDTH, STYLE_THUMB_HEIGHT);
        lv_obj_clear_flag(thumb, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_style(thumb, &style_photo_filter_thumb, LV_PART_MAIN);

        lv_img_set_src(thumb_img, "A:" RES_ICON_PATH "/filter.png");
        lv_obj_center(thumb_img);

        lv_label_set_text(label, g_style_names[i]);
        lv_obj_add_style(label, &SMALL_SIZE, LV_PART_MAIN);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

        data->style_items[i] = item;
        data->style_labels[i] = label;
    }

    // 焦点框
    data->style_focus_frame = lv_obj_create(data->style_panel);
    lv_obj_add_flag(data->style_focus_frame, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(data->style_focus_frame, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(data->style_focus_frame, STYLE_THUMB_WIDTH, STYLE_THUMB_HEIGHT);
    lv_obj_align(data->style_focus_frame, LV_ALIGN_TOP_MID, 0, STYLE_PANEL_TOP_PAD);
    lv_obj_add_style(data->style_focus_frame, &style_photo_filter_focus_frame, LV_PART_MAIN);

    data->loading_overlay = lv_obj_create(data->container);
    lv_obj_set_size(data->loading_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_add_flag(data->loading_overlay, LV_OBJ_FLAG_FLOATING);
    lv_obj_add_flag(data->loading_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(data->loading_overlay, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->loading_overlay, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->loading_overlay, 0, LV_PART_MAIN);
    lv_obj_clear_flag(data->loading_overlay, LV_OBJ_FLAG_SCROLLABLE);

    data->loading_spinner = lv_spinner_create(data->loading_overlay);
    lv_spinner_set_anim_params(data->loading_spinner, 1000, 60);
    lv_obj_set_size(data->loading_spinner, 60, 60);
    lv_obj_align(data->loading_spinner, LV_ALIGN_CENTER, 0, -20);

    data->loading_label = lv_label_create(data->loading_overlay);
    lv_label_set_text(data->loading_label, "正在处理中，请稍后。");
    lv_obj_add_style(data->loading_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->loading_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(data->loading_label, LV_ALIGN_CENTER, 0, 40);

    data->selected_style_index = 0;
    data->panel_visible = 1;
    update_style_selection(data);
    scroll_to_style(data, data->selected_style_index, LV_ANIM_OFF);
    align_focus_frame(data);

    (void)image_process_manager_init();
    image_process_manager_reset();
    gesture_back_enable_event_bubble_recursive(data->container);
    page_set_private_data(data);
}

void page_ai_style_preview_destroy(void)
{
    page_ai_style_preview_data_t* data = (page_ai_style_preview_data_t*)page_get_private_data();

    if (!data) {
        return;
    }

    if (data->ai_key_registered) {
        (void)key_manager_unregister_callback(KEY_ID_ASSISTANT, KEY_EVENT_CLICK, ai_key_cb, data);
        data->ai_key_registered = 0;
    }

    stop_process_poll_timer(data);
    image_process_manager_reset();

    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    image_process_manager_deinit();
    free(data);
}

void page_ai_style_preview_show(void)
{
    page_ai_style_preview_data_t* data = (page_ai_style_preview_data_t*)page_get_private_data();

    if (!data || !data->container) {
        return;
    }

    gesture_back_set_left_edge_swipe_cb(data->container, back_btn_cb);
    key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_long_press_cb, NULL);
    key_manager_register_callback(KEY_ID_OK, KEY_EVENT_CLICK, ok_key_click_cb, data);
    key_manager_register_callback(KEY_ID_LEFT, KEY_EVENT_CLICK, left_key_click_cb, data);
    key_manager_register_callback(KEY_ID_RIGHT, KEY_EVENT_CLICK, right_key_click_cb, data);
    refresh_latest_photo(data);
    set_style_panel_visible(data, true, LV_ANIM_OFF);
    set_loading_visible(data, false);
    data->processing = 0;
    image_process_manager_reset();
    stop_process_poll_timer(data);

    if (!data->ai_key_registered) {
        if (key_manager_register_callback(KEY_ID_ASSISTANT, KEY_EVENT_CLICK, ai_key_cb, data) == 0) {
            data->ai_key_registered = 1;
        } else {
            MLOG_WARN("register ai style preview key callback failed");
        }
    }

    status_bar_show(true);
    status_bar_set_icons(STATUS_BAR_ICON_SD, STATUS_BAR_ICON_WIFI, STATUS_BAR_ICON_BATTERY);

    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_ai_style_preview_hide(void)
{
    page_ai_style_preview_data_t* data = (page_ai_style_preview_data_t*)page_get_private_data();

    if (!data || !data->container) {
        return;
    }

    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_long_press_cb, NULL);
    key_manager_unregister_callback(KEY_ID_OK, KEY_EVENT_CLICK, ok_key_click_cb, data);
    key_manager_unregister_callback(KEY_ID_LEFT, KEY_EVENT_CLICK, left_key_click_cb, data);
    key_manager_unregister_callback(KEY_ID_RIGHT, KEY_EVENT_CLICK, right_key_click_cb, data);

    if (data->ai_key_registered) {
        (void)key_manager_unregister_callback(KEY_ID_ASSISTANT, KEY_EVENT_CLICK, ai_key_cb, data);
        data->ai_key_registered = 0;
    }

    stop_process_poll_timer(data);
    set_loading_visible(data, false);
    data->processing = 0;

    status_bar_show(false);
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_ai_style_preview_update(void)
{
}

void page_ai_style_preview_set_photo_index(int photo_index)
{
    g_target_photo_index = photo_index;
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
