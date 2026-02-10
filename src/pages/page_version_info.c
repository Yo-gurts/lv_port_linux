// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_version_info.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/page_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <stdlib.h>
#include <string.h>

// 软件版本信息 - 实际项目中可能从配置文件或git获取
#define VERSION_INFO_SOFTWARE "V1.0.0-40df3d0"
#define VERSION_INFO_SYSTEM "linux_5.10"
#define VERSION_INFO_PROJECT "DC309"
#define VERSION_INFO_SCREEN "st7701"

#define CLICK_TIMEOUT_MS 500 /* 连点超时时间500ms */

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_version_info.h)
// #############################################################################

/* 版本信息项 */
typedef struct {
    const char* title;
    const char* value;
} version_info_item_t;

static const version_info_item_t info_items[] = {
    { "软件版本", VERSION_INFO_SOFTWARE },
    { "系统版本", VERSION_INFO_SYSTEM },
    { "项目信息", VERSION_INFO_PROJECT },
    { "屏幕信息", VERSION_INFO_SCREEN },
};

#define INFO_COUNT (int)(sizeof(info_items) / sizeof(info_items[0]))

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

/* 系统版本点击回调 - 检测连点 */
static void system_version_click_cb(lv_event_t* e)
{
    page_version_info_data_t* data = (page_version_info_data_t*)lv_event_get_user_data(e);
    uint32_t current_time = lv_tick_get();
    uint32_t elapsed = current_time - data->last_click_time;

    if (elapsed < CLICK_TIMEOUT_MS) {
        data->click_count++;
    } else {
        data->click_count = 1;
    }
    data->last_click_time = current_time;

    /* 连点5次触发测试日志 */
    if (data->click_count >= 5) {
        MLOG_INFO("test");
        data->click_count = 0;
    }
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_version_info_create(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_version_info_data_t* data = (page_version_info_data_t*)malloc(sizeof(page_version_info_data_t));
    if (!data) {
        return;
    }

    memset(data, 0, sizeof(page_version_info_data_t));

    /* 创建页面容器 */
    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_width(data->container, LV_PCT(100));
    lv_obj_set_height(data->container, LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_set_layout(data->container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->container, LV_FLEX_FLOW_COLUMN);
    lv_obj_refr_size(data->container);

    /* =======================
     * 1. 顶部导航栏
     * ======================= */
    data->nav_bar = lv_obj_create(data->container);
    lv_obj_set_width(data->nav_bar, lv_pct(100));
    lv_obj_set_height(data->nav_bar, 50);
    lv_obj_clear_flag(data->nav_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(data->nav_bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(data->nav_bar, &style_common_cont_top, LV_PART_MAIN);

    /* 返回按钮 - 左上角 */
    lv_obj_t* back_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(back_btn, 50, 50);
    lv_obj_add_style(back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(back_btn, page_manager_back_cb, LV_EVENT_CLICKED, pm);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_t* back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-fill.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    /* 标题文字 - 居中 */
    lv_obj_t* title_label = lv_label_create(data->nav_bar);
    lv_label_set_text(title_label, "版本信息");
    lv_obj_add_style(title_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    /* =======================
     * 2. 信息列表区域 - 占满剩余空间
     * ======================= */
    data->info_container = lv_obj_create(data->container);
    lv_obj_set_width(data->info_container, lv_pct(100));
    lv_obj_set_flex_grow(data->info_container, 1);
    lv_obj_set_style_bg_opa(data->info_container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->info_container, lv_color_hex(0x121212), LV_PART_MAIN);
    lv_obj_set_layout(data->info_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->info_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(data->info_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 创建信息项 */
    for (int i = 0; i < INFO_COUNT; i++) {
        lv_obj_t* item_container = lv_obj_create(data->info_container);
        lv_obj_set_width(item_container, lv_pct(100));
        lv_obj_set_height(item_container, 55);
        lv_obj_clear_flag(item_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(item_container, LV_SCROLLBAR_MODE_OFF);
        lv_obj_add_style(item_container, &style_settings_item, LV_PART_MAIN);

        /* 标题文字 - 靠左 */
        lv_obj_t* title = lv_label_create(item_container);
        lv_label_set_text(title, info_items[i].title);
        lv_obj_add_style(title, &NORMAL_SIZE, LV_PART_MAIN);
        lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

        /* 值文字 - 靠右 */
        lv_obj_t* value = lv_label_create(item_container);
        lv_label_set_text(value, info_items[i].value);
        lv_obj_add_style(value, &NORMAL_SIZE, LV_PART_MAIN);
        lv_obj_set_style_text_color(value, lv_color_hex(0xFFD700), LV_PART_MAIN);
        lv_obj_align(value, LV_ALIGN_RIGHT_MID, -10, 0);

        /* 系统版本项添加点击事件用于检测连点 */
        if (i == 1) { /* 系统版本是第2项 */
            lv_obj_add_flag(item_container, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(item_container, system_version_click_cb, LV_EVENT_CLICKED, data);
            data->click_label = item_container;
        }
    }

    /* 保存 private_data */
    page_set_private_data(pm, data);
}

void page_version_info_destroy(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_version_info_data_t* data = page_get_private_data(pm);
    if (!data) {
        return;
    }

    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    free(data);
}

void page_version_info_show(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_version_info_data_t* data = page_get_private_data(pm);
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("Version info page show");
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);

    /* 重置连点计数 */
    data->click_count = 0;
    data->last_click_time = 0;
}

void page_version_info_hide(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_version_info_data_t* data = page_get_private_data(pm);
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("Version info page hide");
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_version_info_update(page_manager_t* pm)
{
    LV_UNUSED(pm);
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
