// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_chat.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/page_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include "ui/gesture_back.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 布局常量 */
#define BOTTOM_BAR_HEIGHT 70

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_chat.h)
// #############################################################################

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

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
static void volume_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    MLOG_INFO("Volume button clicked");
}

/* 按住说话按钮回调 */
static void voice_btn_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        MLOG_INFO("Voice button pressed");
    } else if (code == LV_EVENT_RELEASED) {
        MLOG_INFO("Voice button released");
    }
}

/* 音色按钮回调 */
static void timbre_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    MLOG_INFO("Timbre button clicked");
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_chat_create(void)
{
    page_chat_data_t* data = (page_chat_data_t*)malloc(sizeof(page_chat_data_t));
    if (!data) {
        return;
    }

    memset(data, 0, sizeof(page_chat_data_t));

    /* 创建页面容器 */
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
     * 消息列表区域 - 占满中间空间
     * ======================= */
    data->msg_container = lv_obj_create(data->container);
    lv_obj_set_width(data->msg_container, lv_pct(100));
    lv_obj_set_flex_grow(data->msg_container, 1);
    lv_obj_set_style_bg_opa(data->msg_container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->msg_container, lv_color_hex(0x121212), LV_PART_MAIN);
    lv_obj_set_layout(data->msg_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->msg_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(data->msg_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(data->msg_container, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(data->msg_container, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_top(data->msg_container, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(data->msg_container, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->msg_container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(data->msg_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(data->msg_container, LV_SCROLLBAR_MODE_OFF);

    /* =======================
     * 底部工具栏
     * ======================= */
    data->bottom_bar = lv_obj_create(data->container);
    lv_obj_set_width(data->bottom_bar, lv_pct(100));
    lv_obj_set_height(data->bottom_bar, BOTTOM_BAR_HEIGHT);
    lv_obj_clear_flag(data->bottom_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(data->bottom_bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(data->bottom_bar, &style_noboarder, LV_PART_MAIN);

    /* 返回按钮 - 左下角 */
    data->back_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->back_btn, 50, 50);
    lv_obj_add_style(data->back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->back_btn, page_manager_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_t* back_icon = lv_img_create(data->back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle-white.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    /* 音量按钮 - 返回按钮右边 */
    data->volume_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->volume_btn, 50, 50);
    lv_obj_add_style(data->volume_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->volume_btn, volume_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->volume_btn, LV_ALIGN_LEFT_MID, 70, 0);
    lv_obj_t* volume_icon = lv_img_create(data->volume_btn);
    lv_img_set_src(volume_icon, "A:" RES_ICON_PATH "/volume.png");
    lv_obj_align(volume_icon, LV_ALIGN_CENTER, 0, 0);

    /* 按住说话按钮 - 居中，大尺寸，圆角 */
    data->voice_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->voice_btn, 320, 60);
    lv_obj_add_style(data->voice_btn, &style_common_btn_back, LV_PART_MAIN);
    lv_obj_set_style_radius(data->voice_btn, 30, LV_PART_MAIN);
    lv_obj_add_event_cb(data->voice_btn, voice_btn_cb, LV_EVENT_PRESSED | LV_EVENT_RELEASED, NULL);
    lv_obj_align(data->voice_btn, LV_ALIGN_CENTER, 0, 0);

    /* 按住说话图标 */
    data->voice_icon = lv_img_create(data->voice_btn);
    lv_img_set_src(data->voice_icon, "A:" RES_ICON_PATH "/microphone.png");
    lv_obj_align(data->voice_icon, LV_ALIGN_LEFT_MID, 50, 0);

    /* 按住说话文字 */
    data->voice_label = lv_label_create(data->voice_btn);
    lv_label_set_text(data->voice_label, "按住说话");
    lv_obj_add_style(data->voice_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->voice_label, LV_ALIGN_CENTER, 20, 0);

    /* 音色按钮 - 右侧 */
    data->timbre_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->timbre_btn, 50, 50);
    lv_obj_add_style(data->timbre_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->timbre_btn, timbre_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->timbre_btn, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_t* timbre_icon = lv_img_create(data->timbre_btn);
    lv_img_set_src(timbre_icon, "A:" RES_ICON_PATH "/gender-female.png");
    lv_obj_align(timbre_icon, LV_ALIGN_CENTER, 0, 0);

    /* 启用整页事件冒泡，确保子对象按压事件传递到父容器。 */
    gesture_back_enable_event_bubble_recursive(data->container);

    /* 保存 private_data */
    page_set_private_data(data);
}

void page_chat_destroy(void)
{
    page_chat_data_t* data = page_get_private_data();
    if (!data) {
        return;
    }

    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    free(data);
}

void page_chat_show(void)
{
    page_chat_data_t* data = page_get_private_data();
    if (!data || !data->container) {
        return;
    }

    gesture_back_set_left_edge_swipe_cb(data->container, page_manager_back_cb);

    MLOG_INFO("Chat page show");
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_chat_hide(void)
{
    page_chat_data_t* data = page_get_private_data();
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("Chat page hide");
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_chat_update(void)
{
    /* no-op */
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
