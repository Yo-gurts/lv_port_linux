#include "core/image_recognition_manager.h"

#include <stddef.h>

int image_recognition_manager_init(void)
{
    return 0;
}

void image_recognition_manager_deinit(void)
{
}

int image_recognition_manager_start(const char* input_real_path, const char* prompt)
{
    (void)input_real_path;
    (void)prompt;
    return -1;
}

image_recognition_state_t image_recognition_manager_get_state(void)
{
    return IMAGE_RECOGNITION_STATE_IDLE;
}

int image_recognition_manager_get_error(void)
{
    return -1;
}

int image_recognition_manager_get_result_text(char* out_text, size_t out_size)
{
    (void)out_text;
    (void)out_size;
    return -1;
}

void image_recognition_manager_reset(void)
{
}
