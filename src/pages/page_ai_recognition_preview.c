// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_ai_recognition_preview.h"
#include "config.h"
#include "core/file_manager.h"
#include "core/font_manager.h"
#include "core/image_recognition_manager.h"
#include "core/key_manager.h"
#include "core/page_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include "ui/gesture_back.h"
#include "ui/status_bar.h"
#include "ui/top_notice.h"
#include <stdlib.h>
#include <string.h>

#define PREVIEW_BACK_BTN_SIZE 50
#define RECOG_THUMB_WIDTH 200
#define RECOG_THUMB_HEIGHT 140
#define RECOG_LEFT_X 30
#define RECOG_TEXT_TOP_Y 74
#define RECOG_GAP_X 20
#define RECOG_TEXT_BOX_WIDTH 360
#define RECOG_TEXT_BOX_BOTTOM_MARGIN 16
#define RECOG_TEXT_BOX_HEIGHT (V_RES - RECOG_TEXT_TOP_Y - RECOG_TEXT_BOX_BOTTOM_MARGIN)
#define RECOG_READ_BTN_WIDTH 200
#define RECOG_READ_BTN_HEIGHT 48
#define RECOG_POLL_MS 100

/* 参考 dc309 AI识万物模式提示词 */
static const char* g_recognition_prompt = "用50字左右介绍一下这张图，注意抓关键信息";

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义
// #############################################################################

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static void refresh_latest_photo(page_ai_recognition_preview_data_t* data);
static void start_recognition(page_ai_recognition_preview_data_t* data);
static void stop_recog_poll_timer(page_ai_recognition_preview_data_t* data);

static void back_btn_cb(lv_event_t* e);
static void read_btn_cb(lv_event_t* e);
static void recog_poll_timer_cb(lv_timer_t* timer);
static void ai_key_cb(key_id_t key, key_event_type_t event_type, void* user_data);

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数
// #############################################################################

static void refresh_latest_photo(page_ai_recognition_preview_data_t* data)
{
    int total_photos;

    if (data == NULL || data->thumb_img == NULL) {
        return;
    }

    if (file_manager_refresh_photo_list() != 0) {
        MLOG_WARN("Recognition preview refresh photo list failed");
    }

    total_photos = file_manager_get_photo_count();
    if (total_photos <= 0) {
        data->latest_thumb_path[0] = '\0';
        data->latest_photo_real_path[0] = '\0';
        lv_img_set_src(data->thumb_img, NULL);
        return;
    }

    if (file_manager_get_photo_thumbnail_path(0, data->latest_thumb_path, sizeof(data->latest_thumb_path), FILE_PATH_LV) != 0) {
        if (file_manager_get_photo_path(0, data->latest_thumb_path, sizeof(data->latest_thumb_path), FILE_PATH_LV) != 0) {
            data->latest_thumb_path[0] = '\0';
            data->latest_photo_real_path[0] = '\0';
            lv_img_set_src(data->thumb_img, NULL);
            return;
        }
    }

    if (file_manager_get_photo_path(0, data->latest_photo_real_path, sizeof(data->latest_photo_real_path), FILE_PATH_REAL) != 0) {
        data->latest_photo_real_path[0] = '\0';
    }

    lv_img_set_src(data->thumb_img, data->latest_thumb_path);
    lv_obj_center(data->thumb_img);
}

static void stop_recog_poll_timer(page_ai_recognition_preview_data_t* data)
{
    if (!data)
        return;

    if (data->recog_poll_timer) {
        lv_timer_del(data->recog_poll_timer);
        data->recog_poll_timer = NULL;
    }
}

