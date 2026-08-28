// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_photo_resolution_test.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/key_manager.h"
#include "core/media_manager.h"
#include "core/page_manager.h"
#include "core/power_manager.h"
#include "core/style_manager.h"
#include "mlog.h"
#include "ui/top_notice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRT_TOP_BAR_HEIGHT 56
#define PRT_BTN_HEIGHT 50 /* 操作按钮统一高度（<=55px，尽量不遮画面） */
#define PRT_COUNT_BTN_WIDTH 110
#define PRT_START_BTN_WIDTH 110
#define PRT_BTN_MARGIN 10

#define PRT_TARGET_INFINITE UINT32_MAX /* 无穷次哨兵：done 永不可达，只能手动暂停/返回 */
#define PRT_STEP_INTERVAL_MS 500 /* 两次分辨率切换之间的固定停顿 */

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

/* 次数档：区别于模式切换测试无「1」，用户指定 100/1000/10000/无穷。 */
static const uint32_t g_prt_options[PAGE_PHOTO_RES_TEST_OPTION_COUNT]
    = { 100U, 1000U, 10000U, PRT_TARGET_INFINITE };

/* 分辨率档位文字：下标与 res_idx / PHOTO_RESOLUTION_* 一致（0..4），
 * 与拍照设置页 page_photo_settings.c 的分辨率选项文案保持一致。 */
static const char* const g_prt_res_names[PAGE_PHOTO_RES_RESOLUTION_COUNT]
    = { "8M(3840x2160)", "12M(4000x3000)", "24M(5600x4200)", "48M(8000x6000)",
          "64M(8192x8192)" };

/* 存活实例指针：异步完成回调 / 延时定时器可能在页面销毁后才到，用它做生命周期守卫，
 * 避免解引用已 free 的 data。create 末尾置本实例，destroy 开头(free 前)置 NULL。 */
static page_photo_resolution_test_data_t* g_prt_active = NULL;

static void stop_loop(page_photo_resolution_test_data_t* data);
static void refresh_progress(page_photo_resolution_test_data_t* data);
static void start_next_resolution(page_photo_resolution_test_data_t* data);
static void finish_to_boot_then_stop(page_photo_resolution_test_data_t* data, int keep_progress);
static void on_switch_done(int result);

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

static void refresh_progress(page_photo_resolution_test_data_t* data)
{
    char buf[32];

    if (data == NULL || data->progress_label == NULL) {
        return;
    }
    if (data->target == PRT_TARGET_INFINITE) {
        snprintf(buf, sizeof(buf), "%u/无穷", (unsigned)data->done);
    } else {
        snprintf(buf, sizeof(buf), "%u/%u", (unsigned)data->done, (unsigned)data->target);
    }
    lv_label_set_text(data->progress_label, buf);
}

