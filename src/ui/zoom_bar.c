#include "ui/zoom_bar.h"
#include "core/font_manager.h"
#include "core/media_manager.h"
#include "core/param_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <stdint.h>
#include <string.h>

#define ZOOM_BAR_LEVEL_COUNT 16
#define ZOOM_BAR_MIN_VALUE 1
#define ZOOM_BAR_MAX_VALUE 16
#define ZOOM_PANEL_WIDTH 56
#define ZOOM_PANEL_HEIGHT 190
#define ZOOM_ITEM_SIZE 36
#define ZOOM_ITEM_GAP 10
#define ZOOM_ITEM_STEP (ZOOM_ITEM_SIZE + ZOOM_ITEM_GAP)
#define ZOOM_FOCUS_SIZE 40

typedef struct {
    lv_obj_t* container;
    lv_obj_t* list;
    lv_obj_t* focus_frame;
    lv_obj_t* btns[ZOOM_BAR_LEVEL_COUNT];
    int applied_index; /* 已下发到 param/media 的档位下标 */
    int visual_index; /* 当前居中高亮档位下标（拖动中实时变化） */
    uint8_t syncing; /* 程序化同步滚动中，抑制 SCROLL_END 吸附 */
    uint8_t inited;
} zoom_bar_ctx_t;

static zoom_bar_ctx_t g_zoom_bar;

static int zoom_index_from_value(int zoom_value)
{
    if (zoom_value < ZOOM_BAR_MIN_VALUE || zoom_value > ZOOM_BAR_MAX_VALUE)
        return 0;

    return zoom_value - ZOOM_BAR_MIN_VALUE;
}

static void update_selection_style(void)
{
    int i;

    for (i = 0; i < ZOOM_BAR_LEVEL_COUNT; i++) {
        lv_obj_t* btn = g_zoom_bar.btns[i];
        lv_obj_t* label;

        if (!btn)
            continue;

        label = lv_obj_get_child(btn, 0);
        if (i == g_zoom_bar.visual_index) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
            if (label)
                lv_obj_add_state(label, LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(btn, LV_STATE_CHECKED);
            if (label)
                lv_obj_remove_state(label, LV_STATE_CHECKED);
        }
    }
}

static void scroll_to_index(int index, lv_anim_enable_t anim_en)
{
    int32_t target_y;
    int32_t max_scroll_y;

    if (g_zoom_bar.list == NULL || index < 0 || index >= ZOOM_BAR_LEVEL_COUNT) {
        return;
    }

    target_y = index * ZOOM_ITEM_STEP;
    if (target_y < 0) {
        target_y = 0;
    }

    max_scroll_y = lv_obj_get_scroll_top(g_zoom_bar.list) + lv_obj_get_scroll_bottom(g_zoom_bar.list);
    if (target_y > max_scroll_y) {
        target_y = max_scroll_y;
    }
    lv_obj_scroll_to_y(g_zoom_bar.list, target_y, anim_en);
}

/* 设置选中下标：applied 变化时才下发 param/media，visual 始终同步刷新。 */
static void set_selected_index(int index, const char* source)
{
    int zoom_value;
    int ret;

    if (index < 0 || index >= ZOOM_BAR_LEVEL_COUNT) {
        return;
    }

    if (g_zoom_bar.applied_index != index) {
        zoom_value = index + ZOOM_BAR_MIN_VALUE;
        ret = param_manager_set(PARAM_ID_ZOOM, zoom_value);
        if (ret != 0) {
            MLOG_ERR("Set zoom param failed: zoom=%d ret=%d", zoom_value, ret);
            return;
        }

        ret = media_manager_execute(MEDIA_OP_SET_ZOOM, zoom_value);
        if (ret != 0)
            MLOG_ERR("Set zoom media op failed: zoom=%d ret=%d", zoom_value, ret);

        g_zoom_bar.applied_index = index;
        MLOG_INFO("Zoom selected [%s]: x%d", (source == NULL) ? "unknown" : source, zoom_value);
    }

    g_zoom_bar.visual_index = index;
    update_selection_style();
}

