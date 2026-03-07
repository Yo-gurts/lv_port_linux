// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_album.h"
#include "pages/page_photo_preview.h"
#include "config.h"
#include "core/file_manager.h"
#include "core/font_manager.h"
#include "core/key_manager.h"
#include "core/page_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include "ui/gesture_back.h"
#include "ui/top_notice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 布局常量 */
#define NAV_BAR_HEIGHT 50
#define GRID_ITEM_WIDTH 200
#define GRID_ITEM_HEIGHT 140
#define GRID_GAP_X 4
#define GRID_GAP_Y 4
#define GRID_BUFFER_ROWS 2
#define FAST_SCROLLBAR_WIDTH 24
#define FAST_SCROLLBAR_KNOB_HEIGHT 56
#define FAST_SCROLLBAR_KNOB_RADIUS 12
#define FAST_SCROLLBAR_KNOB_PAD_Y ((FAST_SCROLLBAR_KNOB_HEIGHT - FAST_SCROLLBAR_WIDTH) / 2)
#define FAST_SCROLLBAR_TOUCH_PAD 10
#define SELECT_BOX_SIZE 22
#define SELECT_BOX_OFFSET_X -6
#define SELECT_BOX_OFFSET_Y -6

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_album.h)
// #############################################################################

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static void refresh_visible_items(page_album_data_t* data, bool force_refresh);
static void update_scroll_content_height(page_album_data_t* data);
static int get_max_scroll_y(page_album_data_t* data);
static void sync_fast_scrollbar_from_scroll(page_album_data_t* data);
static void set_fast_scrollbar_visible(page_album_data_t* data, bool visible);
static bool ensure_selected_buffer(page_album_data_t* data, int required_count);
static void clear_selection_state(page_album_data_t* data);
static void set_selection_mode(page_album_data_t* data, bool enable);
static void update_item_selection_visual(page_album_data_t* data, album_item_t* item);
static void refresh_selection_overlays(page_album_data_t* data);
static void update_nav_bar_state(page_album_data_t* data);
static void enter_selection_mode(page_album_data_t* data);
static void exit_selection_mode(page_album_data_t* data);
static void apply_nav_text_btn_style(lv_obj_t* btn);
static album_item_t* find_item_by_container(page_album_data_t* data, lv_obj_t* container);
static void toggle_item_selected(page_album_data_t* data, album_item_t* item);
static int delete_selected_photos(page_album_data_t* data);
static void album_item_event_cb(lv_event_t* e);
static void select_btn_cb(lv_event_t* e);
static void cancel_btn_cb(lv_event_t* e);
static void select_all_btn_cb(lv_event_t* e);
static void show_fast_scrollbar_progress_notice(page_album_data_t* data, bool force);
static void sync_fast_scrollbar_deferred_cb(void* user_data);
static void show_photo_preview(page_album_data_t* data, int photo_index);
static int get_scroll_y_for_photo_index(page_album_data_t* data, int photo_index);

static int g_album_focus_photo_index = -1;

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
        MLOG_WARN("Album refresh photo list failed before counting");

    return file_manager_get_photo_count();
}

static bool ensure_selected_buffer(page_album_data_t* data, int required_count)
{
    bool* new_flags;
    int new_capacity;

    if (!data)
        return false;

    if (required_count <= 0)
        required_count = 1;

    if (data->selected_capacity >= required_count)
        return true;

    new_capacity = data->selected_capacity > 0 ? data->selected_capacity : 16;
    while (new_capacity < required_count)
        new_capacity *= 2;

    new_flags = (bool*)realloc(data->selected_flags, (size_t)new_capacity * sizeof(bool));
    if (!new_flags)
        return false;

    memset(new_flags + data->selected_capacity, 0, (size_t)(new_capacity - data->selected_capacity) * sizeof(bool));
    data->selected_flags = new_flags;
    data->selected_capacity = new_capacity;
    return true;
}

static void clear_selection_state(page_album_data_t* data)
{
    if (!data)
        return;

    if (data->selected_flags && data->selected_capacity > 0)
        memset(data->selected_flags, 0, (size_t)data->selected_capacity * sizeof(bool));
    data->selected_count = 0;
}

static void set_selection_mode(page_album_data_t* data, bool enable)
{
    if (!data)
        return;

    data->selection_mode = enable;
    if (!enable)
        clear_selection_state(data);
}

