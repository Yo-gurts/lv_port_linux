// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_ai_photo.h"
#include "config.h"
#include "core/file_manager.h"
#include "core/font_manager.h"
#include "core/key_manager.h"
#include "core/media_manager.h"
#include "core/page_manager.h"
#include "core/param_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include "ui/filter_panel.h"
#include "ui/gesture_back.h"
#include "ui/status_bar.h"
#include "ui/zoom_bar.h"
#include <stdlib.h>
#include <string.h>

/* 布局常量 */
#define TOP_BAR_HEIGHT 50
#define BOTTOM_BAR_HEIGHT 50
#define FOCUS_BOX_WIDTH 120
#define FOCUS_BOX_HEIGHT 120
#define FOCUS_BOX_BORDER_WIDTH 3
#define FOCUS_CORNER_LEN 30

/* AI模式选项 - 与ai_photo_settings保持一致 */
static const char* ai_mode_options[] = {
    "风格变换", "AI识万物", "拍照翻译"
};

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_ai_photo.h)
// #############################################################################

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

static void update_photo_count_display(page_ai_photo_data_t* data)
{
    uint32_t remaining = 0U;
    char text_buf[16];
    int resolution_index;
    int quality_index;

    if (data == NULL || data->photo_count_label == NULL) {
        return;
    }

    if (param_manager_get(PARAM_ID_SD_READY) != SD_READY_TRUE) {
        lv_label_set_text(data->photo_count_label, "0");
        return;
    }

    resolution_index = param_manager_get(PARAM_ID_RESOLUTION);
    quality_index = param_manager_get(PARAM_ID_QUALITY);
    (void)file_manager_get_remaining_photo_count(resolution_index, quality_index, &remaining);
    lv_snprintf(text_buf, sizeof(text_buf), "%u", (unsigned int)remaining);
    lv_label_set_text(data->photo_count_label, text_buf);
}

