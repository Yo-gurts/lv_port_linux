// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "pages/page_loop_vtest.h"
#include "config.h"
#include "core/file_manager.h"
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

#define LVT_TOP_BAR_HEIGHT 56
#define LVT_BTN_HEIGHT 50 /* 操作按钮统一高度（<=55px，尽量不遮画面） */
#define LVT_COUNT_BTN_WIDTH 110
#define LVT_START_BTN_WIDTH 110
#define LVT_BTN_MARGIN 10

#define LVT_TARGET_INFINITE UINT32_MAX /* 无穷次哨兵：done 永不可达，只能手动停止/返回 */
#define LVT_RECORD_SECONDS 4 /* 每轮录像固定时长（秒），倒计时逐秒刷新 notice */
#define LVT_RECORD_TICK_MS 1000 /* 录像倒计时定时器周期 */
#define LVT_MIN_FREE_BYTES (500ULL * 1024ULL * 1024ULL) /* SD 卡最小可用空间，低于此值视为满、停止测试 */

/* 一轮循环的步骤：入页默认已在 boot，start 从 STEP_VIDEO 起。
 * 每个异步 op 完成回调推进到下一步；STEP_START_REC 完成后起 4s 定时器再进 STEP_STOP_REC。
 * STEP_BOOT 完成 = 一整轮结束（done+1，回 boot，正是入页默认态）。 */
typedef enum {
    LVT_STEP_VIDEO = 0, /* 切录像模式 */
    LVT_STEP_SET_VIDEO_RES, /* 设随机录像分辨率 */
    LVT_STEP_START_REC, /* 开始录像（完成后起 4s 定时器） */
    LVT_STEP_STOP_REC, /* 停止录像 */
    LVT_STEP_BOOT, /* 切回 boot 模式（本步完成即一轮结束） */
    LVT_STEP_COUNT
} lvt_step_t;

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

/* 次数档：1/100/无穷。 */
static const uint32_t g_lvt_options[PAGE_LOOP_VTEST_OPTION_COUNT] = { 1U, 100U, LVT_TARGET_INFINITE };

/* 存活实例指针：异步完成回调 / 延时定时器可能在页面销毁后才到，用它做生命周期守卫，
 * 避免解引用已 free 的 data。create 末尾置本实例，destroy 开头(free 前)置 NULL。 */
static page_loop_vtest_data_t* g_lvt_active = NULL;

static void stop_loop(page_loop_vtest_data_t* data);
static void refresh_progress(page_loop_vtest_data_t* data);
static void start_step(page_loop_vtest_data_t* data);
static void on_step_done(int result);

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

static void refresh_progress(page_loop_vtest_data_t* data)
{
    char buf[32];

    if (data == NULL || data->progress_label == NULL) {
        return;
    }
    if (data->target == LVT_TARGET_INFINITE) {
        snprintf(buf, sizeof(buf), "%u/无穷", (unsigned)data->done);
    } else {
        snprintf(buf, sizeof(buf), "%u/%u", (unsigned)data->done, (unsigned)data->target);
    }
    lv_label_set_text(data->progress_label, buf);
}

/* 运行中锁定次数按钮（不可改次数）；非运行态恢复可选。 */
static void set_options_enabled(page_loop_vtest_data_t* data, int enabled)
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
static void refresh_count_label(page_loop_vtest_data_t* data)
{
    char buf[16];

    if (data == NULL || data->count_label == NULL) {
        return;
    }
    if (data->target == LVT_TARGET_INFINITE) {
        lv_label_set_text(data->count_label, "无穷");
    } else {
        snprintf(buf, sizeof(buf), "%u", (unsigned)data->target);
        lv_label_set_text(data->count_label, buf);
    }
}

/* 删除在途的录像延时定时器（防悬空触发）。 */
static void kill_rec_timer(page_loop_vtest_data_t* data)
{
    if (data != NULL && data->rec_timer != NULL) {
        lv_timer_del(data->rec_timer);
        data->rec_timer = NULL;
    }
}

/* 拆掉运行态：删定时器、清 running/异步标志、次数按钮恢复可选、按钮标签回「开始」。
 * 不动 done（进度显示由调用方决定）。注意：不能取消已在途的异步请求，只清本地状态，
 * 靠 g_lvt_active 与 running 守卫让晚到的回调空跑。 */
static void teardown_running(page_loop_vtest_data_t* data)
{
    if (data == NULL) {
        return;
    }
    kill_rec_timer(data);
    data->running = 0U;
    data->waiting = 0U;
    data->step = 0U;
    data->stop_pending = 0U;
    data->back_pending = 0U;
    set_options_enabled(data, 1);
    if (data->start_label != NULL) {
        lv_label_set_text(data->start_label, "开始");
    }
}

