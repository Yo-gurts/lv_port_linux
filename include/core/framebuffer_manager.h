/**
 * @file framebuffer_manager.h
 * @brief Framebuffer 管理器
 *
 * 用于在 LVGL flush 完成后执行硬件特定的 ioctl 调用
 */

#ifndef FRAMEBUFFER_MANAGER_H
#define FRAMEBUFFER_MANAGER_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

struct framebuffer_manager_t;
typedef struct framebuffer_manager_t framebuffer_manager_t;

/**
 * @brief 创建 framebuffer 管理器并注册 flush 完成回调
 * @param device framebuffer 设备路径，如 /dev/fb0
 * @param disp LVGL display 指针
 * @return 管理器实例，失败返回 NULL
 */
framebuffer_manager_t* framebuffer_manager_create(const char* device, lv_display_t* disp);

/**
 * @brief 销毁 framebuffer 管理器
 * @param mgr 管理器实例
 */
void framebuffer_manager_destroy(framebuffer_manager_t* mgr);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_MANAGER_H */
