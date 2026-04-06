#include "core/image_process_manager.h"

#include <stddef.h>

int image_process_manager_init(void)
{
    return 0;
}

void image_process_manager_deinit(void)
{
}

int image_process_manager_start_style(const char* input_real_path, const char* prompt)
{
    (void)input_real_path;
    (void)prompt;
    return -1;
}

image_process_state_t image_process_manager_get_state(void)
{
    return IMAGE_PROCESS_STATE_IDLE;
}

int image_process_manager_get_error(void)
{
    return -1;
}

int image_process_manager_get_result_path(char* out_path, size_t out_size, int lv_path)
{
    (void)out_path;
    (void)out_size;
    (void)lv_path;
    return -1;
}

void image_process_manager_reset(void)
{
}
