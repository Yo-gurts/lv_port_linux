// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "core/framebuffer_manager.h"
#include "mlog.h"
#include <fcntl.h>
#include <ioctl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义
// #############################################################################

struct framebuffer_manager_t {
    int fbfd; // framebuffer 文件描述符
    struct fb_var_screeninfo vinfo; // 可变屏幕信息
};

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

static framebuffer_manager_t* g_fb_mgr = NULL;

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

/**
 * LVGL flush 完成事件回调
 * 在 lv_display_flush_ready() 被调用后触发，此时数据已经拷贝到 framebuffer
 * 我们在这里调用 FBIOPAN_DISPLAY 通知硬件刷新显示
 */
static void flush_finish_event_cb(lv_event_t* event)
{
    LV_UNUSED(event);

    if (g_fb_mgr == NULL || g_fb_mgr->fbfd < 0) {
        return;
    }

    // 调用 FBIOPAN_DISPLAY 通知硬件显示内容已更新
    if (ioctl(g_fb_mgr->fbfd, FBIOPAN_DISPLAY, &g_fb_mgr->vinfo) < 0) {
        MLOG_ERR("FBIOPAN_DISPLAY failed");
    }
}

// #endregion
// #############################################################################
// ! #region 5. 对外接口函数
// #############################################################################

framebuffer_manager_t* framebuffer_manager_create(const char* device, lv_display_t* disp)
{
    framebuffer_manager_t* mgr = (framebuffer_manager_t*)malloc(sizeof(framebuffer_manager_t));
    if (!mgr) {
        MLOG_ERR("Failed to allocate framebuffer manager");
        return NULL;
    }

    memset(mgr, 0, sizeof(framebuffer_manager_t));

    // 打开 framebuffer 设备
    mgr->fbfd = open(device, O_RDWR);
    if (mgr->fbfd < 0) {
        MLOG_ERR("Failed to open framebuffer device: %s", device);
        free(mgr);
        return NULL;
    }

    // 获取可变屏幕信息
    if (ioctl(mgr->fbfd, FBIOGET_VSCREENINFO, &mgr->vinfo) < 0) {
        MLOG_ERR("FBIOGET_VSCREENINFO failed");
        close(mgr->fbfd);
        free(mgr);
        return NULL;
    }

    /* Make sure that the display is on. */
    if (ioctl(dsc->fbfd, FBIOBLANK, FB_BLANK_UNBLANK) != 0) {
        perror("ioctl(FBIOBLANK)");
        /* Don't return. Some framebuffer drivers like efifb or simplefb don't implement FBIOBLANK.*/
    }

    g_fb_mgr = mgr;

    // 注册 LV_EVENT_FLUSH_FINISH 事件回调
    // 该事件在 flush_cb 返回后、lv_display_flush_ready() 内部触发
    lv_display_add_event_cb(disp, flush_finish_event_cb, LV_EVENT_FLUSH_FINISH, NULL);

    MLOG_INFO("Framebuffer manager created: %s", device);
    return mgr;
}

void framebuffer_manager_destroy(framebuffer_manager_t* mgr)
{
    if (!mgr) {
        return;
    }

    if (mgr->fbfd >= 0) {
        close(mgr->fbfd);
    }

    g_fb_mgr = NULL;
    free(mgr);
}

// #endregion
// #############################################################################
// ! #region 6. 线程函数
// #############################################################################

// #endregion
// #############################################################################
// ! #region 7. 事件回调函数（按键、手势、定时器等）
// #############################################################################

// #endregion
// #############################################################################
// ! #region 8. 初始化/销毁/资源管理函数
// #############################################################################

// #endregion
// #############################################################################
// ! #region 9. 调试与测试代码
// #############################################################################
