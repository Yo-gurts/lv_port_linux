// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_album.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/page_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <stdlib.h>
#include <string.h>

/* 布局常量 */
#define NAV_BAR_HEIGHT 50
#define GRID_ITEM_WIDTH 200
#define GRID_ITEM_HEIGHT 140
#define GRID_COLS 3
#define GRID_ROWS 3
#define GRID_GAP_X 4
#define GRID_GAP_Y 2

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义 (见 page_album.h)
// #############################################################################

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

/* 页面切换回调 */
static void tile_change_cb(lv_event_t* e)
{
    page_album_data_t* data = (page_album_data_t*)lv_event_get_user_data(e);
    if (!data)
        return;

    lv_obj_t* active_tile = lv_tileview_get_tile_active(data->grid_container);
    MLOG_INFO("Album page changed");
    LV_UNUSED(active_tile);
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

/* 返回按钮回调 */
static void back_btn_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm)
        return;

    MLOG_INFO("Back button clicked");
    page_manager_back(pm);
}

/* 拍照按钮回调 */
static void photo_btn_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm)
        return;

    MLOG_INFO("Photo button clicked");
}

/* 录像按钮回调 */
static void video_btn_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm)
        return;

    MLOG_INFO("Video button clicked");
}

/* 删除全部按钮回调 */
static void delete_all_btn_cb(lv_event_t* e)
{
    page_album_data_t* data = (page_album_data_t*)lv_event_get_user_data(e);
    if (!data)
        return;

    MLOG_INFO("Delete all button clicked");
    /* TODO: 弹出确认对话框 */
}

