// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_album.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/page_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <stdlib.h>
#include <string.h>

/* 布局常量 */
#define NAV_BAR_HEIGHT 50
#define GRID_ITEM_WIDTH 200
#define GRID_ITEM_HEIGHT 140
#define GRID_GAP_X 4
#define GRID_GAP_Y 2
#define GRID_BUFFER_ROWS 2
#define FAST_SCROLLBAR_WIDTH 16
#define FAST_SCROLLBAR_KNOB_HEIGHT 40
#define FAST_SCROLLBAR_KNOB_RADIUS 15
#define FAST_SCROLLBAR_KNOB_PAD_Y ((FAST_SCROLLBAR_KNOB_HEIGHT - FAST_SCROLLBAR_WIDTH) / 2)
#define FAST_SCROLLBAR_HIDE_DELAY_MS 1200
#define FAST_SCROLLBAR_TOUCH_PAD 10

/* 目前未接入 photo_manager，先使用占位数据模拟大图库场景 */
#define ALBUM_PLACEHOLDER_COUNT 10000

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_album.h)
// #############################################################################

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static void refresh_visible_items(page_album_data_t* data, bool force_refresh);
static void sync_fast_scrollbar_from_scroll(page_album_data_t* data);
static void set_fast_scrollbar_visible(page_album_data_t* data, bool visible);
static void schedule_fast_scrollbar_hide(page_album_data_t* data);
static void cancel_fast_scrollbar_hide(page_album_data_t* data);

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
    return ALBUM_PLACEHOLDER_COUNT;
}

static int get_total_rows(const page_album_data_t* data)
{
    int total_rows = (data->total_photos + data->layout.cols - 1) / data->layout.cols;
    if (total_rows < 1)
        total_rows = 1;
    return total_rows;
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

    if (get_max_scroll_y(data) <= 0)
        visible = false;

    if (visible)
        lv_obj_clear_flag(data->fast_scrollbar, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(data->fast_scrollbar, LV_OBJ_FLAG_HIDDEN);
}

static void fast_scrollbar_hide_timer_cb(lv_timer_t* timer)
{
    page_album_data_t* data = (page_album_data_t*)lv_timer_get_user_data(timer);
    if (!data)
        return;

    set_fast_scrollbar_visible(data, false);
    if (data->fast_scrollbar_hide_timer)
        lv_timer_pause(data->fast_scrollbar_hide_timer);
}

static void cancel_fast_scrollbar_hide(page_album_data_t* data)
{
    if (!data || !data->fast_scrollbar_hide_timer)
        return;

    lv_timer_pause(data->fast_scrollbar_hide_timer);
}

static void schedule_fast_scrollbar_hide(page_album_data_t* data)
{
    if (!data || !data->fast_scrollbar_hide_timer)
        return;

    lv_timer_set_period(data->fast_scrollbar_hide_timer, FAST_SCROLLBAR_HIDE_DELAY_MS);
    lv_timer_set_repeat_count(data->fast_scrollbar_hide_timer, 1);
    lv_timer_reset(data->fast_scrollbar_hide_timer);
    lv_timer_resume(data->fast_scrollbar_hide_timer);
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

static void update_item_content(album_item_t* item, int index)
{
    uint8_t shade = (uint8_t)(0x2C + (index % 5) * 0x0C);
    uint32_t color_hex = ((uint32_t)shade << 16) | ((uint32_t)shade << 8) | shade;

    lv_label_set_text_fmt(item->label, "%d", index + 1);
    lv_obj_set_style_bg_color(item->img, lv_color_hex(color_hex), LV_PART_MAIN);
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
        lv_obj_set_style_bg_opa(item->img, LV_OPA_40, LV_PART_MAIN);
        lv_obj_center(item->img);

        item->label = lv_label_create(item->container);
        lv_obj_add_style(item->label, &NORMAL_SIZE, LV_PART_MAIN);
        lv_obj_set_style_text_color(item->label, lv_color_white(), LV_PART_MAIN);
        lv_obj_center(item->label);
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

            if (item->index != target_index) {
                item->index = target_index;
                update_item_content(item, target_index);
            }

            if (!item->is_visible) {
                lv_obj_clear_flag(item->container, LV_OBJ_FLAG_HIDDEN);
                item->is_visible = true;
            }
        } else {
            item->index = -1;
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

    MLOG_INFO("Back button clicked");
    page_manager_back();
}

/* 拍照按钮回调 */
static void photo_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);

    MLOG_INFO("Photo button clicked");
}

