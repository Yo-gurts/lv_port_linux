/**
 * @file    sim_automation.c
 * @brief   SDL 模拟器自动化实现：FIFO 命令通道 + 纯 UI 截图 + 按键注入
 *
 * 仅在 SDL 仿真构建（FB_RUN=0）编译生效；板端 FB 构建中所有接口为空实现。
 */

#include "core/sim_automation.h"

#if defined(FB_RUN) && !FB_RUN

#include "core/key_manager.h"
#include "core/page_manager.h"
#include "lvgl/lvgl.h"
#include "mlog.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <SDL2/SDL.h>

/* =======================
 * 内部状态
 * ======================= */
static int g_fifo_fd = -1; /* FIFO 读端，非阻塞 */
static char g_line_buf[512];
static size_t g_line_len = 0;

/* 延迟抬键：key 命令按下后经若干个 poll 周期再抬起，形成短按 CLICK */
#define SIM_AUTO_RELEASE_DELAY_POLLS 4
static int g_pending_release_key = -1;
static int g_release_countdown = 0;

/* =======================
 * 按键名映射
 * ======================= */
static const struct {
    const char* name;
    key_id_t id;
} g_key_names[] = {
    { "power", KEY_ID_POWER },
    { "assistant", KEY_ID_ASSISTANT },
    { "volup", KEY_ID_VOLUME_UP },
    { "voldown", KEY_ID_VOLUME_DOWN },
    { "focus", KEY_ID_FOCUS },
    { "camera", KEY_ID_CAMERA },
    { "mode", KEY_ID_MODE },
    { "menu", KEY_ID_MENU },
    { "up", KEY_ID_UP },
    { "down", KEY_ID_DOWN },
    { "left", KEY_ID_LEFT },
    { "right", KEY_ID_RIGHT },
    { "ok", KEY_ID_OK },
};

static key_id_t sim_auto_key_from_name(const char* name)
{
    size_t i;

    if (name == NULL || name[0] == '\0') {
        return KEY_ID_ANY;
    }
    for (i = 0; i < sizeof(g_key_names) / sizeof(g_key_names[0]); i++) {
        if (strcmp(g_key_names[i].name, name) == 0) {
            return g_key_names[i].id;
        }
    }
    return KEY_ID_ANY;
}

/* =======================
 * 截图：读 LVGL draw buffer → SDL Surface → BMP
 * ======================= */
static lv_obj_t* sim_auto_find_mouse_cursor(void)
{
    lv_indev_t* indev = NULL;

    while ((indev = lv_indev_get_next(indev)) != NULL) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            return lv_indev_get_cursor(indev);
        }
    }
    return NULL;
}

