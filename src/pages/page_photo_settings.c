// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_photo_settings.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/key_manager.h"
#include "core/media_manager.h"
#include "core/page_manager.h"
#include "core/param_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include "ui/gesture_back.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_photo_settings.h)
// #############################################################################

/* 滚轮选项定义 */
static const char* resolution_options[] = {
    "8M(3840x2160)", "12M(4000x3000)", "24M(5600x4200)", "48M(8000x6000)", "64M(8192x8192)"
};
static const char* white_balance_options[] = {
    "自动", "晴天", "阴天", "白炽灯", "荧光灯"
};
static const char* iso_options[] = {
    "自动", "ISO100", "ISO400", "ISO800", "ISO1600", "ISO3200"
};
static const char* exposure_options[] = {
    "EV-2.0", "EV-1.5", "EV-1.0", "EV-0.5", "EV0", "EV+0.5", "EV+1.0", "EV+1.5", "EV+2.0"
};
static const char* quality_options[] = {
    "超高画质", "高画质", "普通画质"
};

/* 设置项配置 - 静态数据 */
static const setting_config_t configs[] = {
    {
        .title = "分辨率",
        .icon_path = "A" RES_ICON_PATH "/camera.png",
        .value = "8M(3840x2160)",
        .roller_options = resolution_options,
        .roller_count = 5,
        .type = SETTING_TYPE_NORMAL,
        .param_id = PARAM_ID_RESOLUTION,
    },
    {
        .title = "白平衡",
        .icon_path = "A" RES_ICON_PATH "/white-balance.png",
        .value = "自动",
        .roller_options = white_balance_options,
        .roller_count = 5,
        .type = SETTING_TYPE_NORMAL,
        .param_id = PARAM_ID_WHITE_BALANCE,
    },
    {
        .title = "感光度",
        .icon_path = "A" RES_ICON_PATH "/iso.png",
        .value = "自动",
        .roller_options = iso_options,
        .roller_count = 6,
        .type = SETTING_TYPE_NORMAL,
        .param_id = PARAM_ID_ISO,
    },
    {
        .title = "曝光设置",
        .icon_path = "A" RES_ICON_PATH "/exposure.png",
        .value = "EV0",
        .roller_options = exposure_options,
        .roller_count = 9,
        .type = SETTING_TYPE_NORMAL,
        .param_id = PARAM_ID_EXPOSURE,
    },
    {
        .title = "画质",
        .icon_path = "A" RES_ICON_PATH "/quality.png",
        .value = "超高画质",
        .roller_options = quality_options,
        .roller_count = 3,
        .type = SETTING_TYPE_NORMAL,
        .param_id = PARAM_ID_QUALITY,
    },
    {
        .title = "人脸检测",
        .icon_path = "A" RES_ICON_PATH "/face-detection.png",
        .value = "已关闭",
        .type = SETTING_TYPE_TOGGLE,
        .param_id = PARAM_ID_FACE_DETECTION,
    },
    {
        .title = "笑脸抓拍",
        .icon_path = "A" RES_ICON_PATH "/smile.png",
        .value = "已关闭",
        .type = SETTING_TYPE_TOGGLE,
        .param_id = PARAM_ID_SMILE_CAPTURE,
    },
};

#define SETTINGS_COUNT (int)(sizeof(configs) / sizeof(configs[0]))
#define ROLLER_OPTIONS_BUF_SIZE 256

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

/* 将字符串数组构建为lv_roller选项字符串（用\n分隔） */
static void build_roller_options(const char** options, int count, char* buf, int buflen)
{
    buf[0] = '\0';
    for (int i = 0; i < count && buflen > 1; i++) {
        int len = strlen(options[i]);
        if (len >= buflen - 1) {
            len = buflen - 1;
        }
        strncpy(buf, options[i], len);
        buf += len;
        buflen -= len;
        if (i < count - 1 && buflen > 1) {
            *buf++ = '\n';
            buflen--;
        }
    }
    *buf = '\0';
}

