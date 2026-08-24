// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_test.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/page_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <stdlib.h>
#include <string.h>

#define TEST_TOP_BAR_HEIGHT 56
#define TEST_MENU_ITEM_WIDTH 560
#define TEST_MENU_ITEM_HEIGHT 88

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

static void back_btn_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_manager_back();
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

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

static lv_obj_t* create_menu_item(lv_obj_t* parent, const char* text, lv_event_cb_t cb)
{
    lv_obj_t* item = lv_btn_create(parent);
    lv_obj_t* label;

    lv_obj_set_size(item, TEST_MENU_ITEM_WIDTH, TEST_MENU_ITEM_HEIGHT);
    lv_obj_set_style_radius(item, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(item, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(item, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_add_event_cb(item, cb, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(item);
    lv_label_set_text(label, text);
    lv_obj_add_style(label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 24, 0);

    return item;
}

void page_test_create(void)
{
    page_test_data_t* data = (page_test_data_t*)malloc(sizeof(page_test_data_t));
    lv_obj_t* top_bar;
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
    lv_obj_set_style_bg_color(data->container, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_refr_size(data->container);

    top_bar = lv_obj_create(data->container);
    lv_obj_set_size(top_bar, LV_PCT(100), TEST_TOP_BAR_HEIGHT);
    lv_obj_add_style(top_bar, &style_noboarder, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(top_bar, LV_SCROLLBAR_MODE_OFF);

    back_btn = lv_btn_create(top_bar);
    lv_obj_set_size(back_btn, 50, 50);
    lv_obj_add_style(back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    data->title_label = lv_label_create(top_bar);
    lv_label_set_text(data->title_label, "测试");
    lv_obj_add_style(data->title_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->title_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(data->title_label, LV_ALIGN_LEFT_MID, 70, 0);

    list = lv_obj_create(data->container);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, TEST_TOP_BAR_HEIGHT + 40);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(list, 24, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

    data->key_touch_item = create_menu_item(list, "触摸与按键", key_touch_item_cb);
    data->boot_switch_item = create_menu_item(list, "模式切换测试", boot_switch_item_cb);

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
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_test_hide(void)
{
    page_test_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Test menu page hide");
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_test_update(void)
{
}

// #endregion
