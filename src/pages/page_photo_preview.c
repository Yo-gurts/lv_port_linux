// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_photo_preview.h"
#include "config.h"
#include "core/file_manager.h"
#include "core/font_manager.h"
#include "core/key_manager.h"
#include "core/page_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include "pages/page_ai_style_preview.h"
#include "pages/page_album.h"
#include "ui/gesture_back.h"
#include "ui/top_notice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PREVIEW_BACK_BTN_SIZE 50
#define PREVIEW_DRAG_DIR_NONE 0
#define PREVIEW_DRAG_DIR_NEXT 1
#define PREVIEW_DRAG_DIR_PREV -1
#define PREVIEW_DRAG_DEADZONE_PX 8
#define PREVIEW_SWIPE_ANIM_TIME_MS 180

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static int g_initial_photo_index = -1;

static int clamp_int(int value, int min_value, int max_value);
static int get_album_total_count(void);
static int get_wrapped_index(const page_photo_preview_data_t* data, int index);
static void set_image_by_index(lv_obj_t* image, int photo_index);
static void render_current_photo(page_photo_preview_data_t* data);
static void reset_swipe_state(page_photo_preview_data_t* data);
static void update_slide_positions(page_photo_preview_data_t* data, int offset_x);
static void start_swipe_anim(page_photo_preview_data_t* data, int current_end_x, int target_end_x, int commit_switch);
static int get_current_photo_index(const page_photo_preview_data_t* data);
static void back_btn_cb(lv_event_t* e);
static void left_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data);
static void right_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data);
static void swipe_anim_ready_cb(lv_anim_t* a);
static void swipe_event_cb(lv_event_t* e);

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

static int clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value)
        return min_value;
    if (value > max_value)
        return max_value;
    return value;
}

static int get_album_total_count(void)
{
    if (file_manager_refresh_photo_list() != 0)
        MLOG_WARN("Album preview refresh photo list failed before counting");

    return file_manager_get_photo_count();
}

static int get_wrapped_index(const page_photo_preview_data_t* data, int index)
{
    if (!data || data->total_photos <= 0)
        return -1;

    if (index < 0)
        return data->total_photos - 1;
    if (index >= data->total_photos)
        return 0;
    return index;
}

static void set_image_by_index(lv_obj_t* image, int photo_index)
{
    char subpic_path[FILE_MANAGER_MAX_PATH_LEN];

    if (!image || photo_index < 0)
        return;

    if (file_manager_get_photo_subpic_path(photo_index, subpic_path, sizeof(subpic_path), FILE_PATH_LV) == 0)
        lv_img_set_src(image, subpic_path);
    else
        lv_img_set_src(image, NULL);
}

static void render_current_photo(page_photo_preview_data_t* data)
{
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char text[32];
    int photo_index;

    if (!data || !data->current_image || !data->file_name_label)
        return;

    if (data->total_photos <= 0) {
        lv_img_set_src(data->current_image, NULL);
        lv_img_set_src(data->target_image, NULL);
        lv_label_set_text(data->file_name_label, "");
        top_notice_show_for("0/0", TOP_NOTICE_TYPE_INFO, 3000);
        return;
    }

    data->current_display_index = clamp_int(data->current_display_index, 0, data->total_photos - 1);
    photo_index = data->current_display_index;

    set_image_by_index(data->current_image, photo_index);
    if (file_manager_get_photo_name(photo_index, file_name, sizeof(file_name)) == 0)
        lv_label_set_text(data->file_name_label, file_name);
    else
        lv_label_set_text(data->file_name_label, "");

    if (snprintf(text, sizeof(text), "%d/%d", data->current_display_index + 1, data->total_photos) < (int)sizeof(text))
        top_notice_show_for(text, TOP_NOTICE_TYPE_INFO, 3000);
}

