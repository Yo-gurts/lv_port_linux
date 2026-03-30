#include "core/player_manager.h"

#include "media_init.h"
#include "mlog.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define PLAYER_MANAGER_OK 0
#define PLAYER_MANAGER_EINVAL (-1)
#define PLAYER_MANAGER_ESTATE (-2)

typedef struct {
    bool inited;
    bool prepared;
    bool paused;
    int total_sec;
} player_manager_ctx_t;

static player_manager_ctx_t g_player_ctx = {
    .inited = false,
    .prepared = false,
    .paused = true,
    .total_sec = 0,
};

static PLAYER_SERVICE_HANDLE_T player_manager_get_handle(void)
{
    MEDIA_PARAM_INIT_S* media = MEDIA_GetCtx();
    if (!media) {
        return NULL;
    }
    return media->SysServices.PsHdl;
}

static const char* player_manager_to_real_path(const char* path)
{
    if (path && path[0] == 'A' && path[1] == ':') {
        return path + 2;
    }
    return path;
}

static int player_manager_update_total_duration(void)
{
    PLAYER_MEDIA_INFO_S info;
    PLAYER_SERVICE_HANDLE_T handle = player_manager_get_handle();

    if (!handle) {
        return PLAYER_MANAGER_ESTATE;
    }

    memset(&info, 0, sizeof(info));
    if (PLAYER_SERVICE_GetMediaInfo(handle, &info) != 0) {
        return PLAYER_MANAGER_ESTATE;
    }

    if (info.duration_sec > 0.0) {
        g_player_ctx.total_sec = (int)(info.duration_sec + 0.5);
    }

    return PLAYER_MANAGER_OK;
}

int player_manager_init(void)
{
    g_player_ctx.inited = true;
    g_player_ctx.prepared = false;
    g_player_ctx.paused = true;
    g_player_ctx.total_sec = 0;
    return PLAYER_MANAGER_OK;
}

void player_manager_deinit(void)
{
    (void)player_manager_stop();
    g_player_ctx.inited = false;
}

int player_manager_prepare(const char* video_path)
{
    int32_t ret;
    const char* real_path;
    PLAYER_SERVICE_HANDLE_T handle;
    MEDIA_PARAM_INIT_S* media;

    if (!g_player_ctx.inited || !video_path || video_path[0] == '\0') {
        return PLAYER_MANAGER_EINVAL;
    }

    handle = player_manager_get_handle();
    media = MEDIA_GetCtx();
    if (!handle || !media) {
        MLOG_ERR("player_manager_prepare failed: player handle is null");
        return PLAYER_MANAGER_ESTATE;
    }

    real_path = player_manager_to_real_path(video_path);
    g_player_ctx.total_sec = 0;

    MAPI_AO_SetAmplifier(media->SysHandle.aohdl, CVI_FALSE);
    MAPI_AO_Mute(media->SysHandle.aohdl);

#ifdef SERVICES_PLAYER_SUBVIDEO
    PLAYER_SERVICE_SetPlaySubStreamFlag(handle, true);
#endif

    ret = PLAYER_SERVICE_SetInput(handle, real_path);
    if (ret != 0) {
        MLOG_ERR("player_manager_prepare set input failed: %s ret=%d", real_path, (int)ret);
        MAPI_AO_Unmute(media->SysHandle.aohdl);
        MAPI_AO_SetAmplifier(media->SysHandle.aohdl, CVI_TRUE);
        g_player_ctx.prepared = false;
        g_player_ctx.paused = true;
        return PLAYER_MANAGER_ESTATE;
    }

    ret = PLAYER_SERVICE_Play(handle);
    if (ret != 0) {
        MLOG_ERR("player_manager_prepare play failed: %s ret=%d", real_path, (int)ret);
        MAPI_AO_Unmute(media->SysHandle.aohdl);
        MAPI_AO_SetAmplifier(media->SysHandle.aohdl, CVI_TRUE);
        g_player_ctx.prepared = false;
        g_player_ctx.paused = true;
        return PLAYER_MANAGER_ESTATE;
    }

    ret = PLAYER_SERVICE_TouchSeekPause(handle, 0);
    if (ret != 0) {
        MLOG_WARN("player_manager_prepare touch seek pause failed: ret=%d", (int)ret);
        (void)PLAYER_SERVICE_Pause(handle);
    }

    (void)player_manager_update_total_duration();

    g_player_ctx.prepared = true;
    g_player_ctx.paused = true;
    return PLAYER_MANAGER_OK;
}

