#define DEBUG

#include "core/message_manager.h"
#include "core/param_manager.h"
#include "appcomm.h"
#include "mlog.h"
#include "photomng.h"
#include "ui/status_bar.h"
#include "ui/top_notice.h"
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define EVENT_UI_TOUCH APPCOMM_EVENT_ID(APP_MOD_UI, 0)
#ifndef UI_ARRAY_SIZE
#define UI_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

typedef struct {
    MESSAGE_S msg;
    bool msg_processed;
    message_manager_result_cb_t result_cb;
    TOPIC_ID success_topic;
    TOPIC_ID failure_topic;
    bool use_result_topics;
    pthread_mutex_t msg_mutex;
} message_context_t;

typedef struct {
    char text[64];
    top_notice_type_t type;
    uint32_t duration_ms;
} async_top_notice_param_t;

static message_context_t g_msg_ctx = {
    .msg_processed = true,
    .result_cb = NULL,
    .success_topic = 0,
    .failure_topic = 0,
    .use_result_topics = false,
    .msg_mutex = PTHREAD_MUTEX_INITIALIZER,
};

static pthread_cond_t g_sync_cond = PTHREAD_COND_INITIALIZER;
static int32_t g_sync_result = MESSAGE_MANAGER_ESTATE;
static bool g_sync_done = false;

static EVENTHUB_SUBSCRIBER_S* g_subscriber_desc = NULL;
static MW_PTR g_subscriber_hdl = NULL;
static bool g_msgmgr_created = false;

/* EventHub 线程只置 pending，UI 线程 message_manager_poll() 消费——不碰 LVGL。
 * 通知类事件（SD 卡插拔/格式化）稀疏且不突发，top-notice 单槽 last-wins 足够。
 * 全部由 g_msg_ctx.msg_mutex 保护（与回包匹配同一把锁）。 */
static bool g_pending_status_refresh = false;
static bool g_pending_top_notice = false;
static async_top_notice_param_t g_pending_notice;

static void message_manager_post_top_notice_async(const char* text, top_notice_type_t type, uint32_t duration_ms)
{
    if (text == NULL) {
        return;
    }

    MUTEX_LOCK(g_msg_ctx.msg_mutex);
    if (snprintf(g_pending_notice.text, sizeof(g_pending_notice.text), "%s", text) >= (int)sizeof(g_pending_notice.text)) {
        g_pending_notice.text[0] = '\0';
    }
    g_pending_notice.type = type;
    g_pending_notice.duration_ms = duration_ms;
    g_pending_top_notice = true;
    MUTEX_UNLOCK(g_msg_ctx.msg_mutex);
}

/* UI 线程：消费 EventHub 线程置的 status-bar 刷新 / top-notice 待处理项，
 * 在 UI 上下文调 LVGL。仿 param_manager_poll「锁内快照+清标志、锁外执行」。 */
void message_manager_poll(void)
{
    bool do_status_refresh;
    bool do_top_notice;
    async_top_notice_param_t notice;

    MUTEX_LOCK(g_msg_ctx.msg_mutex);
    do_status_refresh = g_pending_status_refresh;
    do_top_notice = g_pending_top_notice;
    if (do_top_notice) {
        notice = g_pending_notice;
    }
    g_pending_status_refresh = false;
    g_pending_top_notice = false;
    MUTEX_UNLOCK(g_msg_ctx.msg_mutex);

    if (do_status_refresh) {
        status_bar_refresh();
    }
    if (do_top_notice) {
        top_notice_show_for(notice.text, notice.type, notice.duration_ms);
    }
}

