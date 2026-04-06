#ifndef __IMAGE_RECOGNITION_MANAGER_H__
#define __IMAGE_RECOGNITION_MANAGER_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IMAGE_RECOGNITION_STATE_IDLE = 0,
    IMAGE_RECOGNITION_STATE_RUNNING,
    IMAGE_RECOGNITION_STATE_SUCCESS,
    IMAGE_RECOGNITION_STATE_FAILED,
} image_recognition_state_t;

int image_recognition_manager_init(void);
void image_recognition_manager_deinit(void);

int image_recognition_manager_start(const char* input_real_path, const char* prompt);
image_recognition_state_t image_recognition_manager_get_state(void);
int image_recognition_manager_get_error(void);
int image_recognition_manager_get_result_text(char* out_text, size_t out_size);

void image_recognition_manager_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __IMAGE_RECOGNITION_MANAGER_H__ */