static void start_recognition(page_ai_recognition_preview_data_t* data)
{
    if (!data)
        return;

    if (data->recognizing)
        return;

    if (data->latest_photo_real_path[0] == '\0') {
        refresh_latest_photo(data);
    }
    if (data->latest_photo_real_path[0] == '\0') {
        top_notice_show("暂无可识别图片", TOP_NOTICE_TYPE_WARNING);
        return;
    }

    if (image_recognition_manager_start(data->latest_photo_real_path, g_recognition_prompt) != 0) {
        top_notice_show("启动识别失败，请重试", TOP_NOTICE_TYPE_WARNING);
        return;
    }

    data->recognizing = 1;
    lv_label_set_text(data->text_label, "正在识别，请稍后。");

    stop_recog_poll_timer(data);
    data->recog_poll_timer = lv_timer_create(recog_poll_timer_cb, RECOG_POLL_MS, data);
    if (data->recog_poll_timer) {
        lv_timer_ready(data->recog_poll_timer);
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

static void read_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    MLOG_INFO("Recognition preview read text clicked");
}

static void recog_poll_timer_cb(lv_timer_t* timer)
{
    page_ai_recognition_preview_data_t* data;
    image_recognition_state_t state;
    char result_text[4096];

    if (!timer)
        return;

    data = (page_ai_recognition_preview_data_t*)lv_timer_get_user_data(timer);
    if (!data || !data->container)
        return;

    state = image_recognition_manager_get_state();
    if (state == IMAGE_RECOGNITION_STATE_RUNNING) {
        return;
    }

    if (state == IMAGE_RECOGNITION_STATE_SUCCESS) {
        if (image_recognition_manager_get_result_text(result_text, sizeof(result_text)) == 0) {
            lv_label_set_text(data->text_label, result_text);
            MLOG_INFO("Recognition success");
        }
    } else if (state == IMAGE_RECOGNITION_STATE_FAILED) {
        int err = image_recognition_manager_get_error();
        char notice[64];
        snprintf(notice, sizeof(notice), "识别失败(%d)，请重试", err);
        top_notice_show(notice, TOP_NOTICE_TYPE_WARNING);
        lv_label_set_text(data->text_label, "识别失败，请按 AI 键重试。");
        MLOG_ERR("Recognition failed: %d", err);
    }

    data->recognizing = 0;
    stop_recog_poll_timer(data);
}

static void ai_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_ai_recognition_preview_data_t* data = (page_ai_recognition_preview_data_t*)user_data;

    if (key != KEY_ID_ASSISTANT || event_type != KEY_EVENT_CLICK)
        return;
    if (!data || !data->container)
        return;
    if (lv_obj_has_flag(data->container, LV_OBJ_FLAG_HIDDEN))
        return;

    start_recognition(data);
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_ai_recognition_preview_create(void)
{
    page_ai_recognition_preview_data_t* data = (page_ai_recognition_preview_data_t*)malloc(sizeof(page_ai_recognition_preview_data_t));
    lv_obj_t* back_icon;
    int text_x;
    int thumb_y;

    if (data == NULL) {
        return;
    }

    memset(data, 0, sizeof(page_ai_recognition_preview_data_t));

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

    data->back_btn = lv_btn_create(data->container);
    lv_obj_set_size(data->back_btn, PREVIEW_BACK_BTN_SIZE, PREVIEW_BACK_BTN_SIZE);
    lv_obj_add_style(data->back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_flag(data->back_btn, LV_OBJ_FLAG_FLOATING);
    lv_obj_add_flag(data->back_btn, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_align(data->back_btn, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_event_cb(data->back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);

    back_icon = lv_img_create(data->back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle-white.png");
    lv_obj_center(back_icon);

    data->title_label = lv_label_create(data->container);
    lv_label_set_text(data->title_label, "万物识别");
    lv_obj_add_style(data->title_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->title_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_add_flag(data->title_label, LV_OBJ_FLAG_FLOATING);
    lv_obj_add_flag(data->title_label, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_align(data->title_label, LV_ALIGN_TOP_MID, 0, 14);

    thumb_y = (V_RES - RECOG_THUMB_HEIGHT) / 2;
    data->thumb_wrap = lv_obj_create(data->container);
    lv_obj_set_size(data->thumb_wrap, RECOG_THUMB_WIDTH, RECOG_THUMB_HEIGHT);
    lv_obj_set_pos(data->thumb_wrap, RECOG_LEFT_X, thumb_y);
    lv_obj_set_style_bg_color(data->thumb_wrap, lv_color_hex(0x1A1A1A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->thumb_wrap, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(data->thumb_wrap, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->thumb_wrap, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(data->thumb_wrap, lv_color_hex(0x2D2D2D), LV_PART_MAIN);
    lv_obj_set_style_pad_all(data->thumb_wrap, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(data->thumb_wrap, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(data->thumb_wrap, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_clip_corner(data->thumb_wrap, true, LV_PART_MAIN);

    data->thumb_img = lv_img_create(data->thumb_wrap);
    lv_obj_center(data->thumb_img);

    data->read_btn = lv_btn_create(data->container);
    lv_obj_set_size(data->read_btn, RECOG_READ_BTN_WIDTH, RECOG_READ_BTN_HEIGHT);
    lv_obj_set_pos(data->read_btn, RECOG_LEFT_X, thumb_y + RECOG_THUMB_HEIGHT + 16);
    lv_obj_set_style_radius(data->read_btn, 24, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->read_btn, lv_color_hex(0x2A2A2A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->read_btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->read_btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(data->read_btn, lv_color_hex(0x4A4A4A), LV_PART_MAIN);
    lv_obj_set_style_pad_left(data->read_btn, 18, LV_PART_MAIN);
    lv_obj_set_style_pad_right(data->read_btn, 18, LV_PART_MAIN);
    lv_obj_set_style_pad_top(data->read_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(data->read_btn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(data->read_btn, read_btn_cb, LV_EVENT_CLICKED, NULL);

    data->read_icon = lv_img_create(data->read_btn);
    lv_img_set_src(data->read_icon, "A:" RES_ICON_PATH "/volume-100.png");
    lv_obj_align(data->read_icon, LV_ALIGN_LEFT_MID, 0, 0);

    data->read_label = lv_label_create(data->read_btn);
    lv_label_set_text(data->read_label, "朗读文本");
    lv_obj_add_style(data->read_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->read_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(data->read_label, LV_ALIGN_LEFT_MID, 42, 0);

    text_x = RECOG_LEFT_X + RECOG_THUMB_WIDTH + RECOG_GAP_X;
    data->text_box = lv_obj_create(data->container);
    lv_obj_set_size(data->text_box, RECOG_TEXT_BOX_WIDTH, RECOG_TEXT_BOX_HEIGHT);
    lv_obj_set_pos(data->text_box, text_x, RECOG_TEXT_TOP_Y);
    lv_obj_set_style_bg_color(data->text_box, lv_color_hex(0x171717), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->text_box, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(data->text_box, 14, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->text_box, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(data->text_box, lv_color_hex(0x2E2E2E), LV_PART_MAIN);
    lv_obj_set_style_pad_left(data->text_box, 18, LV_PART_MAIN);
    lv_obj_set_style_pad_right(data->text_box, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_top(data->text_box, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(data->text_box, 14, LV_PART_MAIN);
    lv_obj_set_scroll_dir(data->text_box, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(data->text_box, LV_SCROLLBAR_MODE_AUTO);

    data->text_label = lv_label_create(data->text_box);
    lv_obj_set_width(data->text_label, RECOG_TEXT_BOX_WIDTH - 30);
    lv_obj_add_style(data->text_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->text_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_align(data->text_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_label_set_long_mode(data->text_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(data->text_label, "按 AI 键开始识别");
    lv_obj_align(data->text_label, LV_ALIGN_TOP_LEFT, 0, 0);

    (void)image_recognition_manager_init();
    image_recognition_manager_reset();
    gesture_back_enable_event_bubble_recursive(data->container);
    page_set_private_data(data);
}

void page_ai_recognition_preview_destroy(void)
{
    page_ai_recognition_preview_data_t* data = (page_ai_recognition_preview_data_t*)page_get_private_data();

    if (data == NULL) {
        return;
    }

    if (data->ai_key_registered) {
        (void)key_manager_unregister_callback(KEY_ID_ASSISTANT, KEY_EVENT_CLICK, ai_key_cb, data);
        data->ai_key_registered = 0;
    }

    stop_recog_poll_timer(data);
    image_recognition_manager_deinit();

    if (data->container != NULL) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    free(data);
}

void page_ai_recognition_preview_show(void)
{
    page_ai_recognition_preview_data_t* data = (page_ai_recognition_preview_data_t*)page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    gesture_back_set_left_edge_swipe_cb(data->container, back_btn_cb);
    key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_long_press_cb, NULL);
    refresh_latest_photo(data);
    image_recognition_manager_reset();
    data->recognizing = 0;
    stop_recog_poll_timer(data);

    if (!data->ai_key_registered) {
        if (key_manager_register_callback(KEY_ID_ASSISTANT, KEY_EVENT_CLICK, ai_key_cb, data) == 0) {
            data->ai_key_registered = 1;
        } else {
            MLOG_WARN("register ai recognition key callback failed");
        }
    }

    status_bar_show(true);
    status_bar_set_icons(STATUS_BAR_ICON_SD, STATUS_BAR_ICON_WIFI, STATUS_BAR_ICON_BATTERY);

    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
    start_recognition(data);
}

void page_ai_recognition_preview_hide(void)
{
    page_ai_recognition_preview_data_t* data = (page_ai_recognition_preview_data_t*)page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_long_press_cb, NULL);

    if (data->ai_key_registered) {
        (void)key_manager_unregister_callback(KEY_ID_ASSISTANT, KEY_EVENT_CLICK, ai_key_cb, data);
        data->ai_key_registered = 0;
    }

    stop_recog_poll_timer(data);
    data->recognizing = 0;
    status_bar_show(false);
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_ai_recognition_preview_update(void)
{
    page_ai_recognition_preview_data_t* data = (page_ai_recognition_preview_data_t*)page_get_private_data();

    if (data == NULL) {
        return;
    }

    refresh_latest_photo(data);
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