/* 停止并复位：进度归零。「停止」/中止走此路径（用户约定：停止即复位，再点从头重跑）。 */
static void stop_loop(page_loop_vtest_data_t* data)
{
    if (data == NULL) {
        return;
    }
    teardown_running(data);
    data->done = 0U;
    refresh_progress(data);
}

/* 收尾：停止（keep_progress 决定进度归零/保留），需要则退回上一页。 */
static void finish_settle(page_loop_vtest_data_t* data, int keep_progress)
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

/* 当前步对应的 media 异步 op 与 args（分辨率步现算随机档位）。 */
static media_operation_t lvt_step_op(uint8_t step, int32_t* args)
{
    *args = 0;
    switch (step) {
    case LVT_STEP_VIDEO:
        return MEDIA_OP_SWITCH_TO_VIDEO_MODE;
    case LVT_STEP_SET_VIDEO_RES:
        *args = (int32_t)(rand() % PAGE_LOOP_VTEST_VIDEO_RES_COUNT);
        return MEDIA_OP_SET_VIDEO_RESOLUTION;
    case LVT_STEP_START_REC:
        return MEDIA_OP_START_RECORD;
    case LVT_STEP_STOP_REC:
        return MEDIA_OP_STOP_RECORD;
    case LVT_STEP_BOOT:
    default:
        return MEDIA_OP_SWITCH_TO_BOOT_MODE;
    }
}

/* 每步一条顶部 notice 提示当前进行到哪一步（含随机分辨率档位/轮次）。UI 线程调用。 */
static void notify_step(const page_loop_vtest_data_t* data, uint8_t step, int32_t args)
{
    char buf[64];
    unsigned round = (unsigned)data->done + 1U; /* 当前正在跑第几轮（done 为已完成数） */

    switch (step) {
    case LVT_STEP_VIDEO:
        snprintf(buf, sizeof(buf), "第%u轮: 切录像模式", round);
        break;
    case LVT_STEP_SET_VIDEO_RES:
        snprintf(buf, sizeof(buf), "第%u轮: 设录像分辨率 档%d", round, (int)args);
        break;
    case LVT_STEP_START_REC:
        snprintf(buf, sizeof(buf), "第%u轮: 开始录像", round);
        break;
    case LVT_STEP_STOP_REC:
        snprintf(buf, sizeof(buf), "第%u轮: 停止录像", round);
        break;
    case LVT_STEP_BOOT:
    default:
        snprintf(buf, sizeof(buf), "第%u轮: 切回启动页", round);
        break;
    }
    top_notice_show(buf, TOP_NOTICE_TYPE_INFO);
}

/* 检查 SD 卡可用空间：足够返回 1；不足 LVT_MIN_FREE_BYTES 则弹 ERROR notice 并返回 0。
 * 用于「第一次运行前」与「每轮末」两处拦截。file_manager 层已含 5% 预留。 */
static int lvt_check_space(void)
{
    uint64_t avail = 0U;

    if (file_manager_get_storage_space_bytes(&avail) != 0) {
        MLOG_WARN("循环录像测试: 读取 SD 卡剩余空间失败, 放行不拦截");
        return 1; /* 读不到就不误拦，交由后续步骤各自处理 */
    }
    if (avail < LVT_MIN_FREE_BYTES) {
        char buf[48];
        snprintf(buf, sizeof(buf), "SD卡已满(剩 %uMB), 已停止", (unsigned)(avail / (1024ULL * 1024ULL)));
        top_notice_show(buf, TOP_NOTICE_TYPE_ERROR);
        MLOG_WARN("循环录像测试: SD 卡剩余 %uMB < 500MB, 停止", (unsigned)(avail / (1024ULL * 1024ULL)));
        return 0;
    }
    return 1;
}

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

/* 发起当前 step 的异步 op。发出后立即返回（不阻塞 UI），完成时 on_step_done 在 UI 线程回调。 */
static void start_step(page_loop_vtest_data_t* data)
{
    media_operation_t op;
    int32_t args;
    int ret;

    if (data == NULL || !data->running) {
        return;
    }

    op = lvt_step_op(data->step, &args);
    notify_step(data, data->step, args);
    ret = media_manager_execute_async(op, args, on_step_done);
    if (ret != MEDIA_MANAGER_OK) {
        MLOG_ERR("循环录像测试: step=%d op=%d 发起失败 ret=%d, 停止", (int)data->step, (int)op, ret);
        stop_loop(data);
        return;
    }
    data->waiting = 1U;
}