/* 运行中锁定次数按钮（不可改次数）；非运行态恢复可选。 */
static void set_options_enabled(page_photo_resolution_test_data_t* data, int enabled)
{
    if (data == NULL || data->count_btn == NULL) {
        return;
    }
    if (enabled) {
        lv_obj_clear_state(data->count_btn, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(data->count_btn, LV_STATE_DISABLED);
    }
}

/* 刷新次数按钮文字为当前选中次数（无穷显示「无穷」）。 */
static void refresh_count_label(page_photo_resolution_test_data_t* data)
{
    char buf[16];

    if (data == NULL || data->count_label == NULL) {
        return;
    }
    if (data->target == PRT_TARGET_INFINITE) {
        lv_label_set_text(data->count_label, "无穷");
    } else {
        snprintf(buf, sizeof(buf), "%u", (unsigned)data->target);
        lv_label_set_text(data->count_label, buf);
    }
}

/* 删除在途的步进延时定时器（防悬空触发）。 */
static void kill_step_timer(page_photo_resolution_test_data_t* data)
{
    if (data != NULL && data->step_timer != NULL) {
        lv_timer_del(data->step_timer);
        data->step_timer = NULL;
    }
}

/* 拆掉运行态：删步进定时器、清 running/异步标志、次数按钮恢复可选、按钮标签回「开始」。
 * 不动 done（进度显示由调用方决定）。注意：不能取消已在途的异步请求，只清本地状态，
 * 靠 g_prt_active 与 running 守卫让晚到的回调空跑。 */
static void teardown_running(page_photo_resolution_test_data_t* data)
{
    if (data == NULL) {
        return;
    }
    kill_step_timer(data);
    data->running = 0U;
    data->waiting = 0U;
    data->entering = 0U;
    data->finishing = 0U;
    data->stop_pending = 0U;
    data->back_pending = 0U;
    set_options_enabled(data, 1);
    if (data->start_label != NULL) {
        lv_label_set_text(data->start_label, "开始");
    }
}

/* 停止并复位：进度归零。「暂停」/中止走此路径（用户约定：暂停即停止复位，再点从头重跑）。 */
static void stop_loop(page_photo_resolution_test_data_t* data)
{
    if (data == NULL) {
        return;
    }
    teardown_running(data);
    data->done = 0U;
    data->res_idx = 0U;
    refresh_progress(data);
}

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

/* 发起下一档分辨率的异步切换（res_idx 指向的档位）。发出后立即返回（不阻塞 UI），
 * 完成时 on_switch_done 在 UI 线程被回调。 */
static void start_next_resolution(page_photo_resolution_test_data_t* data)
{
    int ret;

    if (data == NULL || !data->running) {
        return;
    }

    /* notice bar 提示本次切到的分辨率档位。 */
    if (data->res_idx < PAGE_PHOTO_RES_RESOLUTION_COUNT) {
        char notice[48];
        snprintf(notice, sizeof(notice), "切换分辨率: %s", g_prt_res_names[data->res_idx]);
        top_notice_show(notice, TOP_NOTICE_TYPE_INFO);
    }

    ret = media_manager_execute_async(MEDIA_OP_SET_PHOTO_RESOLUTION, (int32_t)data->res_idx, on_switch_done);
    if (ret != MEDIA_MANAGER_OK) {
        MLOG_ERR("拍照分辨率测试: 发起异步切分辨率失败 idx=%d ret=%d, 收尾停止", (int)data->res_idx, ret);
        finish_to_boot_then_stop(data, 0);
        return;
    }
    data->waiting = 1U;
}

/* 步进延时定时器回调：500ms 到后发起下一档分辨率切换（单次，回调里自删）。 */
static void step_timer_cb(lv_timer_t* timer)
{
    page_photo_resolution_test_data_t* data = g_prt_active;

    lv_timer_del(timer);
    if (data == NULL || data->step_timer != timer) {
        return; /* 页面已销毁 / 定时器已被替换：空跑 */
    }
    data->step_timer = NULL;
    if (!data->running) {
        return;
    }
    start_next_resolution(data);
}

/* 收尾停止的最终动作：keep_progress 为真则保留满进度显示(达标)，否则归零(暂停/返回)。 */
static void finish_settle(page_photo_resolution_test_data_t* data, int keep_progress)
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

/* 收尾并停止（保证落在 boot）：拍照模式一轮末尾停在拍照，需异步切一次 boot（finishing，不计数），
 * 其完成回调里再 finish_settle。keep_progress 语义见 finish_settle。 */
static void finish_to_boot_then_stop(page_photo_resolution_test_data_t* data, int keep_progress)
{
    int ret;

    if (data == NULL) {
        return;
    }

    kill_step_timer(data); /* 若在 500ms 停顿窗口内触发收尾，先干掉待发的下一档 */

    MLOG_INFO("拍照分辨率测试: 收尾切回 boot");
    data->finishing = keep_progress ? 2U : 1U;
    ret = media_manager_execute_async(MEDIA_OP_SWITCH_TO_BOOT_MODE, 0, on_switch_done);
    if (ret != MEDIA_MANAGER_OK) {
        MLOG_ERR("拍照分辨率测试: 收尾切 boot 失败 ret=%d, 直接停止", ret);
        data->finishing = 0U;
        finish_settle(data, keep_progress);
        return;
    }
    data->waiting = 1U;
}

/* 异步完成回调（media 层已 hop 到 UI 线程，可安全操作 lv_obj）。
 * 每切一档分辨率 done + 1；暂停/返回/达标时统一收尾回 boot，保证停在 boot。
 * 用 g_prt_active + running 守卫：回调晚到、页面已销毁则空跑，绝不解引用悬空 data。 */
static void on_switch_done(int result)
{
    page_photo_resolution_test_data_t* data = g_prt_active;
    uint8_t need_back;

    if (data == NULL || !data->running) {
        return;
    }
    data->waiting = 0U;

    if (result != 0) {
        need_back = data->back_pending;
        MLOG_ERR("拍照分辨率测试: 操作失败 result=%d, 停止", result);
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
        MLOG_INFO("拍照分辨率测试: 收尾已落 boot, 已完成 %u 次", (unsigned)data->done);
        finish_settle(data, keep_progress);
        return;
    }

    /* 进拍照模式完成：开始第一档分辨率切换（不计数）。 */
    if (data->entering) {
        data->entering = 0U;
        MLOG_INFO("拍照分辨率测试: 已进拍照模式, 开始遍历分辨率");
        start_next_resolution(data);
        return;
    }

    /* 一档分辨率切换完成：计数 + 1，指向下一档。 */
    data->done++;
    data->res_idx = (uint8_t)((data->res_idx + 1U) % PAGE_PHOTO_RES_RESOLUTION_COUNT);
    refresh_progress(data);

    /* 收尾优先级：暂停/返回请求 -> 到达目标 -> 停顿 500ms 后继续下一档。 */
    if (data->stop_pending) {
        MLOG_INFO("拍照分辨率测试: 收尾暂停, 已完成 %u 次", (unsigned)data->done);
        finish_to_boot_then_stop(data, 0);
        return;
    }

    if (data->done >= data->target) { /* 无穷次 target==UINT32_MAX 恒不成立 */
        MLOG_INFO("拍照分辨率测试完成: %u 次", (unsigned)data->done);
        finish_to_boot_then_stop(data, 1);
        return;
    }

    /* 固定停顿 PRT_STEP_INTERVAL_MS 再切下一档：起单次延时定时器。 */
    kill_step_timer(data);
    data->step_timer = lv_timer_create(step_timer_cb, PRT_STEP_INTERVAL_MS, NULL);
}

/* 返回：非运行态直接返回。运行态则「先暂停收尾回 boot 再退」——置 stop_pending+back_pending，
 * 在一档完成点(on_switch_done)收尾回 boot 后再 page_manager_back。满足「返回必须先暂停再返回」。 */
static void back_btn_cb(lv_event_t* e)
{
    /* 触摸时从 event 取 data；按键调用(e==NULL)时用存活实例指针。 */
    page_photo_resolution_test_data_t* data
        = e ? (page_photo_resolution_test_data_t*)lv_event_get_user_data(e) : g_prt_active;

    if (data != NULL && data->running) {
        MLOG_INFO("拍照分辨率测试运行中点返回: 等切回 boot 完成后返回");
        data->stop_pending = 1U;
        data->back_pending = 1U;
        /* 若正卡在 500ms 停顿窗口(无在途异步)，此刻立刻收尾，无需等下一档完成。 */
        if (!data->waiting) {
            finish_to_boot_then_stop(data, 0);
        }
        return;
    }
    page_manager_back();
}

/* 次数按钮：点一下循环切下一个次数（100->1000->10000->无穷->回到 100）。 */
static void count_btn_cb(lv_event_t* e)
{
    /* 触摸时从 event 取 data；按键调用(e==NULL)时用存活实例指针。 */
    page_photo_resolution_test_data_t* data
        = e ? (page_photo_resolution_test_data_t*)lv_event_get_user_data(e) : g_prt_active;

    if (data == NULL || data->running) {
        return;
    }
    data->count_idx = (uint8_t)((data->count_idx + 1U) % PAGE_PHOTO_RES_TEST_OPTION_COUNT);
    data->target = g_prt_options[data->count_idx];
    refresh_count_label(data);
    refresh_progress(data);
}

/* 开始/暂停。非运行态点击 = 从头开始：先异步进拍照模式(entering)，完成后开始遍历分辨率。
 * 运行态点击 = 暂停：置 stop_pending，在一档完成点收尾回 boot；若正卡在 500ms 停顿窗口
 * (无在途异步)则立即收尾。保证暂停一定停在 boot 模式。异步全程不阻塞 UI。 */
static void start_btn_cb(lv_event_t* e)
{
    /* 触摸时从 event 取 data；按键调用(e==NULL)时用存活实例指针。 */
    page_photo_resolution_test_data_t* data
        = e ? (page_photo_resolution_test_data_t*)lv_event_get_user_data(e) : g_prt_active;
    int ret;

    if (data == NULL) {
        return;
    }

    if (data->running) {
        MLOG_INFO("拍照分辨率测试暂停: 待切回 boot 收尾; 已完成 %u 次", (unsigned)data->done);
        data->stop_pending = 1U;
        if (!data->waiting) {
            finish_to_boot_then_stop(data, 0);
        }
        return;
    }

    data->done = 0U;
    data->res_idx = 0U;
    data->running = 1U;
    data->waiting = 0U;
    data->entering = 1U;
    data->finishing = 0U;
    data->stop_pending = 0U;
    data->back_pending = 0U;
    refresh_progress(data);

    set_options_enabled(data, 0);
    if (data->start_label != NULL) {
        lv_label_set_text(data->start_label, "暂停");
    }

    MLOG_INFO("拍照分辨率测试开始: 目标 %u 次", (unsigned)data->target);
    ret = media_manager_execute_async(MEDIA_OP_SWITCH_TO_PHOTO_MODE, 0, on_switch_done);
    if (ret != MEDIA_MANAGER_OK) {
        MLOG_ERR("拍照分辨率测试: 进拍照模式失败 ret=%d, 停止", ret);
        stop_loop(data);
        return;
    }
    data->waiting = 1U;
}

/* Mode 键：切换计数（复用次数按钮点击逻辑）。 */
static void mode_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)key;
    (void)event_type;
    (void)user_data;
    count_btn_cb(NULL);
}

