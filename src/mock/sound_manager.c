#include "core/sound_manager.h"

#include "mlog.h"

/* SDL 仿真用的按键音空实现。
 * 板级（FB）走 src/core/sound_manager.c + VOICEPLAY；仿真环境没有
 * AO/codec 与 SDK 头，故此处只保留接口，行为为空。 */

int sound_manager_init(void)
{
    MLOG_INFO("sound_manager (mock) init");
    return 0;
}

void sound_manager_deinit(void)
{
}

void sound_manager_play(int voice_idx)
{
    (void)voice_idx;
}

void sound_manager_play_keytone(void)
{
}

bool sound_manager_keytone_enabled(void)
{
    return true;
}

void sound_manager_set_keytone_enabled(bool enabled)
{
    (void)enabled;
}