static void update_nav_bar_state(page_album_data_t* data)
{
    if (!data)
        return;

    if (data->selection_mode) {
        lv_obj_add_flag(data->back_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(data->photo_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(data->video_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(data->select_btn, LV_OBJ_FLAG_HIDDEN);

        lv_obj_clear_flag(data->cancel_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(data->select_all_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(data->selected_count_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(data->delete_btn, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_fmt(data->selected_count_label, "已选择 %d 项", data->selected_count);
    } else {
        lv_obj_clear_flag(data->back_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(data->photo_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(data->video_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(data->select_btn, LV_OBJ_FLAG_HIDDEN);

        lv_obj_add_flag(data->cancel_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(data->select_all_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(data->selected_count_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(data->delete_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

static void enter_selection_mode(page_album_data_t* data)
{
    if (!data || data->selection_mode)
        return;
    if (!ensure_selected_buffer(data, data->total_photos))
        return;

    set_selection_mode(data, true);
    data->suppress_next_item_click = false;
    update_nav_bar_state(data);
    refresh_selection_overlays(data);
}

static void exit_selection_mode(page_album_data_t* data)
{
    if (!data)
        return;

    set_selection_mode(data, false);
    data->suppress_next_item_click = false;
    update_nav_bar_state(data);
    refresh_selection_overlays(data);
}

static void apply_nav_text_btn_style(lv_obj_t* btn)
{
    if (!btn)
        return;

    lv_obj_set_style_bg_opa(btn, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x2A2A2A), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x5A5A5A), LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 18, LV_PART_MAIN);
    lv_obj_set_style_pad_left(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_right(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_top(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
}

static void update_item_selection_visual(page_album_data_t* data, album_item_t* item)
{
    bool selected = false;

    if (!data || !item || !item->select_box)
        return;

    if (!data->selection_mode || item->photo_index < 0 || item->photo_index >= data->selected_capacity) {
        lv_obj_add_flag(item->select_box, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_clear_flag(item->select_box, LV_OBJ_FLAG_HIDDEN);
    selected = data->selected_flags[item->photo_index];
    lv_obj_set_style_bg_opa(item->select_box, selected ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_color(item->select_box, lv_color_hex(0x28A745), LV_PART_MAIN);
    lv_obj_set_style_border_color(item->select_box, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(item->select_box, 2, LV_PART_MAIN);
}

static void refresh_selection_overlays(page_album_data_t* data)
{
    int i;

    if (!data || !data->item_pool)
        return;

    for (i = 0; i < data->pool_size; i++)
        update_item_selection_visual(data, &data->item_pool[i]);
}

static album_item_t* find_item_by_container(page_album_data_t* data, lv_obj_t* container)
{
    int i;

    if (!data || !container || !data->item_pool)
        return NULL;

    for (i = 0; i < data->pool_size; i++) {
        if (data->item_pool[i].container == container)
            return &data->item_pool[i];
    }

    return NULL;
}

static void toggle_item_selected(page_album_data_t* data, album_item_t* item)
{
    bool new_value;

    if (!data || !item)
        return;
    if (!data->selection_mode)
        return;
    if (item->photo_index < 0 || item->photo_index >= data->selected_capacity)
        return;

    new_value = !data->selected_flags[item->photo_index];
    data->selected_flags[item->photo_index] = new_value;
    if (new_value)
        data->selected_count++;
    else if (data->selected_count > 0)
        data->selected_count--;

    update_item_selection_visual(data, item);
    if (data->selection_mode)
        lv_label_set_text_fmt(data->selected_count_label, "已选择 %d 项", data->selected_count);
}

static int delete_selected_photos(page_album_data_t* data)
{
    int photo_index;
    int deleted = 0;
    int prev_scroll_y;
    int target_scroll_y;

    if (!data || !data->selected_flags || data->selected_count <= 0)
        return -1;

    prev_scroll_y = clamp_int(lv_obj_get_scroll_y(data->grid_container), 0, get_max_scroll_y(data));

    data->deleting_in_progress = true;
    data->prev_input_block_mask = key_manager_get_block_non_power();
    key_manager_set_block_non_power((uint8_t)(data->prev_input_block_mask | KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2));
    if (data->op_block_mask) {
        lv_obj_clear_flag(data->op_block_mask, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(data->op_block_mask);
    }
    top_notice_show_for("正在删除，请稍候...", TOP_NOTICE_TYPE_BLOCKING, 60000);

    for (photo_index = data->total_photos - 1; photo_index >= 0; photo_index--) {
        if (photo_index >= data->selected_capacity)
            continue;
        if (!data->selected_flags[photo_index])
            continue;
        if (file_manager_delete_photo_by_index(photo_index) == 0)
            deleted++;
    }

    key_manager_set_block_non_power(data->prev_input_block_mask);
    if (data->op_block_mask)
        lv_obj_add_flag(data->op_block_mask, LV_OBJ_FLAG_HIDDEN);
    data->deleting_in_progress = false;

    if (deleted <= 0) {
        top_notice_show("删除失败", TOP_NOTICE_TYPE_ERROR);
        return -1;
    }

    data->total_photos = get_album_total_count();
    if (!ensure_selected_buffer(data, data->total_photos))
        MLOG_WARN("Album selected buffer ensure failed after delete");
    update_scroll_content_height(data);
    target_scroll_y = clamp_int(prev_scroll_y, 0, get_max_scroll_y(data));
    data->first_visible_row = -1;
    lv_obj_scroll_to_y(data->grid_container, target_scroll_y, LV_ANIM_OFF);
    exit_selection_mode(data);
    refresh_visible_items(data, true);
    sync_fast_scrollbar_from_scroll(data);
    set_fast_scrollbar_visible(data, true);
    top_notice_show("删除成功", TOP_NOTICE_TYPE_SUCCESS);
    return 0;
}

static void album_item_event_cb(lv_event_t* e)
{
    page_album_data_t* data = (page_album_data_t*)lv_event_get_user_data(e);
    lv_obj_t* target = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    album_item_t* item;

    if (!data || !target)
        return;
    if (data->deleting_in_progress)
        return;

    item = find_item_by_container(data, target);
    if (!item || item->photo_index < 0)
        return;

    if (code == LV_EVENT_LONG_PRESSED) {
        if (!data->selection_mode)
            enter_selection_mode(data);
        toggle_item_selected(data, item);
        data->suppress_next_item_click = true;
        return;
    }

    if (code == LV_EVENT_CLICKED) {
        if (data->suppress_next_item_click) {
            data->suppress_next_item_click = false;
            return;
        }
        if (!data->selection_mode) {
            show_photo_preview(data, item->photo_index);
            return;
        }
        toggle_item_selected(data, item);
    }
}

static void select_btn_cb(lv_event_t* e)
{
    page_album_data_t* data = (page_album_data_t*)lv_event_get_user_data(e);
    if (!data || data->deleting_in_progress)
        return;

    enter_selection_mode(data);
}

static void cancel_btn_cb(lv_event_t* e)
{
    page_album_data_t* data = (page_album_data_t*)lv_event_get_user_data(e);
    if (!data || data->deleting_in_progress)
        return;

    exit_selection_mode(data);
}

static void select_all_btn_cb(lv_event_t* e)
{
    page_album_data_t* data = (page_album_data_t*)lv_event_get_user_data(e);
    int i;

    if (!data || data->deleting_in_progress)
        return;
    if (!data->selection_mode)
        return;

    if (!ensure_selected_buffer(data, data->total_photos))
        return;

    for (i = 0; i < data->selected_capacity; i++)
        data->selected_flags[i] = false;
    for (i = 0; i < data->total_photos; i++)
        data->selected_flags[i] = true;
    data->selected_count = data->total_photos;
    lv_label_set_text_fmt(data->selected_count_label, "已选择 %d 项", data->selected_count);
    refresh_selection_overlays(data);
}

static void show_photo_preview(page_album_data_t* data, int photo_index)
{
    if (!data)
        return;
    if (photo_index < 0 || photo_index >= data->total_photos)
        return;

    page_photo_preview_set_initial_photo_index(photo_index);
    page_manager_navigate("photo_preview");
}

static int get_total_rows(const page_album_data_t* data)
{
    int total_rows = (data->total_photos + data->layout.cols - 1) / data->layout.cols;
    if (total_rows < 1)
        total_rows = 1;
    return total_rows;
}

static int get_scroll_y_for_photo_index(page_album_data_t* data, int photo_index)
{
    int display_index;
    int target_row;
    int viewport_height;
    int target_y;
    int max_scroll_y;

    if (!data || data->total_photos <= 0)
        return 0;
    if (photo_index < 0 || photo_index >= data->total_photos)
        return 0;

    display_index = photo_index;
    target_row = display_index / data->layout.cols;
    viewport_height = lv_obj_get_content_height(data->grid_container);
    if (viewport_height <= 0)
        viewport_height = data->layout.row_height;

    /* 回到九宫格时尽量让目标项居中可见 */
    target_y = target_row * data->layout.row_height - (viewport_height - data->layout.row_height) / 2;
    max_scroll_y = get_max_scroll_y(data);
    return clamp_int(target_y, 0, max_scroll_y);
}

static void calculate_layout(page_album_data_t* data)
{
    int available_width = lv_obj_get_content_width(data->grid_container);
    int available_height = lv_obj_get_content_height(data->grid_container);
    int used_width;
    int cols;

    if (available_width <= 0)
        available_width = H_RES;
    if (available_height <= 0)
        available_height = V_RES - NAV_BAR_HEIGHT;

    data->layout.item_width = GRID_ITEM_WIDTH;
    data->layout.item_height = GRID_ITEM_HEIGHT;
    data->layout.gap_x = GRID_GAP_X;
    data->layout.gap_y = GRID_GAP_Y;
    data->layout.row_height = data->layout.item_height + data->layout.gap_y;

    cols = (available_width + data->layout.gap_x) / (data->layout.item_width + data->layout.gap_x);
    if (cols < 1)
        cols = 1;
    data->layout.cols = cols;

    used_width = data->layout.cols * data->layout.item_width + (data->layout.cols - 1) * data->layout.gap_x;
    data->layout.start_x = (available_width - used_width) / 2;
    if (data->layout.start_x < 0)
        data->layout.start_x = 0;

    data->layout.visible_rows = available_height / data->layout.row_height + 1;
    if (data->layout.visible_rows < 1)
        data->layout.visible_rows = 1;

    data->layout.pool_rows = data->layout.visible_rows + GRID_BUFFER_ROWS;
}

static void update_scroll_content_height(page_album_data_t* data)
{
    int total_rows;
    int total_height;

    total_rows = get_total_rows(data);

    total_height = total_rows * data->layout.row_height - data->layout.gap_y;
    if (total_height < 1)
        total_height = 1;

    lv_obj_set_width(data->scroll_content, LV_PCT(100));
    lv_obj_set_height(data->scroll_content, total_height);
}

static int get_max_scroll_y(page_album_data_t* data)
{
    int viewport_height;
    int content_height;
    int max_scroll_y;

    viewport_height = lv_obj_get_content_height(data->grid_container);
    content_height = lv_obj_get_height(data->scroll_content);
    max_scroll_y = content_height - viewport_height;

    if (max_scroll_y < 0)
        max_scroll_y = 0;

    return max_scroll_y;
}

static void set_fast_scrollbar_visible(page_album_data_t* data, bool visible)
{
    if (!data || !data->fast_scrollbar)
        return;

    if (visible)
        lv_obj_clear_flag(data->fast_scrollbar, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(data->fast_scrollbar, LV_OBJ_FLAG_HIDDEN);
}

static int get_last_visible_photo_index_1based(page_album_data_t* data)
{
    int scroll_y;
    int viewport_height;
    int max_scroll_y;
    int bottom_y;
    int last_visible_row;
    int last_visible_index_1based;

    if (!data || data->total_photos <= 0)
        return 0;

    max_scroll_y = get_max_scroll_y(data);
    scroll_y = clamp_int(lv_obj_get_scroll_y(data->grid_container), 0, max_scroll_y);
    viewport_height = lv_obj_get_content_height(data->grid_container);
    if (viewport_height <= 0)
        viewport_height = data->layout.row_height;

    bottom_y = scroll_y + viewport_height - 1;
    if (bottom_y < 0)
        bottom_y = 0;

    last_visible_row = bottom_y / data->layout.row_height;
    last_visible_index_1based = (last_visible_row + 1) * data->layout.cols;
    if (last_visible_index_1based > data->total_photos)
        last_visible_index_1based = data->total_photos;

    return last_visible_index_1based;
}

static void show_fast_scrollbar_progress_notice(page_album_data_t* data, bool force)
{
    char text[32];
    int last_visible_index;

    if (!data)
        return;

    last_visible_index = get_last_visible_photo_index_1based(data);
    if (!force && last_visible_index == data->last_notice_index)
        return;
    if (snprintf(text, sizeof(text), "%d/%d", last_visible_index, data->total_photos) >= (int)sizeof(text))
        return;

    top_notice_show_for(text, TOP_NOTICE_TYPE_INFO, 600);
    data->last_notice_index = last_visible_index;
}

static void sync_fast_scrollbar_from_scroll(page_album_data_t* data)
{
    int max_scroll_y;
    int scroll_y;
    int slider_value;
    bool need_update_range;
    bool need_update_value;

    if (!data || !data->fast_scrollbar)
        return;

    max_scroll_y = get_max_scroll_y(data);
    scroll_y = lv_obj_get_scroll_y(data->grid_container);
    scroll_y = clamp_int(scroll_y, 0, max_scroll_y);
    slider_value = max_scroll_y - scroll_y;

    need_update_range = (data->fast_scrollbar_range_max != max_scroll_y);
    need_update_value = (data->fast_scrollbar_last_value != slider_value);
    if (!need_update_range && !need_update_value)
        return;

    data->syncing_fast_scrollbar = true;
    if (need_update_range) {
        lv_slider_set_range(data->fast_scrollbar, 0, max_scroll_y);
        data->fast_scrollbar_range_max = max_scroll_y;
    }
    if (need_update_value || need_update_range) {
        lv_slider_set_value(data->fast_scrollbar, slider_value, LV_ANIM_OFF);
        data->fast_scrollbar_last_value = slider_value;
    }
    data->syncing_fast_scrollbar = false;
}

static void update_item_content(page_album_data_t* data, album_item_t* item, int display_index)
{
    char thumb_path[FILE_MANAGER_MAX_PATH_LEN];
    item->photo_index = display_index;

    if (file_manager_get_photo_thumbnail_path(item->photo_index, thumb_path, sizeof(thumb_path), FILE_PATH_LV) == 0) {
        lv_img_set_src(item->img, thumb_path);
        lv_obj_set_style_bg_opa(item->img, LV_OPA_TRANSP, LV_PART_MAIN);
    } else {
        lv_img_set_src(item->img, NULL);
        lv_obj_set_style_bg_opa(item->img, LV_OPA_40, LV_PART_MAIN);
    }

    update_item_selection_visual(data, item);
}

static void sync_fast_scrollbar_deferred_cb(void* user_data)
{
    page_album_data_t* data = (page_album_data_t*)user_data;
    if (!data || !data->container || !data->grid_container || !data->fast_scrollbar)
        return;

    lv_obj_update_layout(data->container);
    refresh_visible_items(data, true);
    sync_fast_scrollbar_from_scroll(data);
}

static void update_item_position(const page_album_data_t* data, album_item_t* item, int index)
{
    int row = index / data->layout.cols;
    int col = index % data->layout.cols;
    int x = data->layout.start_x + col * (data->layout.item_width + data->layout.gap_x);
    int y = row * data->layout.row_height;

    lv_obj_set_pos(item->container, x, y);
}

static int create_item_pool(page_album_data_t* data)
{
    int i;

    data->pool_size = data->layout.cols * data->layout.pool_rows;
    if (data->pool_size <= 0)
        return -1;

    data->item_pool = (album_item_t*)calloc((size_t)data->pool_size, sizeof(album_item_t));
    if (!data->item_pool)
        return -1;

    for (i = 0; i < data->pool_size; i++) {
        album_item_t* item = &data->item_pool[i];

        item->index = -1;
        item->photo_index = -1;
        item->is_visible = false;

        item->container = lv_obj_create(data->scroll_content);
        lv_obj_set_size(item->container, data->layout.item_width, data->layout.item_height);
        lv_obj_add_style(item->container, &style_settings_item, LV_PART_MAIN);
        lv_obj_set_style_pad_all(item->container, 0, LV_PART_MAIN);
        lv_obj_clear_flag(item->container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(item->container, LV_SCROLLBAR_MODE_OFF);
        lv_obj_add_flag(item->container, LV_OBJ_FLAG_HIDDEN);

        item->img = lv_img_create(item->container);
        lv_obj_set_size(item->img, data->layout.item_width, data->layout.item_height);
        lv_obj_set_style_bg_opa(item->img, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_center(item->img);

        item->select_box = lv_obj_create(item->container);
        lv_obj_remove_style_all(item->select_box);
        lv_obj_set_size(item->select_box, SELECT_BOX_SIZE, SELECT_BOX_SIZE);
        lv_obj_align(item->select_box, LV_ALIGN_BOTTOM_RIGHT, SELECT_BOX_OFFSET_X, SELECT_BOX_OFFSET_Y);
        lv_obj_set_style_radius(item->select_box, 5, LV_PART_MAIN);
        lv_obj_clear_flag(item->select_box, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(item->select_box, LV_OBJ_FLAG_HIDDEN);

        lv_obj_add_event_cb(item->container, album_item_event_cb, LV_EVENT_CLICKED, data);
        lv_obj_add_event_cb(item->container, album_item_event_cb, LV_EVENT_LONG_PRESSED, data);

    }

    return 0;
}

static void destroy_item_pool(page_album_data_t* data)
{
    if (!data->item_pool)
        return;

    free(data->item_pool);
    data->item_pool = NULL;
    data->pool_size = 0;
}

static void refresh_visible_items(page_album_data_t* data, bool force_refresh)
{
    int scroll_y;
    int first_row;
    int first_index;
    int total_rows;
    int max_first_row;
    int i;

    if (!data || !data->item_pool || data->pool_size <= 0)
        return;

    scroll_y = lv_obj_get_scroll_y(data->grid_container);
    scroll_y = clamp_int(scroll_y, 0, get_max_scroll_y(data));

    first_row = scroll_y / data->layout.row_height;

    total_rows = get_total_rows(data);

    max_first_row = total_rows - data->layout.pool_rows;
    if (max_first_row < 0)
        max_first_row = 0;

    first_row = clamp_int(first_row, 0, max_first_row);
    if (!force_refresh && first_row == data->first_visible_row)
        return;

    first_index = first_row * data->layout.cols;
    data->first_visible_row = first_row;

    for (i = 0; i < data->pool_size; i++) {
        album_item_t* item = &data->item_pool[i];
        int target_index = first_index + i;

        if (target_index < data->total_photos) {
            update_item_position(data, item, target_index);

            if (force_refresh || item->index != target_index) {
                item->index = target_index;
                update_item_content(data, item, target_index);
            }

            if (!item->is_visible) {
                lv_obj_clear_flag(item->container, LV_OBJ_FLAG_HIDDEN);
                item->is_visible = true;
            }
        } else {
            item->index = -1;
            item->photo_index = -1;
            update_item_selection_visual(data, item);
            if (item->is_visible) {
                lv_obj_add_flag(item->container, LV_OBJ_FLAG_HIDDEN);
                item->is_visible = false;
            }
        }
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

/* 返回按钮回调 */
static void back_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_album_data_t* data = page_get_private_data();
    if (data && data->deleting_in_progress)
        return;

    MLOG_INFO("Back button clicked");
    page_manager_back();
}

/* 拍照按钮回调 */
static void photo_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_album_data_t* data = page_get_private_data();
    if (data && data->deleting_in_progress)
        return;

    MLOG_INFO("Photo button clicked");
}

/* 录像按钮回调 */
static void video_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_album_data_t* data = page_get_private_data();
    if (data && data->deleting_in_progress)
        return;

    MLOG_INFO("Video button clicked");
}

/* 删除按钮回调 */
static void delete_btn_cb(lv_event_t* e)
{
    page_album_data_t* data = (page_album_data_t*)lv_event_get_user_data(e);
    if (!data)
        return;
    if (data->deleting_in_progress)
        return;
    if (!data->selection_mode)
        return;

    MLOG_INFO("Delete button clicked");
    if (data->selected_count <= 0) {
        top_notice_show("请先选择要删除的照片", TOP_NOTICE_TYPE_INFO);
        return;
    }

    if (delete_selected_photos(data) != 0)
        MLOG_WARN("Delete selected photos failed");
}

/* 滚动事件回调：仅在跨行时刷新，避免每像素滚动都触发重排 */
static void scroll_event_cb(lv_event_t* e)
{
    page_album_data_t* data = (page_album_data_t*)lv_event_get_user_data(e);
    lv_event_code_t code;

    if (!data)
        return;

    code = lv_event_get_code(e);
    if (code == LV_EVENT_SCROLL_BEGIN) {
        set_fast_scrollbar_visible(data, true);
        sync_fast_scrollbar_from_scroll(data);
        show_fast_scrollbar_progress_notice(data, true);
        return;
    }

    refresh_visible_items(data, false);
    sync_fast_scrollbar_from_scroll(data);
    show_fast_scrollbar_progress_notice(data, false);
}

/* 右侧可拖动快速滚动条 */
static void fast_scrollbar_event_cb(lv_event_t* e)
{
    page_album_data_t* data = (page_album_data_t*)lv_event_get_user_data(e);
    lv_event_code_t code;
    int target_y;
    int max_scroll_y;

    if (!data || !data->fast_scrollbar)
        return;
    if (data->deleting_in_progress)
        return;

    code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        set_fast_scrollbar_visible(data, true);
        show_fast_scrollbar_progress_notice(data, true);
        return;
    }
    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
        return;
    if (code != LV_EVENT_VALUE_CHANGED)
        return;

    if (data->syncing_fast_scrollbar)
        return;

    set_fast_scrollbar_visible(data, true);
    max_scroll_y = get_max_scroll_y(data);
    target_y = max_scroll_y - lv_slider_get_value(data->fast_scrollbar);
    target_y = clamp_int(target_y, 0, max_scroll_y);
    lv_obj_scroll_to_y(data->grid_container, target_y, LV_ANIM_OFF);
    show_fast_scrollbar_progress_notice(data, false);
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_album_create(void)
{
    page_album_data_t* data = (page_album_data_t*)malloc(sizeof(page_album_data_t));
    if (!data)
        return;

    memset(data, 0, sizeof(page_album_data_t));
    data->first_visible_row = -1;
    data->last_notice_index = -1;
    data->fast_scrollbar_range_max = -1;
    data->fast_scrollbar_last_value = -1;

    /* =======================
     * 1. 页面容器
     * ======================= */
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
     * 2. 顶部导航栏
     * ======================= */
    data->nav_bar = lv_obj_create(data->container);
    lv_obj_set_width(data->nav_bar, lv_pct(100));
    lv_obj_set_height(data->nav_bar, NAV_BAR_HEIGHT);
    lv_obj_add_style(data->nav_bar, &style_common_cont_top, LV_PART_MAIN);
    lv_obj_clear_flag(data->nav_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(data->nav_bar, LV_SCROLLBAR_MODE_OFF);

    /* 返回按钮 - 左上角 */
    data->back_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(data->back_btn, NAV_BAR_HEIGHT, NAV_BAR_HEIGHT);
    lv_obj_add_style(data->back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->back_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t* back_icon = lv_img_create(data->back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle-white.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    /* 取消按钮（选择模式） */
    data->cancel_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(data->cancel_btn, 100, 46);
    lv_obj_add_style(data->cancel_btn, &style_noboarder, LV_PART_MAIN);
    apply_nav_text_btn_style(data->cancel_btn);
    lv_obj_add_event_cb(data->cancel_btn, cancel_btn_cb, LV_EVENT_CLICKED, data);
    lv_obj_align(data->cancel_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t* cancel_label = lv_label_create(data->cancel_btn);
    lv_label_set_text(cancel_label, "取消");
    lv_obj_add_style(cancel_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(cancel_label, LV_ALIGN_CENTER, 0, 1);

    /* 全选按钮（选择模式） */
    data->select_all_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(data->select_all_btn, 100, 46);
    lv_obj_add_style(data->select_all_btn, &style_noboarder, LV_PART_MAIN);
    apply_nav_text_btn_style(data->select_all_btn);
    lv_obj_add_event_cb(data->select_all_btn, select_all_btn_cb, LV_EVENT_CLICKED, data);
    lv_obj_align(data->select_all_btn, LV_ALIGN_LEFT_MID, 104, 0);
    lv_obj_t* select_all_label = lv_label_create(data->select_all_btn);
    lv_label_set_text(select_all_label, "全选");
    lv_obj_add_style(select_all_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(select_all_label, LV_ALIGN_CENTER, 0, 1);

    /* 拍照按钮 */
    data->photo_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(data->photo_btn, NAV_BAR_HEIGHT, NAV_BAR_HEIGHT);
    lv_obj_add_style(data->photo_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->photo_btn, photo_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->photo_btn, LV_ALIGN_CENTER, -50, 0);
    lv_obj_t* photo_icon = lv_img_create(data->photo_btn);
    lv_img_set_src(photo_icon, "A:" RES_ICON_PATH "/camera.png");
    lv_obj_align(photo_icon, LV_ALIGN_CENTER, 0, 0);

    /* 录像按钮 */
    data->video_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(data->video_btn, NAV_BAR_HEIGHT, NAV_BAR_HEIGHT);
    lv_obj_add_style(data->video_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->video_btn, video_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->video_btn, LV_ALIGN_CENTER, 50, 0);
    lv_obj_t* video_icon = lv_img_create(data->video_btn);
    lv_img_set_src(video_icon, "A:" RES_ICON_PATH "/video-white.png");
    lv_obj_align(video_icon, LV_ALIGN_CENTER, 0, 0);

    /* 普通模式：选择按钮（在删除位置） */
    data->select_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(data->select_btn, 100, 46);
    lv_obj_add_style(data->select_btn, &style_noboarder, LV_PART_MAIN);
    apply_nav_text_btn_style(data->select_btn);
    lv_obj_add_event_cb(data->select_btn, select_btn_cb, LV_EVENT_CLICKED, data);
    lv_obj_align(data->select_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_t* select_label = lv_label_create(data->select_btn);
    lv_label_set_text(select_label, "选择");
    lv_obj_add_style(select_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(select_label, LV_ALIGN_CENTER, 0, 1);

    /* 选择模式：已选择数量 */
    data->selected_count_label = lv_label_create(data->nav_bar);
    lv_obj_add_style(data->selected_count_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_label_set_text(data->selected_count_label, "已选择 0 项");
    lv_obj_align(data->selected_count_label, LV_ALIGN_CENTER, 0, 0);

    /* 删除按钮 */
    data->delete_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(data->delete_btn, 100, 46);
    lv_obj_add_style(data->delete_btn, &style_noboarder, LV_PART_MAIN);
    apply_nav_text_btn_style(data->delete_btn);
    lv_obj_add_event_cb(data->delete_btn, delete_btn_cb, LV_EVENT_CLICKED, data);
    lv_obj_align(data->delete_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_t* delete_label = lv_label_create(data->delete_btn);
    lv_label_set_text(delete_label, "删除");
    lv_obj_add_style(delete_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(delete_label, LV_ALIGN_CENTER, 0, 1);

    update_nav_bar_state(data);

    /* =======================
     * 3. 虚拟列表网格区域
     * ======================= */
    data->grid_container = lv_obj_create(data->container);
    lv_obj_set_width(data->grid_container, LV_PCT(100));
    lv_obj_set_flex_grow(data->grid_container, 1);
    lv_obj_set_style_bg_opa(data->grid_container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->grid_container, lv_color_hex(0x121212), LV_PART_MAIN);
    lv_obj_set_style_border_width(data->grid_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_top(data->grid_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(data->grid_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(data->grid_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_right(data->grid_container, 0, LV_PART_MAIN);
    lv_obj_set_scroll_dir(data->grid_container, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(data->grid_container, LV_SCROLLBAR_MODE_OFF);

    data->scroll_content = lv_obj_create(data->grid_container);
    lv_obj_set_pos(data->scroll_content, 0, 0);
    lv_obj_set_width(data->scroll_content, LV_PCT(100));
    lv_obj_set_height(data->scroll_content, 1);
    lv_obj_add_style(data->scroll_content, &style_noboarder, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->scroll_content, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->scroll_content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(data->scroll_content, 0, LV_PART_MAIN);
    lv_obj_clear_flag(data->scroll_content, LV_OBJ_FLAG_SCROLLABLE);

    data->fast_scrollbar = lv_slider_create(data->grid_container);
    /* 与主题过渡样式隔离，避免 transition 动画链路引发悬空访问 */
    lv_obj_remove_style_all(data->fast_scrollbar);
    lv_obj_set_size(data->fast_scrollbar, FAST_SCROLLBAR_WIDTH, LV_PCT(85));
    lv_obj_align(data->fast_scrollbar, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_slider_set_orientation(data->fast_scrollbar, LV_SLIDER_ORIENTATION_VERTICAL);
    lv_obj_add_flag(data->fast_scrollbar, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_ext_click_area(data->fast_scrollbar, FAST_SCROLLBAR_TOUCH_PAD);
    lv_obj_set_style_bg_opa(data->fast_scrollbar, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->fast_scrollbar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(data->fast_scrollbar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->fast_scrollbar, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(data->fast_scrollbar, 0, LV_PART_INDICATOR);
    lv_obj_set_style_radius(data->fast_scrollbar, FAST_SCROLLBAR_KNOB_RADIUS, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(data->fast_scrollbar, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_bg_color(data->fast_scrollbar, lv_color_hex(0xE6E9EF), LV_PART_KNOB);
    lv_obj_set_style_bg_grad_color(data->fast_scrollbar, lv_color_hex(0xB9C0CC), LV_PART_KNOB);
    lv_obj_set_style_bg_grad_dir(data->fast_scrollbar, LV_GRAD_DIR_VER, LV_PART_KNOB);
    lv_obj_set_style_border_width(data->fast_scrollbar, 1, LV_PART_KNOB);
    lv_obj_set_style_border_color(data->fast_scrollbar, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_set_style_shadow_width(data->fast_scrollbar, 12, LV_PART_KNOB);
    lv_obj_set_style_shadow_color(data->fast_scrollbar, lv_color_hex(0x000000), LV_PART_KNOB);
    lv_obj_set_style_shadow_opa(data->fast_scrollbar, LV_OPA_40, LV_PART_KNOB);
    lv_obj_set_style_pad_top(data->fast_scrollbar, FAST_SCROLLBAR_KNOB_PAD_Y, LV_PART_KNOB);
    lv_obj_set_style_pad_bottom(data->fast_scrollbar, FAST_SCROLLBAR_KNOB_PAD_Y, LV_PART_KNOB);
    lv_obj_set_style_pad_left(data->fast_scrollbar, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_right(data->fast_scrollbar, 0, LV_PART_KNOB);
    lv_obj_add_event_cb(data->fast_scrollbar, fast_scrollbar_event_cb, LV_EVENT_VALUE_CHANGED, data);
    lv_obj_add_event_cb(data->fast_scrollbar, fast_scrollbar_event_cb, LV_EVENT_PRESSED, data);
    lv_obj_add_event_cb(data->fast_scrollbar, fast_scrollbar_event_cb, LV_EVENT_RELEASED, data);
    lv_obj_add_event_cb(data->fast_scrollbar, fast_scrollbar_event_cb, LV_EVENT_PRESS_LOST, data);

    /* 删除期间拦截操作的遮罩 */
    data->op_block_mask = lv_obj_create(data->container);
    lv_obj_add_flag(data->op_block_mask, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(data->op_block_mask, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->op_block_mask, &style_overlay_mask, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->op_block_mask, LV_OPA_20, LV_PART_MAIN);
    lv_obj_add_flag(data->op_block_mask, LV_OBJ_FLAG_HIDDEN);

    lv_obj_update_layout(data->container);

    data->total_photos = get_album_total_count();
    if (!ensure_selected_buffer(data, data->total_photos)) {
        MLOG_ERR("Album selected buffer alloc failed");
        if (data->container) {
            lv_obj_del(data->container);
            data->container = NULL;
        }
        free(data);
        return;
    }
    calculate_layout(data);
    update_scroll_content_height(data);

    if (create_item_pool(data) != 0) {
        MLOG_ERR("Album create item pool failed");
        if (data->container) {
            lv_obj_del(data->container);
            data->container = NULL;
        }
        free(data->selected_flags);
        data->selected_flags = NULL;
        free(data);
        return;
    }

    lv_obj_add_event_cb(data->grid_container, scroll_event_cb, LV_EVENT_SCROLL_BEGIN, data);
    lv_obj_add_event_cb(data->grid_container, scroll_event_cb, LV_EVENT_SCROLL, data);
    lv_obj_add_event_cb(data->grid_container, scroll_event_cb, LV_EVENT_SCROLL_END, data);

    lv_obj_scroll_to_y(data->grid_container, 0, LV_ANIM_OFF);
    refresh_visible_items(data, true);
    sync_fast_scrollbar_from_scroll(data);
    set_fast_scrollbar_visible(data, true);
    lv_async_call(sync_fast_scrollbar_deferred_cb, data);

    /* 启用整页事件冒泡，确保子对象按压事件传递到父容器。 */
    gesture_back_enable_event_bubble_recursive(data->container);

    /* 保存 private_data */
    page_set_private_data(data);
}

void page_album_destroy(void)
{
    page_album_data_t* data = page_get_private_data();
    if (!data)
        return;

    if (data->deleting_in_progress) {
        key_manager_set_block_non_power(data->prev_input_block_mask);
        data->deleting_in_progress = false;
    }
    destroy_item_pool(data);
    free(data->selected_flags);
    data->selected_flags = NULL;
    data->selected_capacity = 0;
    data->selected_count = 0;

    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    free(data);
}

void page_album_show(void)
{
    page_album_data_t* data = page_get_private_data();
    int new_total_photos;
    bool has_focus_target = false;
    int target_scroll_y = 0;
    if (!data || !data->container)
        return;

    gesture_back_set_left_edge_swipe_cb(data->container, page_manager_back_cb);

    MLOG_INFO("Album page show");
    new_total_photos = get_album_total_count();
    MLOG_INFO("Album page show: new_total_photos=%d old_total_photos=%d", new_total_photos, data->total_photos);
    if (new_total_photos != data->total_photos) {
        data->total_photos = new_total_photos;
        data->first_visible_row = -1;
        update_scroll_content_height(data);
        if (!ensure_selected_buffer(data, data->total_photos))
            MLOG_WARN("Album selected buffer ensure failed");
        exit_selection_mode(data);
    }
    if (g_album_focus_photo_index >= 0) {
        if (g_album_focus_photo_index < data->total_photos) {
            has_focus_target = true;
            target_scroll_y = get_scroll_y_for_photo_index(data, g_album_focus_photo_index);
        }
        g_album_focus_photo_index = -1;
    }

    update_nav_bar_state(data);
    data->last_notice_index = -1;
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
    if (has_focus_target) {
        data->first_visible_row = -1;
        lv_obj_scroll_to_y(data->grid_container, target_scroll_y, LV_ANIM_OFF);
    } else {
        /* 非预览返回场景（如从 Home 进入），默认回到顶部。 */
        data->first_visible_row = -1;
        lv_obj_scroll_to_y(data->grid_container, 0, LV_ANIM_OFF);
    }
    refresh_visible_items(data, true);
    sync_fast_scrollbar_from_scroll(data);
    set_fast_scrollbar_visible(data, true);
    lv_async_call(sync_fast_scrollbar_deferred_cb, data);
}

void page_album_hide(void)
{
    page_album_data_t* data = page_get_private_data();
    if (!data || !data->container)
        return;
    if (data->deleting_in_progress)
        return;

    MLOG_INFO("Album page hide");
    exit_selection_mode(data);
    data->last_notice_index = -1;
    top_notice_hide();
    set_fast_scrollbar_visible(data, true);
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_album_update(void)
{
    /* no-op */
}

void page_album_set_focus_photo_index(int photo_index)
{
    g_album_focus_photo_index = photo_index;
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
