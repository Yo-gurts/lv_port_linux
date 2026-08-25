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
#define BST_BTN_HEIGHT 50 /* 三个操作按钮统一高度（<=55px，尽量不遮画面） */
#define BST_ACTION_BTN_WIDTH 150
#define BST_COUNT_BTN_WIDTH 110
#define BST_START_BTN_WIDTH 110
#define BST_BTN_MARGIN 10

#define BST_TARGET_INFINITE UINT32_MAX /* 无穷次哨兵：done 永不可达，只能手动暂停/返回 */

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static const uint32_t g_bst_options[PAGE_BOOT_SWITCH_TEST_OPTION_COUNT]
    = { 1U, 100U, 1000U, 10000U, BST_TARGET_INFINITE };

static const char* const g_bst_action_labels[PAGE_BOOT_SWITCH_TEST_ACTION_COUNT] = {
    "拍照-boot",
    "录像-boot",
    "拍照-录像",
};

/* 存活实例指针：异步切换完成回调可能在页面已销毁后才到，用它做生命周期守卫，
 * 避免解引用已 free 的 data。create 末尾置为本实例，destroy 开头(free 前)置 NULL。 */
static page_boot_switch_test_data_t* g_bst_active = NULL;

static void stop_loop(page_boot_switch_test_data_t* data);
static void refresh_progress(page_boot_switch_test_data_t* data);
static void start_next_half_step(page_boot_switch_test_data_t* data);
static void on_switch_done(int result);

/* 当前动作两端模式：op_a 为一轮先切到的模式，op_b 为一轮末尾切到的模式。
 * 拍照-boot/录像-boot 的 op_b 即 boot（一轮末尾天然落在 boot）；
 * 拍照-录像 的 op_b 是录像，需额外收尾半步回 boot（不计数）。 */
static void bst_action_modes(bst_action_t action, media_operation_t* op_a, media_operation_t* op_b)
{
    switch (action) {
    case BST_ACTION_VIDEO_BOOT:
        *op_a = MEDIA_OP_SWITCH_TO_VIDEO_MODE;
        *op_b = MEDIA_OP_SWITCH_TO_BOOT_MODE;
        break;
    case BST_ACTION_PHOTO_VIDEO:
        *op_a = MEDIA_OP_SWITCH_TO_PHOTO_MODE;
        *op_b = MEDIA_OP_SWITCH_TO_VIDEO_MODE;
        break;
    case BST_ACTION_PHOTO_BOOT:
    default:
        *op_a = MEDIA_OP_SWITCH_TO_PHOTO_MODE;
        *op_b = MEDIA_OP_SWITCH_TO_BOOT_MODE;
        break;
    }
}

/* 一轮末尾是否天然落在 boot（op_b==boot）。拍照-录像动作为否，需要收尾半步。 */
static int bst_ends_at_boot(bst_action_t action)
{
    return action != BST_ACTION_PHOTO_VIDEO;
}

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
    if (data->target == BST_TARGET_INFINITE) {
        snprintf(buf, sizeof(buf), "%u/无穷", (unsigned)data->done);
    } else {
        snprintf(buf, sizeof(buf), "%u/%u", (unsigned)data->done, (unsigned)data->target);
    }
    lv_label_set_text(data->progress_label, buf);
}

