// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_test.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/key_manager.h"
#include "core/page_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <stdlib.h>
#include <string.h>

#define TEST_NAV_BAR_HEIGHT 50
#define TEST_MENU_ITEM_HEIGHT 55

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

static void menu_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)key;
    (void)event_type;
    (void)user_data;
    page_manager_back();
}

/* 刷新选中高亮：移除旧项、给新项加选中样式。 */
static void update_selection_highlight(page_test_data_t* data, int old_index, int new_index)
{
    if (data == NULL) {
        return;
    }
    if (old_index >= 0 && old_index < PAGE_TEST_ITEM_COUNT && data->items[old_index] != NULL) {
        lv_obj_remove_style(data->items[old_index], &style_settings_item_selected, LV_PART_MAIN);
    }
    if (new_index >= 0 && new_index < PAGE_TEST_ITEM_COUNT && data->items[new_index] != NULL) {
        lv_obj_add_style(data->items[new_index], &style_settings_item_selected, LV_PART_MAIN);
    }
}

/* 上键：选中项上移，循环。 */
static void up_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_test_data_t* data = (page_test_data_t*)user_data;
    int old_index;

    if (key != KEY_ID_UP || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (data == NULL || data->container == NULL) {
        return;
    }
    old_index = data->selected_index;
    if (data->selected_index > 0) {
        data->selected_index--;
    } else {
        data->selected_index = PAGE_TEST_ITEM_COUNT - 1;
    }
    update_selection_highlight(data, old_index, data->selected_index);
}

/* 下键：选中项下移，循环。 */
static void down_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_test_data_t* data = (page_test_data_t*)user_data;
    int old_index;

    if (key != KEY_ID_DOWN || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (data == NULL || data->container == NULL) {
        return;
    }
    old_index = data->selected_index;
    if (data->selected_index < PAGE_TEST_ITEM_COUNT - 1) {
        data->selected_index++;
    } else {
        data->selected_index = 0;
    }
    update_selection_highlight(data, old_index, data->selected_index);
}

/* OK 键：进入当前选中项（复用触摸点击逻辑）。 */
static void ok_key_click_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    page_test_data_t* data = (page_test_data_t*)user_data;

    if (key != KEY_ID_OK || event_type != KEY_EVENT_CLICK) {
        return;
    }
    if (data == NULL || data->container == NULL) {
        return;
    }
    if (data->selected_index < 0 || data->selected_index >= PAGE_TEST_ITEM_COUNT) {
        return;
    }
    lv_obj_send_event(data->items[data->selected_index], LV_EVENT_CLICKED, NULL);
}

static void key_touch_item_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_manager_navigate("key_touch_test");
}

static void boot_switch_item_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_manager_navigate("boot_switch_test");
}

static void photo_resolution_item_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_manager_navigate("photo_resolution_test");
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

static lv_obj_t* create_menu_item(lv_obj_t* parent, const char* text, lv_event_cb_t cb)
{
    lv_obj_t* item = lv_obj_create(parent);
    lv_obj_t* label;

    lv_obj_set_width(item, lv_pct(100));
    lv_obj_set_height(item, TEST_MENU_ITEM_HEIGHT);
    lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(item, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(item, &style_settings_item, LV_PART_MAIN);
    lv_obj_add_event_cb(item, cb, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(item);
    lv_label_set_text(label, text);
    lv_obj_add_style(label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    return item;
}

void page_test_create(void)
{
    page_test_data_t* data = (page_test_data_t*)malloc(sizeof(page_test_data_t));
    lv_obj_t* nav_bar;
    lv_obj_t* back_btn;
    lv_obj_t* back_icon;
    lv_obj_t* list;

    if (data == NULL) {
        return;
    }
    memset(data, 0, sizeof(page_test_data_t));

    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(data->container, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_set_layout(data->container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->container, LV_FLEX_FLOW_COLUMN);
    lv_obj_refr_size(data->container);

    /* =======================
     * 1. 顶部导航栏
     * ======================= */
    nav_bar = lv_obj_create(data->container);
    lv_obj_set_width(nav_bar, lv_pct(100));
    lv_obj_set_height(nav_bar, TEST_NAV_BAR_HEIGHT);
    lv_obj_clear_flag(nav_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(nav_bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(nav_bar, &style_common_cont_top, LV_PART_MAIN);

    back_btn = lv_btn_create(nav_bar);
    lv_obj_set_size(back_btn, 50, 50);
    lv_obj_add_style(back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_add_event_cb(back_btn, page_manager_back_cb, LV_EVENT_CLICKED, NULL);
    back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle-white.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    data->title_label = lv_label_create(nav_bar);
    lv_label_set_text(data->title_label, "测试");
    lv_obj_add_style(data->title_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_align(data->title_label, LV_ALIGN_CENTER, 0, 0);

    /* =======================
     * 2. 测试项列表 - 占满剩余空间
     * ======================= */
    list = lv_obj_create(data->container);
    lv_obj_set_width(list, lv_pct(100));
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x121212), LV_PART_MAIN);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

    data->items[0] = create_menu_item(list, "触摸与按键", key_touch_item_cb);
    data->items[1] = create_menu_item(list, "模式切换测试", boot_switch_item_cb);
    data->items[2] = create_menu_item(list, "拍照分辨率切换测试", photo_resolution_item_cb);
    data->selected_index = 0;

    page_set_private_data(data);
}

void page_test_destroy(void)
{
    page_test_data_t* data = page_get_private_data();

    if (data == NULL) {
        return;
    }

    if (data->container != NULL) {
        lv_obj_del(data->container);
        data->container = NULL;
    }
    free(data);
}

void page_test_show(void)
{
    page_test_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Test menu page show");
    key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    key_manager_register_callback(KEY_ID_UP, KEY_EVENT_CLICK, up_key_click_cb, data);
    key_manager_register_callback(KEY_ID_DOWN, KEY_EVENT_CLICK, down_key_click_cb, data);
    key_manager_register_callback(KEY_ID_OK, KEY_EVENT_CLICK, ok_key_click_cb, data);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);

    /* 保持上次选中项不变（从下级返回时停在原处）；先清掉所有项高亮再高亮当前项，
     * 避免旧高亮残留导致多项同时高亮。 */
    for (int i = 0; i < PAGE_TEST_ITEM_COUNT; ++i) {
        if (data->items[i] != NULL) {
            lv_obj_remove_style(data->items[i], &style_settings_item_selected, LV_PART_MAIN);
        }
    }
    if (data->selected_index < 0 || data->selected_index >= PAGE_TEST_ITEM_COUNT) {
        data->selected_index = 0;
    }
    update_selection_highlight(data, -1, data->selected_index);
}

void page_test_hide(void)
{
    page_test_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Test menu page hide");
    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    key_manager_unregister_callback(KEY_ID_UP, KEY_EVENT_CLICK, up_key_click_cb, data);
    key_manager_unregister_callback(KEY_ID_DOWN, KEY_EVENT_CLICK, down_key_click_cb, data);
    key_manager_unregister_callback(KEY_ID_OK, KEY_EVENT_CLICK, ok_key_click_cb, data);
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_test_update(void)
{
}

// #endregion
