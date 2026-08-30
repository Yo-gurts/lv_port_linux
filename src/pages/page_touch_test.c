// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_touch_test.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/key_manager.h"
#include "core/page_manager.h"
#include "core/power_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <stdlib.h>
#include <string.h>

#define TT_TOP_BAR_HEIGHT 56
#define TT_TOUCH_AREA_MARGIN 10 /* 触摸框到屏幕左右/下边缘留白 */
#define TT_TOUCH_AREA_TOP_GAP 12 /* 触摸框到顶栏的间距 */
#define TT_TOUCH_AREA_BORDER 2
/* 屏幕 640x480：宽 = 640 - 2*margin；高 = 480 - 顶栏 - top_gap - 下 margin */
#define TT_TOUCH_AREA_WIDTH (640 - 2 * TT_TOUCH_AREA_MARGIN)
#define TT_TOUCH_AREA_HEIGHT (480 - TT_TOP_BAR_HEIGHT - TT_TOUCH_AREA_TOP_GAP - TT_TOUCH_AREA_MARGIN)
/* 内容区尺寸（外框减两侧边框），判定与引导线均以此为坐标系 */
#define TT_CONTENT_W (TT_TOUCH_AREA_WIDTH - 2 * TT_TOUCH_AREA_BORDER)
#define TT_CONTENT_H (TT_TOUCH_AREA_HEIGHT - 2 * TT_TOUCH_AREA_BORDER)
#define TT_TOUCH_BAND_PX 40 /* 轨迹偏离引导对角线的容差带半宽(像素) */
#define TT_TOUCH_END_ZONE_PCT 12 /* 起/终点须落在距端 12% 区内(贴角) */

/* 阶段 */
#define TT_STAGE_WAIT_DOWN 0 /* 等左对角线 ↘ */
#define TT_STAGE_WAIT_UP 1 /* 左过，等右对角线 ↗ */
#define TT_STAGE_PASSED 2 /* 全部通过 */

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static void clear_touch_trace(page_touch_test_data_t* data);
static lv_obj_t* create_guide_group(page_touch_test_data_t* data, int down, lv_coord_t width, lv_coord_t height);
static void set_stage_guides(page_touch_test_data_t* data);
static void update_hint_by_stage(page_touch_test_data_t* data);
static int abs_int(int x);

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

static void update_overall_result(page_touch_test_data_t* data)
{
    if (data == NULL || data->summary_label == NULL) {
        return;
    }

    if (data->touch_passed) {
        lv_label_set_text(data->summary_label, "结果: PASS");
        lv_obj_set_style_text_color(data->summary_label, lv_color_hex(0x27AE60), LV_PART_MAIN);
    } else {
        lv_label_set_text(data->summary_label, "结果: WAITING");
        lv_obj_set_style_text_color(data->summary_label, lv_color_hex(0xF5A623), LV_PART_MAIN);
    }
}

static void clear_touch_trace(page_touch_test_data_t* data)
{
    uint16_t i;

    if (data == NULL) {
        return;
    }
    for (i = 0; i < data->trace_dot_count; ++i) {
        if (data->trace_dots[i] != NULL) {
            lv_obj_del(data->trace_dots[i]);
            data->trace_dots[i] = NULL;
        }
    }
    data->trace_dot_count = 0;
}

static void reset_test_state(page_touch_test_data_t* data)
{
    if (data == NULL) {
        return;
    }
    data->touch_tracking = 0U;
    data->touch_passed = 0U;
    data->stage = TT_STAGE_WAIT_DOWN;
    data->stroke_in_band = 0U;
    data->stroke_start_ok = 0U;
    clear_touch_trace(data);
    set_stage_guides(data);
    update_hint_by_stage(data);
    update_overall_result(data);
}

static void add_trace_dot(page_touch_test_data_t* data, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t* dot;

    if (data == NULL || data->touch_area == NULL || data->trace_dot_count >= PAGE_TOUCH_TEST_MAX_TRACE_DOTS) {
        return;
    }

    dot = lv_obj_create(data->touch_area);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 4, 4);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0x00D2FF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(dot, x - 2, y - 2);

    data->trace_dots[data->trace_dot_count++] = dot;
}

