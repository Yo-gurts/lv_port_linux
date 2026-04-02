#ifndef __MESSAGE_MANAGER_H__
#define __MESSAGE_MANAGER_H__

#include <stdint.h>
#include "mode.h"
#include "sysutils_eventhub.h"

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
int32_t message_manager_send_async(const MESSAGE_S* msg, message_manager_result_cb_t cb);
int32_t message_manager_send_sync_timeout(const MESSAGE_S* msg, uint32_t timeout_ms);
int32_t message_manager_send_sync_topics_timeout(
    const MESSAGE_S* msg, TOPIC_ID success_topic, TOPIC_ID failure_topic, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* __MESSAGE_MANAGER_H__ */