/* 录像按钮回调 */
static void video_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);

    MLOG_INFO("Video button clicked");
}

/* 删除全部按钮回调 */
static void delete_all_btn_cb(lv_event_t* e)
{
    page_album_data_t* data = (page_album_data_t*)lv_event_get_user_data(e);
    if (!data)
        return;

    MLOG_INFO("Delete all button clicked");
    /* TODO: 弹出确认对话框 */
}

/* 删除按钮回调 */
static void delete_btn_cb(lv_event_t* e)
{
    page_album_data_t* data = (page_album_data_t*)lv_event_get_user_data(e);
    if (!data)
        return;

    MLOG_INFO("Delete button clicked");
    /* TODO: 删除选中的图片 */
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
        cancel_fast_scrollbar_hide(data);
        set_fast_scrollbar_visible(data, true);
        sync_fast_scrollbar_from_scroll(data);
        return;
    }

    refresh_visible_items(data, false);
    sync_fast_scrollbar_from_scroll(data);

    if (code == LV_EVENT_SCROLL_END)
        schedule_fast_scrollbar_hide(data);
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

    code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        cancel_fast_scrollbar_hide(data);
        set_fast_scrollbar_visible(data, true);
        return;
    }
    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        schedule_fast_scrollbar_hide(data);
        return;
    }
    if (code != LV_EVENT_VALUE_CHANGED)
        return;

    if (data->syncing_fast_scrollbar)
        return;

    cancel_fast_scrollbar_hide(data);
    set_fast_scrollbar_visible(data, true);
    max_scroll_y = get_max_scroll_y(data);
    target_y = max_scroll_y - lv_slider_get_value(data->fast_scrollbar);
    target_y = clamp_int(target_y, 0, max_scroll_y);
    lv_obj_scroll_to_y(data->grid_container, target_y, LV_ANIM_OFF);
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
    lv_obj_add_event_cb(data->container, page_manager_swipe_right_cb, LV_EVENT_GESTURE, NULL);

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

    /* 拍照按钮 */
    data->photo_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(data->photo_btn, NAV_BAR_HEIGHT, NAV_BAR_HEIGHT);
    lv_obj_add_style(data->photo_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->photo_btn, photo_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->photo_btn, LV_ALIGN_CENTER, -50, 0);
    lv_obj_t* photo_icon = lv_img_create(data->photo_btn);
    lv_img_set_src(photo_icon, "A:" RES_ICON_PATH "/photo-white.png");
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

    /* 删除全部按钮 */
    data->delete_all_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(data->delete_all_btn, NAV_BAR_HEIGHT, NAV_BAR_HEIGHT);
    lv_obj_add_style(data->delete_all_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->delete_all_btn, delete_all_btn_cb, LV_EVENT_CLICKED, data);
    lv_obj_align(data->delete_all_btn, LV_ALIGN_RIGHT_MID, -70, 0);
    lv_obj_t* delete_all_icon = lv_img_create(data->delete_all_btn);
    lv_img_set_src(delete_all_icon, "A:" RES_ICON_PATH "/delete-all.png");
    lv_obj_align(delete_all_icon, LV_ALIGN_CENTER, 0, 0);

    /* 删除按钮 */
    data->delete_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(data->delete_btn, NAV_BAR_HEIGHT, NAV_BAR_HEIGHT);
    lv_obj_add_style(data->delete_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->delete_btn, delete_btn_cb, LV_EVENT_CLICKED, data);
    lv_obj_align(data->delete_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_t* delete_icon = lv_img_create(data->delete_btn);
    lv_img_set_src(delete_icon, "A:" RES_ICON_PATH "/delete.png");
    lv_obj_align(delete_icon, LV_ALIGN_CENTER, 0, 0);

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
    lv_obj_add_flag(data->fast_scrollbar, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_ext_click_area(data->fast_scrollbar, FAST_SCROLLBAR_TOUCH_PAD);
    lv_obj_set_style_radius(data->fast_scrollbar, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->fast_scrollbar, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->fast_scrollbar, lv_color_hex(0x404040), LV_PART_MAIN);
    lv_obj_set_style_pad_all(data->fast_scrollbar, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(data->fast_scrollbar, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(data->fast_scrollbar, LV_OPA_10, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(data->fast_scrollbar, lv_color_hex(0x909090), LV_PART_INDICATOR);
    lv_obj_set_style_radius(data->fast_scrollbar, FAST_SCROLLBAR_KNOB_RADIUS, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(data->fast_scrollbar, LV_OPA_70, LV_PART_KNOB);
    lv_obj_set_style_bg_color(data->fast_scrollbar, lv_color_hex(0xD8D8D8), LV_PART_KNOB);
    lv_obj_set_style_pad_top(data->fast_scrollbar, FAST_SCROLLBAR_KNOB_PAD_Y, LV_PART_KNOB);
    lv_obj_set_style_pad_bottom(data->fast_scrollbar, FAST_SCROLLBAR_KNOB_PAD_Y, LV_PART_KNOB);
    lv_obj_set_style_pad_left(data->fast_scrollbar, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_right(data->fast_scrollbar, 0, LV_PART_KNOB);
    lv_obj_add_event_cb(data->fast_scrollbar, fast_scrollbar_event_cb, LV_EVENT_VALUE_CHANGED, data);
    lv_obj_add_event_cb(data->fast_scrollbar, fast_scrollbar_event_cb, LV_EVENT_PRESSED, data);
    lv_obj_add_event_cb(data->fast_scrollbar, fast_scrollbar_event_cb, LV_EVENT_RELEASED, data);
    lv_obj_add_event_cb(data->fast_scrollbar, fast_scrollbar_event_cb, LV_EVENT_PRESS_LOST, data);
    lv_obj_add_flag(data->fast_scrollbar, LV_OBJ_FLAG_HIDDEN);
    data->fast_scrollbar_hide_timer = lv_timer_create(fast_scrollbar_hide_timer_cb, FAST_SCROLLBAR_HIDE_DELAY_MS, data);
    if (data->fast_scrollbar_hide_timer) {
        lv_timer_set_auto_delete(data->fast_scrollbar_hide_timer, false);
        lv_timer_set_repeat_count(data->fast_scrollbar_hide_timer, 1);
        lv_timer_pause(data->fast_scrollbar_hide_timer);
    }

    lv_obj_update_layout(data->container);

    data->total_photos = get_album_total_count();
    calculate_layout(data);
    update_scroll_content_height(data);

    if (create_item_pool(data) != 0) {
        MLOG_ERR("Album create item pool failed");
        if (data->container) {
            lv_obj_del(data->container);
            data->container = NULL;
        }
        free(data);
        return;
    }

    lv_obj_add_event_cb(data->grid_container, scroll_event_cb, LV_EVENT_SCROLL_BEGIN, data);
    lv_obj_add_event_cb(data->grid_container, scroll_event_cb, LV_EVENT_SCROLL, data);
    lv_obj_add_event_cb(data->grid_container, scroll_event_cb, LV_EVENT_SCROLL_END, data);

    refresh_visible_items(data, true);
    sync_fast_scrollbar_from_scroll(data);
    set_fast_scrollbar_visible(data, false);

    /* 保存 private_data */
    page_set_private_data(data);
}

void page_album_destroy(void)
{
    page_album_data_t* data = page_get_private_data();
    if (!data)
        return;

    if (data->fast_scrollbar_hide_timer) {
        lv_timer_delete(data->fast_scrollbar_hide_timer);
        data->fast_scrollbar_hide_timer = NULL;
    }

    destroy_item_pool(data);

    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    free(data);
}

void page_album_show(void)
{
    page_album_data_t* data = page_get_private_data();
    if (!data || !data->container)
        return;

    MLOG_INFO("Album page show");
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
    cancel_fast_scrollbar_hide(data);
    sync_fast_scrollbar_from_scroll(data);
    set_fast_scrollbar_visible(data, false);
}

void page_album_hide(void)
{
    page_album_data_t* data = page_get_private_data();
    if (!data || !data->container)
        return;

    MLOG_INFO("Album page hide");
    cancel_fast_scrollbar_hide(data);
    set_fast_scrollbar_visible(data, false);
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_album_update(void)
{
    /* no-op */
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