static bool handle_sd_card_event_notice(TOPIC_ID topic)
{
    const char* notice_text = NULL;
    top_notice_type_t notice_type = TOP_NOTICE_TYPE_INFO;
    uint32_t notice_duration_ms = 2000;
    sd_ready_state_t sd_ready = SD_READY_FALSE;

    switch (topic) {
    case EVENT_MODEMNG_CARD_REMOVE:
        notice_text = "SD卡已拔出";
        notice_type = TOP_NOTICE_TYPE_WARNING;
        sd_ready = SD_READY_FALSE;
        break;
    case EVENT_MODEMNG_CARD_AVAILABLE:
        notice_text = "SD卡已就绪";
        notice_type = TOP_NOTICE_TYPE_SUCCESS;
        sd_ready = SD_READY_TRUE;
        break;
    case EVENT_MODEMNG_CARD_CHECKING:
        notice_text = "SD卡检测中...";
        notice_duration_ms = 1500;
        sd_ready = SD_READY_FALSE;
        break;
    case EVENT_MODEMNG_CARD_UNAVAILABLE:
    case EVENT_MODEMNG_CARD_ERROR:
    case EVENT_MODEMNG_CARD_FSERROR:
    case EVENT_MODEMNG_CARD_MOUNT_FAILED:
        notice_text = "SD卡不可用";
        notice_type = TOP_NOTICE_TYPE_ERROR;
        sd_ready = SD_READY_FALSE;
        break;
    case EVENT_MODEMNG_CARD_READ_ONLY:
        notice_text = "SD卡为只读模式";
        notice_type = TOP_NOTICE_TYPE_WARNING;
        sd_ready = SD_READY_TRUE;
        break;
    case EVENT_MODEMNG_CARD_SLOW:
        notice_text = "SD卡速度较慢";
        notice_type = TOP_NOTICE_TYPE_WARNING;
        sd_ready = SD_READY_TRUE;
        break;
    case EVENT_MODEMNG_CARD_FORMAT:
    case EVENT_MODEMNG_CARD_FORMATING:
        notice_text = "SD卡格式化中...";
        notice_duration_ms = 1500;
        sd_ready = SD_READY_FALSE;
        break;
    case EVENT_MODEMNG_CARD_FORMAT_SUCCESSED:
        notice_text = "SD卡格式化完成";
        notice_type = TOP_NOTICE_TYPE_SUCCESS;
        sd_ready = SD_READY_TRUE;
        break;
    case EVENT_MODEMNG_CARD_FORMAT_FAILED:
        notice_text = "SD卡格式化失败";
        notice_type = TOP_NOTICE_TYPE_ERROR;
        sd_ready = SD_READY_FALSE;
        break;
    default:
        return false;
    }

    (void)param_manager_set(PARAM_ID_SD_READY, sd_ready);
    MUTEX_LOCK(g_msg_ctx.msg_mutex);
    g_pending_status_refresh = true;
    MUTEX_UNLOCK(g_msg_ctx.msg_mutex);
    message_manager_post_top_notice_async(notice_text, notice_type, notice_duration_ms);
    return true;
}

static void message_manager_reset_request_locked(bool processed)
{
    g_msg_ctx.msg_processed = processed;
    g_msg_ctx.result_cb = NULL;
    g_msg_ctx.success_topic = 0;
    g_msg_ctx.failure_topic = 0;
    g_msg_ctx.use_result_topics = false;
}

/* 同步发送模式下的回包处理：记录结果并唤醒等待线程。 */
static int32_t message_manager_sync_result_proc(EVENT_S* evt)
{
    g_sync_result = evt->s32Result;
    g_sync_done = true;
    pthread_cond_signal(&g_sync_cond);
    return g_sync_result;
}

/* 等待成功/失败 topic 的同步回包处理。 */
static int32_t message_manager_sync_topics_result_proc(EVENT_S* evt)
{
    if (evt->topic == g_msg_ctx.success_topic) {
        g_sync_result = (evt->s32Result == 0) ? 0 : evt->s32Result;
    } else if (evt->topic == g_msg_ctx.failure_topic) {
        g_sync_result = (evt->s32Result != 0) ? evt->s32Result : MESSAGE_MANAGER_ESTATE;
    } else {
        g_sync_result = MESSAGE_MANAGER_ESTATE;
    }

    g_sync_done = true;
    pthread_cond_signal(&g_sync_cond);
    return g_sync_result;
}

