/**
 * @file    status_bar.h
 * @brief   全局状态栏组件 - 统一管理电池和WiFi图标
 */
#ifndef __STATUS_BAR_H__
#define __STATUS_BAR_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STATUS_BAR_ICON_NONE = 0,
    STATUS_BAR_ICON_BATTERY,
    STATUS_BAR_ICON_WIFI,
    STATUS_BAR_ICON_SD
} status_bar_icon_type_t;

/**
 * @brief 初始化状态栏（需在 ui_main 中调用）
 */
void status_bar_init(void);

/**
 * @brief 显示/隐藏状态栏
 * @param show true-显示，false-隐藏
 */
void status_bar_show(bool show);

/**
 * @brief 设置状态栏 3 个槽位（从左到右）的图标类型
 * @param left 左侧槽位图标类型
 * @param middle 中间槽位图标类型
 * @param right 右侧槽位图标类型
 */
void status_bar_set_icons(status_bar_icon_type_t left, status_bar_icon_type_t middle, status_bar_icon_type_t right);

/**
 * @brief 立即刷新电池和WiFi图标
 */
void status_bar_refresh(void);

#ifdef __cplusplus
}
#endif

#endif /* __STATUS_BAR_H__ */
