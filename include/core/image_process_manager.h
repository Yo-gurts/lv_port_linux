#ifndef __IMAGE_PROCESS_MANAGER_H__
#define __IMAGE_PROCESS_MANAGER_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IMAGE_PROCESS_STATE_IDLE = 0,
    IMAGE_PROCESS_STATE_RUNNING,
    IMAGE_PROCESS_STATE_SUCCESS,
    IMAGE_PROCESS_STATE_FAILED,
} image_process_state_t;

int image_process_manager_init(void);
void image_process_manager_deinit(void);

int image_process_manager_start_style(const char* input_real_path, const char* prompt);
image_process_state_t image_process_manager_get_state(void);
int image_process_manager_get_error(void);
int image_process_manager_get_result_path(char* out_path, size_t out_size, int lv_path);

void image_process_manager_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __IMAGE_PROCESS_MANAGER_H__ */