/* 在持锁状态下匹配回包并执行对应结果回调。 */
static int32_t message_manager_process_result_locked(EVENT_S* evt)
{
    int32_t ret = 0;

    /* 当前没有进行中的请求，不需要做回包匹配。 */
    if (g_msg_ctx.msg_processed) {
        return 0;
    }

    if (g_msg_ctx.use_result_topics) {
        if (evt->topic == g_msg_ctx.success_topic || evt->topic == g_msg_ctx.failure_topic) {
            if (g_msg_ctx.result_cb != NULL) {
                ret = g_msg_ctx.result_cb(evt);
            }
            g_msg_ctx.msg_processed = true;
        }
    } else if ((g_msg_ctx.msg.topic == evt->topic) && (g_msg_ctx.msg.arg1 == evt->arg1) && (g_msg_ctx.msg.arg2 == evt->arg2)) {
        /* 通过 (topic,arg1,arg2) 匹配回包，完成当前请求。 */
        if (g_msg_ctx.result_cb != NULL) {
            ret = g_msg_ctx.result_cb(evt);
        }
        g_msg_ctx.msg_processed = true;
    }

    return ret;
}

/* 统一分发订阅到的事件，处理默认日志与后续扩展入口。 */
static int32_t message_manager_dispatch_event(EVENT_S* evt)
{
    (void)handle_sd_card_event_notice(evt->topic);

    switch (evt->topic) {
    case EVENT_MODEMNG_RESET:
    case EVENT_MODEMNG_MODEOPEN:
    case EVENT_MODEMNG_MODECLOSE:
    case EVENT_MODEMNG_MODESWITCH:
    case EVENT_MODEMNG_START_PIV:
    case EVENT_MODEMNG_SETTING:
    case EVENT_MODEMNG_RECODER_STARTSTATU:
    case EVENT_MODEMNG_RECODER_STOPSTATU:
    case EVENT_MODEMNG_RECODER_SPLITREC:
    case EVENT_MODEMNG_RECODER_STARTEVENTSTAUE:
    case EVENT_MODEMNG_RECODER_STOPEVENTSTAUE:
    case EVENT_MODEMNG_RECODER_STARTEMRSTAUE:
    case EVENT_MODEMNG_RECODER_STOPEMRSTAUE:
    case EVENT_MODEMNG_RECODER_STARTPIVSTAUE:
    case EVENT_MODEMNG_RECODER_STOPPIVSTAUE:
    case EVENT_MODEMNG_SET_WHITE_BALANCE:
    case EVENT_MODEMNG_SET_ISO:
    case EVENT_MODEMNG_SET_EXPOSURE:
    case EVENT_MODEMNG_PHOTO_INDEXED:
    case EVENT_MODEMNG_PHOTO_INDEX_FAILED:
    case EVENT_PHOTOMNG_PIV_START:
    case EVENT_PHOTOMNG_PIV_END:
    case EVENT_PHOTOMNG_PIV_ERROR:
        MLOG_DBG("处理 topic=%s(0x%x) result=%d", event_topic_get_name(evt->topic), evt->topic, evt->s32Result);
        break;
    default:
        break;
    }

    return 0;
}

/* EventHub 事件回调：先处理请求回包，再做事件分发。 */
static int32_t message_manager_event_cb(void* argv, EVENT_S* evt)
{
    (void)argv;
    if (evt == NULL) {
        return MESSAGE_MANAGER_EINVAL;
    }

    MUTEX_LOCK(g_msg_ctx.msg_mutex);
    /* 先处理等待中的请求结果，再按常规路径分发事件。 */
    (void)message_manager_process_result_locked(evt);
    MUTEX_UNLOCK(g_msg_ctx.msg_mutex);

    return message_manager_dispatch_event(evt);
}

