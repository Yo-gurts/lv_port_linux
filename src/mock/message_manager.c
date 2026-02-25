#include <stdio.h>
#include <stdint.h>

typedef struct {
    uint32_t topic;
    uint32_t arg1;
    uint32_t arg2;
    char aszPayload[128];
} MESSAGE_S;

typedef struct {
    uint32_t topic;
    uint32_t arg1;
    uint32_t arg2;
    int32_t s32Result;
} EVENT_S;

typedef int32_t (*message_manager_result_cb_t)(EVENT_S* evt);

#define MESSAGE_MANAGER_EINVAL (-1001)

int32_t message_manager_create(void);
void message_manager_destroy(void);
int32_t message_manager_send_async(const MESSAGE_S* msg, message_manager_result_cb_t cb);
int32_t message_manager_send_sync_timeout(const MESSAGE_S* msg, uint32_t timeout_ms);

/* mock 初始化：保持空实现。 */
int32_t message_manager_create(void)
{
    return 0;
}

/* mock 销毁：保持空实现。 */
void message_manager_destroy(void)
{
}

/* mock 异步发送：仅打印消息关键信息。 */
int32_t message_manager_send_async(const MESSAGE_S* msg, message_manager_result_cb_t cb)
{
    (void)cb;

    if (msg == NULL) {
        return MESSAGE_MANAGER_EINVAL;
    }

    printf("[message_manager_mock] async topic=0x%x arg1=0x%x arg2=0x%x\\n",
        (unsigned int)msg->topic,
        (unsigned int)msg->arg1,
        (unsigned int)msg->arg2);

    return 0;
}

/* mock 同步发送：仅打印消息关键信息并直接返回成功。 */
int32_t message_manager_send_sync_timeout(const MESSAGE_S* msg, uint32_t timeout_ms)
{
    if (msg == NULL) {
        return MESSAGE_MANAGER_EINVAL;
    }

    printf("[message_manager_mock] sync topic=0x%x arg1=0x%x arg2=0x%x timeout=%u ms\\n",
        (unsigned int)msg->topic,
        (unsigned int)msg->arg1,
        (unsigned int)msg->arg2,
        (unsigned int)timeout_ms);

    return 0;
}
