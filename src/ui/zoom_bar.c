#include "ui/zoom_bar.h"
#include "core/font_manager.h"
#include "core/media_manager.h"
#include "core/param_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <string.h>

#define ZOOM_BAR_LEVEL_COUNT 4
#define ZOOM_CONTAINER_WIDTH 56
#define ZOOM_CONTAINER_PADDING 8
#define ZOOM_BTN_SIZE 36
#define ZOOM_BTN_GAP 10

typedef struct {
    lv_obj_t* container;
    lv_obj_t* btns[ZOOM_BAR_LEVEL_COUNT];
    uint8_t inited;
} zoom_bar_ctx_t;

static zoom_bar_ctx_t g_zoom_bar;

static const int g_zoom_values[ZOOM_BAR_LEVEL_COUNT] = {1, 2, 3, 6};
static const char* g_zoom_texts[ZOOM_BAR_LEVEL_COUNT] = {"1", "2", "3", "6"};

static int zoom_index_from_value(int zoom_value)
{
    int i;

    for (i = 0; i < ZOOM_BAR_LEVEL_COUNT; i++) {
        if (g_zoom_values[i] == zoom_value)
            return i;
    }

    return 0;
}

static int zoom_next_value(int zoom_value)
{
    int index = zoom_index_from_value(zoom_value);

    if (index < ZOOM_BAR_LEVEL_COUNT - 1) {
        return g_zoom_values[index + 1];
    }
    return g_zoom_values[index];
}

static int zoom_prev_value(int zoom_value)
{
    int index = zoom_index_from_value(zoom_value);

    if (index > 0) {
        return g_zoom_values[index - 1];
    }
    return g_zoom_values[index];
}

static void zoom_bar_apply_value(int zoom_value)
{
    int ret;

    ret = param_manager_set(PARAM_ID_ZOOM, zoom_value);
    if (ret != 0) {
        MLOG_ERR("Set zoom param failed: zoom=%d ret=%d", zoom_value, ret);
        return;
    }

    ret = media_manager_execute(MEDIA_OP_SET_ZOOM, zoom_value);
    if (ret != 0)
        MLOG_ERR("Set zoom media op failed: zoom=%d ret=%d", zoom_value, ret);

    zoom_bar_set_value(zoom_value);
    MLOG_INFO("Zoom selected: x%d", zoom_value);
}

static void zoom_btn_cb(lv_event_t* e)
{
    lv_obj_t* btn = lv_event_get_target(e);
    int i;

    if (!btn)
        return;

    for (i = 0; i < ZOOM_BAR_LEVEL_COUNT; i++) {
        if (g_zoom_bar.btns[i] != btn)
            continue;

        zoom_bar_apply_value(g_zoom_values[i]);
        return;
    }
}

void zoom_bar_set_value(int zoom_value)
{
    int i;
    int active_index;

    if (!g_zoom_bar.inited)
        return;

    active_index = zoom_index_from_value(zoom_value);
    for (i = 0; i < ZOOM_BAR_LEVEL_COUNT; i++) {
        lv_obj_t* btn = g_zoom_bar.btns[i];
        lv_obj_t* label;

        if (!btn)
            continue;

        label = lv_obj_get_child(btn, 0);
        if (i == active_index) {
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

void zoom_bar_init(void)
{
    int i;
    int total_zoom_height;
    int zoom_container_height;

    if (g_zoom_bar.inited)
        return;

    memset(&g_zoom_bar, 0, sizeof(g_zoom_bar));

    total_zoom_height = ZOOM_BAR_LEVEL_COUNT * ZOOM_BTN_SIZE + (ZOOM_BAR_LEVEL_COUNT - 1) * ZOOM_BTN_GAP;
    zoom_container_height = total_zoom_height + ZOOM_CONTAINER_PADDING * 2;

    g_zoom_bar.container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(g_zoom_bar.container, ZOOM_CONTAINER_WIDTH, zoom_container_height);
    lv_obj_align(g_zoom_bar.container, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_add_style(g_zoom_bar.container, &style_zoom_container, LV_PART_MAIN);
    lv_obj_clear_flag(g_zoom_bar.container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(g_zoom_bar.container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(g_zoom_bar.container, LV_OBJ_FLAG_HIDDEN);

    for (i = 0; i < ZOOM_BAR_LEVEL_COUNT; i++) {
        lv_obj_t* zoom_label;

        g_zoom_bar.btns[i] = lv_btn_create(g_zoom_bar.container);
        lv_obj_set_size(g_zoom_bar.btns[i], ZOOM_BTN_SIZE, ZOOM_BTN_SIZE);
        lv_obj_align(g_zoom_bar.btns[i], LV_ALIGN_TOP_MID, 0, ZOOM_CONTAINER_PADDING + i * (ZOOM_BTN_SIZE + ZOOM_BTN_GAP));
        lv_obj_add_style(g_zoom_bar.btns[i], &style_zoom_btn, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(g_zoom_bar.btns[i], &style_zoom_btn_active, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_add_event_cb(g_zoom_bar.btns[i], zoom_btn_cb, LV_EVENT_CLICKED, NULL);

        zoom_label = lv_label_create(g_zoom_bar.btns[i]);
        lv_label_set_text(zoom_label, g_zoom_texts[i]);
        lv_obj_add_style(zoom_label, &TINY_SIZE, LV_PART_MAIN);
        lv_obj_add_style(zoom_label, &style_zoom_label, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(zoom_label, &style_zoom_label_active, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_align(zoom_label, LV_ALIGN_CENTER, 0, 0);
    }

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

void zoom_bar_zoom_in(void)
{
    int current_zoom = param_manager_get(PARAM_ID_ZOOM);
    int next_zoom = zoom_next_value(current_zoom);

    if (next_zoom == current_zoom) {
        return;
    }

    zoom_bar_apply_value(next_zoom);
}

void zoom_bar_zoom_out(void)
{
    int current_zoom = param_manager_get(PARAM_ID_ZOOM);
    int prev_zoom = zoom_prev_value(current_zoom);

    if (prev_zoom == current_zoom) {
        return;
    }

    zoom_bar_apply_value(prev_zoom);
}