/* 内部创建并注册消息订阅者，订阅 UI 关注的所有主题。 */
static int32_t message_manager_subscribe(void)
{
    /* UI message manager 统一接收的事件主题列表。 */
    static TOPIC_ID topics[] = {
        EVENT_MODEMNG_CARD_REMOVE,
        EVENT_MODEMNG_CARD_AVAILABLE,
        EVENT_MODEMNG_CARD_UNAVAILABLE,
        EVENT_MODEMNG_CARD_ERROR,
        EVENT_MODEMNG_CARD_FSERROR,
        EVENT_MODEMNG_CARD_SLOW,
        EVENT_MODEMNG_CARD_CHECKING,
        EVENT_MODEMNG_CARD_FORMAT,
        EVENT_MODEMNG_CARD_FORMATING,
        EVENT_MODEMNG_CARD_FORMAT_SUCCESSED,
        EVENT_MODEMNG_CARD_FORMAT_FAILED,
        EVENT_MODEMNG_CARD_READ_ONLY,
        EVENT_MODEMNG_CARD_MOUNT_FAILED,
        EVENT_MODEMNG_RESET,
        EVENT_MODEMNG_MODESWITCH,
        EVENT_MODEMNG_MODEOPEN,
        EVENT_MODEMNG_MODECLOSE,
        EVENT_MODEMNG_START_PIV,
        EVENT_MODEMNG_SETTING,
        EVENT_MODEMNG_RECODER_STARTSTATU,
        EVENT_MODEMNG_RECODER_STOPSTATU,
        EVENT_MODEMNG_RECODER_SPLITREC,
        EVENT_MODEMNG_RECODER_STARTEVENTSTAUE,
        EVENT_MODEMNG_RECODER_STOPEVENTSTAUE,
        EVENT_MODEMNG_RECODER_STARTEMRSTAUE,
        EVENT_MODEMNG_RECODER_STOPEMRSTAUE,
        EVENT_MODEMNG_RECODER_STARTPIVSTAUE,
        EVENT_MODEMNG_RECODER_STOPPIVSTAUE,
        EVENT_MODEMNG_SET_WHITE_BALANCE,
        EVENT_MODEMNG_SET_ISO,
        EVENT_MODEMNG_SET_EXPOSURE,
        EVENT_MODEMNG_PHOTO_INDEXED,
        EVENT_MODEMNG_PHOTO_INDEX_FAILED,
        EVENT_PHOTOMNG_PIV_START,
        EVENT_PHOTOMNG_PIV_END,
        EVENT_PHOTOMNG_PIV_ERROR,
        EVENT_UI_TOUCH,
    };

    uint32_t i;
    int32_t ret;
    int32_t final_ret = 0;

    if (!g_msgmgr_created) {
        return MESSAGE_MANAGER_ESTATE;
    }

    if (g_subscriber_hdl != NULL) {
        return 0;
    }

    (void)EVENTHUB_RegisterTopic(EVENT_UI_TOUCH);

    g_subscriber_desc = (EVENTHUB_SUBSCRIBER_S*)calloc(1, sizeof(EVENTHUB_SUBSCRIBER_S));
    if (g_subscriber_desc == NULL) {
        return MESSAGE_MANAGER_ESTATE;
    }

    snprintf(g_subscriber_desc->asName, sizeof(g_subscriber_desc->asName), "%s", "ui_msgmgr");
    g_subscriber_desc->argv = NULL;
    g_subscriber_desc->new_msg_cb = message_manager_event_cb;
    g_subscriber_desc->sync = false;

    ret = EVENTHUB_CreateSubscriber(g_subscriber_desc, &g_subscriber_hdl);
    if (ret != 0) {
        free(g_subscriber_desc);
        g_subscriber_desc = NULL;
        return MESSAGE_MANAGER_ESTATE;
    }

    for (i = 0; i < UI_ARRAY_SIZE(topics); i++) {
        ret = EVENTHUB_Subcribe(g_subscriber_hdl, topics[i]);
        if (ret != 0) {
            MLOG_WARN("Subscribe topic(0x%x) failed: %d", topics[i], ret);
            final_ret = MESSAGE_MANAGER_ESTATE;
        }
    }

    return final_ret;
}

