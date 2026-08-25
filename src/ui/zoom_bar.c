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
    /* 程序化滚动的目标档位下标；SCROLL_END 唯一事实源。
     * >=0 表示有在飞的目标滚动：只有滚到该档（nearest==pending）才 commit，
     * 半路被 lv_obj_scroll_to_y 内 lv_anim_delete 触发的 deleted_cb→SCROLL_END
     * 会读到 nearest!=pending，直接忽略，杜绝点击/回授时的中途误下发。
     * -1 表示无待确认目标（纯拖动松手停下），此时才由 SCROLL_END 自吸附。 */
    int pending_index;
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

/* 程序化滚动到目标档中心，并记录 pending_index 供 SCROLL_END 一致性确认。 */
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

    /* 先记目标再启动滚动：lv_obj_scroll_to_y 会先 lv_anim_delete 杀在飞动画，
     * 其 deleted_cb 在半路发 SCROLL_END——届时 nearest!=pending_index 被忽略。 */
    g_zoom_bar.pending_index = index;
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

/* 读当前滚动位置换算最近档位下标（先偏移半档再 clamp 再整除，
 * 避免顶部弹性负位置时 C 整除朝零取整先产出 -1 的瞬时窗口）。 */
static int nearest_center_index(void)
{
    int32_t scroll_y;
    int32_t v;

    if (g_zoom_bar.list == NULL) {
        return 0;
    }

    scroll_y = lv_obj_get_scroll_top(g_zoom_bar.list);
    v = scroll_y + ZOOM_ITEM_STEP / 2;
    if (v < 0) {
        v = 0;
    }
    return (int)(v / ZOOM_ITEM_STEP);
}

/* 拖动中实时高亮居中档位（仅视觉，不下发）。 */
static void list_scroll_cb(lv_event_t* e)
{
    int nearest_index;

    LV_UNUSED(e);
    if (g_zoom_bar.list == NULL) {
        return;
    }

    nearest_index = nearest_center_index();
    if (nearest_index >= ZOOM_BAR_LEVEL_COUNT) {
        nearest_index = ZOOM_BAR_LEVEL_COUNT - 1;
    }

    if (nearest_index != g_zoom_bar.visual_index) {
        g_zoom_bar.visual_index = nearest_index;
        update_selection_style();
    }
}

/* 滚动结束。靠事件 param 区分两类来源（LVGL 源码保证）：
 *  A) 用户手指拖动松手：indev != NULL（lv_indev_scroll.c 的 throw handler 发出，
 *     param=indev）。手指刚重新接管过滚动，任何挂起的程序化 pending 都已失效
 *     （其吸附动画被这次拖动打断了）——作废 pending，按当前 nearest 自吸附。
 *     这是「拖动打断上一次吸附动画后 pending 陈旧、卡两档中间」的根治点。
 *  B) 动画完成/被删：indev == NULL（lv_obj_scroll.c scroll_end_cb 发出，param=NULL）。
 *     只有 nearest==pending 才是「到位」——commit 一次并清 pending；
 *     nearest!=pending 是 lv_obj_scroll_to_y 内 lv_anim_delete 杀在飞动画触发的
 *     半路 deleted_cb→SCROLL_END，直接忽略，杜绝点击/回授时的中途误下发。 */
static void list_scroll_end_cb(lv_event_t* e)
{
    int nearest_index;
    lv_indev_t* indev;

    if (g_zoom_bar.list == NULL) {
        return;
    }

    nearest_index = nearest_center_index();
    if (nearest_index >= ZOOM_BAR_LEVEL_COUNT) {
        nearest_index = ZOOM_BAR_LEVEL_COUNT - 1;
    }

    indev = lv_event_get_indev(e);
    if (indev != NULL) {
        /* A) 用户拖动松手：陈旧 pending 作废，按当前位置自吸附 */
        g_zoom_bar.pending_index = -1;
        if (nearest_index * ZOOM_ITEM_STEP == lv_obj_get_scroll_top(g_zoom_bar.list)) {
            set_selected_index(nearest_index, "scroll_snap"); /* 恰好停在中心 */
        } else {
            scroll_to_index(nearest_index, LV_ANIM_ON); /* 自吸附到中心，到位再下发 */
        }
        return;
    }

    /* B) 动画完成/被删 */
    if (g_zoom_bar.pending_index >= 0) {
        if (nearest_index != g_zoom_bar.pending_index) {
            return; /* 半路被删动画的 SCROLL_END：忽略 */
        }
        g_zoom_bar.pending_index = -1;
        set_selected_index(nearest_index, "scroll_snap");
    }
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

    /* 点击即时下发（用户预期立即生效）；随后滚动到该档中心。
     * scroll_to_index 记 pending=index，到位后的 SCROLL_END 因 applied 已一致
     * 不重复下发；途中被删动画的半路 SCROLL_END 因 nearest!=pending 被忽略。 */
    set_selected_index(index, "item_click");
    scroll_to_index(index, LV_ANIM_ON);
}

void zoom_bar_set_value(int zoom_value)
{
    int active_index;

    if (!g_zoom_bar.inited)
        return;

    active_index = zoom_index_from_value(zoom_value);

    /* 早退只看 applied_index：visual_index 会被 list_scroll_cb 在滚动动画
     * 途中实时改写，若把它纳入早退条件，则我们自己下发 zoom 触发的参数回授
     * （param_cb → zoom_bar_set_value）会随机命中「不早退」分支，从而杀掉在飞
     * 的点击/吸附动画并 ANIM_OFF 跳变，造成档位回退（GLM 分析确认的第三条路径）。
     * 回授值等于已下发值时本就无事可做。 */
    if (g_zoom_bar.applied_index == active_index) {
        return;
    }

    g_zoom_bar.applied_index = active_index;
    g_zoom_bar.visual_index = active_index;
    update_selection_style();

    /* 程序化定位到目标档：scroll_to_index 记 pending=active_index，
     * 其 ANIM_OFF 滚动删除在飞动画产生的半路 SCROLL_END 因 nearest!=pending
     * 被忽略；到位（ANIM_OFF 即时到位）的 SCROLL_END 因 applied 已一致不重复下发。 */
    scroll_to_index(active_index, LV_ANIM_OFF);
}

void zoom_bar_init(void)
{
    int i;

    if (g_zoom_bar.inited)
        return;

    memset(&g_zoom_bar, 0, sizeof(g_zoom_bar));
    g_zoom_bar.pending_index = -1;

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
    /* 不开原生 SNAP_CENTER：原生 snap 在惯性结束时「先启动 snap 动画、再立即
     * 发 SCROLL_END」（lv_indev_scroll.c），事件到达时位置未到位，且与自己的
     * scroll_to 形成双定位竞争。吸附完全收回自己手里：拖动松手的 SCROLL_END
     * 里算最近档并 scroll_to_index(ANIM_ON) 自吸附，到位后再唯一下发。 */
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

void zoom_bar_zoom_in(void)
{
    int index = g_zoom_bar.applied_index + 1;

    if (index >= ZOOM_BAR_LEVEL_COUNT) {
        return;
    }

    set_selected_index(index, "key_zoom_in");
    scroll_to_index(index, LV_ANIM_ON);
}

void zoom_bar_zoom_out(void)
{
    int index = g_zoom_bar.applied_index - 1;

    if (index < 0) {
        return;
    }

    set_selected_index(index, "key_zoom_out");
    scroll_to_index(index, LV_ANIM_ON);
}