/* 吸附到最近档位并下发。 */
static void snap_to_center(lv_anim_enable_t anim_en)
{
    int32_t scroll_y;
    int nearest_index;

    if (g_zoom_bar.list == NULL) {
        return;
    }

    scroll_y = lv_obj_get_scroll_top(g_zoom_bar.list);
    nearest_index = (scroll_y + ZOOM_ITEM_STEP / 2) / ZOOM_ITEM_STEP;
    if (nearest_index < 0) {
        nearest_index = 0;
    }
    if (nearest_index >= ZOOM_BAR_LEVEL_COUNT) {
        nearest_index = ZOOM_BAR_LEVEL_COUNT - 1;
    }

    set_selected_index(nearest_index, "scroll_snap");
    scroll_to_index(nearest_index, anim_en);
}

/* 拖动中实时高亮居中档位（仅视觉，不下发）。 */
static void list_scroll_cb(lv_event_t* e)
{
    int32_t scroll_y;
    int nearest_index;

    LV_UNUSED(e);
    if (g_zoom_bar.list == NULL) {
        return;
    }

    scroll_y = lv_obj_get_scroll_top(g_zoom_bar.list);
    nearest_index = (scroll_y + ZOOM_ITEM_STEP / 2) / ZOOM_ITEM_STEP;
    if (nearest_index < 0) {
        nearest_index = 0;
    }
    if (nearest_index >= ZOOM_BAR_LEVEL_COUNT) {
        nearest_index = ZOOM_BAR_LEVEL_COUNT - 1;
    }

    if (nearest_index != g_zoom_bar.visual_index) {
        g_zoom_bar.visual_index = nearest_index;
        update_selection_style();
    }
}

static void list_scroll_end_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    if (g_zoom_bar.syncing) {
        return;
    }
    snap_to_center(LV_ANIM_ON);
}

static void zoom_item_click_cb(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_current_target(e);
    int index;

    if (target == NULL) {
        return;
    }

    index = (int)(intptr_t)lv_obj_get_user_data(target);
    if (index < 0 || index >= ZOOM_BAR_LEVEL_COUNT) {
        return;
    }

    set_selected_index(index, "item_click");
    scroll_to_index(index, LV_ANIM_ON);
}

void zoom_bar_set_value(int zoom_value)
{
    int active_index;

    if (!g_zoom_bar.inited)
        return;

    active_index = zoom_index_from_value(zoom_value);
    if (g_zoom_bar.applied_index == active_index && g_zoom_bar.visual_index == active_index) {
        return; /* 状态已一致：不动滚动，避免打断点击/吸附动画引发档位回退 */
    }

    g_zoom_bar.applied_index = active_index;
    g_zoom_bar.visual_index = active_index;
    update_selection_style();

    /* 程序化同步滚动期间抑制 SCROLL_END 吸附：
     * ANIM_OFF 滚动会删除在飞的滚动动画，其 deleted_cb 会在半路位置发出
     * SCROLL_END，若此刻吸附会把旧档位重新下发并滚回去（点击回退问题）。 */
    g_zoom_bar.syncing = 1;
    scroll_to_index(active_index, LV_ANIM_OFF);
    g_zoom_bar.syncing = 0;
}

