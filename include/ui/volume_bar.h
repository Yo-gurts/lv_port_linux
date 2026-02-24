#ifndef __VOLUME_BAR_H__
#define __VOLUME_BAR_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化音量控制条
 * @note 在系统启动时调用，创建音量控制条组件
 */
void volume_bar_init(void);

/**
 * @brief 显示音量控制条
 * @note 按当前音量参数显示，2秒后自动淡出
 */
void volume_bar_show(void);

/**
 * @brief 隐藏音量控制条
 * @note 立即隐藏音量控制条
 */
void volume_bar_hide(void);

/**
 * @brief 设置音量值
 * @param volume 音量值 (0-100)
 * @note 更新音量值并确保可见，2秒后自动淡出
 */
void volume_bar_set_value(int volume);

/**
 * @brief 获取当前音量值
 * @return 当前音量值 (0-100)
 */
int volume_bar_get_value(void);

/**
 * @brief 重置自动隐藏计时器
 * @note 用户交互后调用，重置2秒计时器
 */
void volume_bar_reset_timer(void);

#ifdef __cplusplus
}
#endif

#endif /* __VOLUME_BAR_H__ */
