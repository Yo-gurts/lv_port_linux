// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_system_settings.h"
#include "config.h"
#include "core/page_manager.h"
#include "font_manager.h"
#include "mlog.h"
#include "styles/style_common.h"
#include <stdlib.h>
#include <string.h>

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_system_settings.h)
// #############################################################################

/* 设置项配置 */
typedef struct {
    const char* icon_path;
    const char* title;
    const char* value;
    const char* toggle_on;
    const char* toggle_off;
    setting_type_t type;
} setting_config_t;

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static const setting_config_t settings_config[] = {
    { "A" RES_ICON_PATH "/language.png", "语言", "简体中文", NULL, NULL, SETTING_TYPE_NORMAL },
    { "A" RES_ICON_PATH "/wifi.png", "WiFi设置", "未连接", NULL, NULL, SETTING_TYPE_NORMAL },
    { "A" RES_ICON_PATH "/datetime.png", "时间和日期", "2026-02-07 12:00", NULL, NULL, SETTING_TYPE_NORMAL },
    { "A" RES_ICON_PATH "/volume.png", "音量设置", "80%", NULL, NULL, SETTING_TYPE_NORMAL },
    { "A" RES_ICON_PATH "/format.png", "格式化", "请确认", NULL, NULL, SETTING_TYPE_NORMAL },
    { "A" RES_ICON_PATH "/factory.png", "出厂设置", "请确认", NULL, NULL, SETTING_TYPE_NORMAL },
    { "A" RES_ICON_PATH "/info.png", "版本信息", "V1.0.0", NULL, NULL, SETTING_TYPE_NORMAL },
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

/* 设置项点击回调 */
static void setting_item_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_current_target(e);
    page_system_settings_data_t* data = (page_system_settings_data_t*)lv_event_get_user_data(e);
    int index = (int)lv_obj_get_user_data(obj);
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);

    if (index < 0 || index >= SETTINGS_COUNT) {
        return;
    }

    system_setting_item_t* item = &data->settings[index];

    if (item->type == SETTING_TYPE_TOGGLE) {
        /* 切换开关状态 */
        item->is_on = !item->is_on;
        const char* new_value = item->is_on ? item->toggle_on : item->toggle_off;
        lv_label_set_text(item->value_label, new_value);
        MLOG_INFO("Setting '%s' toggled to: %s", item->title, new_value);
    } else {
        /* 版本信息跳转 */
        if (index == SETTINGS_COUNT - 1) {
            page_manager_navigate(pm, "version_info");
        } else {
            MLOG_INFO("Setting '%s' clicked, value: %s", item->title, item->value);
        }
    }
}

/* 返回按钮回调 */
static void back_btn_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    MLOG_INFO("Back button clicked");
    page_manager_back(pm);
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_system_settings_create(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_system_settings_data_t* data = (page_system_settings_data_t*)malloc(sizeof(page_system_settings_data_t));
    if (!data) {
        return;
    }

    memset(data, 0, sizeof(page_system_settings_data_t));

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
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, pm);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_t* back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-fill.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    /* 标题文字 - 居中 */
    lv_obj_t* title_label = lv_label_create(data->nav_bar);
    lv_label_set_text(title_label, "系统设置");
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

    /* 创建7个设置项 */
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        system_setting_item_t* item = &data->settings[i];

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

        /* 参数文字 */
        item->value_label = lv_label_create(item->container);
        lv_label_set_text(item->value_label, settings_config[i].value);
        lv_obj_add_style(item->value_label, &NORMAL_SIZE, LV_PART_MAIN);
        lv_obj_set_style_text_color(item->value_label, lv_color_hex(0xFFD700), LV_PART_MAIN);
        lv_obj_align(item->value_label, LV_ALIGN_RIGHT_MID, 0, 0);

        /* 保存配置 */
        item->icon_path = settings_config[i].icon_path;
        item->title = settings_config[i].title;
        item->value = settings_config[i].value;
        item->toggle_on = settings_config[i].toggle_on;
        item->toggle_off = settings_config[i].toggle_off;
        item->type = settings_config[i].type;
        item->is_on = 0;

        /* 点击事件 */
        lv_obj_add_event_cb(item->container, setting_item_cb, LV_EVENT_CLICKED, pm);
        lv_obj_set_user_data(item->container, (void*)(intptr_t)i);
    }

    /* 保存 private_data */
    page_set_private_data(pm, data);
}

void page_system_settings_destroy(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_system_settings_data_t* data = page_get_private_data(pm);
    if (!data) {
        return;
    }

    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    free(data);
}

void page_system_settings_show(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_system_settings_data_t* data = page_get_private_data(pm);
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("System settings page show");
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_system_settings_hide(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_system_settings_data_t* data = page_get_private_data(pm);
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("System settings page hide");
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_system_settings_update(page_manager_t* pm)
{
    LV_UNUSED(pm);
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
