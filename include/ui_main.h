/**
 * @file    ui_main.h
 * @brief   UI 库入口头文件
 */

#ifndef UI_MAIN_H
#define UI_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   UI 库初始化入口
 * @return  0 成功，-1 失败
 * @note    板端运行时由外部 main 调用此函数初始化 UI
 */
int ui_main(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_MAIN_H */
