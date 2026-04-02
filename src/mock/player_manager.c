#include "core/player_manager.h"

#include "mlog.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define PLAYER_MANAGER_OK 0
#define PLAYER_MANAGER_EINVAL (-1)

typedef struct {
    bool inited;
    bool prepared;
    bool paused;
    int total_sec;
    int current_sec;
    uint64_t last_tick_ms;
} player_manager_ctx_t;

static player_manager_ctx_t g_player_ctx = {
    .inited = false,
    .prepared = false,
    .paused = true,
    .total_sec = 180,
    .current_sec = 0,
    .last_tick_ms = 0,
};

static uint64_t player_manager_now_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

static void player_manager_update_progress_locked(void)
{
    uint64_t now_ms;
    uint64_t delta_ms;
    int delta_sec;

    if (!g_player_ctx.prepared || g_player_ctx.paused) {
        return;
    }

    now_ms = player_manager_now_ms();
    if (g_player_ctx.last_tick_ms == 0) {
        g_player_ctx.last_tick_ms = now_ms;
        return;
    }

    delta_ms = now_ms - g_player_ctx.last_tick_ms;
    if (delta_ms < 1000) {
        return;
    }

    delta_sec = (int)(delta_ms / 1000);
    g_player_ctx.last_tick_ms += (uint64_t)delta_sec * 1000ULL;
    g_player_ctx.current_sec += delta_sec;

    if (g_player_ctx.current_sec >= g_player_ctx.total_sec) {
        g_player_ctx.current_sec = g_player_ctx.total_sec;
        g_player_ctx.paused = true;
    }
}

int player_manager_init(void)
{
    g_player_ctx.inited = true;
    g_player_ctx.prepared = false;
    g_player_ctx.paused = true;
    g_player_ctx.total_sec = 180;
    g_player_ctx.current_sec = 0;
    g_player_ctx.last_tick_ms = 0;
    MLOG_DBG("mock player_manager_init ok");
    return PLAYER_MANAGER_OK;
}

void player_manager_deinit(void)
{
    MLOG_DBG("mock player_manager_deinit begin");
    g_player_ctx.inited = false;
    g_player_ctx.prepared = false;
    g_player_ctx.paused = true;
    g_player_ctx.total_sec = 180;
    g_player_ctx.current_sec = 0;
    g_player_ctx.last_tick_ms = 0;
    MLOG_DBG("mock player_manager_deinit ok");
}

int player_manager_prepare(const char* video_path)
{
    if (!g_player_ctx.inited || !video_path || video_path[0] == '\0') {
        return PLAYER_MANAGER_EINVAL;
    }

    MLOG_DBG("mock player_manager_prepare request: path=%s", video_path);

    g_player_ctx.prepared = true;
    g_player_ctx.paused = true;
    g_player_ctx.current_sec = 0;
    g_player_ctx.total_sec = 180;
    g_player_ctx.last_tick_ms = player_manager_now_ms();

    MLOG_DBG("mock player_manager_prepare ok: path=%s total_sec=%d", video_path, g_player_ctx.total_sec);
    return PLAYER_MANAGER_OK;
}

int player_manager_play(void)
{
    if (!g_player_ctx.inited || !g_player_ctx.prepared) {
        return PLAYER_MANAGER_EINVAL;
    }

    MLOG_DBG("mock player_manager_play request: current_sec=%d paused=%d",
        g_player_ctx.current_sec,
        g_player_ctx.paused ? 1 : 0);
    g_player_ctx.paused = false;
    g_player_ctx.last_tick_ms = player_manager_now_ms();
    MLOG_DBG("mock player_manager_play ok");
    return PLAYER_MANAGER_OK;
}

int player_manager_pause(void)
{
    if (!g_player_ctx.inited || !g_player_ctx.prepared) {
        return PLAYER_MANAGER_EINVAL;
    }

    MLOG_DBG("mock player_manager_pause request: current_sec=%d paused=%d",
        g_player_ctx.current_sec,
        g_player_ctx.paused ? 1 : 0);
    player_manager_update_progress_locked();
    g_player_ctx.paused = true;
    MLOG_DBG("mock player_manager_pause ok: current_sec=%d", g_player_ctx.current_sec);
    return PLAYER_MANAGER_OK;
}

int player_manager_stop(void)
{
    if (!g_player_ctx.inited) {
        return PLAYER_MANAGER_EINVAL;
    }

    MLOG_DBG("mock player_manager_stop request: prepared=%d current_sec=%d",
        g_player_ctx.prepared ? 1 : 0,
        g_player_ctx.current_sec);
    g_player_ctx.prepared = false;
    g_player_ctx.paused = true;
    g_player_ctx.current_sec = 0;
    g_player_ctx.last_tick_ms = 0;
    MLOG_DBG("mock player_manager_stop ok");
    return PLAYER_MANAGER_OK;
}

int player_manager_seek_sec(int sec)
{
    if (!g_player_ctx.inited || !g_player_ctx.prepared) {
        return PLAYER_MANAGER_EINVAL;
    }

    if (sec < 0) {
        sec = 0;
    }

    if (g_player_ctx.total_sec > 0 && sec > g_player_ctx.total_sec) {
        sec = g_player_ctx.total_sec;
    }

    MLOG_DBG("mock player_manager_seek_sec request: sec=%d paused=%d", sec, g_player_ctx.paused ? 1 : 0);
    g_player_ctx.current_sec = sec;
    g_player_ctx.last_tick_ms = player_manager_now_ms();
    MLOG_DBG("mock player_manager_seek_sec ok: current_sec=%d", g_player_ctx.current_sec);
    return PLAYER_MANAGER_OK;
}

int player_manager_get_progress(int* current_sec, int* total_sec)
{
    if (!current_sec || !total_sec) {
        return PLAYER_MANAGER_EINVAL;
    }

    if (!g_player_ctx.inited || !g_player_ctx.prepared) {
        *current_sec = 0;
        *total_sec = 0;
        return PLAYER_MANAGER_EINVAL;
    }

    player_manager_update_progress_locked();

    *current_sec = g_player_ctx.current_sec;
    *total_sec = g_player_ctx.total_sec;
    return PLAYER_MANAGER_OK;
}

int player_manager_is_paused(int* out_paused)
{
    if (!out_paused || !g_player_ctx.inited) {
        return PLAYER_MANAGER_EINVAL;
    }

    *out_paused = g_player_ctx.paused ? 1 : 0;
    return PLAYER_MANAGER_OK;
}