static void reset_swipe_state(page_photo_preview_data_t* data)
{
    lv_coord_t width;

    if (!data)
        return;

    data->drag_active = 0;
    data->drag_ignore = 0;
    data->drag_start_x = 0;
    data->drag_last_x = 0;
    data->drag_offset_x = 0;
    data->drag_direction = PREVIEW_DRAG_DIR_NONE;
    data->drag_last_move_dir = PREVIEW_DRAG_DIR_NONE;
    data->drag_target_index = -1;
    data->swipe_anim_running = 0;
    data->swipe_commit_on_anim_end = 0;

    if (data->current_slide) {
        lv_anim_del(data->current_slide, (lv_anim_exec_xcb_t)lv_obj_set_x);
        lv_obj_set_x(data->current_slide, 0);
    }

    if (data->target_slide) {
        width = data->image_area ? lv_obj_get_width(data->image_area) : 0;
        if (width <= 0)
            width = H_RES;
        lv_anim_del(data->target_slide, (lv_anim_exec_xcb_t)lv_obj_set_x);
        lv_obj_set_x(data->target_slide, width);
        lv_obj_add_flag(data->target_slide, LV_OBJ_FLAG_HIDDEN);
    }
}

static void update_slide_positions(page_photo_preview_data_t* data, int offset_x)
{
    lv_coord_t width;
    lv_coord_t target_start_x;

    if (!data || !data->current_slide || !data->target_slide)
        return;

    width = lv_obj_get_width(data->image_area);
    lv_obj_set_x(data->current_slide, offset_x);

    if (data->drag_direction == PREVIEW_DRAG_DIR_NEXT)
        target_start_x = width;
    else
        target_start_x = -width;

    lv_obj_set_x(data->target_slide, target_start_x + offset_x);
}

static void swipe_anim_ready_cb(lv_anim_t* a)
{
    page_photo_preview_data_t* data;

    LV_UNUSED(a);
    data = (page_photo_preview_data_t*)page_get_private_data();
    if (!data || !data->swipe_anim_running)
        return;

    data->swipe_anim_running = 0;
    if (data->swipe_commit_on_anim_end && data->drag_target_index >= 0)
        data->current_display_index = data->drag_target_index;

    reset_swipe_state(data);
    render_current_photo(data);
}

static void start_swipe_anim(page_photo_preview_data_t* data, int current_end_x, int target_end_x, int commit_switch)
{
    lv_anim_t anim_current;
    lv_anim_t anim_target;

    if (!data || !data->current_slide || !data->target_slide)
        return;

    data->swipe_anim_running = 1;
    data->swipe_commit_on_anim_end = commit_switch ? 1 : 0;

    lv_anim_del(data->current_slide, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_del(data->target_slide, (lv_anim_exec_xcb_t)lv_obj_set_x);

    lv_anim_init(&anim_current);
    lv_anim_set_var(&anim_current, data->current_slide);
    lv_anim_set_values(&anim_current, lv_obj_get_x(data->current_slide), current_end_x);
    lv_anim_set_time(&anim_current, PREVIEW_SWIPE_ANIM_TIME_MS);
    lv_anim_set_exec_cb(&anim_current, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_start(&anim_current);

    lv_anim_init(&anim_target);
    lv_anim_set_var(&anim_target, data->target_slide);
    lv_anim_set_values(&anim_target, lv_obj_get_x(data->target_slide), target_end_x);
    lv_anim_set_time(&anim_target, PREVIEW_SWIPE_ANIM_TIME_MS);
    lv_anim_set_exec_cb(&anim_target, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_set_ready_cb(&anim_target, swipe_anim_ready_cb);
    lv_anim_start(&anim_target);
}

static int get_current_photo_index(const page_photo_preview_data_t* data)
{
    if (!data || data->total_photos <= 0)
        return -1;

    return clamp_int(data->current_display_index, 0, data->total_photos - 1);
}

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

static void back_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_photo_preview_data_t* data = (page_photo_preview_data_t*)page_get_private_data();
    int current_photo_index = get_current_photo_index(data);
    if (current_photo_index >= 0)
        page_album_set_focus_photo_index(current_photo_index);
    page_manager_back();
    reset_swipe_state(data);
}

static void menu_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)key;
    (void)event_type;
    (void)user_data;
    page_photo_preview_data_t* data = (page_photo_preview_data_t*)page_get_private_data();
    int current_photo_index = get_current_photo_index(data);
    if (current_photo_index >= 0)
        page_album_set_focus_photo_index(current_photo_index);
    page_manager_back();
    reset_swipe_state(data);
}

