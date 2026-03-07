#ifndef __POWER_MANAGER_H__
#define __POWER_MANAGER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*power_manager_shutdown_prepare_cb_t)(void* user_data);

int power_manager_init(void);
void power_manager_deinit(void);
void power_manager_poll(void);

void power_manager_mark_activity(void);
void power_manager_disable_auto_sleep(void);
void power_manager_enable_auto_sleep(void);

void power_manager_register_shutdown_prepare_cb(power_manager_shutdown_prepare_cb_t cb, void* user_data);
void power_manager_unregister_shutdown_prepare_cb(power_manager_shutdown_prepare_cb_t cb, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* __POWER_MANAGER_H__ */