/* 更新所有设置项显示值 */
static void update_all_settings_value(void)
{
    page_photo_settings_data_t* data = page_get_private_data();
    if (!data || !data->items) {
        return;
    }

    for (int i = 0; i < SETTINGS_COUNT; i++) {
        const setting_config_t* config = &data->configs[i];
        setting_item_t* item = &data->items[i];

        if (config->param_id != PARAM_ID_NONE) {
            item->current_index = param_manager_get((param_id_t)config->param_id);
        }

        if (config->type == SETTING_TYPE_TOGGLE) {
            const char* toggle_text = item->current_index ? "已开启" : "已关闭";
            lv_label_set_text(item->value_label, toggle_text);
        } else {
            if (item->current_index >= config->roller_count || item->current_index < 0) {
                item->current_index = 0;
            }
            lv_label_set_text(item->value_label, config->roller_options[item->current_index]);
        }
    }
}

/* 获取滚轮当前选中值并应用到UI */
static void apply_roller_selection(page_photo_settings_data_t* data)
{
    int index = data->current_setting_index;
    int ret = 0;
    if (index < 0 || index >= SETTINGS_COUNT) {
        return;
    }

    const setting_config_t* config = &data->configs[index];
    setting_item_t* item = &data->items[index];
    int selected = lv_roller_get_selected(data->roller);

    if (config->param_id == PARAM_ID_RESOLUTION) {
        ret = media_manager_execute(MEDIA_OP_SET_PHOTO_RESOLUTION, selected);
    } else if (config->param_id == PARAM_ID_WHITE_BALANCE) {
        ret = media_manager_execute(MEDIA_OP_SET_WHITE_BALANCE, selected);
    } else if (config->param_id == PARAM_ID_ISO) {
        ret = media_manager_execute(MEDIA_OP_SET_ISO, selected);
    } else if (config->param_id == PARAM_ID_EXPOSURE) {
        ret = media_manager_execute(MEDIA_OP_SET_EXPOSURE, selected);
    } else if (config->param_id == PARAM_ID_QUALITY) {
        ret = media_manager_execute(MEDIA_OP_SET_QUALITY, selected);
    } else if (config->param_id != PARAM_ID_NONE) {
        ret = param_manager_set((param_id_t)config->param_id, selected);
    }
    if (ret != 0) {
        MLOG_ERR("设置'%s'失败: selected=%d ret=%d", config->title, selected, ret);
        return;
    }

    item->current_index = selected;
    lv_label_set_text(item->value_label, config->roller_options[selected]);
    lv_obj_add_flag(data->roller_popup, LV_OBJ_FLAG_HIDDEN);

    MLOG_INFO("Setting '%s' selected: %s", config->title, config->roller_options[selected]);
}

static int is_roller_popup_visible(page_photo_settings_data_t* data)
{
    return data != NULL && data->roller_popup != NULL && !lv_obj_has_flag(data->roller_popup, LV_OBJ_FLAG_HIDDEN);
}

static void close_roller_popup(page_photo_settings_data_t* data)
{
    if (data == NULL || data->roller_popup == NULL) {
        return;
    }
    lv_obj_add_flag(data->roller_popup, LV_OBJ_FLAG_HIDDEN);
}

static void ensure_setting_visible(page_photo_settings_data_t* data, int index)
{
    if (data == NULL || data->items == NULL || index < 0 || index >= SETTINGS_COUNT) {
        return;
    }
    if (data->items[index].container == NULL) {
        return;
    }

    lv_obj_scroll_to_view(data->items[index].container, LV_ANIM_ON);
}

static void open_setting_editor(page_photo_settings_data_t* data, int index)
{
    const setting_config_t* config;
    setting_item_t* item;

    if (data == NULL || data->items == NULL || data->roller == NULL || index < 0 || index >= SETTINGS_COUNT) {
        return;
    }

    data->current_setting_index = index;
    ensure_setting_visible(data, index);
    config = &data->configs[index];
    item = &data->items[index];

    if (config->type == SETTING_TYPE_TOGGLE) {
        item->current_index = !item->current_index;
        const char* new_value = item->current_index ? "已开启" : "已关闭";
        int ret = 0;

        if (config->param_id == PARAM_ID_FACE_DETECTION) {
            ret = media_manager_execute(MEDIA_OP_SET_FACE_DETECTION, item->current_index);
        } else if (config->param_id == PARAM_ID_SMILE_CAPTURE) {
            ret = media_manager_execute(MEDIA_OP_SET_SMILE_CAPTURE, item->current_index);
        } else if (config->param_id != PARAM_ID_NONE) {
            ret = param_manager_set((param_id_t)config->param_id, item->current_index);
        }
        if (ret != 0) {
            item->current_index = !item->current_index;
            MLOG_ERR("设置'%s'失败: selected=%d ret=%d", config->title, item->current_index, ret);
            return;
        }

        lv_label_set_text(item->value_label, new_value);
        MLOG_INFO("Setting '%s' toggled to: %s", config->title, new_value);
        return;
    }

    char options_buf[ROLLER_OPTIONS_BUF_SIZE];
    build_roller_options(config->roller_options, config->roller_count, options_buf, sizeof(options_buf));
    lv_roller_set_options(data->roller, options_buf, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(data->roller, item->current_index, LV_ANIM_OFF);
    lv_obj_clear_flag(data->roller_popup, LV_OBJ_FLAG_HIDDEN);

    MLOG_INFO("Setting '%s' opened, value: %s", config->title, config->roller_options[item->current_index]);
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

static void setting_item_cb(lv_event_t* e);

static void menu_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)key;
    (void)event_type;
    page_photo_settings_data_t* data = (page_photo_settings_data_t*)user_data;

    if (is_roller_popup_visible(data)) {
        close_roller_popup(data);
        return;
    }

    page_manager_back();
}

