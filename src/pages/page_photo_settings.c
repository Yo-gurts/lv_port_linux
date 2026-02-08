// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_photo_settings.h"
#include "config.h"
#include "core/page_manager.h"
#include "font_manager.h"
#include "mlog.h"
#include "styles/style_common.h"
#include <stdlib.h>
#include <string.h>

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_photo_settings.h)
// #############################################################################

/* 设置项配置 */
typedef struct {
    const char* icon_path;
    const char* title;
    const char* value;
    const char* toggle_on;
    const char* toggle_off;
    setting_type_t type;
    const char* roller_options; /* 滚轮选项，用\n分隔 */
} setting_config_t;

/* 滚轮选项定义 */
#define RESOLUTION_OPTIONS "8M(3840x2160)\n12M(4000x3000)\n24M(5600x4200)\n48M(8000x6000)\n64M(8192x8192)"
#define WHITE_BALANCE_OPTIONS "自动\n晴天\n阴天\n白炽灯\n荧光灯"
#define ISO_OPTIONS "自动\nISO100\nISO400\nISO800\nISO1600\nISO3200"
#define EXPOSURE_OPTIONS "EV-2.0\nEV-1.5\nEV-1.0\nEV-0.5\nEV0\nEV+0.5\nEV+1.0\nEV+1.5\nEV+2.0"
#define QUALITY_OPTIONS "超高画质\n高画质\n普通画质"

#define SETTINGS_COUNT 7

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static const setting_config_t settings_config[] = {
    { "A" RES_ICON_PATH "/camera.png", "分辨率", "8M(3840x2160)", NULL, NULL, SETTING_TYPE_NORMAL, RESOLUTION_OPTIONS },
    { "A" RES_ICON_PATH "/white-balance.png", "白平衡", "自动", NULL, NULL, SETTING_TYPE_NORMAL, WHITE_BALANCE_OPTIONS },
    { "A" RES_ICON_PATH "/iso.png", "感光度", "自动", NULL, NULL, SETTING_TYPE_NORMAL, ISO_OPTIONS },
    { "A" RES_ICON_PATH "/exposure.png", "曝光设置", "EV0", NULL, NULL, SETTING_TYPE_NORMAL, EXPOSURE_OPTIONS },
    { "A" RES_ICON_PATH "/quality.png", "画质", "超高画质", NULL, NULL, SETTING_TYPE_NORMAL, QUALITY_OPTIONS },
    { "A" RES_ICON_PATH "/face-detection.png", "人脸检测", "关闭", "开启", "关闭", SETTING_TYPE_TOGGLE, NULL },
    { "A" RES_ICON_PATH "/smile.png", "笑脸抓拍", "关闭", "开启", "关闭", SETTING_TYPE_TOGGLE, NULL },
};

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

/* 从完整选项字符串中获取第idx个选项（返回副本） */
static void get_option_by_index(const char* options, int idx, char* buf, int buflen)
{
    const char* p = options;
    int i = 0;

    /* 跳过前idx个换行符 */
    while (i < idx && *p) {
        if (*p == '\n') {
            i++;
        }
        p++;
    }

    /* 复制当前选项 */
    while (*p && *p != '\n' && buflen > 1) {
        *buf++ = *p++;
        buflen--;
    }
    *buf = '\0';
}

/* 获取滚轮当前选中值并应用到UI */
static void apply_roller_selection(page_photo_settings_data_t* data)
{
    int index = data->current_setting_index;
    if (index < 0 || index >= SETTINGS_COUNT) {
        return;
    }

    photo_setting_item_t* item = &data->settings[index];
    int selected = lv_roller_get_selected(data->roller);

    item->current_index = selected;

    char buf[64];
    get_option_by_index(item->roller_options, selected, buf, sizeof(buf));
    lv_label_set_text(item->value_label, buf);
    lv_obj_add_flag(data->roller_popup, LV_OBJ_FLAG_HIDDEN);

    MLOG_INFO("Setting '%s' selected: %s", item->title, buf);
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

/* 滚轮选中回调 */
static void roller_select_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    page_photo_settings_data_t* data = (page_photo_settings_data_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_VALUE_CHANGED || code == LV_EVENT_CLICKED) {
        apply_roller_selection(data);
    }
}

/* 点击遮罩关闭弹窗回调 */
static void modal_click_cb(lv_event_t* e)
{
    page_photo_settings_data_t* data = (page_photo_settings_data_t*)lv_event_get_user_data(e);

    /* 应用当前选中值后关闭弹窗 */
    apply_roller_selection(data);
}

