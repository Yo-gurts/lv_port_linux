#ifndef __MEDIA_GRID_COMMON_H__
#define __MEDIA_GRID_COMMON_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int media_grid_clamp_int(int value, int min_value, int max_value);
int media_grid_get_total_rows(int total_count, int cols);
int media_grid_get_last_visible_index_1based(int first_visible_row, int visible_rows, int total_count, int cols);
bool media_grid_ensure_selected_buffer(bool** selected_flags, int* selected_capacity, int required_count);
void media_grid_clear_selection_state(bool* selected_flags, int selected_capacity, int* selected_count);

#ifdef __cplusplus
}
#endif

#endif /* __MEDIA_GRID_COMMON_H__ */