static void update_selection_highlight(page_photo_settings_data_t* data, int old_index, int new_index)
{
    if (!data) return;
    if (old_index >= 0 && old_index < SETTINGS_COUNT && data->items[old_index].container) {
        lv_obj_remove_style(data->items[old_index].container, &style_settings_item_selected, LV_PART_MAIN);
    }
    if (new_index >= 0 && new_index < SETTINGS_COUNT && data->items[new_index].container) {
        lv_obj_add_style(data->items[new_index].container, &style_settings_item_selected, LV_PART_MAIN);
        ensure_setting_visible(data, new_index);
    }
}

static void up_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_photo_settings_data_t* data = (page_photo_settings_data_t*)user_data;
    if (key != KEY_ID_UP || event_type != KEY_EVENT_CLICK)
        return;
    if (!data || !data->container)
        return;

    if (is_roller_popup_visible(data)) {
        const setting_config_t* config = &data->configs[data->current_setting_index];
        int roller_count = config->roller_count;
        int selected = lv_roller_get_selected(data->roller);
        if (selected > 0) {
            lv_roller_set_selected(data->roller, selected - 1, LV_ANIM_ON);
        } else {
            lv_roller_set_selected(data->roller, roller_count - 1, LV_ANIM_ON);
        }
        return;
    }

    int old_index = data->current_setting_index;
    if (data->current_setting_index > 0) {
        data->current_setting_index--;
    } else {
        data->current_setting_index = SETTINGS_COUNT - 1;
    }
    update_selection_highlight(data, old_index, data->current_setting_index);
}

static void down_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_photo_settings_data_t* data = (page_photo_settings_data_t*)user_data;
    if (key != KEY_ID_DOWN || event_type != KEY_EVENT_CLICK)
        return;
    if (!data || !data->container)
        return;

    if (is_roller_popup_visible(data)) {
        const setting_config_t* config = &data->configs[data->current_setting_index];
        int roller_count = config->roller_count;
        int selected = lv_roller_get_selected(data->roller);
        if (selected < roller_count - 1) {
            lv_roller_set_selected(data->roller, selected + 1, LV_ANIM_ON);
        } else {
            lv_roller_set_selected(data->roller, 0, LV_ANIM_ON);
        }
        return;
    }

    int old_index = data->current_setting_index;
    if (data->current_setting_index < SETTINGS_COUNT - 1) {
        data->current_setting_index++;
    } else {
        data->current_setting_index = 0;
    }
    update_selection_highlight(data, old_index, data->current_setting_index);
}

static void ok_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_photo_settings_data_t* data = (page_photo_settings_data_t*)user_data;

    if (key != KEY_ID_OK || event_type != KEY_EVENT_CLICK)
        return;
    if (!data || !data->container)
        return;

    if (is_roller_popup_visible(data)) {
        apply_roller_selection(data);
        return;
    }

    open_setting_editor(data, data->current_setting_index);
}

static void left_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_photo_settings_data_t* data = (page_photo_settings_data_t*)user_data;
    uint16_t selected;

    if (key != KEY_ID_LEFT || event_type != KEY_EVENT_CLICK)
        return;
    if (!data || !data->container)
        return;
    if (!data->roller_popup || lv_obj_has_flag(data->roller_popup, LV_OBJ_FLAG_HIDDEN))
        return;

    selected = lv_roller_get_selected(data->roller);
    if (selected > 0) {
        lv_roller_set_selected(data->roller, selected - 1, LV_ANIM_ON);
    }
}