/* 初始化 message_manager 并完成事件订阅。 */
int32_t message_manager_create(void)
{
    int32_t ret;
    if (g_msgmgr_created) {
        return 0;
    }

    g_msgmgr_created = true;
    ret = message_manager_subscribe();
    if (ret != 0) {
        g_msgmgr_created = false;
        return ret;
    }

    return 0;
}

/* 销毁 message_manager，释放订阅并唤醒可能阻塞的同步请求。 */
void message_manager_destroy(void)
{
    if (!g_msgmgr_created) {
        return;
    }

    MUTEX_LOCK(g_msg_ctx.msg_mutex);
    message_manager_reset_request_locked(true);
    g_sync_done = true;
    pthread_cond_signal(&g_sync_cond);
    MUTEX_UNLOCK(g_msg_ctx.msg_mutex);

    if (g_subscriber_hdl != NULL) {
        (void)EVENTHUB_DestroySubscriber(g_subscriber_hdl);
    }

    g_subscriber_hdl = NULL;
    g_subscriber_desc = NULL;
    g_msgmgr_created = false;
}

/* 发送异步消息，并登记一次性回包回调。 */
int32_t message_manager_send_async(const MESSAGE_S* msg, message_manager_result_cb_t cb)
{
    int32_t ret;

    if (msg == NULL) {
        return MESSAGE_MANAGER_EINVAL;
    }

    if (!g_msgmgr_created) {
        return MESSAGE_MANAGER_ESTATE;
    }

    MUTEX_LOCK(g_msg_ctx.msg_mutex);
    if (!g_msg_ctx.msg_processed) {
        MUTEX_UNLOCK(g_msg_ctx.msg_mutex);
        return MESSAGE_MANAGER_EBUSY;
    }

    message_manager_reset_request_locked(false);
    g_msg_ctx.result_cb = cb;
    g_msg_ctx.msg.topic = msg->topic;
    g_msg_ctx.msg.arg1 = msg->arg1;
    g_msg_ctx.msg.arg2 = msg->arg2;
    memcpy(g_msg_ctx.msg.aszPayload, msg->aszPayload, sizeof(g_msg_ctx.msg.aszPayload));

    ret = MODEMNG_SendMessage(msg);
    if (ret != 0) {
        message_manager_reset_request_locked(true);
        MUTEX_UNLOCK(g_msg_ctx.msg_mutex);
        return MESSAGE_MANAGER_ESEND;
    }
    MUTEX_UNLOCK(g_msg_ctx.msg_mutex);

    return 0;
}

/* 发送同步消息，并在超时内等待匹配回包结果。 */
int32_t message_manager_send_sync_timeout(const MESSAGE_S* msg, uint32_t timeout_ms)
{
    int32_t ret;
    struct timespec ts;

    if (msg == NULL) {
        return MESSAGE_MANAGER_EINVAL;
    }

    if (!g_msgmgr_created) {
        return MESSAGE_MANAGER_ESTATE;
    }

    MUTEX_LOCK(g_msg_ctx.msg_mutex);
    if (!g_msg_ctx.msg_processed) {
        MUTEX_UNLOCK(g_msg_ctx.msg_mutex);
        return MESSAGE_MANAGER_EBUSY;
    }

    g_sync_done = false;
    g_sync_result = MESSAGE_MANAGER_ETIMEOUT;
    message_manager_reset_request_locked(false);
    g_msg_ctx.result_cb = message_manager_sync_result_proc;
    g_msg_ctx.msg.topic = msg->topic;
    g_msg_ctx.msg.arg1 = msg->arg1;
    g_msg_ctx.msg.arg2 = msg->arg2;
    memcpy(g_msg_ctx.msg.aszPayload, msg->aszPayload, sizeof(g_msg_ctx.msg.aszPayload));

    ret = MODEMNG_SendMessage(msg);
    if (ret != 0) {
        message_manager_reset_request_locked(true);
        MUTEX_UNLOCK(g_msg_ctx.msg_mutex);
        return MESSAGE_MANAGER_ESEND;
    }

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += ts.tv_nsec / 1000000000L;
        ts.tv_nsec %= 1000000000L;
    }

    while (!g_sync_done) {
        ret = pthread_cond_timedwait(&g_sync_cond, &g_msg_ctx.msg_mutex, &ts);
        if (ret == ETIMEDOUT) {
            /* 超时仅结束调用方等待；晚到回包仍可能到达，但会被忽略。 */
            message_manager_reset_request_locked(true);
            MUTEX_UNLOCK(g_msg_ctx.msg_mutex);
            MLOG_WARN("通过发送消息超时: topic=%s(0x%x) arg1=%d arg2=%d",
                event_topic_get_name(msg->topic), msg->topic, msg->arg1, msg->arg2);
            return MESSAGE_MANAGER_ETIMEOUT;
        }
    }

    ret = g_sync_result;
    message_manager_reset_request_locked(true);
    MUTEX_UNLOCK(g_msg_ctx.msg_mutex);

    return ret;
}

