// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "ui/status_bar.h"
#include "config.h"
#include "core/param_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include "lvgl/lvgl.h"
#include <stdlib.h>
#include <string.h>

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义
// #############################################################################

typedef struct {
    lv_obj_t* container;
    lv_obj_t* slot_icons[3];
    status_bar_icon_type_t slot_types[3];
} status_bar_data_t;

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static status_bar_data_t* g_status_bar = NULL;
static const int g_slot_x_offsets[3] = { -110, -65, -10 };

static const char* get_battery_icon_path(int level);
static status_bar_icon_type_t normalize_icon_type(status_bar_icon_type_t type);
static const char* get_icon_path(status_bar_icon_type_t type);
static void update_icons(void);
static void status_bar_param_cb(param_id_t id, int value, void* user_data);
static void status_bar_async_refresh_cb(void* user_data);

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

static const char* get_battery_icon_path(int level)
{
    if (level < 0) {
        return "A:" RES_ICON_PATH "/battery-charging.png";
    }
    if (level >= 80) {
        return "A:" RES_ICON_PATH "/battery-full.png";
    } else if (level >= 50) {
        return "A:" RES_ICON_PATH "/battery-half.png";
    } else if (level >= 20) {
        return "A:" RES_ICON_PATH "/battery-low.png";
    } else {
        return "A:" RES_ICON_PATH "/battery-low.png";
    }
}

static const char* wifi_icon_helper_get_path(int enabled, int connected, int signal_dbm)
{
    if (enabled != 1) {
        return "A:" RES_ICON_PATH "/wifi-off.png";
    }

    /* WiFi 已开启但未连接：显示最弱信号图标，而不是 off。 */
    if (connected != 1) {
        return "A:" RES_ICON_PATH "/wifi-1.png";
    }

    if (signal_dbm <= -75) {
        return "A:" RES_ICON_PATH "/wifi-1.png";
    }
    if (signal_dbm <= -60) {
        return "A:" RES_ICON_PATH "/wifi-2.png";
    }

    return "A:" RES_ICON_PATH "/wifi.png";
}

static status_bar_icon_type_t normalize_icon_type(status_bar_icon_type_t type)
{
    if (type < STATUS_BAR_ICON_NONE || type > STATUS_BAR_ICON_SD) {
        return STATUS_BAR_ICON_NONE;
    }

    return type;
}

static const char* get_icon_path(status_bar_icon_type_t type)
{
    if (type == STATUS_BAR_ICON_SD) {
        bool ready = param_manager_get(PARAM_ID_SD_READY) == SD_READY_TRUE;
        return ready ? "A:" RES_ICON_PATH "/sd_online.png" : "A:" RES_ICON_PATH "/sd_offline.png";
    }

    if (type == STATUS_BAR_ICON_WIFI) {
        int enabled = param_manager_get(PARAM_ID_WIFI_ENABLED);
        int connected = param_manager_get(PARAM_ID_WIFI_CONNECTED);
        int signal_dbm = param_manager_get(PARAM_ID_WIFI_SIGNAL_DBM);
        return wifi_icon_helper_get_path(enabled, connected, signal_dbm);
    }

    if (type == STATUS_BAR_ICON_BATTERY) {
        int level = param_manager_get(PARAM_ID_BATTERY_VAL);
        return get_battery_icon_path(level);
    }

    return NULL;
}

static void update_icons(void)
{
    int i;

    if (g_status_bar == NULL) {
        return;
    }

    for (i = 0; i < 3; i++) {
        lv_obj_t* icon_obj = g_status_bar->slot_icons[i];
        status_bar_icon_type_t type;
        const char* icon_path;

        if (icon_obj == NULL) {
            continue;
        }

        type = normalize_icon_type(g_status_bar->slot_types[i]);
        if (type == STATUS_BAR_ICON_NONE) {
            lv_obj_add_flag(icon_obj, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        icon_path = get_icon_path(type);
        if (icon_path != NULL) {
            lv_img_set_src(icon_obj, icon_path);
            lv_obj_clear_flag(icon_obj, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(icon_obj, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void status_bar_async_refresh_cb(void* user_data)
{
    LV_UNUSED(user_data);
    update_icons();
}

static void status_bar_param_cb(param_id_t id, int value, void* user_data)
{
    LV_UNUSED(value);
    LV_UNUSED(user_data);

    if (id == PARAM_ID_BATTERY_VAL || id == PARAM_ID_WIFI_ENABLED || id == PARAM_ID_WIFI_CONNECTED
        || id == PARAM_ID_WIFI_SIGNAL_DBM || id == PARAM_ID_SD_READY) {
        (void)lv_async_call(status_bar_async_refresh_cb, NULL);
    }
}

// #endregion
// #############################################################################
// ! #region 5. 对外接口函数
// #############################################################################

void status_bar_init(void)
{
    int i;

    if (g_status_bar != NULL) {
        return;
    }

    g_status_bar = (status_bar_data_t*)malloc(sizeof(status_bar_data_t));
    if (g_status_bar == NULL) {
        return;
    }
    memset(g_status_bar, 0, sizeof(status_bar_data_t));

    /* 创建容器 - 放在 lv_layer_top() 最顶层 */
    g_status_bar->container = lv_obj_create(lv_layer_top());
    lv_obj_set_size(g_status_bar->container, LV_PCT(100), 40);
    lv_obj_set_style_bg_opa(g_status_bar->container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_status_bar->container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(g_status_bar->container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(g_status_bar->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(g_status_bar->container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(g_status_bar->container, LV_ALIGN_TOP_MID, 0, 0);

    for (i = 0; i < 3; i++) {
        g_status_bar->slot_icons[i] = lv_img_create(g_status_bar->container);
        lv_obj_align(g_status_bar->slot_icons[i], LV_ALIGN_RIGHT_MID, g_slot_x_offsets[i], 0);
        g_status_bar->slot_types[i] = STATUS_BAR_ICON_NONE;
    }

    status_bar_set_icons(STATUS_BAR_ICON_SD, STATUS_BAR_ICON_WIFI, STATUS_BAR_ICON_BATTERY);

    /* 注册参数回调 */
    param_manager_register_callback(status_bar_param_cb, NULL);

    /* 初始更新图标 */
    update_icons();

    MLOG_INFO("Status bar initialized");
}

void status_bar_show(bool show)
{
    if (g_status_bar == NULL || g_status_bar->container == NULL) {
        return;
    }

    if (show) {
        lv_obj_clear_flag(g_status_bar->container, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(g_status_bar->container);
    } else {
        lv_obj_add_flag(g_status_bar->container, LV_OBJ_FLAG_HIDDEN);
    }
}

void status_bar_set_icons(status_bar_icon_type_t left, status_bar_icon_type_t middle, status_bar_icon_type_t right)
{
    if (g_status_bar == NULL) {
        return;
    }

    g_status_bar->slot_types[0] = normalize_icon_type(left);
    g_status_bar->slot_types[1] = normalize_icon_type(middle);
    g_status_bar->slot_types[2] = normalize_icon_type(right);
    update_icons();
}

void status_bar_refresh(void)
{
    update_icons();
}

// #endregion
// #############################################################################
// ! #region 6. 线程处理函数
// #############################################################################

// status_bar 模块当前不涉及线程处理逻辑。

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

// status_bar 当前无需事件回调。

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

// 初始化在 status_bar_init() 中完成，去初始化暂不需要。

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
