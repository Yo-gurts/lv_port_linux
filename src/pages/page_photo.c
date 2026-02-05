#include "pages/page_photo.h"
#include "config.h"
#include "font_manager.h"
#include "mlog.h"
#include "styles/style_common.h"
#include <stdlib.h>
#include <string.h>

/* 布局常量 */
#define TOP_BAR_HEIGHT 50
#define BOTTOM_BAR_HEIGHT 50

/* 拍照/录像切换回调 */
static void mode_switch_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }

    page_photo_data_t* data = (page_photo_data_t*)page_get_private_data(pm, "page_photo_data");
    if (!data) {
        return;
    }

    /* 切换拍照/录像模式 */
    data->is_video_mode = !data->is_video_mode;

    /* 切换图片 */
    if (data->is_video_mode) {
        lv_img_set_src(data->mode_img, "A:" RES_ICON_PATH "/video.png");
    } else {
        lv_img_set_src(data->mode_img, "A:" RES_ICON_PATH "/photo.png");
    }

    MLOG_INFO("Mode switched to %s", data->is_video_mode ? "video" : "photo");
}

void page_photo_create(page_manager_t* pm, void* user_data)
{
    if (!pm) {
        return;
    }

    LV_UNUSED(user_data);
    MLOG_INFO("Photo page created");

    page_photo_data_t* data = (page_photo_data_t*)malloc(sizeof(page_photo_data_t));
    if (!data) {
        return;
    }

    memset(data, 0, sizeof(page_photo_data_t));
    data->is_video_mode = 0; /* 默认拍照模式 */

    data->root = lv_screen_active();

    /* 背景不透明（SDL模拟时避免拖影） */
    lv_obj_set_style_bg_color(data->root, lv_color_hex(0x1a1a2e), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->root, 0, LV_PART_MAIN);

    /* =======================
     * 顶部状态栏：8M | 100 | [SD] | [🔋]
     * ======================= */
    lv_obj_t* top_bar = lv_obj_create(data->root);
    lv_obj_set_width(top_bar, lv_pct(100));
    lv_obj_set_height(top_bar, TOP_BAR_HEIGHT);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_TRANSP, LV_PART_MAIN); /* 全透明 */
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(top_bar, LV_GRAD_DIR_NONE, LV_PART_MAIN);
    lv_obj_set_style_border_width(top_bar, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(top_bar, LV_SCROLLBAR_MODE_OFF); /* 不可滚动 */
    lv_obj_set_layout(top_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(top_bar, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_right(top_bar, 20, LV_PART_MAIN);

    /* 分辨率 Label - 显示 8M */
    data->resolution_label = lv_label_create(top_bar);
    lv_label_set_text(data->resolution_label, "8M");
    lv_obj_add_style(data->resolution_label, &ttf_font_20, LV_PART_MAIN);

    /* 剩余照片数量 Label - 显示 100 */
    lv_obj_t* photo_count_label = lv_label_create(top_bar);
    lv_label_set_text(photo_count_label, "100");
    lv_obj_add_style(photo_count_label, &ttf_font_20, LV_PART_MAIN);

    /* SD卡图标 */
    data->sd_icon = lv_label_create(top_bar);
    lv_label_set_text(data->sd_icon, LV_SYMBOL_DIRECTORY);
    lv_obj_set_style_text_font(data->sd_icon, &lv_font_montserrat_20, LV_PART_MAIN);

    /* 电池图标 */
    data->battery_icon = lv_label_create(top_bar);
    lv_label_set_text(data->battery_icon, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_font(data->battery_icon, &lv_font_montserrat_20, LV_PART_MAIN);

    /* =======================
     * 底部工具栏：[≡] | [📷] | [◫] | [↻]
     * ======================= */
    lv_obj_t* bottom_bar = lv_obj_create(data->root);
    lv_obj_set_width(bottom_bar, lv_pct(100));
    lv_obj_set_height(bottom_bar, BOTTOM_BAR_HEIGHT);
    lv_obj_align(bottom_bar, LV_ALIGN_BOTTOM_MID, 0, 0); /* 贴在底部 */
    lv_obj_set_style_bg_opa(bottom_bar, LV_OPA_TRANSP, LV_PART_MAIN); /* 全透明 */
    lv_obj_set_style_bg_color(bottom_bar, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(bottom_bar, LV_GRAD_DIR_NONE, LV_PART_MAIN);
    lv_obj_set_style_border_width(bottom_bar, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(bottom_bar, LV_SCROLLBAR_MODE_OFF); /* 不可滚动 */
    lv_obj_set_layout(bottom_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bottom_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 菜单按钮 */
    data->menu_btn = lv_btn_create(bottom_bar);
    lv_obj_set_size(data->menu_btn, 50, 50);
    lv_obj_add_style(data->menu_btn, &style_common_btn_back, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(data->menu_btn, NULL, LV_EVENT_CLICKED, pm);
    lv_obj_t* menu_icon = lv_label_create(data->menu_btn);
    lv_label_set_text(menu_icon, LV_SYMBOL_LIST);
    lv_obj_set_style_text_font(menu_icon, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(menu_icon, LV_ALIGN_CENTER, 0, 0);

    /* 拍照/录像切换按钮 */
    data->mode_btn = lv_btn_create(bottom_bar);
    lv_obj_set_size(data->mode_btn, 50, 50);
    lv_obj_add_event_cb(data->mode_btn, mode_switch_cb, LV_EVENT_CLICKED, pm);
    data->mode_img = lv_img_create(data->mode_btn);
    lv_img_set_src(data->mode_img, "A:" RES_ICON_PATH "/photo.png");
    lv_obj_align(data->mode_img, LV_ALIGN_CENTER, 0, 0);

    /* 滤镜按钮 */
    data->filter_btn = lv_btn_create(bottom_bar);
    lv_obj_set_size(data->filter_btn, 50, 50);
    lv_obj_add_style(data->filter_btn, &style_common_btn_back, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(data->filter_btn, NULL, LV_EVENT_CLICKED, pm);
    lv_obj_t* filter_icon = lv_label_create(data->filter_btn);
    lv_label_set_text(filter_icon, LV_SYMBOL_IMAGE);
    lv_obj_set_style_text_font(filter_icon, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(filter_icon, LV_ALIGN_CENTER, 0, 0);

    /* 摄像头切换按钮 */
    data->switch_btn = lv_btn_create(bottom_bar);
    lv_obj_set_size(data->switch_btn, 50, 50);
    lv_obj_add_style(data->switch_btn, &style_common_btn_back, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(data->switch_btn, NULL, LV_EVENT_CLICKED, pm);
    lv_obj_t* switch_icon = lv_label_create(data->switch_btn);
    lv_label_set_text(switch_icon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(switch_icon, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(switch_icon, LV_ALIGN_CENTER, 0, 0);

    page_set_private_data(pm, "page_photo_data", data);
}

void page_photo_destroy(page_manager_t* pm, void* user_data)
{
    if (!pm) {
        return;
    }

    LV_UNUSED(user_data);
    MLOG_INFO("Photo page destroyed");

    page_photo_data_t* data = (page_photo_data_t*)page_get_private_data(pm, "page_photo_data");
    if (!data) {
        return;
    }

    /* 删除根对象（子对象会自动删除） */
    if (data->root) {
        lv_obj_del(data->root);
        data->root = NULL;
    }

    free(data);
}

void page_photo_show(page_manager_t* pm, void* user_data)
{
    if (!pm) {
        return;
    }

    LV_UNUSED(user_data);
    MLOG_INFO("Photo page shown");
}

void page_photo_hide(page_manager_t* pm, void* user_data)
{
    if (!pm) {
        return;
    }

    LV_UNUSED(user_data);
    MLOG_INFO("Photo page hidden");
}

void page_photo_update(page_manager_t* pm, void* user_data)
{
    LV_UNUSED(pm);
    LV_UNUSED(user_data);
}