/* 设置项点击回调 */
static void setting_item_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_current_target(e);
    page_photo_settings_data_t* data = (page_photo_settings_data_t*)lv_event_get_user_data(e);
    int index = (int)lv_obj_get_user_data(obj);

    if (index < 0 || index >= SETTINGS_COUNT) {
        return;
    }

    photo_setting_item_t* item = &data->settings[index];

    if (item->type == SETTING_TYPE_TOGGLE) {
        /* 切换开关状态 */
        item->is_on = !item->is_on;
        const char* new_value = item->is_on ? item->toggle_on : item->toggle_off;
        lv_label_set_text(item->value_label, new_value);
        MLOG_INFO("Setting '%s' toggled to: %s", item->title, new_value);
    } else {
        /* 普通设置，弹出滚轮 */
        data->current_setting_index = index;

        /* 更新滚轮选项 */
        lv_roller_set_options(data->roller, item->roller_options, LV_ROLLER_MODE_NORMAL);

        /* 设置当前选中项 */
        lv_roller_set_selected(data->roller, item->current_index, LV_ANIM_OFF);

        /* 显示弹窗 */
        lv_obj_clear_flag(data->roller_popup, LV_OBJ_FLAG_HIDDEN);

        char buf[64];
        get_option_by_index(item->roller_options, item->current_index, buf, sizeof(buf));
        MLOG_INFO("Setting '%s' clicked, value: %s", item->title, buf);
    }
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_photo_settings_create(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_photo_settings_data_t* data = (page_photo_settings_data_t*)malloc(sizeof(page_photo_settings_data_t));
    if (!data) {
        return;
    }

    memset(data, 0, sizeof(page_photo_settings_data_t));

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
    lv_label_set_text(title_label, "拍照设置");
    lv_obj_add_style(title_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    /* =======================
     * 2. 设置列表区域 - 占满剩余空间
     * ======================= */
    data->settings_container = lv_obj_create(data->container);
    lv_obj_set_width(data->settings_container, lv_pct(100));
    lv_obj_set_flex_grow(data->settings_container, 1);
    lv_obj_set_style_bg_opa(data->settings_container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->settings_container, lv_color_hex(0x121212), LV_PART_MAIN);
    lv_obj_set_layout(data->settings_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->settings_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(data->settings_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 创建7个设置项 */
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        photo_setting_item_t* item = &data->settings[i];

        /* 设置项容器 */
        item->container = lv_obj_create(data->settings_container);
        lv_obj_set_width(item->container, lv_pct(100));
        lv_obj_set_height(item->container, 55);
        lv_obj_clear_flag(item->container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(item->container, LV_SCROLLBAR_MODE_OFF);
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

        /* 保存配置 */
        item->icon_path = settings_config[i].icon_path;
        item->title = settings_config[i].title;
        item->roller_options = settings_config[i].roller_options;
        item->toggle_on = settings_config[i].toggle_on;
        item->toggle_off = settings_config[i].toggle_off;
        item->type = settings_config[i].type;
        item->is_on = 0;

        /* 参数文字 */
        item->value_label = lv_label_create(item->container);
        lv_obj_add_style(item->value_label, &NORMAL_SIZE, LV_PART_MAIN);
        lv_obj_set_style_text_color(item->value_label, lv_color_hex(0xFFD700), LV_PART_MAIN);
        lv_obj_align(item->value_label, LV_ALIGN_RIGHT_MID, -10, 0);

        if (item->type == SETTING_TYPE_TOGGLE) {
            /* 开关类型：直接显示初始值 */
            item->current_index = 0;
            lv_label_set_text(item->value_label, settings_config[i].value);
        } else {
            /* 滚轮类型：计算初始索引 */
            const char* opts = item->roller_options;
            const char* val = settings_config[i].value;
            int len = strlen(val);
            int idx = 0;
            const char* p = opts;

            while (*p) {
                if (strncmp(p, val, len) == 0 && (p[len] == '\n' || p[len] == '\0')) {
                    break;
                }
                if (*p == '\n') {
                    idx++;
                }
                p++;
            }
            item->current_index = idx;
            char buf[64];
            get_option_by_index(item->roller_options, item->current_index, buf, sizeof(buf));
            lv_label_set_text(item->value_label, buf);
        }

        /* 点击事件 */
        lv_obj_add_event_cb(item->container, setting_item_cb, LV_EVENT_CLICKED, data);
        lv_obj_set_user_data(item->container, (void*)(intptr_t)i);
    }

    /* =======================
     * 3. 滚轮弹窗
     * ======================= */
    /* 半透明遮罩 - 直接创建在屏幕上，覆盖整个屏幕 */
    lv_obj_t* modal = lv_obj_create(lv_screen_active());
    lv_obj_set_size(modal, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(modal, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_bg_color(modal, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(modal, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(modal, 0, LV_PART_MAIN);
    lv_obj_clear_flag(modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(modal, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(modal, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(modal, LV_OBJ_FLAG_HIDDEN);
    data->roller_popup = modal;
    /* 点击遮罩关闭 */
    lv_obj_add_event_cb(data->roller_popup, modal_click_cb, LV_EVENT_CLICKED, data);

    /* 滚轮 - 只显示选项，居中 */
    data->roller = lv_roller_create(data->roller_popup);
    lv_obj_set_size(data->roller, 300, 200);
    lv_obj_center(data->roller);
    lv_roller_set_visible_row_count(data->roller, 5);
    lv_obj_add_style(data->roller, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->roller, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->roller, lv_color_hex(0x2A2A2A), LV_PART_MAIN);
    lv_obj_set_style_radius(data->roller, 20, LV_PART_MAIN);
    /* 点击滚轮选中后关闭 */
    lv_obj_add_event_cb(data->roller, roller_select_cb, LV_EVENT_VALUE_CHANGED | LV_EVENT_CLICKED, data);

    /* 保存 private_data */
    page_set_private_data(pm, data);
}

void page_photo_settings_destroy(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_photo_settings_data_t* data = page_get_private_data(pm);
    if (!data) {
        return;
    }

    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    free(data);
}

void page_photo_settings_show(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_photo_settings_data_t* data = page_get_private_data(pm);
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("Photo settings page show");
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);

    /* 隐藏弹窗 */
    if (data->roller_popup) {
        lv_obj_add_flag(data->roller_popup, LV_OBJ_FLAG_HIDDEN);
    }
}

void page_photo_settings_hide(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    page_photo_settings_data_t* data = page_get_private_data(pm);
    if (!data || !data->container) {
        return;
    }

    MLOG_INFO("Photo settings page hide");
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_photo_settings_update(page_manager_t* pm)
{
    LV_UNUSED(pm);
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
