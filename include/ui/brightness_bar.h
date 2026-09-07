#ifndef __BRIGHTNESS_BAR_H__
#define __BRIGHTNESS_BAR_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化屏幕亮度控制条
 * @note 在系统启动时调用，创建屏幕亮度控制条组件
 */
void brightness_bar_init(void);

/**
 * @brief 显示屏幕亮度控制条
 * @note 仅显示亮度控制条，3秒后自动隐藏
 */
void brightness_bar_show(void);

/**
 * @brief 隐藏屏幕亮度控制条
 * @note 立即隐藏亮度控制条
 */
void brightness_bar_hide(void);

/**
 * @brief 重置自动隐藏计时器
 * @note 用户交互后调用，重置3秒计时器
 */
void brightness_bar_reset_timer(void);

#ifdef __cplusplus
}
#endif

#endif /* __BRIGHTNESS_BAR_H__ */
