/**
 * @file    ui_common.h
 * @brief   UI 对外公共入口定义
 */

#ifndef __UI_COMMON_H__
#define __UI_COMMON_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   UI 主循环入口（模拟器可直接调用）
 * @return  0 成功，-1 失败
 */
int32_t ui_main(void);

/**
 * @brief   UIAPP 启动入口（板端通过该接口启动 UI）
 * @return  0 成功，-1 失败
 */
int32_t UIAPP_Start(void);

#ifdef __cplusplus
}
#endif

#endif /* __UI_COMMON_H__ */