static lv_obj_t* make_dot(lv_obj_t* parent, lv_coord_t x, lv_coord_t y, lv_coord_t sz, uint32_t color, lv_opa_t opa)
{
    lv_obj_t* dot = lv_obj_create(parent);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, sz, sz);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dot, opa, LV_PART_MAIN);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(dot, x - sz / 2, y - sz / 2);
    return dot;
}

/* 对角线端点：down=1 为 (0,0)->(w,h)，down=0 为 (0,h)->(w,0) */
static void diag_point(int down, lv_coord_t w, lv_coord_t h, int i, int segments, int* px, int* py)
{
    *px = ((int)(w - 1) * i) / segments;
    *py = down ? (((int)(h - 1) * i) / segments)
               : ((int)(h - 1) - ((int)(h - 1) * i) / segments);
}

/* 半透明高亮带：沿对角线密铺直径=2*BAND 的半透明圆点，重叠连成圆头斜带，标出可滑动区间。
 * 用对角线长度算段数、直径两条相同 → 两条带天然等宽。 */
static void plot_band(lv_obj_t* parent, int down, lv_coord_t w, lv_coord_t h)
{
    int i;
    int diam = 2 * TT_TOUCH_BAND_PX; /* 带宽 = ±BAND */
    int segments = (int)w / 6; /* 足够密以重叠成连续带 */
    if (segments < 40) {
        segments = 40;
    }
    for (i = 0; i <= segments; ++i) {
        int x, y;
        diag_point(down, w, h, i, segments, &x, &y);
        (void)make_dot(parent, (lv_coord_t)x, (lv_coord_t)y, (lv_coord_t)diam, 0xFFFFFF, LV_OPA_20);
    }
}

/* 中线（细实线）：沿对角线密铺小点，作为理想对角线参考。 */
static void plot_centerline(lv_obj_t* parent, int down, lv_coord_t w, lv_coord_t h)
{
    int i;
    int segments = (int)w / 4;
    if (segments < 40) {
        segments = 40;
    }
    for (i = 0; i <= segments; ++i) {
        int x, y;
        diag_point(down, w, h, i, segments, &x, &y);
        (void)make_dot(parent, (lv_coord_t)x, (lv_coord_t)y, 3, 0xFFFFFF, LV_OPA_COVER);
    }
}

/* 用点铺一条线段 (x0,y0)->(x1,y1)，构成画线式箭头的一笔。 */
static void plot_seg(lv_obj_t* parent, int x0, int y0, int x1, int y1, lv_coord_t sz, uint32_t color)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int adx = abs_int(dx);
    int ady = abs_int(dy);
    int steps = (adx > ady ? adx : ady) / 2;
    int i;
    if (steps < 1) {
        steps = 1;
    }
    for (i = 0; i <= steps; ++i) {
        int x = x0 + dx * i / steps;
        int y = y0 + dy * i / steps;
        (void)make_dot(parent, (lv_coord_t)x, (lv_coord_t)y, sz, color, LV_OPA_COVER);
    }
}

/* 画线式方向箭头：位于中线中点，箭头尖朝终点方向（↘ 指右下、↗ 指右上），
 * 由指向终点的两笔短线段构成 V 形箭羽，贴合 45° 对角方向、尺寸放大。 */
static void make_arrow(lv_obj_t* parent, int down, lv_coord_t w, lv_coord_t h)
{
    int cx = w / 2;
    int cy = h / 2;
    int len = 26; /* 箭羽长度 */
    lv_coord_t sz = 4; /* 线粗 */
    uint32_t color = 0xFFFFFF;
    int tipx, tipy; /* 箭头尖(朝终点方向偏出中点) */
    int a1x, a1y, a2x, a2y;

    if (down) {
        /* 方向 (1,1)/√2；尖在中点朝右下 */
        tipx = cx + len;
        tipy = cy + len;
        /* 两笔从尖回收：一笔往左(-x)、一笔往上(-y)，构成朝右下的箭头 */
        a1x = tipx - len;
        a1y = tipy; /* 水平回羽 */
        a2x = tipx;
        a2y = tipy - len; /* 垂直回羽 */
    } else {
        /* 方向 (1,-1)/√2；尖在中点朝右上 */
        tipx = cx + len;
        tipy = cy - len;
        a1x = tipx - len;
        a1y = tipy; /* 水平回羽 */
        a2x = tipx;
        a2y = tipy + len; /* 垂直回羽 */
    }
    plot_seg(parent, tipx, tipy, a1x, a1y, sz, color);
    plot_seg(parent, tipx, tipy, a2x, a2y, sz, color);
}

