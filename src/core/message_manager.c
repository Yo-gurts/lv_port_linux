#include "core/message_manager.h"
#include "appcomm.h"
#include "mlog.h"
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
    pthread_mutex_t msg_mutex;
} message_context_t;

static message_context_t g_msg_ctx = {
    .msg_processed = true,
    .result_cb = NULL,
    .msg_mutex = PTHREAD_MUTEX_INITIALIZER,
};

static pthread_cond_t g_sync_cond = PTHREAD_COND_INITIALIZER;
static int32_t g_sync_result = MESSAGE_MANAGER_ESTATE;
static bool g_sync_done = false;

static EVENTHUB_SUBSCRIBER_S* g_subscriber_desc = NULL;
static MW_PTR g_subscriber_hdl = NULL;
static bool g_msgmgr_created = false;

/* 同步发送模式下的回包处理：记录结果并唤醒等待线程。 */
static int32_t message_manager_sync_result_proc(EVENT_S* evt)
{
    g_sync_result = evt->s32Result;
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

    /* 通过 (topic,arg1,arg2) 匹配回包，完成当前请求。 */
    if ((g_msg_ctx.msg.topic == evt->topic) && (g_msg_ctx.msg.arg1 == evt->arg1) && (g_msg_ctx.msg.arg2 == evt->arg2)) {
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
    switch (evt->topic) {
    case EVENT_MODEMNG_MODEOPEN:
    case EVENT_MODEMNG_MODECLOSE:
    case EVENT_MODEMNG_MODESWITCH:
    case EVENT_MODEMNG_CARD_REMOVE:
    case EVENT_MODEMNG_CARD_AVAILABLE:
    case EVENT_MODEMNG_CARD_UNAVAILABLE:
    case EVENT_MODEMNG_CARD_ERROR:
    case EVENT_MODEMNG_CARD_FSERROR:
    case EVENT_MODEMNG_CARD_SLOW:
    case EVENT_MODEMNG_CARD_CHECKING:
    case EVENT_MODEMNG_CARD_READ_ONLY:
    case EVENT_MODEMNG_CARD_MOUNT_FAILED:
    case EVENT_MODEMNG_CARD_FORMATING:
    case EVENT_MODEMNG_CARD_FORMAT_SUCCESSED:
    case EVENT_MODEMNG_CARD_FORMAT_FAILED:
    case EVENT_MODEMNG_RECODER_STARTSTATU:
    case EVENT_MODEMNG_RECODER_STOPSTATU:
    case EVENT_MODEMNG_RECODER_SPLITREC:
    case EVENT_MODEMNG_RECODER_STARTEVENTSTAUE:
    case EVENT_MODEMNG_RECODER_STOPEVENTSTAUE:
    case EVENT_MODEMNG_RECODER_STARTEMRSTAUE:
    case EVENT_MODEMNG_RECODER_STOPEMRSTAUE:
    case EVENT_MODEMNG_RECODER_STARTPIVSTAUE:
    case EVENT_MODEMNG_RECODER_STOPPIVSTAUE:
        MLOG_DBG("message_manager handled topic=0x%x result=%d", evt->topic, evt->s32Result);
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
    g_msg_ctx.msg_processed = true;
    g_msg_ctx.result_cb = NULL;
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

    g_msg_ctx.msg_processed = false;
    g_msg_ctx.result_cb = cb;
    g_msg_ctx.msg.topic = msg->topic;
    g_msg_ctx.msg.arg1 = msg->arg1;
    g_msg_ctx.msg.arg2 = msg->arg2;
    memcpy(g_msg_ctx.msg.aszPayload, msg->aszPayload, sizeof(g_msg_ctx.msg.aszPayload));

    ret = MODEMNG_SendMessage(msg);
    if (ret != 0) {
        g_msg_ctx.msg_processed = true;
        g_msg_ctx.result_cb = NULL;
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
    g_msg_ctx.msg_processed = false;
    g_msg_ctx.result_cb = message_manager_sync_result_proc;
    g_msg_ctx.msg.topic = msg->topic;
    g_msg_ctx.msg.arg1 = msg->arg1;
    g_msg_ctx.msg.arg2 = msg->arg2;
    memcpy(g_msg_ctx.msg.aszPayload, msg->aszPayload, sizeof(g_msg_ctx.msg.aszPayload));

    ret = MODEMNG_SendMessage(msg);
    if (ret != 0) {
        g_msg_ctx.msg_processed = true;
        g_msg_ctx.result_cb = NULL;
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
            g_msg_ctx.msg_processed = true;
            g_msg_ctx.result_cb = NULL;
            MUTEX_UNLOCK(g_msg_ctx.msg_mutex);
            return MESSAGE_MANAGER_ETIMEOUT;
        }
    }

    ret = g_sync_result;
    g_msg_ctx.msg_processed = true;
    g_msg_ctx.result_cb = NULL;
    MUTEX_UNLOCK(g_msg_ctx.msg_mutex);

    return ret;
}