/* 运行中锁定动作与次数按钮（不可改动作/次数）；非运行态恢复可选。 */
static void set_options_enabled(page_boot_switch_test_data_t* data, int enabled)
{
    lv_obj_t* btns[2];
    int i;

    if (data == NULL) {
        return;
    }
    btns[0] = data->action_btn;
    btns[1] = data->count_btn;
    for (i = 0; i < 2; ++i) {
        if (btns[i] == NULL) {
            continue;
        }
        if (enabled) {
            lv_obj_clear_state(btns[i], LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(btns[i], LV_STATE_DISABLED);
        }
    }
}

/* 刷新动作按钮文字为当前选中动作。 */
static void refresh_action_label(page_boot_switch_test_data_t* data)
{
    if (data == NULL || data->action_label == NULL) {
        return;
    }
    lv_label_set_text(data->action_label, g_bst_action_labels[data->action]);
}

/* 刷新次数按钮文字为当前选中次数（无穷显示「无穷」）。 */
static void refresh_count_label(page_boot_switch_test_data_t* data)
{
    char buf[16];

    if (data == NULL || data->count_label == NULL) {
        return;
    }
    if (data->target == BST_TARGET_INFINITE) {
        lv_label_set_text(data->count_label, "无穷");
    } else {
        snprintf(buf, sizeof(buf), "%u", (unsigned)data->target);
        lv_label_set_text(data->count_label, buf);
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
    data->finishing = 0U;
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

/* 发起当前 phase 对应的异步半步：phase==0 切 op_a，phase==1 切 op_b。
 * 发出后立即返回（不阻塞 UI），完成时 on_switch_done 在 UI 线程被回调。 */
static void start_next_half_step(page_boot_switch_test_data_t* data)
{
    media_operation_t op_a, op_b, op;
    int ret;

    if (data == NULL || !data->running) {
        return;
    }

    bst_action_modes(data->action, &op_a, &op_b);
    op = (data->phase == 0U) ? op_a : op_b;
    ret = media_manager_execute_async(op, 0, on_switch_done);
    if (ret != MEDIA_MANAGER_OK) {
        MLOG_ERR("模式切换测试: 发起异步切换失败 op=%d ret=%d, 停止", op, ret);
        stop_loop(data);
        return;
    }
    data->waiting = 1U;
}

/* 收尾停止的最终动作：keep_progress 为真则保留满进度显示(达标)，否则归零(暂停/返回)。 */
static void finish_settle(page_boot_switch_test_data_t* data, int keep_progress)
{
    uint8_t need_back = data->back_pending;

    if (keep_progress) {
        teardown_running(data);
    } else {
        stop_loop(data);
    }
    if (need_back) {
        page_manager_back();
    }
}

/* 收尾并停止（保证落在 boot）：拍照-录像动作一轮末尾停在录像，需额外异步切一次 boot
 * (finishing=1, 不计数)，其完成回调里再 finish_settle；其余动作末尾已在 boot，直接停。
 * keep_progress 语义见 finish_settle。 */
static void finish_to_boot_then_stop(page_boot_switch_test_data_t* data, int keep_progress)
{
    int ret;

    if (data == NULL) {
        return;
    }

    if (bst_ends_at_boot(data->action)) {
        /* op_b==boot，此刻已在 boot，直接收尾。 */
        finish_settle(data, keep_progress);
        return;
    }

    /* 拍照-录像：多切一次 boot 收尾（不计数）。记住 keep_progress 到 finishing 回调。 */
    MLOG_INFO("模式切换测试: 收尾切回 boot");
    data->finishing = keep_progress ? 2U : 1U; /* 1=归零收尾, 2=保留进度收尾 */
    ret = media_manager_execute_async(MEDIA_OP_SWITCH_TO_BOOT_MODE, 0, on_switch_done);
    if (ret != MEDIA_MANAGER_OK) {
        MLOG_ERR("模式切换测试: 收尾切 boot 失败 ret=%d, 直接停止", ret);
        data->finishing = 0U;
        finish_settle(data, keep_progress);
        return;
    }
    data->waiting = 1U;
}

/* 异步半步完成回调（media 层已 hop 到 UI 线程，可安全操作 lv_obj）。
 * 一整轮「op_a -> op_b」完成才算 done + 1；暂停/返回/达标时统一收尾回 boot，保证停在 boot。
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
        MLOG_ERR("模式切换测试: 切换失败 result=%d, 停止", result);
        stop_loop(data);
        if (need_back) {
            page_manager_back();
        }
        return;
    }

    /* 收尾半步（切回 boot）完成：真正停止/返回。finishing==2 保留进度, ==1 归零。 */
    if (data->finishing) {
        int keep_progress = (data->finishing == 2U);
        data->finishing = 0U;
        MLOG_INFO("模式切换测试: 收尾已落 boot, 已完成 %u 次", (unsigned)data->done);
        finish_settle(data, keep_progress);
        return;
    }

    if (data->phase == 0U) {
        /* 切 op_a 完成，接着切 op_b。 */
        data->phase = 1U;
        start_next_half_step(data);
        return;
    }

    /* 切 op_b 完成：一整轮结束。 */
    data->phase = 0U;
    data->done++;
    refresh_progress(data);

    /* 收尾优先级：暂停/返回请求 -> 到达目标 -> 继续下一轮。 */
    if (data->stop_pending) {
        MLOG_INFO("模式切换测试: 收尾暂停, 已完成 %u 次", (unsigned)data->done);
        finish_to_boot_then_stop(data, 0); /* 归零进度 */
        return;
    }

    if (data->done >= data->target) { /* 无穷次 target==UINT32_MAX 恒不成立 */
        MLOG_INFO("模式切换测试完成: %u 次", (unsigned)data->done);
        finish_to_boot_then_stop(data, 1); /* 保留满进度显示 */
        return;
    }

    start_next_half_step(data);
}

/* 返回：非运行态直接返回。运行态则「等切回 boot 再退」——仅当已在 boot 边界(op_b==boot、
 * phase==0、无在途)可立即停并返回；否则置 stop_pending+back_pending，在一轮完成点
 * (on_switch_done)收尾回 boot 后再 page_manager_back，保证退菜单时系统落在 boot。 */
static void back_btn_cb(lv_event_t* e)
{
    page_boot_switch_test_data_t* data = (page_boot_switch_test_data_t*)lv_event_get_user_data(e);

    if (data != NULL && data->running) {
        if (data->phase == 0U && !data->waiting && bst_ends_at_boot(data->action)) {
            MLOG_INFO("模式切换测试运行中点返回: 已在 boot 边界, 直接返回");
            stop_loop(data);
            page_manager_back();
        } else {
            MLOG_INFO("模式切换测试运行中点返回: 等切回 boot 完成后返回");
            data->stop_pending = 1U;
            data->back_pending = 1U;
        }
        return;
    }
    page_manager_back();
}

/* 次数按钮：点一下循环切下一个次数（1->100->1000->10000->无穷->回到 1）。 */
static void count_btn_cb(lv_event_t* e)
{
    page_boot_switch_test_data_t* data = (page_boot_switch_test_data_t*)lv_event_get_user_data(e);

    if (data == NULL || data->running) {
        return;
    }
    data->count_idx = (uint8_t)((data->count_idx + 1U) % PAGE_BOOT_SWITCH_TEST_OPTION_COUNT);
    data->target = g_bst_options[data->count_idx];
    refresh_count_label(data);
    refresh_progress(data);
}

/* 动作按钮：点一下循环切下一个动作（拍照-boot->录像-boot->拍照-录像->回到拍照-boot）。 */
static void action_btn_cb(lv_event_t* e)
{
    page_boot_switch_test_data_t* data = (page_boot_switch_test_data_t*)lv_event_get_user_data(e);

    if (data == NULL || data->running) {
        return;
    }
    data->action = (bst_action_t)(((int)data->action + 1) % PAGE_BOOT_SWITCH_TEST_ACTION_COUNT);
    refresh_action_label(data);
}

/* 开始/暂停 切换。非运行态点击 = 从头开始，发起第一个异步半步。运行态点击 = 暂停：
 * 若已在 boot 边界(op_b==boot、phase==0、无在途)立即停；否则置 stop_pending，在一轮完成点
 * 收尾回 boot，保证暂停一定停在 boot 模式。异步不阻塞 UI，最多等一次切换但界面全程响应。 */
static void start_btn_cb(lv_event_t* e)
{
    page_boot_switch_test_data_t* data = (page_boot_switch_test_data_t*)lv_event_get_user_data(e);

    if (data == NULL) {
        return;
    }

    if (data->running) {
        if (data->phase == 0U && !data->waiting && bst_ends_at_boot(data->action)) {
            MLOG_INFO("模式切换测试暂停: 已在 boot 边界; 已完成 %u 次", (unsigned)data->done);
            stop_loop(data);
        } else {
            MLOG_INFO("模式切换测试暂停: 待切回 boot 收尾; 已完成 %u 次", (unsigned)data->done);
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
    data->finishing = 0U;
    data->stop_pending = 0U;
    data->back_pending = 0U;
    refresh_progress(data);

    set_options_enabled(data, 0);
    if (data->start_label != NULL) {
        lv_label_set_text(data->start_label, "暂停");
    }

    MLOG_INFO("模式切换测试开始: 动作 %d 目标 %u 次", (int)data->action, (unsigned)data->target);
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

    if (data == NULL) {
        return;
    }
    memset(data, 0, sizeof(page_boot_switch_test_data_t));
    data->target = 1U;
    data->action = BST_ACTION_PHOTO_BOOT;

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
    lv_label_set_text(data->title_label, "模式切换测试");
    lv_obj_add_style(data->title_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->title_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(data->title_label, LV_ALIGN_LEFT_MID, 70, 0);

    /* 动作按钮（顶栏最右，与返回按钮同一行）：点一下循环切下一个动作，文字显示当前动作。 */
    data->action_btn = lv_btn_create(top_bar);
    lv_obj_set_size(data->action_btn, BST_ACTION_BTN_WIDTH, BST_BTN_HEIGHT);
    lv_obj_set_style_radius(data->action_btn, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->action_btn, lv_color_hex(0x2F80ED), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->action_btn, LV_OPA_80, LV_PART_MAIN);
    lv_obj_align(data->action_btn, LV_ALIGN_RIGHT_MID, -BST_BTN_MARGIN, 0);
    lv_obj_add_event_cb(data->action_btn, action_btn_cb, LV_EVENT_CLICKED, data);
    data->action_label = lv_label_create(data->action_btn);
    lv_obj_add_style(data->action_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->action_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(data->action_label);

    /* 次数按钮（左下角）：点一下循环切下一个次数，文字显示当前次数。 */
    data->count_btn = lv_btn_create(data->container);
    lv_obj_set_size(data->count_btn, BST_COUNT_BTN_WIDTH, BST_BTN_HEIGHT);
    lv_obj_set_style_radius(data->count_btn, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->count_btn, lv_color_hex(0x2F80ED), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->count_btn, LV_OPA_80, LV_PART_MAIN);
    lv_obj_align(data->count_btn, LV_ALIGN_BOTTOM_LEFT, BST_BTN_MARGIN, -BST_BTN_MARGIN);
    lv_obj_add_event_cb(data->count_btn, count_btn_cb, LV_EVENT_CLICKED, data);
    data->count_label = lv_label_create(data->count_btn);
    lv_obj_add_style(data->count_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->count_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(data->count_label);

    /* 开始按钮（右下角）：开始/暂停。 */
    data->start_btn = lv_btn_create(data->container);
    lv_obj_set_size(data->start_btn, BST_START_BTN_WIDTH, BST_BTN_HEIGHT);
    lv_obj_set_style_radius(data->start_btn, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->start_btn, lv_color_hex(0x27AE60), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->start_btn, LV_OPA_80, LV_PART_MAIN);
    lv_obj_align(data->start_btn, LV_ALIGN_BOTTOM_RIGHT, -BST_BTN_MARGIN, -BST_BTN_MARGIN);
    lv_obj_add_event_cb(data->start_btn, start_btn_cb, LV_EVENT_CLICKED, data);
    data->start_label = lv_label_create(data->start_btn);
    lv_label_set_text(data->start_label, "开始");
    lv_obj_add_style(data->start_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->start_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(data->start_label);

    /* 进度显示：底部居中，白色小字。 */
    data->progress_label = lv_label_create(data->container);
    lv_obj_add_style(data->progress_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->progress_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(data->progress_label, LV_ALIGN_BOTTOM_MID, 0, -BST_BTN_MARGIN - 6);

    refresh_action_label(data);
    refresh_count_label(data);
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
