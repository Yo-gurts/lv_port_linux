#ifndef __MESSAGE_MANAGER_H__
#define __MESSAGE_MANAGER_H__

#include <stdint.h>

/* 宏隔离：板级/FB 构建（FB_RUN 未定义或 =1）走底层真身；
 * SDL 仿真（CMake 定义 FB_RUN=0）无 SDK 头可引，用本地最小类型镜像。
 * 镜像布局对齐 sysutils_eventhub.h 的 EVENT_S（MESSAGE_S 同
 * sysutils_hfsm.h 为其别名）；事件常量（EVENT_MODEMNG_* 等）仅
 * SDK-only 的 core 实现使用，SDL 路径不引用，故不在此提供。 */
#if defined(FB_RUN) && !FB_RUN
typedef uint32_t TOPIC_ID;

typedef struct ps_msg_s {
    uint32_t topic; /* Message topic */
    int32_t arg1;
    int32_t arg2;
    int32_t s32Result;
    uint64_t u64CreateTime;
    uint8_t aszPayload[128];
} EVENT_S;

typedef EVENT_S MESSAGE_S;
#else
#include "mode.h"
#include "sysutils_eventhub.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MESSAGE_MANAGER_EINVAL (-1001)
#define MESSAGE_MANAGER_EBUSY (-1002)
#define MESSAGE_MANAGER_ESEND (-1003)
#define MESSAGE_MANAGER_ETIMEOUT (-1004)
#define MESSAGE_MANAGER_ESTATE (-1005)

typedef int32_t (*message_manager_result_cb_t)(EVENT_S* evt);

int32_t message_manager_create(void);
void message_manager_destroy(void);
/* UI 主循环每帧调用：消费 status-bar 刷新 / top-notice 待处理项并在 UI 线程执行。 */
void message_manager_poll(void);
int32_t message_manager_send_async(const MESSAGE_S* msg, message_manager_result_cb_t cb);
/* 异步发送并按成功/失败 topic 匹配回包（不阻塞，发起即返回）：用于请求 topic 与完成
 * 回包 topic 不同的场景（如 START_PIV -> PHOTO_INDEXED/FAILED），回包到达时经订阅线程
 * 回调 cb。同一时刻仅允许一个在途请求，忙时返回 EBUSY。 */
int32_t message_manager_send_async_topics(
    const MESSAGE_S* msg, TOPIC_ID success_topic, TOPIC_ID failure_topic, message_manager_result_cb_t cb);
int32_t message_manager_send_sync_timeout(const MESSAGE_S* msg, uint32_t timeout_ms);
int32_t message_manager_send_sync_topics_timeout(
    const MESSAGE_S* msg, TOPIC_ID success_topic, TOPIC_ID failure_topic, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* __MESSAGE_MANAGER_H__ */
