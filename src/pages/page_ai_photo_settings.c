// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_ai_photo_settings.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/page_manager.h"
#include "mlog.h"
#include "styles/style_common.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_ai_photo_settings.h)
// #############################################################################

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static const ai_setting_config_t settings_config[] = {
    { "A" RES_ICON_PATH "/filter_default.png", "风格变换", AI_SETTING_STYLE_TRANSFORM },
    { "A" RES_ICON_PATH "/ai.png", "AI识万物", AI_SETTING_RECOGNITION },
    { "A" RES_ICON_PATH "/translate.png", "拍照翻译", AI_SETTING_TRANSLATION },
};

#define SETTINGS_COUNT (int)(sizeof(settings_config) / sizeof(settings_config[0]))

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

/* 设置项点击回调 - 3选1模式 */
static void setting_item_cb(lv_event_t* e)
{
    page_ai_photo_settings_data_t* data = (page_ai_photo_settings_data_t*)lv_event_get_user_data(e);
    int index = (int)lv_obj_get_user_data(lv_event_get_current_target(e));

    if (index < 0 || index >= data->settings_count) {
        return;
    }

    /* 3选1模式：先取消所有选项的选中状态，再选中当前点击的选项 */
    for (int i = 0; i < data->settings_count; i++) {
        bool selected = (i == index);
        lv_obj_add_style(data->items[i].container, selected ? &style_settings_item_selected : &style_settings_item, LV_PART_MAIN);
        if (selected) {
            lv_obj_clear_flag(data->items[i].check_icon, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(data->items[i].check_icon, LV_OBJ_FLAG_HIDDEN);
        }
    }

    MLOG_INFO("AI Setting '%s' selected", data->configs[index].title);
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_ai_photo_settings_create(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_ai_photo_settings_data_t* data = (page_ai_photo_settings_data_t*)malloc(sizeof(page_ai_photo_settings_data_t));
    if (!data) {
        return;
    }

    /* 指向静态配置 */
    data->configs = settings_config;
    data->settings_count = SETTINGS_COUNT;
    data->container = NULL;
    data->nav_bar = NULL;
    data->settings_container = NULL;
    data->items = NULL;
    data->items = (ai_setting_item_t*)malloc(sizeof(ai_setting_item_t) * SETTINGS_COUNT);
    if (!data->items) {
        free(data);
        return;
    }

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
    lv_obj_set_height(data->nav_bar, 50); /* 50高度 */
    lv_obj_clear_flag(data->nav_bar, LV_OBJ_FLAG_SCROLLABLE); /* 禁用滚动 */
    lv_obj_set_scrollbar_mode(data->nav_bar, LV_SCROLLBAR_MODE_OFF); /* 隐藏滚动条 */
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
    lv_label_set_text(title_label, "AI拍照设置");
    lv_obj_add_style(title_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    /* =======================
     * 2. 设置列表区域 - 占满剩余空间
     * ======================= */
    data->settings_container = lv_obj_create(data->container);
    lv_obj_set_width(data->settings_container, lv_pct(100));
    lv_obj_set_flex_grow(data->settings_container, 1); /* 填满剩余空间 */
    lv_obj_set_style_bg_opa(data->settings_container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->settings_container, lv_color_hex(0x121212), LV_PART_MAIN);
    lv_obj_set_layout(data->settings_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->settings_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(data->settings_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 创建3个设置项 */
    for (int i = 0; i < data->settings_count; i++) {
        ai_setting_item_t* item = &data->items[i];

        /* 设置项容器 */
        item->container = lv_obj_create(data->settings_container);
        lv_obj_set_width(item->container, lv_pct(100));
        lv_obj_set_height(item->container, 55);
        lv_obj_clear_flag(item->container, LV_OBJ_FLAG_SCROLLABLE); /* 禁用滚动 */
        lv_obj_set_scrollbar_mode(item->container, LV_SCROLLBAR_MODE_OFF); /* 隐藏滚动条 */
        lv_obj_add_style(item->container, &style_settings_item, LV_PART_MAIN);

        /* 左侧图标 */
        item->icon = lv_img_create(item->container);
        lv_img_set_src(item->icon, settings_config[i].icon_path);
        lv_obj_align(item->icon, LV_ALIGN_LEFT_MID, 0, 0);

        /* 标题文字 */
        item->title_label = lv_label_create(item->container);
        lv_label_set_text(item->title_label, settings_config[i].title);
        lv_obj_add_style(item->title_label, &NORMAL_SIZE, LV_PART_MAIN);
        lv_obj_set_style_text_color(item->title_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_align(item->title_label, LV_ALIGN_LEFT_MID, 55, 0);

        /* 选中图标 - 右侧 */
        item->check_icon = lv_img_create(item->container);
        lv_img_set_src(item->check_icon, "A:" RES_ICON_PATH "/check.png");
        lv_obj_align(item->check_icon, LV_ALIGN_RIGHT_MID, -10, 0);

        /* 默认选中第一个 */
        if (i == 0) {
            lv_obj_add_style(item->container, &style_settings_item_selected, LV_PART_MAIN);
        } else {
            lv_obj_add_flag(item->check_icon, LV_OBJ_FLAG_HIDDEN);
        }

        /* 点击事件 */
        lv_obj_add_event_cb(item->container, setting_item_cb, LV_EVENT_CLICKED, data);
        lv_obj_set_user_data(item->container, (void*)(intptr_t)i);
    }

    /* 保存 private_data */
    page_set_private_data(pm, data);
}

void page_ai_photo_settings_destroy(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_ai_photo_settings_data_t* data = page_get_private_data(pm);
    if (!data) {
        return;
    }

    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    free(data->items);
    free(data);
}

void page_ai_photo_settings_show(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_ai_photo_settings_data_t* data = page_get_private_data(pm);
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("AI photo settings page show");
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_ai_photo_settings_hide(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_ai_photo_settings_data_t* data = page_get_private_data(pm);
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("AI photo settings page hide");
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_ai_photo_settings_update(page_manager_t* pm)
{
    LV_UNUSED(pm);
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