int player_manager_play(void)
{
    int32_t ret;
    PLAYER_SERVICE_HANDLE_T handle;
    MEDIA_PARAM_INIT_S* media;

    if (!g_player_ctx.inited || !g_player_ctx.prepared) {
        return PLAYER_MANAGER_EINVAL;
    }

    handle = player_manager_get_handle();
    media = MEDIA_GetCtx();
    if (!handle || !media) {
        return PLAYER_MANAGER_ESTATE;
    }

    MAPI_AO_Unmute(media->SysHandle.aohdl);
    MAPI_AO_SetAmplifier(media->SysHandle.aohdl, CVI_TRUE);

    ret = PLAYER_SERVICE_Play(handle);
    if (ret != 0) {
        MLOG_ERR("player_manager_play failed: ret=%d", (int)ret);
        return PLAYER_MANAGER_ESTATE;
    }

    g_player_ctx.paused = false;
    return PLAYER_MANAGER_OK;
}

int player_manager_pause(void)
{
    int32_t ret;
    PLAYER_SERVICE_HANDLE_T handle;

    if (!g_player_ctx.inited || !g_player_ctx.prepared) {
        return PLAYER_MANAGER_EINVAL;
    }

    handle = player_manager_get_handle();
    if (!handle) {
        return PLAYER_MANAGER_ESTATE;
    }

    ret = PLAYER_SERVICE_Pause(handle);
    if (ret != 0) {
        MLOG_ERR("player_manager_pause failed: ret=%d", (int)ret);
        return PLAYER_MANAGER_ESTATE;
    }

    g_player_ctx.paused = true;
    return PLAYER_MANAGER_OK;
}

int player_manager_stop(void)
{
    PLAYER_SERVICE_HANDLE_T handle;

    if (!g_player_ctx.inited) {
        return PLAYER_MANAGER_EINVAL;
    }

    handle = player_manager_get_handle();
    if (handle) {
        (void)PLAYER_SERVICE_Stop(handle);
    }

    g_player_ctx.prepared = false;
    g_player_ctx.paused = true;
    g_player_ctx.total_sec = 0;
    return PLAYER_MANAGER_OK;
}

int player_manager_seek_sec(int sec)
{
    int32_t ret;
    int64_t seek_ms;
    PLAYER_SERVICE_HANDLE_T handle;

    if (!g_player_ctx.inited || !g_player_ctx.prepared) {
        return PLAYER_MANAGER_EINVAL;
    }

    if (sec < 0) {
        sec = 0;
    }
    seek_ms = (int64_t)sec * 1000;

    handle = player_manager_get_handle();
    if (!handle) {
        return PLAYER_MANAGER_ESTATE;
    }

    if (g_player_ctx.paused) {
        ret = PLAYER_SERVICE_TouchSeekPause(handle, seek_ms);
    } else {
        ret = PLAYER_SERVICE_Seek(handle, seek_ms);
    }

    if (ret != 0) {
        MLOG_ERR("player_manager_seek_sec failed: sec=%d ret=%d", sec, (int)ret);
        return PLAYER_MANAGER_ESTATE;
    }

    return PLAYER_MANAGER_OK;
}

int player_manager_get_progress(int* current_sec, int* total_sec)
{
    int seek_ms;

    if (!current_sec || !total_sec) {
        return PLAYER_MANAGER_EINVAL;
    }

    if (!g_player_ctx.inited || !g_player_ctx.prepared) {
        *current_sec = 0;
        *total_sec = 0;
        return PLAYER_MANAGER_EINVAL;
    }

    if (g_player_ctx.total_sec <= 0) {
        (void)player_manager_update_total_duration();
    }

    seek_ms = PLAYER_SERVICE_SeekTime();
    if (seek_ms < 0) {
        seek_ms = 0;
    }

    *current_sec = seek_ms / 1000;
    *total_sec = g_player_ctx.total_sec;

    if (*total_sec > 0 && *current_sec > *total_sec) {
        *current_sec = *total_sec;
    }

    return PLAYER_MANAGER_OK;
}

int player_manager_is_paused(int* out_paused)
{
    if (!out_paused) {
        return PLAYER_MANAGER_EINVAL;
    }
    if (!g_player_ctx.inited) {
        return PLAYER_MANAGER_EINVAL;
    }

    *out_paused = g_player_ctx.paused ? 1 : 0;
    return PLAYER_MANAGER_OK;
}