/* 删除按钮回调 */
static void delete_btn_cb(lv_event_t* e)
{
    page_album_data_t* data = (page_album_data_t*)lv_event_get_user_data(e);
    if (!data)
        return;

    MLOG_INFO("Delete button clicked");
    /* TODO: 删除选中的图片 */
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_album_create(page_manager_t* pm)
{
    if (!pm)
        return;

    page_album_data_t* data = (page_album_data_t*)malloc(sizeof(page_album_data_t));
    if (!data)
        return;

    memset(data, 0, sizeof(page_album_data_t));

    /* =======================
     * 1. 页面容器
     * ======================= */
    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_width(data->container, LV_PCT(100));
    lv_obj_set_height(data->container, LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_set_layout(data->container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->container, LV_FLEX_FLOW_COLUMN);
    lv_obj_refr_size(data->container);

    /* =======================
     * 2. 顶部导航栏
     * ======================= */
    data->nav_bar = lv_obj_create(data->container);
    lv_obj_set_width(data->nav_bar, lv_pct(100));
    lv_obj_set_height(data->nav_bar, NAV_BAR_HEIGHT);
    lv_obj_add_style(data->nav_bar, &style_common_cont_top, LV_PART_MAIN);
    lv_obj_clear_flag(data->nav_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(data->nav_bar, LV_SCROLLBAR_MODE_OFF);

    /* 返回按钮 - 左上角 */
    data->back_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(data->back_btn, NAV_BAR_HEIGHT, NAV_BAR_HEIGHT);
    lv_obj_add_style(data->back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->back_btn, back_btn_cb, LV_EVENT_CLICKED, pm);
    lv_obj_align(data->back_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t* back_icon = lv_img_create(data->back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-fill.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    /* 拍照按钮 */
    data->photo_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(data->photo_btn, NAV_BAR_HEIGHT, NAV_BAR_HEIGHT);
    lv_obj_add_style(data->photo_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->photo_btn, photo_btn_cb, LV_EVENT_CLICKED, pm);
    lv_obj_align(data->photo_btn, LV_ALIGN_CENTER, -50, 0);
    lv_obj_t* photo_icon = lv_img_create(data->photo_btn);
    lv_img_set_src(photo_icon, "A:" RES_ICON_PATH "/photo-white.png");
    lv_obj_align(photo_icon, LV_ALIGN_CENTER, 0, 0);

    /* 录像按钮 */
    data->video_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(data->video_btn, NAV_BAR_HEIGHT, NAV_BAR_HEIGHT);
    lv_obj_add_style(data->video_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->video_btn, video_btn_cb, LV_EVENT_CLICKED, pm);
    lv_obj_align(data->video_btn, LV_ALIGN_CENTER, 50, 0);
    lv_obj_t* video_icon = lv_img_create(data->video_btn);
    lv_img_set_src(video_icon, "A:" RES_ICON_PATH "/video-white.png");
    lv_obj_align(video_icon, LV_ALIGN_CENTER, 0, 0);

    /* 删除全部按钮 */
    data->delete_all_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(data->delete_all_btn, NAV_BAR_HEIGHT, NAV_BAR_HEIGHT);
    lv_obj_add_style(data->delete_all_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->delete_all_btn, delete_all_btn_cb, LV_EVENT_CLICKED, data);
    lv_obj_align(data->delete_all_btn, LV_ALIGN_RIGHT_MID, -70, 0);
    lv_obj_t* delete_all_icon = lv_img_create(data->delete_all_btn);
    lv_img_set_src(delete_all_icon, "A:" RES_ICON_PATH "/delete-all.png");
    lv_obj_align(delete_all_icon, LV_ALIGN_CENTER, 0, 0);

    /* 删除按钮 */
    data->delete_btn = lv_btn_create(data->nav_bar);
    lv_obj_set_size(data->delete_btn, NAV_BAR_HEIGHT, NAV_BAR_HEIGHT);
    lv_obj_add_style(data->delete_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_add_event_cb(data->delete_btn, delete_btn_cb, LV_EVENT_CLICKED, data);
    lv_obj_align(data->delete_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_t* delete_icon = lv_img_create(data->delete_btn);
    lv_img_set_src(delete_icon, "A:" RES_ICON_PATH "/delete.png");
    lv_obj_align(delete_icon, LV_ALIGN_CENTER, 0, 0);

    // /* 标题 - 居中 */
    // lv_obj_t* title_label = lv_label_create(data->nav_bar);
    // lv_label_set_text(title_label, "相册");
    // lv_obj_add_style(title_label, &NORMAL_SIZE, LV_PART_MAIN);
    // lv_obj_set_style_text_color(title_label, lv_color_white(), LV_PART_MAIN);
    // lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    /* =======================
     * 3. 9宫格区域 - 使用tileview实现滑动翻页
     * ======================= */
    data->grid_container = lv_tileview_create(data->container);
    lv_obj_set_width(data->grid_container, LV_PCT(100));
    lv_obj_set_flex_grow(data->grid_container, 1);
    lv_obj_set_style_bg_opa(data->grid_container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->grid_container, lv_color_hex(0x121212), LV_PART_MAIN);

    /* 计算整体 grid 尺寸 */
    int grid_width = GRID_ITEM_WIDTH * GRID_COLS + GRID_GAP_X * (GRID_COLS - 1);

    /* 创建3页（每页9张，共27张） */
    for (int page = 0; page < 3; page++) {
        lv_obj_t* tile = lv_tileview_add_tile(data->grid_container, 0, page, LV_DIR_VER);

        /* 计算水平起始位置（居中） */
        int start_x = (H_RES - grid_width) / 2;
        int start_y = GRID_GAP_Y;

        /* 每页9张图片 - 手动计算位置 */
        for (int i = 0; i < 9; i++) {
            int row = i / GRID_COLS;
            int col = i % GRID_COLS;
            int index = page * 9 + i;

            lv_obj_t* item_container = lv_obj_create(tile);
            lv_obj_set_size(item_container, GRID_ITEM_WIDTH, GRID_ITEM_HEIGHT);
            lv_obj_add_style(item_container, &style_settings_item, LV_PART_MAIN);
            lv_obj_clear_flag(item_container, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_scrollbar_mode(item_container, LV_SCROLLBAR_MODE_OFF);

            /* 计算位置 */
            int x = start_x + col * (GRID_ITEM_WIDTH + GRID_GAP_X);
            int y = start_y + row * (GRID_ITEM_HEIGHT + GRID_GAP_Y);
            lv_obj_set_pos(item_container, x, y);

            /* 图片占位符 */
            lv_obj_t* img = lv_img_create(item_container);
            lv_obj_set_size(img, GRID_ITEM_WIDTH, GRID_ITEM_HEIGHT);
            lv_obj_center(img);
            lv_obj_set_style_bg_opa(img, LV_OPA_20, LV_PART_MAIN);
            lv_obj_set_style_bg_color(img, lv_color_hex(0x333333), LV_PART_MAIN);

            /* 索引标签 */
            lv_obj_t* label = lv_label_create(item_container);
            lv_label_set_text_fmt(label, "%d", index + 1);
            lv_obj_add_style(label, &NORMAL_SIZE, LV_PART_MAIN);
            lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
            lv_obj_center(label);
        }
    }

    /* 设置默认显示第一页 */
    lv_tileview_set_tile_by_index(data->grid_container, 0, 0, LV_ANIM_OFF);

    /* 页面切换事件 */
    lv_obj_add_event_cb(data->grid_container, tile_change_cb, LV_EVENT_VALUE_CHANGED, data);

    /* 保存 private_data */
    page_set_private_data(pm, data);
}

void page_album_destroy(page_manager_t* pm)
{
    if (!pm)
        return;

    page_album_data_t* data = page_get_private_data(pm);
    if (!data)
        return;

    if (data->container) {
        lv_obj_del(data->container);
        data->container = NULL;
    }

    free(data);
}

void page_album_show(page_manager_t* pm)
{
    if (!pm)
        return;

    page_album_data_t* data = page_get_private_data(pm);
    if (!data || !data->container)
        return;

    MLOG_INFO("Album page show");
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_album_hide(page_manager_t* pm)
{
    if (!pm)
        return;

    page_album_data_t* data = page_get_private_data(pm);
    if (!data || !data->container)
        return;

    MLOG_INFO("Album page hide");
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_album_update(page_manager_t* pm)
{
    LV_UNUSED(pm);
}

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
