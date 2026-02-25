#ifndef __KEY_MANAGER_H__
#define __KEY_MANAGER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    KEY_ID_POWER = 0,
    KEY_ID_AI,
    KEY_ID_VOLUME_UP,
    KEY_ID_VOLUME_DOWN,
    KEY_ID_FOCUS,
    KEY_ID_CAMERA,
    KEY_ID_BUTT,
    KEY_ID_ANY = -1
} key_id_t;

typedef enum {
    KEY_EVENT_CLICK = 0,
    KEY_EVENT_LONG_PRESS,
    KEY_EVENT_LONG_PRESS_REPEAT,
    KEY_EVENT_BUTT,
    KEY_EVENT_ANY = -1
} key_event_type_t;

typedef void (*key_event_callback_t)(key_id_t key, key_event_type_t event_type, void* user_data);

int key_manager_init(void);
void key_manager_deinit(void);

/* 初始化按键管理器，打开输入设备并清理内部状态。 */
/* 允许设备打开失败（例如模拟器环境），调用仍返回0。 */
/* 在 GUI 主循环中周期调用，处理输入设备事件与长按状态机。 */
void key_manager_poll(void);

/* 注册按键事件回调：可按具体 key/event 注册，也可用 ANY 订阅通配事件。 */
/* 当前实现为每个 key+event 仅允许注册一个回调。 */
int key_manager_register_callback(key_id_t key, key_event_type_t event_type, key_event_callback_t callback, void* user_data);
/* 注销与注册参数完全匹配的一条回调记录。 */
int key_manager_unregister_callback(key_id_t key, key_event_type_t event_type, key_event_callback_t callback, void* user_data);

/* 全局长按参数，单位毫秒。 */
void key_manager_set_long_press_ms(uint32_t long_press_ms);
void key_manager_set_repeat_ms(uint32_t repeat_ms);

/* 设置是否屏蔽非电源键事件：1=屏蔽，0=恢复。 */
/* 切换屏蔽状态时会清空非电源键当前按压状态，避免解除后补发旧事件。 */
void key_manager_set_block_non_power(uint8_t blocked);
uint8_t key_manager_get_block_non_power(void);

#ifdef __cplusplus
}
#endif

#endif /* __KEY_MANAGER_H__ */