/* 生成一条对角线引导组（container）：半透明高亮带(可滑动区间) + 细中线 + 画线式箭头。
 * down=1 为 ↘(0,0)->(w,h)，down=0 为 ↗(0,h)->(w,0)。返回 container 句柄。 */
static lv_obj_t* create_guide_group(page_touch_test_data_t* data, int down, lv_coord_t width, lv_coord_t height)
{
    lv_obj_t* grp;

    if (data == NULL || data->touch_area == NULL || width <= 2 || height <= 2) {
        return NULL;
    }

    grp = lv_obj_create(data->touch_area);
    lv_obj_remove_style_all(grp);
    lv_obj_set_size(grp, width, height);
    lv_obj_set_pos(grp, 0, 0);
    lv_obj_add_flag(grp, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(grp, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(grp, LV_OBJ_FLAG_SCROLLABLE);

    plot_band(grp, down, width, height); /* 半透明高亮带 = ±45px 可滑动区间 */
    plot_centerline(grp, down, width, height); /* 细中线 = 理想对角线 */
    make_arrow(grp, down, width, height); /* 画线式方向箭头 */

    return grp;
}

/* 按当前 stage 显隐两条引导组：等左→只显左；等右→只显右；通过→都隐 */
static void set_stage_guides(page_touch_test_data_t* data)
{
    int show_down;
    int show_up;

    if (data == NULL) {
        return;
    }
    show_down = (data->stage == TT_STAGE_WAIT_DOWN);
    show_up = (data->stage == TT_STAGE_WAIT_UP);

    if (data->guide_down_grp != NULL) {
        if (show_down) {
            lv_obj_clear_flag(data->guide_down_grp, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(data->guide_down_grp, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (data->guide_up_grp != NULL) {
        if (show_up) {
            lv_obj_clear_flag(data->guide_up_grp, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(data->guide_up_grp, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void update_hint_by_stage(page_touch_test_data_t* data)
{
    if (data == NULL || data->hint == NULL) {
        return;
    }
    switch (data->stage) {
    case TT_STAGE_WAIT_DOWN:
        lv_label_set_text(data->hint, "请沿左上→右下对角线滑动");
        break;
    case TT_STAGE_WAIT_UP:
        lv_label_set_text(data->hint, "很好，再沿左下→右上对角线滑动");
        break;
    default:
        lv_label_set_text(data->hint, "触摸测试通过");
        break;
    }
}

static int get_touch_local_point(page_touch_test_data_t* data, lv_point_t* out_local)
{
    lv_indev_t* indev;
    lv_area_t area;
    lv_point_t p;

    if (data == NULL || out_local == NULL || data->touch_area == NULL) {
        return -1;
    }

    indev = lv_indev_get_act();
    if (indev == NULL) {
        return -1;
    }
    lv_indev_get_point(indev, &p);
    lv_obj_get_coords(data->touch_area, &area);

    if (p.x < area.x1 || p.x > area.x2 || p.y < area.y1 || p.y > area.y2) {
        return -1;
    }

    out_local->x = p.x - area.x1;
    out_local->y = p.y - area.y1;
    return 0;
}

static int max_int(int a, int b)
{
    return (a > b) ? a : b;
}

static int abs_int(int x)
{
    return (x >= 0) ? x : -x;
}

/* 点(x,y)是否在某条对角线的 ±BAND 容差带内（全整型平方比较，避免 sqrt）。
 * down=1: ↘ 线 过(0,0)-(w,h)，法式 h*x - w*y = 0
 * down=0: ↗ 线 过(0,h)-(w,0)，法式 h*x + w*y - w*h = 0
 * 点到直线距离 d = |num| / sqrt(w^2+h^2)，d<=BAND  <=>  num^2 <= BAND^2*(w^2+h^2) */
static int point_in_diag_band(int down, int x, int y, int w, int h)
{
    long num;
    long lhs;
    long rhs;

    if (down) {
        num = (long)h * x - (long)w * y;
    } else {
        num = (long)h * x + (long)w * y - (long)w * h;
    }
    lhs = num * num;
    rhs = (long)TT_TOUCH_BAND_PX * TT_TOUCH_BAND_PX * ((long)w * w + (long)h * h);
    return (lhs <= rhs) ? 1 : 0;
}

/* 点是否落在某角的「距端 25% 区」：cx/cy 传该角期望的 0 或 1（左=0右=1、上=0下=1）。 */
static int point_in_corner_zone(int x, int y, int w, int h, int right, int bottom)
{
    int zx = (w * TT_TOUCH_END_ZONE_PCT) / 100;
    int zy = (h * TT_TOUCH_END_ZONE_PCT) / 100;
    int okx = right ? (x >= w - zx) : (x <= zx);
    int oky = bottom ? (y >= h - zy) : (y <= zy);
    return (okx && oky) ? 1 : 0;
}

static void draw_touch_segment(page_touch_test_data_t* data, lv_point_t from, lv_point_t to)
{
    int dx;
    int dy;
    int step;
    int i;

    if (data == NULL) {
        return;
    }

    dx = to.x - from.x;
    dy = to.y - from.y;
    step = max_int(abs_int(dx), abs_int(dy)) / 4;
    if (step <= 0) {
        add_trace_dot(data, to.x, to.y);
        return;
    }

    for (i = 1; i <= step; ++i) {
        lv_coord_t x = from.x + (lv_coord_t)(dx * i / step);
        lv_coord_t y = from.y + (lv_coord_t)(dy * i / step);
        add_trace_dot(data, x, y);
    }
}

static void touch_mark_passed(page_touch_test_data_t* data)
{
    if (data == NULL || data->touch_passed) {
        return;
    }
    data->touch_passed = 1U;
    update_overall_result(data);
}

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

static void back_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_manager_back();
}

/* MENU 键短按/长按均返回上一页。
 * 用 CLICK + LONG_PRESS 而非 PRESS：PRESS 在按下瞬间就返回，松手时 CLICK
 * 再分发一次会连退两级。CLICK 与 LONG_PRESS 在 key_manager 里互斥（松手发
 * CLICK 的前提是未触发长按），故两者同注册不会重复触发，短按走 CLICK、
 * 长按走 LONG_PRESS，各触发一次。 */
static void menu_key_event_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    LV_UNUSED(event_type);
    LV_UNUSED(user_data);
    if (key == KEY_ID_MENU) {
        page_manager_back();
    }
}

static void register_menu_key(page_touch_test_data_t* data)
{
    if (data == NULL || data->menu_key_registered) {
        return;
    }
    (void)key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_event_cb, data);
    (void)key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_event_cb, data);
    data->menu_key_registered = 1U;
}

static void unregister_menu_key(page_touch_test_data_t* data)
{
    if (data == NULL || !data->menu_key_registered) {
        return;
    }
    (void)key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_event_cb, data);
    (void)key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_event_cb, data);
    data->menu_key_registered = 0U;
}

static void touch_area_event_cb(lv_event_t* e)
{
    page_touch_test_data_t* data = (page_touch_test_data_t*)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_point_t local_point;
    int down; /* 当前目标线是否为 ↘ */
    int w = TT_CONTENT_W;
    int h = TT_CONTENT_H;

    if (data == NULL || data->touch_area == NULL) {
        return;
    }

    if (code == LV_EVENT_PRESSED) {
        if (get_touch_local_point(data, &local_point) != 0) {
            return;
        }
        /* 已全部通过后再按下，视为重新开始一轮 */
        if (data->stage == TT_STAGE_PASSED) {
            data->stage = TT_STAGE_WAIT_DOWN;
            data->touch_passed = 0U;
            set_stage_guides(data);
            update_hint_by_stage(data);
            update_overall_result(data);
        }
        data->touch_tracking = 1U;
        data->last_touch_local = local_point;
        clear_touch_trace(data); /* 每次重按都清上一笔（含已过），行为统一 */

        down = (data->stage == TT_STAGE_WAIT_DOWN);
        /* 起点须落在起角：↘ 起角=左上(0,0)，↗ 起角=左下(0,h) */
        data->stroke_start_ok = (uint8_t)point_in_corner_zone(local_point.x, local_point.y, w, h, 0, down ? 0 : 1);
        /* 起点是否已在带内 */
        data->stroke_in_band = (uint8_t)point_in_diag_band(down, local_point.x, local_point.y, w, h);

        add_trace_dot(data, local_point.x, local_point.y);
        return;
    }

    if (code == LV_EVENT_PRESSING) {
        if (!data->touch_tracking) {
            return;
        }
        if (get_touch_local_point(data, &local_point) != 0) {
            return;
        }
        if (data->stage == TT_STAGE_PASSED) {
            return;
        }

        draw_touch_segment(data, data->last_touch_local, local_point);
        data->last_touch_local = local_point;

        down = (data->stage == TT_STAGE_WAIT_DOWN);
        /* 一旦越出容差带，本笔作废 */
        if (!point_in_diag_band(down, local_point.x, local_point.y, w, h)) {
            data->stroke_in_band = 0U;
        }

        /* 到达终角(↘ 右下 / ↗ 右上) 且 全程在带内 且 起点合规 → 本条线通过 */
        if (data->stroke_in_band && data->stroke_start_ok
            && point_in_corner_zone(local_point.x, local_point.y, w, h, 1, down ? 1 : 0)) {
            if (data->stage == TT_STAGE_WAIT_DOWN) {
                data->stage = TT_STAGE_WAIT_UP;
                set_stage_guides(data); /* 隐藏左对角、显示右对角引导线 */
                clear_touch_trace(data); /* 左对角通过后立即清掉用户划的轨迹线 */
                data->touch_tracking = 0U; /* 本笔结束，避免继续把右对角误判进这一笔 */
                update_hint_by_stage(data);
            } else { /* TT_STAGE_WAIT_UP */
                data->stage = TT_STAGE_PASSED;
                set_stage_guides(data); /* 全部通过，两条引导线都隐藏 */
                clear_touch_trace(data); /* 通过后清掉用户划的轨迹线 */
                update_hint_by_stage(data);
                touch_mark_passed(data);
            }
        }
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        data->touch_tracking = 0U;
        /* 本笔未推进 stage（越界/没走完）→ 保持当前 stage，下次重按清屏重画当前线 */
    }
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_touch_test_create(void)
{
    page_touch_test_data_t* data = (page_touch_test_data_t*)malloc(sizeof(page_touch_test_data_t));
    lv_obj_t* top_bar;
    lv_obj_t* back_btn;
    lv_obj_t* back_icon;
    lv_obj_t* hint;

    if (data == NULL) {
        return;
    }
    memset(data, 0, sizeof(page_touch_test_data_t));

    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(data->container, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->container, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(data->container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_refr_size(data->container);

    top_bar = lv_obj_create(data->container);
    lv_obj_set_size(top_bar, LV_PCT(100), TT_TOP_BAR_HEIGHT);
    lv_obj_add_style(top_bar, &style_noboarder, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(top_bar, LV_SCROLLBAR_MODE_OFF);

    back_btn = lv_btn_create(top_bar);
    lv_obj_set_size(back_btn, 50, 50);
    lv_obj_add_style(back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle-white.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    data->title_label = lv_label_create(top_bar);
    lv_label_set_text(data->title_label, "触摸测试");
    lv_obj_add_style(data->title_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->title_label, LV_ALIGN_LEFT_MID, 70, 0);

    data->summary_label = lv_label_create(top_bar);
    lv_label_set_text(data->summary_label, "结果: WAITING");
    lv_obj_add_style(data->summary_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->summary_label, LV_ALIGN_RIGHT_MID, -16, 0);

    data->touch_area = lv_obj_create(data->container);
    lv_obj_set_size(data->touch_area, TT_TOUCH_AREA_WIDTH, TT_TOUCH_AREA_HEIGHT);
    lv_obj_align(data->touch_area, LV_ALIGN_TOP_MID, 0, TT_TOP_BAR_HEIGHT + TT_TOUCH_AREA_TOP_GAP);
    lv_obj_set_style_bg_color(data->touch_area, lv_color_hex(0x121212), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->touch_area, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(data->touch_area, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_set_style_border_width(data->touch_area, TT_TOUCH_AREA_BORDER, LV_PART_MAIN);
    lv_obj_set_style_radius(data->touch_area, 8, LV_PART_MAIN);
    /* 清零内边距，使 set_pos 的引导点/轨迹点以内容区(=边框内侧)为原点，虚线正好贴四角 */
    lv_obj_set_style_pad_all(data->touch_area, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(data->touch_area, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(data->touch_area, LV_OBJ_FLAG_SCROLLABLE);
    /* 按住即滑时，indev 会因快速位移重新命中测试，把 touch_area 的按压判为"丢失"(PRESS_LOST)，
     * 导致 touch_tracking 被清、后续 PRESSING 被忽略、划线中断。PRESS_LOCK 让按压锁定在
     * touch_area 上，即使手指滑过内部子对象(轨迹点)也不改判，PRESSING 持续下发。 */
    lv_obj_add_flag(data->touch_area, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(data->touch_area, touch_area_event_cb, LV_EVENT_PRESSED, data);
    lv_obj_add_event_cb(data->touch_area, touch_area_event_cb, LV_EVENT_PRESSING, data);
    lv_obj_add_event_cb(data->touch_area, touch_area_event_cb, LV_EVENT_RELEASED, data);
    lv_obj_add_event_cb(data->touch_area, touch_area_event_cb, LV_EVENT_PRESS_LOST, data);
    /* 两条对角线各成一个引导组(中线实线+两侧±60px 边界虚线+中点箭头)，按内容区尺寸铺点。
     * 依 stage 显隐：等左→只显 ↘、等右→只显 ↗、通过→都隐。 */
    data->guide_down_grp = create_guide_group(data, 1, TT_CONTENT_W, TT_CONTENT_H);
    data->guide_up_grp = create_guide_group(data, 0, TT_CONTENT_W, TT_CONTENT_H);
    set_stage_guides(data);

    hint = lv_label_create(data->touch_area);
    lv_label_set_text(hint, "请沿左上→右下对角线滑动");
    lv_obj_add_style(hint, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x777777), LV_PART_MAIN);
    lv_obj_add_flag(hint, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 8);
    data->hint = hint;

    update_overall_result(data);
    page_set_private_data(data);
}

void page_touch_test_destroy(void)
{
    page_touch_test_data_t* data = page_get_private_data();

    if (data == NULL) {
        return;
    }
    if (data->auto_sleep_disabled) {
        power_manager_enable_auto_sleep();
        data->auto_sleep_disabled = 0U;
    }
    unregister_menu_key(data);
    clear_touch_trace(data);
    if (data->container != NULL) {
        lv_obj_del(data->container);
        data->container = NULL;
    }
    free(data);
}

void page_touch_test_show(void)
{
    page_touch_test_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Touch test page show");
    if (!data->auto_sleep_disabled) {
        power_manager_disable_auto_sleep();
        data->auto_sleep_disabled = 1U;
    }
    reset_test_state(data);
    register_menu_key(data);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_touch_test_hide(void)
{
    page_touch_test_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Touch test page hide");
    unregister_menu_key(data);
    if (data->auto_sleep_disabled) {
        power_manager_enable_auto_sleep();
        data->auto_sleep_disabled = 0U;
    }
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_touch_test_update(void)
{
    page_touch_test_data_t* data = page_get_private_data();

    if (data == NULL) {
        return;
    }
    update_overall_result(data);
}

// #endregion