/* 录像倒计时定时器回调：每 1s 一跳，刷新 notice 剩余秒数（重发使隐藏定时器复位，全程常亮）；
 * 减到 0 时删定时器并发起停止录像。周期性 timer，非单次。 */
static void rec_timer_cb(lv_timer_t* timer)
{
    page_loop_vtest_data_t* data = g_lvt_active;
    char buf[48];

    if (data == NULL || data->rec_timer != timer) {
        lv_timer_del(timer); /* 页面已销毁 / 定时器已被替换：删掉自己后空跑 */
        return;
    }
    if (!data->running) {
        lv_timer_del(timer);
        data->rec_timer = NULL;
        return;
    }

    if (data->rec_left_s > 0U) {
        data->rec_left_s--;
    }

    if (data->rec_left_s == 0U) {
        lv_timer_del(timer);
        data->rec_timer = NULL;
        data->step = LVT_STEP_STOP_REC;
        start_step(data);
        return;
    }

    snprintf(buf, sizeof(buf), "录像中… 剩 %us", (unsigned)data->rec_left_s);
    top_notice_show(buf, TOP_NOTICE_TYPE_INFO);
}

/* 一轮完成点：done+1、刷进度，按「停止/返回 -> 到达目标 -> 继续下一轮」优先级处理。 */
static void on_round_complete(page_loop_vtest_data_t* data)
{
    data->done++;
    refresh_progress(data);

    /* 每轮末查 SD 卡：不足 500M 则硬性停止（优先于用户停止/达标；此刻已回 boot 停得干净）。 */
    if (!lvt_check_space()) {
        MLOG_INFO("循环录像测试: SD 卡满, 收尾停止, 已完成 %u 轮", (unsigned)data->done);
        finish_settle(data, 1); /* 保留已完成进度显示 */
        return;
    }

    if (data->stop_pending) {
        char buf[48];
        snprintf(buf, sizeof(buf), "已停止, 完成 %u 轮", (unsigned)data->done);
        top_notice_show(buf, TOP_NOTICE_TYPE_SUCCESS);
        MLOG_INFO("循环录像测试: 本轮跑完, 收尾停止, 已完成 %u 轮", (unsigned)data->done);
        finish_settle(data, 0); /* 归零进度 */
        return;
    }

    if (data->done >= data->target) { /* 无穷次 target==UINT32_MAX 恒不成立 */
        char buf[48];
        snprintf(buf, sizeof(buf), "测试完成, 共 %u 轮", (unsigned)data->done);
        top_notice_show(buf, TOP_NOTICE_TYPE_SUCCESS);
        MLOG_INFO("循环录像测试完成: %u 轮", (unsigned)data->done);
        finish_settle(data, 1); /* 保留满进度显示 */
        return;
    }

    /* 继续下一轮：从 STEP_VIDEO 起。 */
    data->step = LVT_STEP_VIDEO;
    start_step(data);
}

/* 异步 step 完成回调（media 层已 hop 到 UI 线程，可安全操作 lv_obj）。
 * 用 g_lvt_active + running 守卫：回调晚到、页面已销毁则空跑，绝不解引用悬空 data。 */
static void on_step_done(int result)
{
    page_loop_vtest_data_t* data = g_lvt_active;
    uint8_t need_back;

    if (data == NULL || !data->running) {
        return;
    }
    data->waiting = 0U;

    if (result != 0) {
        char buf[48];
        need_back = data->back_pending;
        snprintf(buf, sizeof(buf), "步骤%d 失败, 已停止", (int)data->step);
        top_notice_show(buf, TOP_NOTICE_TYPE_ERROR);
        MLOG_ERR("循环录像测试: step=%d 失败 result=%d, 停止", (int)data->step, result);
        stop_loop(data);
        if (need_back) {
            page_manager_back();
        }
        return;
    }

    /* 一整轮结束（切回 boot 完成）。 */
    if (data->step == LVT_STEP_BOOT) {
        on_round_complete(data);
        return;
    }

    /* 开始录像完成：起逐秒倒计时定时器，notice 全程显示剩余秒；到 0 再停录像
     * （期间 UI 全程响应，停止/返回请求在轮末生效）。 */
    if (data->step == LVT_STEP_START_REC) {
        char buf[48];
        data->rec_left_s = LVT_RECORD_SECONDS;
        snprintf(buf, sizeof(buf), "录像中… 剩 %us", (unsigned)data->rec_left_s);
        top_notice_show(buf, TOP_NOTICE_TYPE_INFO);
        kill_rec_timer(data);
        data->rec_timer = lv_timer_create(rec_timer_cb, LVT_RECORD_TICK_MS, NULL);
        return;
    }

    /* 其余步：推进到下一步。 */
    data->step++;
    start_step(data);
}