static void right_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_photo_settings_data_t* data = (page_photo_settings_data_t*)user_data;
    const setting_config_t* config;
    int roller_count;
    if (key != KEY_ID_RIGHT || event_type != KEY_EVENT_CLICK) return;
    if (!data || !data->container) return;
    if (!data->roller_popup || lv_obj_has_flag(data->roller_popup, LV_OBJ_FLAG_HIDDEN)) return;

    config = &data->configs[data->current_setting_index];
    roller_count = config->roller_count;
    int selected = lv_roller_get_selected(data->roller);
    if (selected < roller_count - 1) {
        lv_roller_set_selected(data->roller, selected + 1, LV_ANIM_ON);
    }
}

/* 滚轮选中回调 */
static void roller_select_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    page_photo_settings_data_t* data = (page_photo_settings_data_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        apply_roller_selection(data);
    }
}

/* 点击遮罩关闭弹窗回调 */
static void modal_click_cb(lv_event_t* e)
{
    page_photo_settings_data_t* data = (page_photo_settings_data_t*)lv_event_get_user_data(e);

    if (lv_event_get_target(e) == data->roller_popup) {
        close_roller_popup(data);
    }
}

/* 设置项点击回调 */
static void setting_item_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_current_target(e);
    page_photo_settings_data_t* data = (page_photo_settings_data_t*)lv_event_get_user_data(e);
    uintptr_t user_data = (uintptr_t)lv_obj_get_user_data(obj);
    int index = (int)user_data;

    if (index < 0 || index >= SETTINGS_COUNT) {
        return;
    }

    int old_index = data->current_setting_index;

    update_selection_highlight(data, old_index, index);
    open_setting_editor(data, index);
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_photo_settings_create(void)
{
    page_photo_settings_data_t* data = (page_photo_settings_data_t*)malloc(sizeof(page_photo_settings_data_t));
    if (!data) {
        return;
    }

    memset(data, 0, sizeof(page_photo_settings_data_t));

    /* 指向静态配置 */
    data->configs = configs;
    data->items = (setting_item_t*)malloc(sizeof(setting_item_t) * SETTINGS_COUNT);
    if (!data->items) {
        free(data);
        return;
    }
    memset(data->items, 0, sizeof(setting_item_t) * SETTINGS_COUNT);

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
    lv_obj_add_event_cb(back_btn, page_manager_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_t* back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle-white.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    /* 标题文字 - 居中 */
    lv_obj_t* title_label = lv_label_create(data->nav_bar);
    lv_label_set_text(title_label, "拍照设置");
    lv_obj_add_style(title_label, &NORMAL_SIZE, LV_PART_MAIN);
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

    /* 创建设置项 */
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        const setting_config_t* config = &data->configs[i];
        setting_item_t* item = &data->items[i];

        /* 设置项容器 */
        item->container = lv_obj_create(data->settings_container);
        lv_obj_set_width(item->container, lv_pct(100));
        lv_obj_set_height(item->container, 55);
        lv_obj_clear_flag(item->container, LV_OBJ_FLAG_SCROLLABLE); /* 禁用滚动 */
        lv_obj_set_scrollbar_mode(item->container, LV_SCROLLBAR_MODE_OFF); /* 隐藏滚动条 */
        lv_obj_add_style(item->container, &style_settings_item, LV_PART_MAIN);

        /* 左侧图标 */
        item->icon = lv_img_create(item->container);
        lv_img_set_src(item->icon, config->icon_path);
        lv_obj_align(item->icon, LV_ALIGN_LEFT_MID, 0, 0);

        /* 标题文字 */
        item->title_label = lv_label_create(item->container);
        lv_label_set_text(item->title_label, config->title);
        lv_obj_add_style(item->title_label, &NORMAL_SIZE, LV_PART_MAIN);
        lv_obj_align(item->title_label, LV_ALIGN_LEFT_MID, 55, 0);

        /* 参数文字 - NORMAL_SIZE 包含白色，需覆盖为黄色 */
        item->value_label = lv_label_create(item->container);
        lv_obj_add_style(item->value_label, &NORMAL_SIZE, LV_PART_MAIN);
        lv_obj_set_style_text_color(item->value_label, lv_color_hex(0xFFD700), LV_PART_MAIN);
        lv_obj_align(item->value_label, LV_ALIGN_RIGHT_MID, -10, 0);

        if (config->type == SETTING_TYPE_TOGGLE) {
            /* 开关类型：从param_manager读取当前值 */
            item->current_index = (config->param_id != PARAM_ID_NONE) ? param_manager_get((param_id_t)config->param_id) : 0;
            const char* toggle_text = item->current_index ? "已开启" : "已关闭";
            lv_label_set_text(item->value_label, toggle_text);
        } else {
            /* 滚轮类型：从param_manager读取当前值 */
            item->current_index = (config->param_id != PARAM_ID_NONE) ? param_manager_get((param_id_t)config->param_id) : 0;
            if (item->current_index >= config->roller_count) {
                item->current_index = 0;
            }
            lv_label_set_text(item->value_label, config->roller_options[item->current_index]);
        }

        /* 点击事件：
         * - 普通项：仅允许点击右侧参数值弹出 roller
         * - 开关项：保持整行可点击切换 */
        if (config->type == SETTING_TYPE_TOGGLE) {
            lv_obj_add_event_cb(item->container, setting_item_cb, LV_EVENT_CLICKED, data);
            lv_obj_set_user_data(item->container, (void*)(intptr_t)i);
        } else {
            lv_obj_add_event_cb(item->container, setting_item_cb, LV_EVENT_CLICKED, data);
            lv_obj_set_user_data(item->container, (void*)(intptr_t)i);
        }
    }

    /* =======================
     * 3. 滚轮弹窗
     * ======================= */
    data->roller_popup = lv_obj_create(lv_screen_active());
    lv_obj_set_size(data->roller_popup, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->roller_popup, &style_roller_popup, LV_PART_MAIN);
    lv_obj_clear_flag(data->roller_popup, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(data->roller_popup, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(data->roller_popup, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(data->roller_popup, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(data->roller_popup, modal_click_cb, LV_EVENT_CLICKED, data);

    data->roller = lv_roller_create(data->roller_popup);
    lv_obj_set_size(data->roller, 300, 200);
    lv_obj_center(data->roller);
    lv_roller_set_visible_row_count(data->roller, 5);
    lv_obj_add_style(data->roller, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_add_style(data->roller, &style_roller, LV_PART_MAIN);
    lv_obj_add_event_cb(data->roller, roller_select_cb, LV_EVENT_CLICKED, data);

    /* 启用整页事件冒泡，确保子对象按压事件传递到父容器。 */
    gesture_back_enable_event_bubble_recursive(data->container);

    /* 保存 private_data */
    page_set_private_data(data);
}

void page_photo_settings_destroy(void)
{
    page_photo_settings_data_t* data = page_get_private_data();
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

void page_photo_settings_show(void)
{
    page_photo_settings_data_t* data = page_get_private_data();
    if (!data || !data->container) {
        return;
    }

    gesture_back_set_left_edge_swipe_cb(data->container, page_manager_back_cb);
    key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, data);
    key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_cb, data);
    key_manager_register_callback(KEY_ID_UP, KEY_EVENT_CLICK, up_key_click_cb, data);
    key_manager_register_callback(KEY_ID_DOWN, KEY_EVENT_CLICK, down_key_click_cb, data);
    key_manager_register_callback(KEY_ID_OK, KEY_EVENT_CLICK, ok_key_click_cb, data);

    MLOG_INFO("Photo settings page show");
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);

    if (data->roller_popup) {
        lv_obj_add_flag(data->roller_popup, LV_OBJ_FLAG_HIDDEN);
    }
    update_selection_highlight(data, -1, data->current_setting_index);
}

void page_photo_settings_hide(void)
{
    page_photo_settings_data_t* data = page_get_private_data();
    if (!data || !data->container) {
        return;
    }

    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, data);
    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_cb, data);
    key_manager_unregister_callback(KEY_ID_UP, KEY_EVENT_CLICK, up_key_click_cb, data);
    key_manager_unregister_callback(KEY_ID_DOWN, KEY_EVENT_CLICK, down_key_click_cb, data);
    key_manager_unregister_callback(KEY_ID_OK, KEY_EVENT_CLICK, ok_key_click_cb, data);

    MLOG_INFO("Photo settings page hide");
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_photo_settings_update(void)
{
    update_all_settings_value();
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
