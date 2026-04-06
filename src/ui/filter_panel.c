// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "ui/filter_panel.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/media_manager.h"
#include "core/param_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FILTER_PANEL_HEIGHT 116
#define FILTER_THUMB_WIDTH 80
#define FILTER_THUMB_HEIGHT 60
#define FILTER_ITEM_HEIGHT 100
#define FILTER_ITEM_GAP 12
#define FILTER_PANEL_TOP_PAD 8
#define FILTER_PANEL_BOTTOM_PAD 2
#define FILTER_FOCUS_Y_OFFSET -1

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义
// #############################################################################

typedef struct {
    const char* name;
    const char* icon_path;
    const char* isp_bin_path;
} filter_panel_item_t;

typedef struct {
    lv_obj_t* overlay;
    lv_obj_t* panel;
    lv_obj_t* list;
    lv_obj_t* focus_frame;
    lv_obj_t* items[FILTER_PANEL_MAX_COUNT];
    lv_obj_t* thumbs[FILTER_PANEL_MAX_COUNT];
    lv_obj_t* labels[FILTER_PANEL_MAX_COUNT];
    filter_panel_item_t panel_items[FILTER_PANEL_MAX_COUNT];
    int selected_index;
    int item_count;
    uint8_t rebuilding;
    uint8_t inited;
} filter_panel_ctx_t;

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static filter_panel_ctx_t g_filter_panel;
static const filter_panel_item_t g_ui_filter_items[] = {
    { "原图", "A:" RES_ICON_PATH "/filter_default.png", RES_ISP_PATH "/cvi_sdr_bin" },
    { "明亮", "A:" RES_ICON_PATH "/filter.png", RES_ISP_PATH "/cvi_sdr_bin_bright" },
    { "胶片", "A:" RES_ICON_PATH "/filter.png", RES_ISP_PATH "/cvi_sdr_bin_film" },
    { "黑白", "A:" RES_ICON_PATH "/filter.png", RES_ISP_PATH "/cvi_sdr_bin_black_white" },
};

#define UI_FILTER_COUNT ((int)(sizeof(g_ui_filter_items) / sizeof(g_ui_filter_items[0])))

static void set_selected_index(int index, const char* source);
static void update_selection_style(void);
static void scroll_to_index(int index, lv_anim_enable_t anim_en);
static void snap_to_center(lv_anim_enable_t anim_en);
static void align_focus_frame(void);
static const char* filter_panel_get_name(int index);
static void rebuild_items(void);
static void ensure_created(void);
static void overlay_click_cb(lv_event_t* e);
static void list_scroll_end_cb(lv_event_t* e);
static void item_click_cb(lv_event_t* e);
static void apply_effect_to_mode(int index);

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

/* 更新滤镜选中态样式。 */
static void update_selection_style(void)
{
    int i;

    for (i = 0; i < g_filter_panel.item_count; i++) {
        lv_obj_t* item = g_filter_panel.items[i];
        lv_obj_t* label = g_filter_panel.labels[i];
        if (item == NULL || label == NULL) {
            continue;
        }

        if (i == g_filter_panel.selected_index) {
            lv_obj_set_style_text_color(label, lv_color_hex(0xF09F20), LV_PART_MAIN);
            lv_obj_set_style_opa(item, LV_OPA_COVER, LV_PART_MAIN);
        } else {
            lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_opa(item, LV_OPA_80, LV_PART_MAIN);
        }
    }
}

/* 根据下标滚动到中间选中框。 */
static void scroll_to_index(int index, lv_anim_enable_t anim_en)
{
    int32_t step;
    int32_t target_x;
    int32_t max_scroll_x;

    if (g_filter_panel.list == NULL || index < 0 || index >= g_filter_panel.item_count) {
        return;
    }

    step = FILTER_THUMB_WIDTH + FILTER_ITEM_GAP;
    target_x = index * step;
    if (target_x < 0) {
        target_x = 0;
    }

    max_scroll_x = lv_obj_get_scroll_left(g_filter_panel.list) + lv_obj_get_scroll_right(g_filter_panel.list);
    if (target_x > max_scroll_x) {
        target_x = max_scroll_x;
    }
    lv_obj_scroll_to_x(g_filter_panel.list, target_x, anim_en);
}

/* 内部设置选中下标并同步参数。 */
static void set_selected_index(int index, const char* source)
{
    if (index < 0 || index >= g_filter_panel.item_count) {
        return;
    }
    if (g_filter_panel.selected_index == index) {
        return;
    }

    g_filter_panel.selected_index = index;
    (void)param_manager_set(PARAM_ID_FILTER_INDEX, index);
    apply_effect_to_mode(index);
    MLOG_INFO("Filter selected [%s]: index=%d, name=%s, path=%s",
        (source == NULL) ? "unknown" : source,
        index,
        filter_panel_get_name(index),
        g_filter_panel.panel_items[index].isp_bin_path);
}