/* 返回：非运行态直接返回。运行态则「等本轮跑完再退」——置 stop_pending+back_pending，
 * 在一轮完成点(on_round_complete)收尾后再 page_manager_back。满足「返回须先等本轮跑完」。 */
static void back_btn_cb(lv_event_t* e)
{
    /* 触摸时从 event 取 data；按键调用(e==NULL)时用存活实例指针。 */
    page_loop_vtest_data_t* data = e ? (page_loop_vtest_data_t*)lv_event_get_user_data(e) : g_lvt_active;

    if (data != NULL && data->running) {
        MLOG_INFO("循环录像测试运行中点返回: 等本轮跑完后返回");
        data->stop_pending = 1U;
        data->back_pending = 1U;
        if (data->start_label != NULL) {
            lv_label_set_text(data->start_label, "正在停止");
        }
        return;
    }
    page_manager_back();
}

/* 次数按钮：点一下循环切下一个次数（1->100->无穷->回到 1）。 */
static void count_btn_cb(lv_event_t* e)
{
    /* 触摸时从 event 取 data；按键调用(e==NULL)时用存活实例指针。 */
    page_loop_vtest_data_t* data = e ? (page_loop_vtest_data_t*)lv_event_get_user_data(e) : g_lvt_active;

    if (data == NULL || data->running) {
        return;
    }
    data->count_idx = (uint8_t)((data->count_idx + 1U) % PAGE_LOOP_VTEST_OPTION_COUNT);
    data->target = g_lvt_options[data->count_idx];
    refresh_count_label(data);
    refresh_progress(data);
}

/* 开始/停止。非运行态点击 = 从头开始，发起第一步。
 * 运行态点击 = 停止：置 stop_pending，在本轮完成点收尾（按钮先显示「正在停止」）。
 * 异步全程不阻塞 UI，最多等本轮跑完但界面全程响应。 */
static void start_btn_cb(lv_event_t* e)
{
    /* 触摸时从 event 取 data；按键调用(e==NULL)时用存活实例指针。 */
    page_loop_vtest_data_t* data = e ? (page_loop_vtest_data_t*)lv_event_get_user_data(e) : g_lvt_active;

    if (data == NULL) {
        return;
    }

    if (data->running) {
        MLOG_INFO("循环录像测试停止: 待本轮跑完; 已完成 %u 轮", (unsigned)data->done);
        data->stop_pending = 1U;
        if (data->start_label != NULL) {
            lv_label_set_text(data->start_label, "正在停止");
        }
        return;
    }

    /* 第一次运行前也查一次 SD 卡：不足则不启动，保持「开始」态。 */
    if (!lvt_check_space()) {
        MLOG_WARN("循环录像测试: SD 卡满, 拒绝启动");
        return;
    }

    if (data->target == 0U) {
        data->target = 1U;
    }

    data->done = 0U;
    data->step = LVT_STEP_VIDEO;
    data->running = 1U;
    data->waiting = 0U;
    data->stop_pending = 0U;
    data->back_pending = 0U;
    refresh_progress(data);

    set_options_enabled(data, 0);
    if (data->start_label != NULL) {
        lv_label_set_text(data->start_label, "停止");
    }

    MLOG_INFO("循环录像测试开始: 目标 %u 轮", (unsigned)data->target);
    start_step(data);
}

/* Mode 键：切换计数（复用次数按钮点击逻辑；运行中被 count_btn_cb 内部忽略）。 */
static void mode_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)key;
    (void)event_type;
    (void)user_data;
    count_btn_cb(NULL);
}

/* OK 键：开始/停止（复用开始按钮点击逻辑）。 */
static void ok_key_cb(key_id_t key, key_event_type_t event_type, void* user_data)
{
    (void)key;
    (void)event_type;
    (void)user_data;
    start_btn_cb(NULL);
}

/* 菜单键：返回上一级（复用返回按钮点击逻辑；运行中会等本轮跑完再退）。 */
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

