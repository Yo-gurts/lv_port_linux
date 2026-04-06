// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_home.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/media_manager.h"
#include "core/param_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include "ui/status_bar.h"
#include "ui/top_notice.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 布局常量 - 可根据设计调整 */
#define STATUS_BAR_HEIGHT 30
#define ICON_BUTTON_PADDING 10 /* 图标按钮内边距 */
#define ICON_SIZE 64 /* 图标基础大小 */
#define LABEL_HEIGHT 24 /* 标签高度 */

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_home.h)
// #############################################################################

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

static void create_icon_button(lv_obj_t* parent,
    const char* symbol, const char* name,
    lv_align_t align, int x_ofs, int y_ofs,
    lv_event_cb_t cb, lv_obj_t** out_btn)
{
    /* 容器：图标 + 文字 */
    lv_obj_t* container = lv_btn_create(parent);
    lv_obj_align(container, align, x_ofs, y_ofs);
    lv_obj_set_width(container, lv_pct(30)); /* 占父容器30%宽度，自适应 */
    lv_obj_set_height(container, lv_pct(40)); /* 占剩余高度约40% */
    lv_obj_add_style(container, &style_common_btn_back, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(container, cb, LV_EVENT_CLICKED, NULL);

    /* 使用 flex 布局垂直排列图标和文字 */
    lv_obj_set_layout(container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(container, ICON_BUTTON_PADDING, LV_PART_MAIN);

    /* 图标 */
    lv_obj_t* icon = lv_label_create(container);
    lv_label_set_text(icon, symbol);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_add_style(icon, &style_common_label_back, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 文字 - 使用中文字体样式 */
    lv_obj_t* label = lv_label_create(container);
    lv_label_set_text(label, name);
    lv_obj_add_style(label, &NORMAL_SIZE, LV_PART_MAIN);

    if (out_btn) {
        *out_btn = container;
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

/* 状态栏更新定时器回调 - 每秒更新一次 */
static void home_update_timer_cb(lv_timer_t* timer)
{
    home_page_data_t* data = (home_page_data_t*)lv_timer_get_user_data(timer);
    if (!data || !data->lv_label_time) {
        return;
    }

    /* 更新时间 */
    time_t current_time = time(NULL);
    struct tm* tm_info = localtime(&current_time);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    lv_label_set_text(data->lv_label_time, time_str);
}

/* Home Page Callbacks */
static void photo_button_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    MLOG_INFO("Photo button clicked");
    (void)media_manager_execute(MEDIA_OP_SWITCH_TO_PHOTO_MODE, 0);
    page_manager_navigate("photo");
}

static void recognition_button_cb(lv_event_t* e)
{
    int ret = 0;

    LV_UNUSED(e);
    MLOG_INFO("AI Photo button clicked");

    ret = media_manager_execute(MEDIA_OP_SWITCH_TO_PHOTO_MODE, 0);
    if (ret != 0) {
        MLOG_ERR("AI拍照入口切换到拍照模式失败: ret=%d", ret);
        return;
    }

    ret = media_manager_execute(MEDIA_OP_SET_PHOTO_RESOLUTION, PHOTO_RESOLUTION_12M);
    if (ret != 0) {
        MLOG_ERR("AI拍照入口设置12M分辨率失败: ret=%d", ret);
        return;
    }

    page_manager_navigate("ai_photo");
}

static void chat_button_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    MLOG_INFO("Chat button clicked");
    page_manager_navigate("chat");
}

static void test_button_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    MLOG_INFO("Test page button clicked");
    page_manager_navigate("test");
}

static void album_button_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    MLOG_INFO("album button clicked");
    if (param_manager_get(PARAM_ID_SD_READY) != SD_READY_TRUE) {
        top_notice_show("SD卡未插入", TOP_NOTICE_TYPE_WARNING);
        return;
    }
    page_manager_navigate("album");
}

static void settings_button_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    MLOG_INFO("Settings button clicked");
    page_manager_navigate("system_settings");
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_home_create(void)
{
    home_page_data_t* data = (home_page_data_t*)malloc(sizeof(home_page_data_t));
    if (!data) {
        return;
    }

    memset(data, 0, sizeof(home_page_data_t));

    /* 创建页面容器 */
    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_width(data->container, LV_PCT(100));
    lv_obj_set_height(data->container, LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_add_style(data->container, &style_home_bg, LV_PART_MAIN);
    lv_obj_refr_size(data->container);

    /* Time label - center */
    data->lv_label_time = lv_label_create(data->container);
    lv_label_set_text(data->lv_label_time, "0000-00-00 00:00:00");
    lv_obj_add_style(data->lv_label_time, &ttf_font_26, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->lv_label_time, lv_color_black(), LV_PART_MAIN);
    lv_obj_align(data->lv_label_time, LV_ALIGN_TOP_MID, 0, 10);

    /* 创建状态栏更新定时器，每秒更新一次 */
    data->home_update_timer = lv_timer_create(home_update_timer_cb, 1000, data);

    /* 2行3列图标容器 - 使用容器和flex布局实现自适应 */
    data->grid_container = lv_obj_create(data->container);
    lv_obj_set_width(data->grid_container, lv_pct(100));
    lv_obj_set_height(data->grid_container, lv_pct(100));
    lv_obj_set_style_bg_opa(data->grid_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->grid_container, 0, LV_PART_MAIN);
    lv_obj_set_layout(data->grid_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->grid_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(data->grid_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 第1行 */
    create_icon_button(data->grid_container, LV_SYMBOL_IMAGE, "拍照",
        LV_ALIGN_TOP_LEFT, 0, 0,
        photo_button_cb, NULL);
    create_icon_button(data->grid_container, LV_SYMBOL_FILE, "AI拍照",
        LV_ALIGN_TOP_LEFT, 0, 0,
        recognition_button_cb, NULL);
    create_icon_button(data->grid_container, LV_SYMBOL_DIRECTORY, "相册",
        LV_ALIGN_TOP_LEFT, 0, 0,
        album_button_cb, NULL);

    /* 第2行 */
    create_icon_button(data->grid_container, LV_SYMBOL_CALL, "AI对话",
        LV_ALIGN_TOP_LEFT, 0, 0,
        chat_button_cb, NULL);
    create_icon_button(data->grid_container, LV_SYMBOL_EDIT, "测试页面",
        LV_ALIGN_TOP_LEFT, 0, 0,
        test_button_cb, NULL);
    create_icon_button(data->grid_container, LV_SYMBOL_SETTINGS, "设置",
        LV_ALIGN_TOP_LEFT, 0, 0,
        settings_button_cb, NULL);

    /* 保存 private_data，供 show/hide/destroy 使用 */
    page_set_private_data(data);
}

void page_home_destroy(void)
{
    home_page_data_t* data = page_get_private_data();
    if (!data) {
        return;
    }

    /* 先删除时间更新定时器，防止回调访问已释放的 data */
    if (data->home_update_timer) {
        lv_timer_delete(data->home_update_timer);
        data->home_update_timer = NULL;
    }

    /* 删除容器（子元素会自动删除） */
    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    free(data);
}

void page_home_show(void)
{
    home_page_data_t* data = page_get_private_data();
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("Home page show");
    /* 显示 UI */
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);

    /* 恢复定时器 */
    if (data->home_update_timer) {
        lv_timer_resume(data->home_update_timer);
    }

    /* 刷新全局状态栏 */
    status_bar_show(true);
    status_bar_set_icons(STATUS_BAR_ICON_SD, STATUS_BAR_ICON_WIFI, STATUS_BAR_ICON_BATTERY);
    status_bar_refresh();

    /* 立即更新时间 */
    if (data->home_update_timer) {
        home_update_timer_cb(data->home_update_timer);
    }
}

void page_home_hide(void)
{
    home_page_data_t* data = page_get_private_data();
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("Home page hide");
    /* 隐藏 UI */
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
    status_bar_show(false);

    /* 暂停定时器，节省资源 */
    if (data->home_update_timer) {
        lv_timer_pause(data->home_update_timer);
    }
}

void page_home_update(void)
{
    /* no-op */
    /* 时间由 timer 自动更新，此处无需处理 */
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