/* 发送同步消息，并等待成功/失败 topic 之一返回。 */
int32_t message_manager_send_sync_topics_timeout(
    const MESSAGE_S* msg, TOPIC_ID success_topic, TOPIC_ID failure_topic, uint32_t timeout_ms)
{
    int32_t ret;
    struct timespec ts;

    if (msg == NULL || success_topic == 0 || failure_topic == 0) {
        return MESSAGE_MANAGER_EINVAL;
    }

    if (!g_msgmgr_created) {
        return MESSAGE_MANAGER_ESTATE;
    }

    MUTEX_LOCK(g_msg_ctx.msg_mutex);
    if (!g_msg_ctx.msg_processed) {
        MUTEX_UNLOCK(g_msg_ctx.msg_mutex);
        return MESSAGE_MANAGER_EBUSY;
    }

    g_sync_done = false;
    g_sync_result = MESSAGE_MANAGER_ETIMEOUT;
    message_manager_reset_request_locked(false);
    g_msg_ctx.result_cb = message_manager_sync_topics_result_proc;
    g_msg_ctx.success_topic = success_topic;
    g_msg_ctx.failure_topic = failure_topic;
    g_msg_ctx.use_result_topics = true;
    g_msg_ctx.msg.topic = msg->topic;
    g_msg_ctx.msg.arg1 = msg->arg1;
    g_msg_ctx.msg.arg2 = msg->arg2;
    memcpy(g_msg_ctx.msg.aszPayload, msg->aszPayload, sizeof(g_msg_ctx.msg.aszPayload));

    ret = MODEMNG_SendMessage(msg);
    if (ret != 0) {
        message_manager_reset_request_locked(true);
        MUTEX_UNLOCK(g_msg_ctx.msg_mutex);
        return MESSAGE_MANAGER_ESEND;
    }

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += ts.tv_nsec / 1000000000L;
        ts.tv_nsec %= 1000000000L;
    }

    while (!g_sync_done) {
        ret = pthread_cond_timedwait(&g_sync_cond, &g_msg_ctx.msg_mutex, &ts);
        if (ret == ETIMEDOUT) {
            message_manager_reset_request_locked(true);
            MUTEX_UNLOCK(g_msg_ctx.msg_mutex);
            MLOG_WARN("等待消息结果超时: request=%s(0x%x) success=%s(0x%x) failure=%s(0x%x)",
                event_topic_get_name(msg->topic),
                msg->topic,
                event_topic_get_name(success_topic),
                success_topic,
                event_topic_get_name(failure_topic),
                failure_topic);
            return MESSAGE_MANAGER_ETIMEOUT;
        }
    }

    ret = g_sync_result;
    message_manager_reset_request_locked(true);
    MUTEX_UNLOCK(g_msg_ctx.msg_mutex);

    return ret;
}