void zoom_bar_init(void)
{
    int i;

    if (g_zoom_bar.inited)
        return;

    memset(&g_zoom_bar, 0, sizeof(g_zoom_bar));

    g_zoom_bar.container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(g_zoom_bar.container, ZOOM_PANEL_WIDTH, ZOOM_PANEL_HEIGHT);
    lv_obj_align(g_zoom_bar.container, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_add_style(g_zoom_bar.container, &style_zoom_container, LV_PART_MAIN);
    lv_obj_clear_flag(g_zoom_bar.container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(g_zoom_bar.container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(g_zoom_bar.container, LV_OBJ_FLAG_HIDDEN);

    g_zoom_bar.list = lv_obj_create(g_zoom_bar.container);
    lv_obj_set_size(g_zoom_bar.list, lv_pct(100), lv_pct(100));
    lv_obj_set_scroll_dir(g_zoom_bar.list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(g_zoom_bar.list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_layout(g_zoom_bar.list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(g_zoom_bar.list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_zoom_bar.list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(g_zoom_bar.list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_zoom_bar.list, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(g_zoom_bar.list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(g_zoom_bar.list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_right(g_zoom_bar.list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_top(g_zoom_bar.list, (ZOOM_PANEL_HEIGHT - ZOOM_ITEM_SIZE) / 2, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(g_zoom_bar.list, (ZOOM_PANEL_HEIGHT - ZOOM_ITEM_SIZE) / 2, LV_PART_MAIN);
    lv_obj_set_style_pad_row(g_zoom_bar.list, ZOOM_ITEM_GAP, LV_PART_MAIN);
    lv_obj_add_event_cb(g_zoom_bar.list, list_scroll_cb, LV_EVENT_SCROLL, NULL);
    lv_obj_add_event_cb(g_zoom_bar.list, list_scroll_end_cb, LV_EVENT_SCROLL_END, NULL);

    for (i = 0; i < ZOOM_BAR_LEVEL_COUNT; i++) {
        lv_obj_t* zoom_label;

        g_zoom_bar.btns[i] = lv_btn_create(g_zoom_bar.list);
        lv_obj_set_size(g_zoom_bar.btns[i], ZOOM_ITEM_SIZE, ZOOM_ITEM_SIZE);
        lv_obj_add_style(g_zoom_bar.btns[i], &style_zoom_btn, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(g_zoom_bar.btns[i], &style_zoom_btn_active, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_clear_flag(g_zoom_bar.btns[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(g_zoom_bar.btns[i], zoom_item_click_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_set_user_data(g_zoom_bar.btns[i], (void*)(intptr_t)i);

        zoom_label = lv_label_create(g_zoom_bar.btns[i]);
        lv_label_set_text_fmt(zoom_label, "%d", i + ZOOM_BAR_MIN_VALUE);
        lv_obj_add_style(zoom_label, &TINY_SIZE, LV_PART_MAIN);
        lv_obj_add_style(zoom_label, &style_zoom_label, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(zoom_label, &style_zoom_label_active, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_align(zoom_label, LV_ALIGN_CENTER, 0, 0);
    }

    g_zoom_bar.focus_frame = lv_obj_create(g_zoom_bar.container);
    lv_obj_add_flag(g_zoom_bar.focus_frame, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(g_zoom_bar.focus_frame, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(g_zoom_bar.focus_frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(g_zoom_bar.focus_frame, ZOOM_FOCUS_SIZE, ZOOM_FOCUS_SIZE);
    lv_obj_align(g_zoom_bar.focus_frame, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(g_zoom_bar.focus_frame, &style_zoom_focus_frame, LV_PART_MAIN);

    g_zoom_bar.inited = 1;
}

void zoom_bar_show(void)
{
    if (!g_zoom_bar.inited)
        zoom_bar_init();
    if (!g_zoom_bar.inited || !g_zoom_bar.container)
        return;

    zoom_bar_set_value(param_manager_get(PARAM_ID_ZOOM));
    lv_obj_clear_flag(g_zoom_bar.container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(g_zoom_bar.container);
}

void zoom_bar_hide(void)
{
    if (!g_zoom_bar.inited || !g_zoom_bar.container)
        return;

    lv_obj_add_flag(g_zoom_bar.container, LV_OBJ_FLAG_HIDDEN);
}

int zoom_bar_is_visible(void)
{
    if (!g_zoom_bar.inited || !g_zoom_bar.container)
        return 0;

    return !lv_obj_has_flag(g_zoom_bar.container, LV_OBJ_FLAG_HIDDEN);
}