/* 吸附到最近条目。 */
static void snap_to_center(lv_anim_enable_t anim_en)
{
    int32_t scroll_x;
    int32_t step;
    int nearest_index;

    if (g_filter_panel.list == NULL || g_filter_panel.item_count <= 0) {
        return;
    }

    step = FILTER_THUMB_WIDTH + FILTER_ITEM_GAP;
    scroll_x = lv_obj_get_scroll_left(g_filter_panel.list);
    nearest_index = (scroll_x + step / 2) / step;
    if (nearest_index < 0) {
        nearest_index = 0;
    }
    if (nearest_index >= g_filter_panel.item_count) {
        nearest_index = g_filter_panel.item_count - 1;
    }

    set_selected_index(nearest_index, "scroll_snap");
    update_selection_style();
    scroll_to_index(nearest_index, anim_en);
}

/* 让固定选中框与缩略图容器严格上下对齐。 */
static void align_focus_frame(void)
{
    lv_obj_t* ref_thumb;
    lv_area_t thumb_coords;
    lv_area_t panel_coords;
    int32_t y_in_panel;

    if (g_filter_panel.focus_frame == NULL || g_filter_panel.panel == NULL || g_filter_panel.list == NULL) {
        return;
    }
    if (g_filter_panel.item_count <= 0) {
        return;
    }
    ref_thumb = g_filter_panel.thumbs[0];
    if (ref_thumb == NULL) {
        return;
    }

    lv_obj_update_layout(g_filter_panel.list);
    lv_obj_get_coords(ref_thumb, &thumb_coords);
    lv_obj_get_coords(g_filter_panel.panel, &panel_coords);
    y_in_panel = thumb_coords.y1 - panel_coords.y1;
    lv_obj_align(g_filter_panel.focus_frame, LV_ALIGN_TOP_MID, 0, y_in_panel + FILTER_FOCUS_Y_OFFSET);
}