void page_loop_vtest_create(void)
{
    page_loop_vtest_data_t* data = (page_loop_vtest_data_t*)malloc(sizeof(page_loop_vtest_data_t));
    lv_obj_t* top_bar;
    lv_obj_t* back_btn;
    lv_obj_t* back_icon;

    if (data == NULL) {
        return;
    }
    memset(data, 0, sizeof(page_loop_vtest_data_t));
    data->target = g_lvt_options[0];

    /* 全透明容器：叠在 sensor 视频层之上，方便透出画面。 */
    data->container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(data->container, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_refr_size(data->container);

    top_bar = lv_obj_create(data->container);
    lv_obj_set_size(top_bar, LV_PCT(100), LVT_TOP_BAR_HEIGHT);
    lv_obj_add_style(top_bar, &style_noboarder, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(top_bar, LV_SCROLLBAR_MODE_OFF);

    back_btn = lv_btn_create(top_bar);
    lv_obj_set_size(back_btn, 50, 50);
    lv_obj_add_style(back_btn, &style_noboarder, LV_PART_MAIN);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, data);
    back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, "A:" RES_ICON_PATH "/back-circle-white.png");
    lv_obj_align(back_icon, LV_ALIGN_CENTER, 0, 0);

    data->title_label = lv_label_create(top_bar);
    lv_label_set_text(data->title_label, "循环录像测试");
    lv_obj_add_style(data->title_label, &NORMAL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->title_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(data->title_label, LV_ALIGN_LEFT_MID, 70, 0);

    /* 次数按钮（左下角）：点一下循环切下一个次数，文字显示当前次数。 */
    data->count_btn = lv_btn_create(data->container);
    lv_obj_set_size(data->count_btn, LVT_COUNT_BTN_WIDTH, LVT_BTN_HEIGHT);
    lv_obj_set_style_radius(data->count_btn, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->count_btn, lv_color_hex(0x2F80ED), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->count_btn, LV_OPA_80, LV_PART_MAIN);
    lv_obj_align(data->count_btn, LV_ALIGN_BOTTOM_LEFT, LVT_BTN_MARGIN, -LVT_BTN_MARGIN);
    lv_obj_add_event_cb(data->count_btn, count_btn_cb, LV_EVENT_CLICKED, data);
    data->count_label = lv_label_create(data->count_btn);
    lv_obj_add_style(data->count_label, &SMALL_SIZE, LV_PART_MAIN);
    lv_obj_set_style_text_color(data->count_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(data->count_label);

    /* 开始按钮（右下角）：开始/停止。 */
    data->start_btn = lv_btn_create(data->container);
    lv_obj_set_size(data->start_btn, LVT_START_BTN_WIDTH, LVT_BTN_HEIGHT);
    lv_obj_set_style_radius(data->start_btn, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->start_btn, lv_color_hex(0x27AE60), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(data->start_btn, LV_OPA_80, LV_PART_MAIN);
    lv_obj_align(data->start_btn, LV_ALIGN_BOTTOM_RIGHT, -LVT_BTN_MARGIN, -LVT_BTN_MARGIN);
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
    lv_obj_align(data->progress_label, LV_ALIGN_BOTTOM_MID, 0, -LVT_BTN_MARGIN - 6);

    refresh_count_label(data);
    refresh_progress(data);
    page_set_private_data(data);
    g_lvt_active = data;
}

void page_loop_vtest_destroy(void)
{
    page_loop_vtest_data_t* data = page_get_private_data();

    if (data == NULL) {
        return;
    }

    /* 先清存活指针：任何在途异步 / 延时回调此后都会安全空跑，不解引用被 free 的 data。 */
    g_lvt_active = NULL;
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

void page_loop_vtest_show(void)
{
    page_loop_vtest_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Loop video test page show");
    if (!data->auto_sleep_disabled) {
        power_manager_disable_auto_sleep();
        data->auto_sleep_disabled = 1U;
    }

    /* 复位测试状态（不自动启动）。 */
    stop_loop(data);
    key_manager_register_callback(KEY_ID_MODE, KEY_EVENT_CLICK, mode_key_cb, NULL);
    key_manager_register_callback(KEY_ID_OK, KEY_EVENT_CLICK, ok_key_cb, NULL);
    key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    key_manager_register_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_cb, NULL);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_loop_vtest_hide(void)
{
    page_loop_vtest_data_t* data = page_get_private_data();

    if (data == NULL || data->container == NULL) {
        return;
    }

    MLOG_INFO("Loop video test page hide");
    key_manager_unregister_callback(KEY_ID_MODE, KEY_EVENT_CLICK, mode_key_cb, NULL);
    key_manager_unregister_callback(KEY_ID_OK, KEY_EVENT_CLICK, ok_key_cb, NULL);
    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_CLICK, menu_key_cb, NULL);
    key_manager_unregister_callback(KEY_ID_MENU, KEY_EVENT_LONG_PRESS, menu_key_cb, NULL);
    stop_loop(data);
    if (data->auto_sleep_disabled) {
        power_manager_enable_auto_sleep();
        data->auto_sleep_disabled = 0U;
    }
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_loop_vtest_update(void)
{
}

// #endregion
