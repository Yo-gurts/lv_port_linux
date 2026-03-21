// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_ai_recognition_preview.h"
#include "config.h"
#include "core/file_manager.h"
#include "core/font_manager.h"
#include "core/page_manager.h"
#include "core/param_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include "ui/gesture_back.h"
#include "ui/wifi_icon_helper.h"
#include <stdlib.h>
#include <string.h>

#define PREVIEW_BACK_BTN_SIZE 50
#define PREVIEW_WIFI_ICON_SIZE 45
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

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义
// #############################################################################

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static void update_wifi_icon(page_ai_recognition_preview_data_t* data);
static void refresh_latest_thumbnail(page_ai_recognition_preview_data_t* data);
static void back_btn_cb(lv_event_t* e);
static void read_btn_cb(lv_event_t* e);
static void recognition_param_cb(param_id_t id, int value, void* user_data);

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数
// #############################################################################

static void update_wifi_icon(page_ai_recognition_preview_data_t* data)
{
    int connected;
    int signal_dbm;

    if (data == NULL || data->wifi_icon == NULL) {
        return;
    }

    connected = param_manager_get(PARAM_ID_WIFI_CONNECTED);
    signal_dbm = param_manager_get(PARAM_ID_WIFI_SIGNAL_DBM);
    lv_img_set_src(data->wifi_icon, wifi_icon_helper_get_path(connected, signal_dbm));
}

static void refresh_latest_thumbnail(page_ai_recognition_preview_data_t* data)
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
        lv_img_set_src(data->thumb_img, NULL);
        return;
    }

    if (file_manager_get_photo_thumbnail_path(0, data->latest_thumb_path, sizeof(data->latest_thumb_path), FILE_PATH_LV) != 0) {
        if (file_manager_get_photo_path(0, data->latest_thumb_path, sizeof(data->latest_thumb_path), FILE_PATH_LV) != 0) {
            data->latest_thumb_path[0] = '\0';
            lv_img_set_src(data->thumb_img, NULL);
            return;
        }
    }

    lv_img_set_src(data->thumb_img, data->latest_thumb_path);
    lv_obj_center(data->thumb_img);
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

static void read_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    MLOG_INFO("Recognition preview read text clicked");
}

static void recognition_param_cb(param_id_t id, int value, void* user_data)
{
    page_ai_recognition_preview_data_t* data = (page_ai_recognition_preview_data_t*)user_data;
    LV_UNUSED(value);

    if (id == PARAM_ID_WIFI_CONNECTED || id == PARAM_ID_WIFI_SIGNAL_DBM) {
        update_wifi_icon(data);
    }
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

    data->wifi_icon = lv_img_create(data->container);
    lv_img_set_src(data->wifi_icon, "A:" RES_ICON_PATH "/wifi-off.png");
    lv_obj_add_flag(data->wifi_icon, LV_OBJ_FLAG_FLOATING);
    lv_obj_add_flag(data->wifi_icon, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(data->wifi_icon, PREVIEW_WIFI_ICON_SIZE, PREVIEW_WIFI_ICON_SIZE);
    lv_obj_align(data->wifi_icon, LV_ALIGN_TOP_RIGHT, -10, 2);

    data->title_label = lv_label_create(data->container);
    lv_label_set_text(data->title_label, "万物识别💥🛫⛔✔🤡");
    lv_obj_add_style(data->title_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->title_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_add_flag(data->title_label, LV_OBJ_FLAG_FLOATING);
    lv_obj_add_flag(data->title_label, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_align(data->title_label, LV_ALIGN_TOP_MID, 0, 14);

    /* 识别图缩略图：尺寸对齐相册九宫格单元 200x140。 */
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
    lv_label_set_text(data->text_label,
        "识别结果：这是一棵树，叶片较密，可能是樟树。\n\n"
        "特征说明：树冠较圆，叶缘平滑，叶片油亮，主干直立。\n\n"
        "可在左下方点击“朗读文本”按钮播放当前识别结果。");
    lv_obj_align(data->text_label, LV_ALIGN_TOP_LEFT, 0, 0);

    gesture_back_enable_event_bubble_recursive(data->container);

    page_set_private_data(data);
    param_manager_register_callback(recognition_param_cb, data);
    update_wifi_icon(data);
}

void page_ai_recognition_preview_destroy(void)
{
    page_ai_recognition_preview_data_t* data = (page_ai_recognition_preview_data_t*)page_get_private_data();

    if (data == NULL) {
        return;
    }

    param_manager_unregister_callback(recognition_param_cb);

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
    refresh_latest_thumbnail(data);
    update_wifi_icon(data);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_ai_recognition_preview_hide(void)
{
    page_ai_recognition_preview_data_t* data = (page_ai_recognition_preview_data_t*)page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_ai_recognition_preview_update(void)
{
    page_ai_recognition_preview_data_t* data = (page_ai_recognition_preview_data_t*)page_get_private_data();

    if (data == NULL) {
        return;
    }

    update_wifi_icon(data);
    refresh_latest_thumbnail(data);
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
