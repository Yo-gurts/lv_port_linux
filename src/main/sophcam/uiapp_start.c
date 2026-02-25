/**
 * @file    ui_appstart_thread.c
 * @brief   板端模式下的 UIAPP_Start 适配（使用 OSAL 线程启动 UI）
 */

#include "ui_common.h"
#include "core/message_manager.h"
#include "mlog.h"
#include "osal.h"
#include <stdbool.h>

static OSAL_TASK_HANDLE_S s_ui_task;
static bool s_ui_started = false;

static void ui_thread_entry(void* arg)
{
    (void)arg;
    (void)ui_main();
}

int32_t UIAPP_Start(void)
{
    int32_t rc;
    OSAL_TASK_ATTR_S ta = {0};

    if (s_ui_started) {
        MLOG_INFO("UI already started");
        return 0;
    }

    rc = message_manager_create();
    if (rc != 0) {
        MLOG_ERR("message manager create failed, %d", rc);
        return -1;
    }

    ta.name = "aicam_ui";
    ta.entry = ui_thread_entry;
    ta.param = NULL;
    ta.priority = OSAL_TASK_PRI_RT_LOWEST;
    ta.detached = false;
    ta.stack_size = 256 * 1024;

    rc = OSAL_TASK_Create(&ta, &s_ui_task);
    if (rc != OSAL_SUCCESS) {
        MLOG_ERR("aicam_ui task create failed, %d", rc);
        message_manager_destroy();
        return -1;
    }

    s_ui_started = true;
    return 0;
}
