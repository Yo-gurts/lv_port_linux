// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "core/framebuffer_manager.h"
#include "mlog.h"
#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h> 
// #endregion
// #############################################################################
// ! #region 2. 数据结构定义
// #############################################################################

struct framebuffer_manager_t {
    int fbfd;
    struct fb_var_screeninfo vinfo;
    uint8_t* fb_mem;       /* mmap 的 framebuffer */
    uint8_t* fb_buf1;      /* framebuffer buffer 1 */
    uint8_t* fb_buf2;      /* framebuffer buffer 2 */
    size_t buf_size;
    int current_fb;        /* 当前显示的 buffer (0 或 1) */
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
 * LVGL flush 回调函数
 * 在 call_flush_cb() 被调用后触发
 * 我们在这里调用 FBIOPAN_DISPLAY 通知硬件刷新显示
 */
static void custom_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* color_p)
{   
    LV_UNUSED(area);
    /* 确定目标 framebuffer */
    uint32_t new_yoffset;
    
    if (g_fb_mgr->current_fb == 0) {
        memcpy(g_fb_mgr->fb_buf2, color_p, g_fb_mgr->buf_size);
        new_yoffset = g_fb_mgr->vinfo.yres;
        g_fb_mgr->current_fb = 1;
    } else {
        memcpy(g_fb_mgr->fb_buf1, color_p, g_fb_mgr->buf_size);
        new_yoffset = 0;
        g_fb_mgr->current_fb = 0;
    }

    /* 切换显示 */
    g_fb_mgr->vinfo.yoffset = new_yoffset;
    ioctl(g_fb_mgr->fbfd, FBIOPAN_DISPLAY, &g_fb_mgr->vinfo);
    lv_display_flush_ready(disp);
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
    mgr->vinfo.xres_virtual = mgr->vinfo.xres;
    mgr->vinfo.yres_virtual = mgr->vinfo.yres * 2;
    mgr->vinfo.activate = FB_ACTIVATE_NOW;
    mgr->buf_size = mgr->vinfo.xres * mgr->vinfo.yres * (mgr->vinfo.bits_per_pixel >> 3);
    size_t fb_total_size = mgr->buf_size * 2;
    if (ioctl(mgr->fbfd, FBIOPUT_VSCREENINFO, &mgr->vinfo) < 0) {
        MLOG_ERR("FBIOPUT_VSCREENINFO failed");
        close(mgr->fbfd);
        free(mgr);
        return NULL;
    }
    mgr->fb_mem = mmap(NULL, fb_total_size, PROT_READ | PROT_WRITE, MAP_SHARED, mgr->fbfd, 0);
    mgr->fb_buf1 = mgr->fb_mem;
    mgr->fb_buf2 = mgr->fb_mem + mgr->buf_size;
    mgr->current_fb = 0;
    // 注册回调函数
    lv_display_set_flush_cb(disp, custom_flush_cb);
    /* Make sure that the display is on. */
    if (ioctl(mgr->fbfd, FBIOBLANK, FB_BLANK_UNBLANK) != 0) {
        perror("ioctl(FBIOBLANK)");
        /* Don't return. Some framebuffer drivers like efifb or simplefb don't implement FBIOBLANK.*/
    }

    g_fb_mgr = mgr;

    MLOG_INFO("Framebuffer manager created: %s", device);
    return mgr;
}

void framebuffer_manager_destroy(framebuffer_manager_t* mgr)
{
    if (!mgr) {
        return;
    }
    if (mgr->fb_mem && mgr->fb_mem != MAP_FAILED) {
        munmap(mgr->fb_mem, mgr->buf_size * 2);
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
