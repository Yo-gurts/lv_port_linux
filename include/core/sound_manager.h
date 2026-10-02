#ifndef __SOUND_MANAGER_H__
#define __SOUND_MANAGER_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 按键音/提示音管理（UI 侧）。
 *
 * 背景：VOICEPLAY（applications/manager/src/vol/volmng.c）的按键音引擎此前
 * 因旧车载 UI 移除而失去全部调用方，res/voice 下的 wav 资产仍在板上但无人播放。
 * 本模块把它重新接进 UI 的按键事件流。
 *
 * 通道分配：按键音走 AO chn 1，视频播放音走 chn 0，由 AO 播放线程求和混音，
 * 因此按键音与视频音可同时出声（互不打断）。 */

/* 初始化：注册音效表并启动 VOICEPLAY 线程。UI 启动时调一次。 */
int sound_manager_init(void);
void sound_manager_deinit(void);

/* 播放按键音（受「按键音」开关控制，内部 bDroppable=true：
 * 连续按键时打断上一条，不排队积压）。 */
void sound_manager_play_keytone(void);

/* 播放指定音效（不受按键音开关限制，用于拍照/录像等提示音）。 */
void sound_manager_play(int voice_idx);

/* 读取/设置「按键音」开关（底层 MENU 配置）。
 * 由本模块封装，避免 UI 页面直接依赖 SDK 的 param.h。 */
bool sound_manager_keytone_enabled(void);
void sound_manager_set_keytone_enabled(bool enabled);

/* 设置系统音量（0~100），下发到 DAC。音量按 DAC 声道共享，
 * 按键音与视频播放音会一起变化。 */
int sound_manager_set_system_volume(int percent);

#ifdef __cplusplus
}
#endif

#endif /* __SOUND_MANAGER_H__ */
