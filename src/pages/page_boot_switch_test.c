// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_boot_switch_test.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/media_manager.h"
#include "core/page_manager.h"
#include "core/power_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BST_TOP_BAR_HEIGHT 56
#define BST_COUNT_BTN_WIDTH 120
#define BST_COUNT_BTN_HEIGHT 72

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static const uint32_t g_bst_options[PAGE_BOOT_SWITCH_TEST_OPTION_COUNT] = { 1U, 10U, 100U, 1000U };

/* 存活实例指针：异步切换完成回调可能在页面已销毁后才到，用它做生命周期守卫，
 * 避免解引用已 free 的 data。create 末尾置为本实例，destroy 开头(free 前)置 NULL。 */
static page_boot_switch_test_data_t* g_bst_active = NULL;

static void stop_loop(page_boot_switch_test_data_t* data);
static void refresh_progress(page_boot_switch_test_data_t* data);
static void start_next_half_step(page_boot_switch_test_data_t* data);
static void on_switch_done(int result);

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

static void refresh_progress(page_boot_switch_test_data_t* data)
{
    char buf[32];

    if (data == NULL || data->progress_label == NULL) {
        return;
    }
    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)data->done, (unsigned)data->target);
    lv_label_set_text(data->progress_label, buf);
}

static void set_options_enabled(page_boot_switch_test_data_t* data, int enabled)
{
    int i;

    if (data == NULL) {
        return;
    }
    for (i = 0; i < PAGE_BOOT_SWITCH_TEST_OPTION_COUNT; ++i) {
        if (data->count_btns[i] == NULL) {
            continue;
        }
        if (enabled) {
            lv_obj_clear_state(data->count_btns[i], LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(data->count_btns[i], LV_STATE_DISABLED);
        }
    }
}

static void highlight_selected_option(page_boot_switch_test_data_t* data)
{
    int i;

    if (data == NULL) {
        return;
    }
    for (i = 0; i < PAGE_BOOT_SWITCH_TEST_OPTION_COUNT; ++i) {
        if (data->count_btns[i] == NULL) {
            continue;
        }
        if (g_bst_options[i] == data->target) {
            lv_obj_set_style_bg_color(data->count_btns[i], lv_color_hex(0x2F80ED), LV_PART_MAIN);
        } else {
            lv_obj_set_style_bg_color(data->count_btns[i], lv_color_hex(0x444444), LV_PART_MAIN);
        }
    }
}

/* 拆掉运行态：清 running/phase/异步标志、次数按钮恢复可选、按钮标签回「开始」。
 * 不动 done（进度显示由调用方决定）。注意：不能取消已在途的异步请求，只清本地状态，
 * 靠 g_bst_active 与 running 守卫让晚到的回调空跑。 */
static void teardown_running(page_boot_switch_test_data_t* data)
{
    if (data == NULL) {
        return;
    }
    data->running = 0U;
    data->phase = 0U;
    data->waiting = 0U;
    data->stop_pending = 0U;
    data->back_pending = 0U;
    set_options_enabled(data, 1);
    if (data->start_label != NULL) {
        lv_label_set_text(data->start_label, "开始");
    }
}

/* 停止并复位：进度归零。「暂停」/中止走此路径（用户约定：暂停即停止复位，
 * 再点从头重新跑）。 */
static void stop_loop(page_boot_switch_test_data_t* data)
{
    if (data == NULL) {
        return;
    }
    teardown_running(data);
    data->done = 0U;
    refresh_progress(data);
}

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

/* 发起当前 phase 对应的异步半步：phase==0 切拍照，phase==1 切 boot。
 * 发出后立即返回（不阻塞 UI），完成时 on_switch_done 在 UI 线程被回调。 */
static void start_next_half_step(page_boot_switch_test_data_t* data)
{
    media_operation_t op;
    int ret;

    if (data == NULL || !data->running) {
        return;
    }

    op = (data->phase == 0U) ? MEDIA_OP_SWITCH_TO_PHOTO_MODE : MEDIA_OP_SWITCH_TO_BOOT_MODE;
    ret = media_manager_execute_async(op, on_switch_done);
    if (ret != MEDIA_MANAGER_OK) {
        MLOG_ERR("boot切换测试: 发起异步切换失败 op=%d ret=%d, 停止", op, ret);
        stop_loop(data);
        return;
    }
    data->waiting = 1U;
}

/* 异步半步完成回调（media 层已 hop 到 UI 线程，可安全操作 lv_obj）。
 * 一整轮「拍照 -> boot」完成才算 done + 1；暂停/返回在 boot 完成点收尾，保证停在 boot。
 * 用 g_bst_active + running 守卫：回调晚到、页面已销毁则空跑，绝不解引用悬空 data。 */
static void on_switch_done(int result)
{
    page_boot_switch_test_data_t* data = g_bst_active;
    uint8_t need_back;

    if (data == NULL || !data->running) {
        return;
    }
    data->waiting = 0U;

    if (result != 0) {
        /* 切换失败：直接收尾。back_pending 会被 stop_loop 清掉，先存下来。 */
        need_back = data->back_pending;
        MLOG_ERR("boot切换测试: 切换失败 result=%d, 停止", result);
        stop_loop(data);
        if (need_back) {
            page_manager_back();
        }
        return;
    }

    if (data->phase == 0U) {
        /* 切拍照完成，接着切 boot。 */
        data->phase = 1U;
        start_next_half_step(data);
        return;
    }

    /* 切 boot 完成：一整轮结束。 */
    data->phase = 0U;
    data->done++;
    refresh_progress(data);

    /* 收尾优先级：暂停/返回请求 -> 到达目标 -> 继续下一轮。此刻已落在 boot。 */
    if (data->stop_pending) {
        need_back = data->back_pending; /* stop_loop 会清 back_pending，先存下来 */
        MLOG_INFO("boot切换测试: 在 boot 收尾暂停, 已完成 %u 次", (unsigned)data->done);
        stop_loop(data);
        if (need_back) {
            page_manager_back();
        }
        return;
    }

    if (data->done >= data->target) {
        MLOG_INFO("boot切换测试完成: %u 次", (unsigned)data->done);
        teardown_running(data); /* 保留满进度显示 target/target */
        return;
    }

    start_next_half_step(data);
}

/* 返回：非运行态直接返回。运行态则「等切回 boot 再退」——若已在 boot 边界(phase==0
 * 且无在途)可立即停并返回；否则置 stop_pending+back_pending，在下一个 boot 完成点
 * (on_switch_done)收尾后再 page_manager_back，保证退菜单时系统落在 boot。 */
static void back_btn_cb(lv_event_t* e)
{
    page_boot_switch_test_data_t* data = (page_boot_switch_test_data_t*)lv_event_get_user_data(e);

    if (data != NULL && data->running) {
        if (data->phase == 0U && !data->waiting) {
            MLOG_INFO("boot切换测试运行中点返回: 已在 boot 边界, 直接返回");
            stop_loop(data);
            page_manager_back();
        } else {
            MLOG_INFO("boot切换测试运行中点返回: 等切回 boot 完成后返回");
            data->stop_pending = 1U;
            data->back_pending = 1U;
        }
        return;
    }
    page_manager_back();
}

static void count_btn_cb(lv_event_t* e)
{
    page_boot_switch_test_data_t* data = (page_boot_switch_test_data_t*)lv_event_get_user_data(e);
    lv_obj_t* btn = lv_event_get_target(e);
    int i;

    if (data == NULL || data->running) {
        return;
    }
    for (i = 0; i < PAGE_BOOT_SWITCH_TEST_OPTION_COUNT; ++i) {
        if (data->count_btns[i] == btn) {
            data->target = g_bst_options[i];
            break;
        }
    }
    highlight_selected_option(data);
    refresh_progress(data);
}

/* 开始/暂停 切换。非运行态点击 = 从头开始，发起第一个异步半步。运行态点击 = 暂停：
 * 若已在 boot 边界(phase==0 且无在途)立即停；否则置 stop_pending，在下一个 boot 完成点
 * 收尾，保证暂停一定停在 boot 模式。异步不阻塞 UI，最多等一次切换但界面全程响应。 */
static void start_btn_cb(lv_event_t* e)
{
    page_boot_switch_test_data_t* data = (page_boot_switch_test_data_t*)lv_event_get_user_data(e);

    if (data == NULL) {
        return;
    }

    if (data->running) {
        if (data->phase == 0U && !data->waiting) {
            MLOG_INFO("boot切换测试暂停: 已在 boot 边界; 已完成 %u 次", (unsigned)data->done);
            stop_loop(data);
        } else {
            MLOG_INFO("boot切换测试暂停: 待切回 boot 收尾; 已完成 %u 次", (unsigned)data->done);
            data->stop_pending = 1U;
        }
        return;
    }

    if (data->target == 0U) {
        data->target = 1U;
    }

    data->done = 0U;
    data->phase = 0U;
    data->running = 1U;
    data->waiting = 0U;
    data->stop_pending = 0U;
    data->back_pending = 0U;
    refresh_progress(data);

    set_options_enabled(data, 0);
    if (data->start_label != NULL) {
        lv_label_set_text(data->start_label, "暂停");
    }

    MLOG_INFO("boot切换测试开始: 目标 %u 次", (unsigned)data->target);
    start_next_half_step(data);
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_boot_switch_test_create(void)
{
    page_boot_switch_test_data_t* data = (page_boot_switch_test_data_t*)malloc(sizeof(page_boot_switch_test_data_t));
    lv_obj_t* top_bar;
    lv_obj_t* back_btn;
    lv_obj_t* back_icon;
    lv_obj_t* option_row;
    int i;

    if (data == NULL) {
        return;
    }
    memset(data, 0, sizeof(page_boot_switch_test_data_t));
    data->target = 1U;

    /* 全透明容器：叠在 sensor 视频层之上，方便透出画面。 */
    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(data->container, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_refr_size(data->container);

    top_bar = lv_obj_create(data->container);
    lv_obj_set_size(top_bar, LV_PCT(100), BST_TOP_BAR_HEIGHT);
    lv_obj_add_style(top_bar, &style_noboarder, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(top_bar, LV_SCROLLBAR_MODE_OFF);

    back_btn = lv_btn_create(top_bar);
    lv_obj_set_size(back_btn, 50, 50);
    lv_obj_add_style(back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, data);
    back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    data->title_label = lv_label_create(top_bar);
    lv_label_set_text(data->title_label, "boot切换开关测试");
    lv_obj_add_style(data->title_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->title_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(data->title_label, LV_ALIGN_LEFT_MID, 70, 0);

    /* 次数选择：1 / 10 / 100 / 1000 */
    option_row = lv_obj_create(data->container);
    lv_obj_remove_style_all(option_row);
    lv_obj_set_size(option_row, LV_PCT(100), BST_COUNT_BTN_HEIGHT + 20);
    lv_obj_align(option_row, LV_ALIGN_TOP_MID, 0, BST_TOP_BAR_HEIGHT + 40);
    lv_obj_set_flex_flow(option_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(option_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(option_row, 16, LV_PART_MAIN);
    lv_obj_clear_flag(option_row, LV_OBJ_FLAG_SCROLLABLE);

    for (i = 0; i < PAGE_BOOT_SWITCH_TEST_OPTION_COUNT; ++i) {
        char label_buf[16];
        lv_obj_t* btn = lv_btn_create(option_row);
        lv_obj_t* lbl;

        lv_obj_set_size(btn, BST_COUNT_BTN_WIDTH, BST_COUNT_BTN_HEIGHT);
        lv_obj_set_style_radius(btn, 12, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_80, LV_PART_MAIN);
        lv_obj_add_event_cb(btn, count_btn_cb, LV_EVENT_CLICKED, data);

        lbl = lv_label_create(btn);
        snprintf(label_buf, sizeof(label_buf), "%u", (unsigned)g_bst_options[i]);
        lv_label_set_text(lbl, label_buf);
        lv_obj_add_style(lbl, &NORMAL_SIZE, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl, lv_color_white(), LV_PART_MAIN);
        lv_obj_center(lbl);

        data->count_btns[i] = btn;
    }

    /* 开始按钮 */
    data->start_btn = lv_btn_create(data->container);
    lv_obj_set_size(data->start_btn, 200, 72);
    lv_obj_set_style_radius(data->start_btn, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->start_btn, lv_color_hex(0x27AE60), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->start_btn, LV_OPA_80, LV_PART_MAIN);
    lv_obj_align(data->start_btn, LV_ALIGN_CENTER, 0, 30);
    lv_obj_add_event_cb(data->start_btn, start_btn_cb, LV_EVENT_CLICKED, data);
    data->start_label = lv_label_create(data->start_btn);
    lv_label_set_text(data->start_label, "开始");
    lv_obj_add_style(data->start_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->start_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(data->start_label);

    /* 进度显示：白色字体 */
    data->progress_label = lv_label_create(data->container);
    lv_obj_add_style(data->progress_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->progress_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(data->progress_label, LV_ALIGN_CENTER, 0, 130);

    highlight_selected_option(data);
    refresh_progress(data);
    page_set_private_data(data);
    g_bst_active = data;
}

void page_boot_switch_test_destroy(void)
{
    page_boot_switch_test_data_t* data = page_get_private_data();

    if (data == NULL) {
        return;
    }

    /* 先清存活指针：任何在途异步切换的完成回调此后都会安全空跑，不解引用被 free 的 data。 */
    g_bst_active = NULL;
    teardown_running(data);
    if (data->auto_sleep_disabled) {
        power_manager_enable_auto_sleep();
        data->auto_sleep_disabled = 0U;
    }
    if (data->container != NULL) {
        lv_obj_del(data->container);
        data->container = NULL;
    }
    free(data);
}

void page_boot_switch_test_show(void)
{
    page_boot_switch_test_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Boot switch test page show");
    if (!data->auto_sleep_disabled) {
        power_manager_disable_auto_sleep();
        data->auto_sleep_disabled = 1U;
    }

    /* 复位测试状态（不自动启动）。 */
    stop_loop(data);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_boot_switch_test_hide(void)
{
    page_boot_switch_test_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Boot switch test page hide");
    stop_loop(data);
    if (data->auto_sleep_disabled) {
        power_manager_enable_auto_sleep();
        data->auto_sleep_disabled = 0U;
    }
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_boot_switch_test_update(void)
{
}

// #endregion