int sim_auto_screenshot(const char* path)
{
    lv_display_t* disp = lv_display_get_default();
    lv_draw_buf_t* buf;
    lv_obj_t* cursor;
    uint8_t cursor_was_hidden = 0;
    SDL_Surface* surface;
    char auto_path[128];
    struct timespec ts;
    int ret;

    if (disp == NULL) {
        MLOG_ERR("[sim_auto] screenshot: no default display");
        return -1;
    }
    if (path == NULL || path[0] == '\0') {
        clock_gettime(CLOCK_REALTIME, &ts);
        snprintf(auto_path, sizeof(auto_path), "sim_%ld.png", (long)ts.tv_sec);
        path = auto_path;
    }

    /* 隐藏鼠标光标，截到纯 UI 画面 */
    cursor = sim_auto_find_mouse_cursor();
    if (cursor != NULL && !lv_obj_has_flag(cursor, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(cursor, LV_OBJ_FLAG_HIDDEN);
        cursor_was_hidden = 1;
    }

    /* 强制立即重绘，保证 buffer 内容与隐藏光标后的画面一致 */
    lv_refr_now(NULL);

    buf = lv_display_get_buf_active(disp);
    if (buf == NULL || buf->data == NULL) {
        MLOG_ERR("[sim_auto] screenshot: no active draw buffer");
        if (cursor_was_hidden) {
            lv_obj_clear_flag(cursor, LV_OBJ_FLAG_HIDDEN);
        }
        return -1;
    }

    /* LVGL ARGB8888 内存布局（字节序 B,G,R,A）与 SDL_PIXELFORMAT_ARGB8888 一致 */
    surface = SDL_CreateRGBSurfaceWithFormatFrom((void*)buf->data, buf->header.w, buf->header.h, 32,
        (int)buf->header.stride, SDL_PIXELFORMAT_ARGB8888);
    if (surface == NULL) {
        MLOG_ERR("[sim_auto] screenshot: SDL_CreateRGBSurfaceWithFormatFrom failed: %s", SDL_GetError());
        if (cursor_was_hidden) {
            lv_obj_clear_flag(cursor, LV_OBJ_FLAG_HIDDEN);
        }
        return -1;
    }

    /* SDL_SaveBMP 只支持 .bmp 后缀；此处以 .png 命名时仍输出 BMP 内容，
     * 由外部脚本统一转 PNG（手册构建期处理）。 */
    ret = SDL_SaveBMP(surface, path);
    SDL_FreeSurface(surface);

    if (cursor_was_hidden) {
        lv_obj_clear_flag(cursor, LV_OBJ_FLAG_HIDDEN);
    }
    if (ret != 0) {
        MLOG_ERR("[sim_auto] screenshot: SDL_SaveBMP(%s) failed: %s", path, SDL_GetError());
        return -1;
    }
    MLOG_INFO("[sim_auto] screenshot saved: %s (%ux%u)", path, buf->header.w, buf->header.h);
    return 0;
}

/* =======================
 * 命令解析与执行
 * ======================= */
static void sim_auto_exec(const char* cmd)
{
    const char* arg;
    key_id_t key;

    /* 跳过前导空白 */
    while (*cmd == ' ' || *cmd == '\t') {
        cmd++;
    }
    if (cmd[0] == '\0') {
        return;
    }

    /* screenshot [path] */
    if (strncmp(cmd, "screenshot", 10) == 0 && (cmd[10] == ' ' || cmd[10] == '\t' || cmd[10] == '\0')) {
        arg = cmd + 10;
        while (*arg == ' ' || *arg == '\t') {
            arg++;
        }
        sim_auto_screenshot(arg);
        return;
    }

    /* key <name> —— 完整短按（按下 + 延迟抬起，产生 CLICK） */
    if (strncmp(cmd, "key ", 4) == 0) {
        key = sim_auto_key_from_name(cmd + 4);
        if (key == KEY_ID_ANY) {
            MLOG_WARN("[sim_auto] unknown key name: '%s'", cmd + 4);
            return;
        }
        key_manager_inject_key(key, 1);
        g_pending_release_key = (int)key;
        g_release_countdown = SIM_AUTO_RELEASE_DELAY_POLLS;
        return;
    }

    /* keydown <name> —— 仅按下（配合 keyup 可产生长按） */
    if (strncmp(cmd, "keydown ", 8) == 0) {
        key = sim_auto_key_from_name(cmd + 8);
        if (key == KEY_ID_ANY) {
            MLOG_WARN("[sim_auto] unknown key name: '%s'", cmd + 8);
            return;
        }
        key_manager_inject_key(key, 1);
        return;
    }

    /* keyup <name> —— 仅抬起 */
    if (strncmp(cmd, "keyup ", 6) == 0) {
        key = sim_auto_key_from_name(cmd + 6);
        if (key == KEY_ID_ANY) {
            MLOG_WARN("[sim_auto] unknown key name: '%s'", cmd + 6);
            return;
        }
        key_manager_inject_key(key, 0);
        return;
    }

    /* nav <page> —— 直接跳转指定页面 */
    if (strncmp(cmd, "nav ", 4) == 0) {
        if (page_manager_navigate(cmd + 4) != 0) {
            MLOG_WARN("[sim_auto] navigate failed: '%s'", cmd + 4);
        }
        return;
    }

    /* back —— 返回上一页 */
    if (strcmp(cmd, "back") == 0) {
        (void)page_manager_back();
        return;
    }

    /* quit —— 退出模拟器 */
    if (strcmp(cmd, "quit") == 0) {
        MLOG_INFO("[sim_auto] quit requested");
        exit(0);
    }

    MLOG_WARN("[sim_auto] unknown command: '%s'", cmd);
}

/* =======================
 * FIFO 通道
 * ======================= */
int sim_auto_init(void)
{
    const char* fifo_path = getenv("SIM_AUTO_FIFO");

    if (fifo_path == NULL || fifo_path[0] == '\0') {
        fifo_path = SIM_AUTO_DEFAULT_FIFO;
    }

    if (mkfifo(fifo_path, 0666) != 0 && errno != EEXIST) {
        MLOG_WARN("[sim_auto] mkfifo(%s) failed: %s", fifo_path, strerror(errno));
        return -1;
    }

    /* O_RDONLY|O_NONBLOCK：无写入者时 read 返回 EAGAIN，不阻塞主循环 */
    g_fifo_fd = open(fifo_path, O_RDONLY | O_NONBLOCK);
    if (g_fifo_fd < 0) {
        MLOG_WARN("[sim_auto] open fifo(%s) failed: %s", fifo_path, strerror(errno));
        return -1;
    }

    g_line_len = 0;
    MLOG_INFO("[sim_auto] automation fifo ready: %s", fifo_path);
    return 0;
}

void sim_auto_poll(void)
{
    char chunk[256];
    ssize_t rd;
    size_t i;

    if (g_fifo_fd < 0) {
        return;
    }

    /* 延迟抬键计数（主循环 5ms 一拍，4 拍 ≈ 20ms 短按） */
    if (g_pending_release_key >= 0 && --g_release_countdown <= 0) {
        key_manager_inject_key((key_id_t)g_pending_release_key, 0);
        g_pending_release_key = -1;
    }

    while ((rd = read(g_fifo_fd, chunk, sizeof(chunk))) > 0) {
        for (i = 0; i < (size_t)rd; i++) {
            if (chunk[i] == '\n') {
                g_line_buf[g_line_len] = '\0';
                sim_auto_exec(g_line_buf);
                g_line_len = 0;
            } else if (g_line_len + 1 < sizeof(g_line_buf)) {
                g_line_buf[g_line_len++] = chunk[i];
            }
        }
    }
}

void sim_auto_deinit(void)
{
    if (g_fifo_fd >= 0) {
        close(g_fifo_fd);
        g_fifo_fd = -1;
    }
}

#endif /* defined(FB_RUN) && !FB_RUN */