/* OK 键：开始/暂停（复用开始按钮点击逻辑）。 */
static void ok_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)key;
    (void)event_type;
    (void)user_data;
    start_btn_cb(NULL);
}

/* 菜单键：返回上一级（复用返回按钮点击逻辑）。 */
static void menu_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)key;
    (void)event_type;
    (void)user_data;
    back_btn_cb(NULL);
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

void page_photo_resolution_test_create(void)
{
    page_photo_resolution_test_data_t* data
        = (page_photo_resolution_test_data_t*)malloc(sizeof(page_photo_resolution_test_data_t));
    lv_obj_t* top_bar;
    lv_obj_t* back_btn;
    lv_obj_t* back_icon;

    if (data == NULL) {
        return;
    }
    memset(data, 0, sizeof(page_photo_resolution_test_data_t));
    data->target = g_prt_options[0];

    /* 全透明容器：叠在 sensor 视频层之上，方便透出画面。 */
    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(data->container, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_refr_size(data->container);

    top_bar = lv_obj_create(data->container);
    lv_obj_set_size(top_bar, LV_PCT(100), PRT_TOP_BAR_HEIGHT);
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
    lv_label_set_text(data->title_label, "拍照分辨率切换测试");
    lv_obj_add_style(data->title_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->title_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(data->title_label, LV_ALIGN_LEFT_MID, 70, 0);

    /* 次数按钮（左下角）：点一下循环切下一个次数，文字显示当前次数。 */
    data->count_btn = lv_btn_create(data->container);
    lv_obj_set_size(data->count_btn, PRT_COUNT_BTN_WIDTH, PRT_BTN_HEIGHT);
    lv_obj_set_style_radius(data->count_btn, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->count_btn, lv_color_hex(0x2F80ED), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->count_btn, LV_OPA_80, LV_PART_MAIN);
    lv_obj_align(data->count_btn, LV_ALIGN_BOTTOM_LEFT, PRT_BTN_MARGIN, -PRT_BTN_MARGIN);
    lv_obj_add_event_cb(data->count_btn, count_btn_cb, LV_EVENT_CLICKED, data);
    data->count_label = lv_label_create(data->count_btn);
    lv_obj_add_style(data->count_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->count_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(data->count_label);

    /* 开始按钮（右下角）：开始/暂停。 */
    data->start_btn = lv_btn_create(data->container);
    lv_obj_set_size(data->start_btn, PRT_START_BTN_WIDTH, PRT_BTN_HEIGHT);
    lv_obj_set_style_radius(data->start_btn, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->start_btn, lv_color_hex(0x27AE60), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->start_btn, LV_OPA_80, LV_PART_MAIN);
    lv_obj_align(data->start_btn, LV_ALIGN_BOTTOM_RIGHT, -PRT_BTN_MARGIN, -PRT_BTN_MARGIN);
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
    lv_obj_align(data->progress_label, LV_ALIGN_BOTTOM_MID, 0, -PRT_BTN_MARGIN - 6);

    refresh_count_label(data);
    refresh_progress(data);
    page_set_private_data(data);
    g_prt_active = data;
}

void page_photo_resolution_test_destroy(void)
{
    page_photo_resolution_test_data_t* data = page_get_private_data();

    if (data == NULL) {
        return;
    }

    /* 先清存活指针：任何在途异步 / 延时回调此后都会安全空跑，不解引用被 free 的 data。 */
    g_prt_active = NULL;
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

void page_photo_resolution_test_show(void)
{
    page_photo_resolution_test_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Photo resolution test page show");
    if (!data->auto_sleep_disabled) {
        power_manager_disable_auto_sleep();
        data->auto_sleep_disabled = 1U;
    }

    /* 复位测试状态（不自动启动）。 */
    stop_loop(data);
    key_manager_register_callback(KEY_ID_MODE, KEY_EVENT_CLICK, mode_key_cb, NULL);
    key_manager_register_callback(KEY_ID_OK, KEY_EVENT_CLICK, ok_key_cb, NULL);
    key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_photo_resolution_test_hide(void)
{
    page_photo_resolution_test_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Photo resolution test page hide");
    key_manager_unregister_callback(KEY_ID_MODE, KEY_EVENT_CLICK, mode_key_cb, NULL);
    key_manager_unregister_callback(KEY_ID_OK, KEY_EVENT_CLICK, ok_key_cb, NULL);
    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    stop_loop(data);
    if (data->auto_sleep_disabled) {
        power_manager_enable_auto_sleep();
        data->auto_sleep_disabled = 0U;
    }
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_photo_resolution_test_update(void)
{
}

// #endregion