static void update_focus_box_display(page_ai_photo_data_t* data, focus_frame_state_t state)
{
    lv_color_t border_color;
    int i;

    if (data == NULL || data->focus_box == NULL) {
        return;
    }

    if (state == FOCUS_FRAME_STATE_HIDDEN) {
        lv_obj_add_flag(data->focus_box, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_clear_flag(data->focus_box, LV_OBJ_FLAG_HIDDEN);
    border_color = (state == FOCUS_FRAME_STATE_LOCKING) ? lv_color_hex(0xF05A28) : lv_color_hex(0xF09F20);

    for (i = 0; i < 4; i++) {
        if (data->focus_corners[i] == NULL) {
            continue;
        }
        lv_obj_set_style_border_color(data->focus_corners[i], border_color, LV_PART_MAIN);
    }
}

static void ai_photo_param_cb(param_id_t id, int value, void* user_data)
{
    page_ai_photo_data_t* data = (page_ai_photo_data_t*)user_data;

    if (data == NULL) {
        return;
    }

    if (id == PARAM_ID_AI_MODE) {
        if (data->resolution_label != NULL) {
            lv_label_set_text(data->resolution_label, ai_mode_options[value]);
        }
        return;
    }

    if (id == PARAM_ID_RESOLUTION || id == PARAM_ID_QUALITY || id == PARAM_ID_SD_READY) {
        LV_UNUSED(value);
        update_photo_count_display(data);
        return;
    }

    if (id == PARAM_ID_FOCUS_FRAME_STATE) {
        update_focus_box_display(data, (focus_frame_state_t)value);
    }
}

static int filter_panel_consume_non_lr_key(key_id_t key)
{
    if (!filter_panel_is_visible()) {
        return 0;
    }
    if (key == KEY_ID_LEFT || key == KEY_ID_RIGHT) {
        return 0;
    }

    filter_panel_hide();
    return 1;
}

static void take_photo_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_ai_photo_data_t* data = (page_ai_photo_data_t*)user_data;
    int ret;
    int ai_mode;

    if (key != KEY_ID_CAMERA || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (!data || !data->container) {
        return;
    }
    if (lv_obj_has_flag(data->container, LV_OBJ_FLAG_HIDDEN)) {
        return;
    }
    if (filter_panel_consume_non_lr_key(key)) {
        return;
    }

    ret = media_manager_execute(MEDIA_OP_TAKE_PHOTO, 0);
    if (ret != 0) {
        MLOG_ERR("AI take photo failed: ret=%d", ret);
        return;
    }
    update_photo_count_display(data);

    ai_mode = param_manager_get(PARAM_ID_AI_MODE);
    if (ai_mode == AI_MODE_STYLE_TRANSFER) {
        page_manager_navigate("ai_style_preview");
    } else if (ai_mode == AI_MODE_OBJECT_RECOGNITION) {
        page_manager_navigate("ai_recognition_preview");
    }
}

static void focus_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_ai_photo_data_t* data = (page_ai_photo_data_t*)user_data;
    int ret = 0;

    if (key != KEY_ID_FOCUS) {
        return;
    }
    if (!data || !data->container) {
        return;
    }
    if (lv_obj_has_flag(data->container, LV_OBJ_FLAG_HIDDEN)) {
        return;
    }
    if (filter_panel_consume_non_lr_key(key)) {
        return;
    }

    if (event_type == KEY_EVENT_PRESS) {
        if (data->focus_locked) {
            /* 锁定态下再次按下：解除对焦锁定，恢复自动对焦并隐藏对焦框，本次不做单次对焦 */
            ret = media_manager_execute(MEDIA_OP_SET_FOCUS_ENABLE, 1);
            if (ret != 0) {
                MLOG_ERR("AI unlock AF by key press failed: ret=%d", ret);
            }
            data->focus_locked = 0;
            data->focus_press_consumed = 1;
            (void)param_manager_set(PARAM_ID_FOCUS_FRAME_STATE, FOCUS_FRAME_STATE_HIDDEN);
            return;
        }
        ret = media_manager_execute(MEDIA_OP_FOCUS_ONCE, 0);
        if (ret != 0) {
            MLOG_ERR("AI focus by key failed: ret=%d", ret);
        }
        (void)param_manager_set(PARAM_ID_FOCUS_FRAME_STATE, FOCUS_FRAME_STATE_NORMAL);
    } else if (event_type == KEY_EVENT_LONG_PRESS_3S) {
        ret = media_manager_execute(MEDIA_OP_SET_FOCUS_ENABLE, 0);
        if (ret != 0) {
            MLOG_ERR("AI disable AF by key long-press failed: ret=%d", ret);
        }
        data->focus_locked = 1;
        (void)param_manager_set(PARAM_ID_FOCUS_FRAME_STATE, FOCUS_FRAME_STATE_LOCKING);
    } else if (event_type == KEY_EVENT_RELEASE) {
        if (data->focus_press_consumed) {
            /* 本次按下已在 PRESS 解锁并发过 enable，松开不再重复发，避免 AF 正忙时报错 */
            data->focus_press_consumed = 0;
            return;
        }
        if (data->focus_locked) {
            /* 已锁定：松开不解锁，保持锁定状态与对焦框 */
            return;
        }
        ret = media_manager_execute(MEDIA_OP_SET_FOCUS_ENABLE, 1);
        if (ret != 0) {
            MLOG_ERR("AI enable AF by key release failed: ret=%d", ret);
        }
        (void)param_manager_set(PARAM_ID_FOCUS_FRAME_STATE, FOCUS_FRAME_STATE_HIDDEN);
    }
}

static void ai_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_ai_photo_data_t* data = (page_ai_photo_data_t*)user_data;

    if (key != KEY_ID_ASSISTANT || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (!data || !data->container) {
        return;
    }
    if (lv_obj_has_flag(data->container, LV_OBJ_FLAG_HIDDEN)) {
        return;
    }

    if (filter_panel_is_visible()) {
        filter_panel_hide();
        return;
    }

    filter_panel_show();
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

static void menu_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)user_data;

    if (key != KEY_ID_MENU || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (filter_panel_consume_non_lr_key(key)) {
        return;
    }

    filter_panel_hide();
    page_manager_navigate("ai_photo_settings");
}

static void menu_key_long_press_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)key;
    (void)event_type;
    (void)user_data;
    page_manager_back();
}