/* 按当前配置重建条目。 */
static void rebuild_items(void)
{
    int i;

    if (g_filter_panel.list == NULL) {
        return;
    }

    g_filter_panel.rebuilding = 1;
    g_filter_panel.item_count = 0;
    g_filter_panel.selected_index = 0;
    memset(g_filter_panel.items, 0, sizeof(g_filter_panel.items));
    memset(g_filter_panel.thumbs, 0, sizeof(g_filter_panel.thumbs));
    memset(g_filter_panel.labels, 0, sizeof(g_filter_panel.labels));
    lv_obj_clean(g_filter_panel.list);

    g_filter_panel.item_count = UI_FILTER_COUNT;
    if (g_filter_panel.item_count > FILTER_PANEL_MAX_COUNT) {
        g_filter_panel.item_count = FILTER_PANEL_MAX_COUNT;
    }

    for (i = 0; i < g_filter_panel.item_count; i++) {
        g_filter_panel.panel_items[i] = g_ui_filter_items[i];
    }

    g_filter_panel.selected_index = param_manager_get(PARAM_ID_FILTER_INDEX);
    if (g_filter_panel.selected_index < 0 || g_filter_panel.selected_index >= g_filter_panel.item_count) {
        g_filter_panel.selected_index = 0;
    }
    (void)param_manager_set(PARAM_ID_FILTER_INDEX, g_filter_panel.selected_index);

    for (i = 0; i < g_filter_panel.item_count; i++) {
        lv_obj_t* item = lv_obj_create(g_filter_panel.list);
        lv_obj_set_size(item, FILTER_THUMB_WIDTH, FILTER_ITEM_HEIGHT);
        lv_obj_set_scrollbar_mode(item, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_style(item, &style_photo_filter_item, LV_PART_MAIN);
        lv_obj_set_layout(item, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(item, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(item, 8, LV_PART_MAIN);
        lv_obj_add_event_cb(item, item_click_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_set_user_data(item, (void*)(intptr_t)i);

        lv_obj_t* thumb = lv_obj_create(item);
        lv_obj_set_size(thumb, FILTER_THUMB_WIDTH, FILTER_THUMB_HEIGHT);
        lv_obj_clear_flag(thumb, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(thumb, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_style(thumb, &style_photo_filter_thumb, LV_PART_MAIN);

        lv_obj_t* thumb_img = lv_img_create(thumb);
        lv_img_set_src(thumb_img, g_filter_panel.panel_items[i].icon_path);
        lv_obj_add_flag(thumb_img, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_align(thumb_img, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t* label = lv_label_create(item);
        lv_label_set_text(label, g_filter_panel.panel_items[i].name);
        lv_obj_add_style(label, &SMALL_SIZE, LV_PART_MAIN);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_add_flag(label, LV_OBJ_FLAG_EVENT_BUBBLE);

        g_filter_panel.items[i] = item;
        g_filter_panel.thumbs[i] = thumb;
        g_filter_panel.labels[i] = label;
    }

    update_selection_style();
    align_focus_frame();
    scroll_to_index(g_filter_panel.selected_index, LV_ANIM_OFF);
    g_filter_panel.rebuilding = 0;
}

/* 创建全局滤镜面板对象，挂到当前屏幕（show 时会置前）。 */
static void ensure_created(void)
{
    if (g_filter_panel.inited) {
        return;
    }

    memset(&g_filter_panel, 0, sizeof(g_filter_panel));

    g_filter_panel.overlay = lv_obj_create(lv_screen_active());
    lv_obj_set_size(g_filter_panel.overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(g_filter_panel.overlay, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_filter_panel.overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(g_filter_panel.overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_filter_panel.overlay, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(g_filter_panel.overlay, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(g_filter_panel.overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(g_filter_panel.overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(g_filter_panel.overlay, overlay_click_cb, LV_EVENT_CLICKED, NULL);

    g_filter_panel.panel = lv_obj_create(g_filter_panel.overlay);
    lv_obj_set_width(g_filter_panel.panel, lv_pct(100));
    lv_obj_set_height(g_filter_panel.panel, FILTER_PANEL_HEIGHT);
    lv_obj_align(g_filter_panel.panel, LV_ALIGN_BOTTOM_MID, 0, -63);
    lv_obj_set_scrollbar_mode(g_filter_panel.panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(g_filter_panel.panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(g_filter_panel.panel, &style_photo_filter_panel, LV_PART_MAIN);
    lv_obj_add_flag(g_filter_panel.panel, LV_OBJ_FLAG_CLICKABLE);

    g_filter_panel.list = lv_obj_create(g_filter_panel.panel);
    lv_obj_set_size(g_filter_panel.list, lv_pct(100), lv_pct(100));
    lv_obj_set_scroll_dir(g_filter_panel.list, LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(g_filter_panel.list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_layout(g_filter_panel.list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(g_filter_panel.list, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(g_filter_panel.list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(g_filter_panel.list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_filter_panel.list, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(g_filter_panel.list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(g_filter_panel.list, (H_RES - FILTER_THUMB_WIDTH) / 2, LV_PART_MAIN);
    lv_obj_set_style_pad_right(g_filter_panel.list, (H_RES - FILTER_THUMB_WIDTH) / 2, LV_PART_MAIN);
    lv_obj_set_style_pad_top(g_filter_panel.list, FILTER_PANEL_TOP_PAD, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(g_filter_panel.list, FILTER_PANEL_BOTTOM_PAD, LV_PART_MAIN);
    lv_obj_set_style_pad_column(g_filter_panel.list, FILTER_ITEM_GAP, LV_PART_MAIN);
    lv_obj_set_style_pad_row(g_filter_panel.list, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(g_filter_panel.list, list_scroll_end_cb, LV_EVENT_SCROLL_END, NULL);

    g_filter_panel.focus_frame = lv_obj_create(g_filter_panel.panel);
    lv_obj_add_flag(g_filter_panel.focus_frame, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(g_filter_panel.focus_frame, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(g_filter_panel.focus_frame, FILTER_THUMB_WIDTH, FILTER_THUMB_HEIGHT);
    lv_obj_align(g_filter_panel.focus_frame, LV_ALIGN_TOP_MID, 0, FILTER_PANEL_TOP_PAD);
    lv_obj_add_style(g_filter_panel.focus_frame, &style_photo_filter_focus_frame, LV_PART_MAIN);

    g_filter_panel.inited = 1;
}

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

static void overlay_click_cb(lv_event_t* e)
{
    if (lv_event_get_target(e) == g_filter_panel.overlay) {
        filter_panel_hide();
    }
}

static void list_scroll_end_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    if (g_filter_panel.rebuilding) {
        return;
    }
    snap_to_center(LV_ANIM_ON);
}

static void item_click_cb(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_current_target(e);
    int index;

    if (target == NULL) {
        return;
    }

    index = (int)(intptr_t)lv_obj_get_user_data(target);
    if (index < 0 || index >= g_filter_panel.item_count) {
        return;
    }

    set_selected_index(index, "item_click");
    update_selection_style();
    scroll_to_index(index, LV_ANIM_ON);
}

// #endregion
// #############################################################################
// ! #region 5. 对外接口函数
// #############################################################################

void filter_panel_init(void)
{
    ensure_created();
}

void filter_panel_show(void)
{
    ensure_created();
    rebuild_items();
    lv_obj_clear_flag(g_filter_panel.overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(g_filter_panel.overlay);
}

void filter_panel_hide(void)
{
    if (!g_filter_panel.inited || g_filter_panel.overlay == NULL) {
        return;
    }
    lv_obj_add_flag(g_filter_panel.overlay, LV_OBJ_FLAG_HIDDEN);
}

int filter_panel_is_visible(void)
{
    if (!g_filter_panel.inited || g_filter_panel.overlay == NULL) {
        return 0;
    }
    return lv_obj_has_flag(g_filter_panel.overlay, LV_OBJ_FLAG_HIDDEN) ? 0 : 1;
}

static const char* filter_panel_get_name(int index)
{
    if (index < 0 || index >= g_filter_panel.item_count) {
        return "unknown";
    }
    return g_filter_panel.panel_items[index].name;
}

static void apply_effect_to_mode(int index)
{
    if (index < 0 || index >= g_filter_panel.item_count) {
        return;
    }

    (void)media_manager_set_filter_with_path(index, g_filter_panel.panel_items[index].isp_bin_path);
}

// #endregion
