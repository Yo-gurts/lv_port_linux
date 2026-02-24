/**
 * @file    ui_appstart_sdl.c
 * @brief   SDL 模式下的 UIAPP_Start 适配
 */

#include "ui_common.h"

int32_t UIAPP_Start(void)
{
    return ui_main();
}