/* VOLUME_UP Click：放大变焦 */
static void zoom_in_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_ai_photo_data_t* data = (page_ai_photo_data_t*)user_data;

    if (key != KEY_ID_VOLUME_UP || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (!data || !data->container || lv_obj_has_flag(data->container, LV_OBJ_FLAG_HIDDEN)) {
        return;
    }
    if (filter_panel_consume_non_lr_key(key)) {
        return;
    }

    zoom_bar_zoom_in();
}

/* VOLUME_DOWN Click：缩小变焦 */
static void zoom_out_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_ai_photo_data_t* data = (page_ai_photo_data_t*)user_data;

    if (key != KEY_ID_VOLUME_DOWN || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (!data || !data->container || lv_obj_has_flag(data->container, LV_OBJ_FLAG_HIDDEN)) {
        return;
    }
    if (filter_panel_consume_non_lr_key(key)) {
        return;
    }

    zoom_bar_zoom_out();
}

static void left_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)user_data;

    if (key != KEY_ID_LEFT || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (!filter_panel_is_visible()) {
        return;
    }

    filter_panel_select_prev();
}

static void right_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)user_data;

    if (key != KEY_ID_RIGHT || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (!filter_panel_is_visible()) {
        return;
    }

    filter_panel_select_next();
}

/* 返回按钮回调：返回上一页 */
static void filter_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);

    if (filter_panel_is_visible()) {
        filter_panel_hide();
        return;
    }

    filter_panel_show();
}

/* 菜单按钮回调：跳转AI拍照设置页面 */
static void menu_back_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    filter_panel_hide();
    MLOG_INFO("Menu clicked, navigate to ai_photo_settings");
    page_manager_navigate("ai_photo_settings");
}

