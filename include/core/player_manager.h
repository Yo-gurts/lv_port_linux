#ifndef __PLAYER_MANAGER_H__
#define __PLAYER_MANAGER_H__

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

#ifdef __cplusplus
}
#endif

#endif /* __PLAYER_MANAGER_H__ */
