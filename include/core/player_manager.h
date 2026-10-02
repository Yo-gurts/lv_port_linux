#ifndef __PLAYER_MANAGER_H__
#define __PLAYER_MANAGER_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int player_manager_init(void);
void player_manager_deinit(void);

int player_manager_prepare(const char* video_path);
int player_manager_play(void);
int player_manager_pause(void);
int player_manager_stop(void);
int player_manager_seek_sec(int sec);

int player_manager_get_progress(int* current_sec, int* total_sec);
int player_manager_is_paused(int* out_paused);

/* 播放服务是否仍然存活。退出回放模式后播放服务会被销毁，届时为 false；
 * UI 侧的定时器等异步入口在动播放器前应先探活。 */
bool player_manager_is_active(void);

#ifdef __cplusplus
}
#endif

#endif /* __PLAYER_MANAGER_H__ */