/* 返回按钮回调：返回上一页前先切回 BOOT 模式，拆除 photo 媒体管线（对齐普通拍照页） */
static void back_btn_cb(lv_event_t* e)
{
    page_ai_photo_data_t* data = page_get_private_data();

    LV_UNUSED(e);

    /* 离开 AI 拍照页前解除对焦锁定：下方 SWITCH_TO_BOOT_MODE 会重建 pipeline 并使能硬件 AF，
     * 这里同步清掉锁定标志与对焦框状态，避免锁定态残留到下次进入本页。 */
    if (data && data->focus_locked) {
        data->focus_locked = 0;
        (void)param_manager_set(PARAM_ID_FOCUS_FRAME_STATE, FOCUS_FRAME_STATE_HIDDEN);
    }

    filter_panel_hide();
    (void)media_manager_execute(MEDIA_OP_SWITCH_TO_BOOT_MODE, 0);
    page_manager_back();
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_ai_photo_create(void)
{
    page_ai_photo_data_t* data = (page_ai_photo_data_t*)malloc(sizeof(page_ai_photo_data_t));
    if (!data) {
        return;
    }

    memset(data, 0, sizeof(page_ai_photo_data_t));

    /* 初始化全局滤镜面板（单例）。 */
    filter_panel_init();
    zoom_bar_init();

    /* 创建页面容器 */
    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_width(data->container, LV_PCT(100));
    lv_obj_set_height(data->container, LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_refr_size(data->container);

    /* 启用滑动手势检测，不将 GESTURE 事件传递给父控件 */
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    /* 添加滑动手势回调：从左往右滑返回上一页 */
    gesture_back_register_events(data->container);
    gesture_back_set_left_edge_swipe_cb(data->container, back_btn_cb);

    /* 中央对焦框（默认隐藏），由 param 回调驱动显示/颜色切换。 */
    data->focus_box = lv_obj_create(data->container);
    lv_obj_set_size(data->focus_box, FOCUS_BOX_WIDTH, FOCUS_BOX_HEIGHT);
    lv_obj_align(data->focus_box, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(data->focus_box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_scrollbar_mode(data->focus_box, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(data->focus_box, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->focus_box, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(data->focus_box, 0, LV_PART_MAIN);

    data->focus_corners[0] = lv_obj_create(data->focus_box); /* top-left */
    data->focus_corners[1] = lv_obj_create(data->focus_box); /* top-right */
    data->focus_corners[2] = lv_obj_create(data->focus_box); /* bottom-left */
    data->focus_corners[3] = lv_obj_create(data->focus_box); /* bottom-right */

    int i;
    for (i = 0; i < 4; i++) {
        lv_obj_t* corner = data->focus_corners[i];
        lv_obj_set_size(corner, FOCUS_CORNER_LEN, FOCUS_CORNER_LEN);
        lv_obj_set_scrollbar_mode(corner, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(corner, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_opa(corner, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_radius(corner, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(corner, FOCUS_BOX_BORDER_WIDTH, LV_PART_MAIN);
        lv_obj_set_style_border_opa(corner, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_pad_all(corner, 0, LV_PART_MAIN);
    }

    lv_obj_set_style_border_side(data->focus_corners[0], LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_LEFT, LV_PART_MAIN);
    lv_obj_set_style_border_side(data->focus_corners[1], LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_border_side(data->focus_corners[2], LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_LEFT, LV_PART_MAIN);
    lv_obj_set_style_border_side(data->focus_corners[3], LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_RIGHT, LV_PART_MAIN);

    lv_obj_align(data->focus_corners[0], LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_align(data->focus_corners[1], LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_align(data->focus_corners[2], LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_align(data->focus_corners[3], LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    update_focus_box_display(data, FOCUS_FRAME_STATE_HIDDEN);

    /* =======================
     * 顶部状态栏：[back][AI] 在左边，剩余拍照数 [SD][battery] 在右边
     * ======================= */
    data->top_bar = lv_obj_create(data->container);
    lv_obj_set_width(data->top_bar, lv_pct(100));
    lv_obj_set_height(data->top_bar, TOP_BAR_HEIGHT);
    lv_obj_add_style(data->top_bar, &style_noboarder, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(data->top_bar, LV_SCROLLBAR_MODE_OFF);

    /* 返回按钮 - 左上角 */
    data->back_btn = lv_btn_create(data->top_bar);
    lv_obj_set_size(data->back_btn, 50, 50);
    lv_obj_add_style(data->back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->back_btn, LV_ALIGN_TOP_LEFT, 10, 0);
    lv_obj_t* back_icon = lv_img_create(data->back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    /* AI标签 - 跟在返回按钮后面 */
    data->resolution_label = lv_label_create(data->top_bar);
    lv_label_set_text(data->resolution_label, "AI拍照");
    lv_obj_add_style(data->resolution_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->resolution_label, LV_ALIGN_LEFT_MID, 70, 0);

    /* 剩余照片数量 Label - 右上角 */
    data->photo_count_label = lv_label_create(data->top_bar);
    lv_label_set_text(data->photo_count_label, "0");
    lv_obj_add_style(data->photo_count_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->photo_count_label, LV_ALIGN_RIGHT_MID, -180, 0);
    update_photo_count_display(data);

    /* =======================
     * 底部工具栏：[filter][...] [menu]
     * ======================= */
    data->bottom_bar = lv_obj_create(data->container);
    lv_obj_set_width(data->bottom_bar, lv_pct(100));
    lv_obj_set_height(data->bottom_bar, BOTTOM_BAR_HEIGHT);
    lv_obj_add_style(data->bottom_bar, &style_noboarder, LV_PART_MAIN);
    lv_obj_align(data->bottom_bar, LV_ALIGN_BOTTOM_MID, 0, -5); /* 底部向上5像素 */
    lv_obj_set_scrollbar_mode(data->bottom_bar, LV_SCROLLBAR_MODE_OFF); /* 不可滚动 */

    /* 滤镜按钮 - 左对齐（原拍照/录像切换按钮位置） */
    data->filter_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->filter_btn, 50, 50);
    lv_obj_add_style(data->filter_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->filter_btn, filter_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->filter_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_t* filter_icon = lv_img_create(data->filter_btn);
    lv_img_set_src(filter_icon, "A:" RES_ICON_PATH "/filter_default.png");
    lv_obj_align(filter_icon, LV_ALIGN_CENTER, 0, 0);

    /* 菜单按钮 - 最右侧 */
    data->menu_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->menu_btn, 50, 50);
    lv_obj_add_style(data->menu_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->menu_btn, menu_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->menu_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_t* menu_icon = lv_img_create(data->menu_btn);
    lv_img_set_src(menu_icon, "A:" RES_ICON_PATH "/menu.png");
    lv_obj_align(menu_icon, LV_ALIGN_CENTER, 0, 0);

    /* 启用整页事件冒泡，确保子对象按压事件传递到父容器。 */
    gesture_back_enable_event_bubble_recursive(data->container);

    /* 保存 private_data，供 show/hide/destroy 使用 */
    page_set_private_data(data);
    param_manager_register_callback(ai_photo_param_cb, data);
}

void page_ai_photo_destroy(void)
{
    page_ai_photo_data_t* data = page_get_private_data();
    if (!data) {
        return;
    }

    /* 删除容器（子元素会自动删除） */
    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    (void)key_manager_unregister_callback(KEY_ID_CAMERA, KEY_EVENT_CLICK, take_photo_key_cb, data);
    (void)key_manager_unregister_callback(KEY_ID_FOCUS, KEY_EVENT_PRESS, focus_key_cb, data);
    (void)key_manager_unregister_callback(KEY_ID_FOCUS, KEY_EVENT_RELEASE, focus_key_cb, data);
    (void)key_manager_unregister_callback(KEY_ID_FOCUS, KEY_EVENT_LONG_PRESS_3S, focus_key_cb, data);
    (void)key_manager_unregister_callback(KEY_ID_ASSISTANT, KEY_EVENT_CLICK, ai_key_cb, data);
    (void)key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    (void)key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_long_press_cb, NULL);
    (void)key_manager_unregister_callback(KEY_ID_VOLUME_UP, KEY_EVENT_CLICK, zoom_in_key_cb, data);
    (void)key_manager_unregister_callback(KEY_ID_VOLUME_DOWN, KEY_EVENT_CLICK, zoom_out_key_cb, data);
    (void)key_manager_unregister_callback(KEY_ID_LEFT, KEY_EVENT_CLICK, left_key_click_cb, NULL);
    (void)key_manager_unregister_callback(KEY_ID_RIGHT, KEY_EVENT_CLICK, right_key_click_cb, NULL);
    filter_panel_hide();
    param_manager_unregister_callback(ai_photo_param_cb);
    free(data);
}

void page_ai_photo_show(void)
{
    page_ai_photo_data_t* data = page_get_private_data();
    if (!data || !data->container) {
        return;
    }

    gesture_back_set_left_edge_swipe_cb(data->container, back_btn_cb);

    MLOG_INFO("AI Photo page show");
    filter_panel_hide();
    zoom_bar_show();
    /* 显示 UI */
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);

    /* 使用全局状态栏 */
    status_bar_show(true);
    status_bar_set_icons(STATUS_BAR_ICON_SD, STATUS_BAR_ICON_WIFI, STATUS_BAR_ICON_BATTERY);
    status_bar_refresh();
    update_photo_count_display(data);
    update_focus_box_display(data, (focus_frame_state_t)param_manager_get(PARAM_ID_FOCUS_FRAME_STATE));

    key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);

    if (key_manager_register_callback(KEY_ID_CAMERA, KEY_EVENT_CLICK, take_photo_key_cb, data) != 0) {
        MLOG_WARN("register ai photo key callback failed");
    }
    if (key_manager_register_callback(KEY_ID_FOCUS, KEY_EVENT_PRESS, focus_key_cb, data) != 0) {
        MLOG_WARN("register ai focus key callback failed");
    }
    if (key_manager_register_callback(KEY_ID_FOCUS, KEY_EVENT_RELEASE, focus_key_cb, data) != 0) {
        MLOG_WARN("register ai focus key release callback failed");
    }
    if (key_manager_register_callback(KEY_ID_FOCUS, KEY_EVENT_LONG_PRESS_3S, focus_key_cb, data) != 0) {
        MLOG_WARN("register ai focus key long-press callback failed");
    }
    if (key_manager_register_callback(KEY_ID_ASSISTANT, KEY_EVENT_CLICK, ai_key_cb, data) != 0) {
        MLOG_WARN("register ai key callback failed");
    }
    if (key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_long_press_cb, NULL) != 0) {
        MLOG_WARN("register menu key long press callback failed");
    }
    if (key_manager_register_callback(KEY_ID_VOLUME_UP, KEY_EVENT_CLICK, zoom_in_key_cb, data) != 0) {
        MLOG_WARN("register zoom in key callback failed");
    }
    if (key_manager_register_callback(KEY_ID_VOLUME_DOWN, KEY_EVENT_CLICK, zoom_out_key_cb, data) != 0) {
        MLOG_WARN("register zoom out key callback failed");
    }
    if (key_manager_register_callback(KEY_ID_LEFT, KEY_EVENT_CLICK, left_key_click_cb, NULL) != 0) {
        MLOG_WARN("register left key callback failed");
    }
    if (key_manager_register_callback(KEY_ID_RIGHT, KEY_EVENT_CLICK, right_key_click_cb, NULL) != 0) {
        MLOG_WARN("register right key callback failed");
    }
}

void page_ai_photo_hide(void)
{
    page_ai_photo_data_t* data = page_get_private_data();
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("AI Photo page hide");
    /* 隐藏 UI */
    filter_panel_hide();
    zoom_bar_hide();
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
    status_bar_show(false);
    (void)key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    (void)key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_long_press_cb, NULL);
    (void)key_manager_unregister_callback(KEY_ID_VOLUME_UP, KEY_EVENT_CLICK, zoom_in_key_cb, data);
    (void)key_manager_unregister_callback(KEY_ID_VOLUME_DOWN, KEY_EVENT_CLICK, zoom_out_key_cb, data);
    (void)key_manager_unregister_callback(KEY_ID_LEFT, KEY_EVENT_CLICK, left_key_click_cb, NULL);
    (void)key_manager_unregister_callback(KEY_ID_RIGHT, KEY_EVENT_CLICK, right_key_click_cb, NULL);
    (void)key_manager_unregister_callback(KEY_ID_CAMERA, KEY_EVENT_CLICK, take_photo_key_cb, data);
    (void)key_manager_unregister_callback(KEY_ID_FOCUS, KEY_EVENT_PRESS, focus_key_cb, data);
    (void)key_manager_unregister_callback(KEY_ID_FOCUS, KEY_EVENT_RELEASE, focus_key_cb, data);
    (void)key_manager_unregister_callback(KEY_ID_FOCUS, KEY_EVENT_LONG_PRESS_3S, focus_key_cb, data);
    (void)key_manager_unregister_callback(KEY_ID_ASSISTANT, KEY_EVENT_CLICK, ai_key_cb, data);
    (void)param_manager_set(PARAM_ID_FOCUS_FRAME_STATE, FOCUS_FRAME_STATE_HIDDEN);
}

void page_ai_photo_update(void)
{
    page_ai_photo_data_t* data = page_get_private_data();
    if (!data || !data->resolution_label) {
        return;
    }

    /* 从param_manager更新AI模式显示 */
    int ai_mode = param_manager_get(PARAM_ID_AI_MODE);
    lv_label_set_text(data->resolution_label, ai_mode_options[ai_mode]);
    update_photo_count_display(data);
    update_focus_box_display(data, (focus_frame_state_t)param_manager_get(PARAM_ID_FOCUS_FRAME_STATE));
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
