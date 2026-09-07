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

/* 屏幕亮度百分比范围（背光占空比），供屏幕亮度设置使用。 */
#define POWER_MANAGER_LUMA_MIN 10
#define POWER_MANAGER_LUMA_MAX 100

/* 设置屏幕背光亮度百分比（10~100），仅运行期生效、不持久化。
 * 屏幕亮度设置通过本封装间接调背光 HAL，避免直接依赖 hal_backlight.h（SDL 无该头）。 */
void power_manager_set_luma(int percent);

/* 获取当前屏幕背光亮度百分比（10~100）。 */
int power_manager_get_luma(void);

#ifdef __cplusplus
}
#endif

#endif /* __POWER_MANAGER_H__ */
