#include "core/sound_manager.h"

#include "config.h"
#include "media_init.h"
#include "mlog.h"
#include "param.h"
#include "volmng.h"

/* 音效索引表：与下面 g_voice_tab 的顺序一一对应。 */
typedef enum {
    VOICE_IDX_KEYTONE = 0,
    VOICE_IDX_PHOTO = 1,
    VOICE_IDX_REC = 2,
    VOICE_IDX_MAX
} sound_voice_idx_t;

#define VOICE_DIR PROJECT_PATH "/res/voice/"

static bool g_sound_inited = false;

static const VOICEPLAY_VOICETABLE_S g_voice_tab[VOICE_IDX_MAX] = {
    { VOICE_IDX_KEYTONE, VOICE_DIR "keytone.wav" },
    { VOICE_IDX_PHOTO, VOICE_DIR "photo.wav" },
    { VOICE_IDX_REC, VOICE_DIR "rec.wav" },
};

/* 取 AO 句柄；若底层尚未初始化则初始化一次。
 *
 * 现状：boot 期的 MEDIA_AoInit 链（ModuleAoStart）在 dashcam_main.c 中已被注释，
 * 而 player 服务只在进入回放模式时才惰性创建 AO。若不在这里补一次，按键音要等到
 * 用户第一次打开视频回放之后才有声。这里按需初始化，避免这个先后依赖。
 * 已初始化时 MAPI_AO_GetHandle 直接成功，不会重复 init。 */
static MAPI_AO_HANDLE_T sound_manager_ensure_ao(void)
{
    MAPI_AO_HANDLE_T hdl = NULL;

    if (MAPI_AO_GetHandle(&hdl) == 0 && hdl != NULL) {
        return hdl;
    }

    if (MEDIA_AoInit() != 0) {
        MLOG_WARN("sound_manager: MEDIA_AoInit failed");
        return NULL;
    }

    if (MAPI_AO_GetHandle(&hdl) != 0) {
        return NULL;
    }

    return hdl;
}

/* 「按键音」开关：取底层 MENU 配置（OFF/ON），默认开启。 */
bool sound_manager_keytone_enabled(void)
{
    int32_t value = MEDIA_AUDIO_KEYTONE_ON;

    if (PARAM_GetKeyTone(&value) != 0) {
        return true; /* 取不到时按开启处理，避免静默无声 */
    }

    return value != MEDIA_AUDIO_KEYTONE_OFF;
}

void sound_manager_set_keytone_enabled(bool enabled)
{
    (void)PARAM_SetMenuParam(0, PARAM_MENU_KEYTONE,
        enabled ? MEDIA_AUDIO_KEYTONE_ON : MEDIA_AUDIO_KEYTONE_OFF);
}

int sound_manager_init(void)
{
    VOICEPLAY_CFG_S cfg = { 0 };
    MAPI_AO_HANDLE_T ao_hdl;

    if (g_sound_inited) {
        return 0;
    }

    ao_hdl = sound_manager_ensure_ao();
    if (ao_hdl == NULL) {
        MLOG_ERR("sound_manager: ao not available, keytone disabled");
        return -1;
    }

    cfg.stAoutOpt.hAudDevHdl = ao_hdl;
    cfg.u32MaxVoiceCnt = VOICE_IDX_MAX;
    cfg.pstVoiceTab = (VOICEPLAY_VOICETABLE_S*)g_voice_tab;

    if (VOICEPLAY_Init(&cfg) != 0) {
        MLOG_ERR("sound_manager: VOICEPLAY_Init failed");
        return -1;
    }

    g_sound_inited = true;
    MLOG_INFO("sound_manager initialized (ao=%p)", ao_hdl);
    return 0;
}

void sound_manager_deinit(void)
{
    if (!g_sound_inited) {
        return;
    }

    (void)VOICEPLAY_DeInit();
    g_sound_inited = false;
}

void sound_manager_play(int voice_idx)
{
    VOICEPLAY_VOICE_S voice = {
        .u32VoiceCnt = 1,
        .au32VoiceIdx = { (uint32_t)voice_idx },
        .bDroppable = true,
    };

    if (!g_sound_inited) {
        return;
    }
    if (voice_idx < 0 || voice_idx >= VOICE_IDX_MAX) {
        return;
    }

    (void)VOICEPLAY_Push(&voice, 0);
}

void sound_manager_play_keytone(void)
{
    if (!sound_manager_keytone_enabled()) {
        return;
    }

    sound_manager_play(VOICE_IDX_KEYTONE);
}

int sound_manager_set_system_volume(int percent)
{
    /* 音量落在共享的 DAC 寄存器上，走 media 层统一入口；AO 未就绪时那边会按需初始化。 */
    return MEDIA_AoSetSystemVolume(percent);
}
