// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_ai_photo.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/page_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <stdlib.h>
#include <string.h>

/* 布局常量 */
#define TOP_BAR_HEIGHT 50
#define BOTTOM_BAR_HEIGHT 50

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

/* 返回按钮回调：返回上一页 */
static void filter_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    MLOG_INFO("Filter button clicked");
}

/* 菜单按钮回调：跳转AI拍照设置页面 */
static void menu_back_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    MLOG_INFO("Menu clicked, navigate to ai_photo_settings");
    page_manager_navigate("ai_photo_settings");
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

    /* 创建页面容器 */
    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_width(data->container, LV_PCT(100));
    lv_obj_set_height(data->container, LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_refr_size(data->container);

    /* 启用滑动手势检测，不将 GESTURE 事件传递给父控件 */
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    /* 添加滑动手势回调：从左往右滑返回上一页 */
    lv_obj_add_event_cb(data->container, page_manager_swipe_right_cb, LV_EVENT_GESTURE, NULL);

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
    lv_obj_add_event_cb(data->back_btn, page_manager_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->back_btn, LV_ALIGN_TOP_LEFT, 10, 0);
    lv_obj_t* back_icon = lv_img_create(data->back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    /* AI标签 - 跟在返回按钮后面 */
    data->resolution_label = lv_label_create(data->top_bar);
    lv_label_set_text(data->resolution_label, "AI拍照");
    lv_obj_add_style(data->resolution_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->resolution_label, lv_color_black(), LV_PART_MAIN);
    lv_obj_align(data->resolution_label, LV_ALIGN_LEFT_MID, 70, 0);

    /* 剩余照片数量 Label - 右上角 */
    data->photo_count_label = lv_label_create(data->top_bar);
    lv_label_set_text(data->photo_count_label, "100");
    lv_obj_add_style(data->photo_count_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->photo_count_label, lv_color_black(), LV_PART_MAIN);
    lv_obj_align(data->photo_count_label, LV_ALIGN_RIGHT_MID, -170, 0);

    /* SD卡图标 - 右上角 */
    data->sd_icon = lv_img_create(data->top_bar);
    lv_img_set_src(data->sd_icon, "A:" RES_ICON_PATH "/sd-card.png");
    lv_obj_align(data->sd_icon, LV_ALIGN_RIGHT_MID, -120, 0);

    /* WiFi图标 - SD卡和电池之间 */
    data->wifi_icon = lv_img_create(data->top_bar);
    lv_img_set_src(data->wifi_icon, "A:" RES_ICON_PATH "/wifi.png");
    lv_obj_align(data->wifi_icon, LV_ALIGN_RIGHT_MID, -65, 0);

    /* 电池图标 - 最右上角 */
    data->battery_icon = lv_img_create(data->top_bar);
    lv_img_set_src(data->battery_icon, "A:" RES_ICON_PATH "/battery33%.png");
    lv_obj_align(data->battery_icon, LV_ALIGN_RIGHT_MID, -10, 0);

    /* =======================
     * 底部工具栏：[filter][...] [switch][menu]
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

    /* 摄像头切换按钮 - 右对齐 */
    data->switch_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->switch_btn, 50, 50);
    lv_obj_add_style(data->switch_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->switch_btn, NULL, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->switch_btn, LV_ALIGN_RIGHT_MID, -70, 0);
    lv_obj_t* switch_icon = lv_img_create(data->switch_btn);
    lv_img_set_src(switch_icon, "A:" RES_ICON_PATH "/switch.png");
    lv_obj_align(switch_icon, LV_ALIGN_CENTER, 0, 0);

    /* 菜单按钮 - 最右侧 */
    data->menu_btn = lv_btn_create(data->bottom_bar);
    lv_obj_set_size(data->menu_btn, 50, 50);
    lv_obj_add_style(data->menu_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->menu_btn, menu_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(data->menu_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_t* menu_icon = lv_img_create(data->menu_btn);
    lv_img_set_src(menu_icon, "A:" RES_ICON_PATH "/menu.png");
    lv_obj_align(menu_icon, LV_ALIGN_CENTER, 0, 0);

    /* 保存 private_data，供 show/hide/destroy 使用 */
    page_set_private_data(data);
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

    free(data);
}

void page_ai_photo_show(void)
{
    page_ai_photo_data_t* data = page_get_private_data();
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("AI Photo page show");
    /* 显示 UI */
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_ai_photo_hide(void)
{
    page_ai_photo_data_t* data = page_get_private_data();
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("AI Photo page hide");
    /* 隐藏 UI */
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_ai_photo_update(void)
{
    /* no-op */
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