static void assistant_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_photo_preview_data_t* data = (page_photo_preview_data_t*)user_data;

    if (key != KEY_ID_ASSISTANT || event_type != KEY_EVENT_CLICK) {
        return;
    }

    page_ai_style_preview_set_photo_index(get_current_photo_index(data));
    page_manager_navigate("ai_style_preview");
}

static void switch_photo_by_key(page_photo_preview_data_t* data, int delta)
{
    int target_index;

    if (!data || !data->container || data->total_photos <= 1 || data->swipe_anim_running)
        return;

    target_index = get_wrapped_index(data, data->current_display_index + delta);
    if (target_index < 0)
        return;

    reset_swipe_state(data);
    data->current_display_index = target_index;
    render_current_photo(data);
}

static void left_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_photo_preview_data_t* data = (page_photo_preview_data_t*)user_data;

    if (key != KEY_ID_LEFT || event_type != KEY_EVENT_CLICK)
        return;

    switch_photo_by_key(data, -1);
}

static void right_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_photo_preview_data_t* data = (page_photo_preview_data_t*)user_data;

    if (key != KEY_ID_RIGHT || event_type != KEY_EVENT_CLICK)
        return;

    switch_photo_by_key(data, 1);
}

static void swipe_event_cb(lv_event_t* e)
{
    page_photo_preview_data_t* data = (page_photo_preview_data_t*)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t* indev;
    lv_point_t point;
    lv_coord_t width;
    int offset_x;
    int move_step_x;
    int final_direction;
    int same_direction;
    int current_end_x;
    int target_end_x;

    if (!data || !data->container || !data->image_area)
        return;
    if (data->total_photos <= 1)
        return;

    indev = lv_indev_get_act();

    if (code == LV_EVENT_PRESSED) {
        if (!indev || data->swipe_anim_running)
            return;

        lv_indev_get_point(indev, &point);
        width = lv_obj_get_width(data->container);
        data->drag_active = 1;
        data->drag_ignore = (point.x <= SWIPE_BACK_EDGE_THRESHOLD_PX
                             || point.x >= (width - SWIPE_BACK_EDGE_THRESHOLD_PX));
        data->drag_start_x = point.x;
        data->drag_last_x = point.x;
        data->drag_offset_x = 0;
        data->drag_direction = PREVIEW_DRAG_DIR_NONE;
        data->drag_last_move_dir = PREVIEW_DRAG_DIR_NONE;
        data->drag_target_index = -1;
        return;
    }

    if (code == LV_EVENT_PRESSING) {
        if (!indev || !data->drag_active || data->swipe_anim_running)
            return;

        lv_indev_get_point(indev, &point);
        move_step_x = point.x - data->drag_last_x;
        if (move_step_x < 0)
            data->drag_last_move_dir = PREVIEW_DRAG_DIR_NEXT;
        else if (move_step_x > 0)
            data->drag_last_move_dir = PREVIEW_DRAG_DIR_PREV;
        data->drag_last_x = point.x;

        width = lv_obj_get_width(data->image_area);
        offset_x = point.x - data->drag_start_x;

        if (data->drag_direction == PREVIEW_DRAG_DIR_NONE) {
            if (abs(offset_x) < PREVIEW_DRAG_DEADZONE_PX)
                return;

            if (offset_x < 0)
                data->drag_direction = PREVIEW_DRAG_DIR_NEXT;
            else
                data->drag_direction = PREVIEW_DRAG_DIR_PREV;

            data->drag_target_index = get_wrapped_index(data,
                                                        data->current_display_index
                                                        + (data->drag_direction == PREVIEW_DRAG_DIR_NEXT ? 1 : -1));
            if (data->drag_target_index < 0) {
                data->drag_direction = PREVIEW_DRAG_DIR_NONE;
                return;
            }

            if (data->drag_ignore) {
                return;
            }

            set_image_by_index(data->target_image, data->drag_target_index);
            lv_obj_clear_flag(data->target_slide, LV_OBJ_FLAG_HIDDEN);
        }

        if (data->drag_ignore) {
            return;
        }

        if (data->drag_direction == PREVIEW_DRAG_DIR_NEXT && offset_x > 0)
            offset_x = 0;
        else if (data->drag_direction == PREVIEW_DRAG_DIR_PREV && offset_x < 0)
            offset_x = 0;

        if (offset_x > width)
            offset_x = width;
        else if (offset_x < -width)
            offset_x = -width;

        data->drag_offset_x = offset_x;
        update_slide_positions(data, offset_x);
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (!data->drag_active) {
            return;
        }

        data->drag_active = 0;
        if (data->drag_direction == PREVIEW_DRAG_DIR_NONE) {
            reset_swipe_state(data);
            return;
        }

        final_direction = data->drag_last_move_dir;
        if (final_direction == PREVIEW_DRAG_DIR_NONE) {
            final_direction = (data->drag_offset_x < 0) ? PREVIEW_DRAG_DIR_NEXT : PREVIEW_DRAG_DIR_PREV;
        }
        same_direction = (final_direction == data->drag_direction);

        if (!same_direction) {
            reset_swipe_state(data);
            return;
        }

        /* 边缘起点手势仅用于返回，由 gesture_back 统一处理。 */
        if (data->drag_ignore) {
            reset_swipe_state(data);
            return;
        }

        width = lv_obj_get_width(data->image_area);
        current_end_x = (data->drag_direction == PREVIEW_DRAG_DIR_NEXT) ? -width : width;
        target_end_x = 0;

        start_swipe_anim(data, current_end_x, target_end_x, 1);
    }
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_photo_preview_set_initial_photo_index(int photo_index)
{
    g_initial_photo_index = photo_index;
}

void page_photo_preview_create(void)
{
    page_photo_preview_data_t* data = (page_photo_preview_data_t*)malloc(sizeof(page_photo_preview_data_t));
    lv_obj_t* back_icon;
    if (!data)
        return;

    memset(data, 0, sizeof(page_photo_preview_data_t));
    data->current_display_index = 0;

    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(data->container, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_set_layout(data->container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(data->container, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    data->image_area = lv_obj_create(data->container);
    lv_obj_set_size(data->image_area, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->image_area, &style_noboarder, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->image_area, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(data->image_area, 0, LV_PART_MAIN);
    lv_obj_clear_flag(data->image_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(data->image_area, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(data->image_area, 0, LV_PART_MAIN);

    data->current_slide = lv_obj_create(data->image_area);
    lv_obj_set_size(data->current_slide, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->current_slide, &style_noboarder, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->current_slide, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(data->current_slide, 0, LV_PART_MAIN);
    lv_obj_clear_flag(data->current_slide, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(data->current_slide, 0, LV_PART_MAIN);

    data->current_image = lv_img_create(data->current_slide);
    lv_obj_center(data->current_image);

    data->target_slide = lv_obj_create(data->image_area);
    lv_obj_set_size(data->target_slide, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->target_slide, &style_noboarder, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->target_slide, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(data->target_slide, 0, LV_PART_MAIN);
    lv_obj_clear_flag(data->target_slide, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(data->target_slide, 0, LV_PART_MAIN);

    data->target_image = lv_img_create(data->target_slide);
    lv_obj_center(data->target_image);
    lv_obj_add_flag(data->target_slide, LV_OBJ_FLAG_HIDDEN);
    {
        lv_coord_t width = lv_obj_get_width(data->image_area);
        lv_obj_set_x(data->target_slide, (width > 0) ? width : H_RES);
    }

    data->back_btn = lv_btn_create(data->container);
    lv_obj_set_size(data->back_btn, PREVIEW_BACK_BTN_SIZE, PREVIEW_BACK_BTN_SIZE);
    lv_obj_add_flag(data->back_btn, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(data->back_btn, LV_OBJ_FLAG_FLOATING);
    lv_obj_add_style(data->back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_align(data->back_btn, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_event_cb(data->back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    back_icon = lv_img_create(data->back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle-white.png");
    lv_obj_center(back_icon);

    data->file_name_label = lv_label_create(data->container);
    lv_obj_add_flag(data->file_name_label, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(data->file_name_label, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_width(data->file_name_label, H_RES - PREVIEW_BACK_BTN_SIZE - 20);
    lv_obj_set_style_text_align(data->file_name_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->file_name_label, lv_color_white(), LV_PART_MAIN);
    lv_label_set_long_mode(data->file_name_label, LV_LABEL_LONG_DOT);
    lv_obj_align(data->file_name_label, LV_ALIGN_TOP_MID, 0, 16);
    lv_label_set_text(data->file_name_label, "");

    lv_obj_add_event_cb(data->container, swipe_event_cb, LV_EVENT_PRESSED, data);
    lv_obj_add_event_cb(data->container, swipe_event_cb, LV_EVENT_PRESSING, data);
    lv_obj_add_event_cb(data->container, swipe_event_cb, LV_EVENT_RELEASED, data);
    lv_obj_add_event_cb(data->container, swipe_event_cb, LV_EVENT_PRESS_LOST, data);
    gesture_back_register_events(data->container);
    gesture_back_enable_event_bubble_recursive(data->container);
    gesture_back_set_left_edge_swipe_cb(data->container, back_btn_cb);

    reset_swipe_state(data);
    page_set_private_data(data);
}

void page_photo_preview_destroy(void)
{
    page_photo_preview_data_t* data = (page_photo_preview_data_t*)page_get_private_data();
    if (!data)
        return;

    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_cb, NULL);
    key_manager_unregister_callback(KEY_ID_ASSISTANT, KEY_EVENT_CLICK, assistant_key_click_cb, data);
    key_manager_unregister_callback(KEY_ID_LEFT, KEY_EVENT_CLICK, left_key_click_cb, data);
    key_manager_unregister_callback(KEY_ID_RIGHT, KEY_EVENT_CLICK, right_key_click_cb, data);
    free(data);
}

void page_photo_preview_show(void)
{
    page_photo_preview_data_t* data = (page_photo_preview_data_t*)page_get_private_data();
    if (!data || !data->container)
        return;

    /* gesture_back 为全局单实例，页面显示时重新声明当前活跃容器。 */
    gesture_back_set_left_edge_swipe_cb(data->container, back_btn_cb);
    key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_cb, NULL);
    key_manager_register_callback(KEY_ID_ASSISTANT, KEY_EVENT_CLICK, assistant_key_click_cb, data);
    key_manager_register_callback(KEY_ID_LEFT, KEY_EVENT_CLICK, left_key_click_cb, data);
    key_manager_register_callback(KEY_ID_RIGHT, KEY_EVENT_CLICK, right_key_click_cb, data);

    reset_swipe_state(data);
    data->total_photos = get_album_total_count();
    if (data->total_photos <= 0) {
        data->current_display_index = 0;
        g_initial_photo_index = -1;
        render_current_photo(data);
        lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (g_initial_photo_index >= 0) {
        int clamped_photo_index = clamp_int(g_initial_photo_index, 0, data->total_photos - 1);
        data->current_display_index = clamped_photo_index;
        g_initial_photo_index = -1;
    } else {
        data->current_display_index = clamp_int(data->current_display_index, 0, data->total_photos - 1);
    }

    render_current_photo(data);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_photo_preview_hide(void)
{
    page_photo_preview_data_t* data = (page_photo_preview_data_t*)page_get_private_data();
    if (!data || !data->container)
        return;

    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_cb, NULL);
    key_manager_unregister_callback(KEY_ID_ASSISTANT, KEY_EVENT_CLICK, assistant_key_click_cb, data);
    key_manager_unregister_callback(KEY_ID_LEFT, KEY_EVENT_CLICK, left_key_click_cb, data);
    key_manager_unregister_callback(KEY_ID_RIGHT, KEY_EVENT_CLICK, right_key_click_cb, data);

    /* 页面隐藏时主动解绑，避免隐藏页继续占用 gesture_back 活跃容器。 */
    gesture_back_clear_active_swipe_cb(data->container);

    reset_swipe_state(data);
    top_notice_hide();
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_photo_preview_update(void)
{
    /* no-op */
}

// #endregion
