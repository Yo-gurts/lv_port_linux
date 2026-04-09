#include "pages/media_grid_common.h"
#include <stdlib.h>
#include <string.h>

int media_grid_clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value)
        return min_value;
    if (value > max_value)
        return max_value;
    return value;
}

int media_grid_get_total_rows(int total_count, int cols)
{
    if (total_count <= 0 || cols <= 0)
        return 0;

    return (total_count + cols - 1) / cols;
}

int media_grid_get_last_visible_index_1based(int first_visible_row, int visible_rows, int total_count, int cols)
{
    int last_row;
    int last_index;

    if (total_count <= 0 || cols <= 0 || visible_rows <= 0)
        return 0;

    last_row = first_visible_row + visible_rows - 1;
    if (last_row < 0)
        last_row = 0;
    last_index = (last_row + 1) * cols;
    if (last_index > total_count)
        last_index = total_count;
    return last_index;
}

bool media_grid_ensure_selected_buffer(bool** selected_flags, int* selected_capacity, int required_count)
{
    bool* new_flags;
    int new_capacity;

    if (selected_flags == NULL || selected_capacity == NULL)
        return false;

    if (required_count <= 0)
        required_count = 1;

    if (*selected_capacity >= required_count)
        return true;

    new_capacity = *selected_capacity > 0 ? *selected_capacity : 16;
    while (new_capacity < required_count)
        new_capacity *= 2;

    new_flags = (bool*)realloc(*selected_flags, (size_t)new_capacity * sizeof(bool));
    if (!new_flags)
        return false;

    memset(new_flags + *selected_capacity, 0, (size_t)(new_capacity - *selected_capacity) * sizeof(bool));
    *selected_flags = new_flags;
    *selected_capacity = new_capacity;
    return true;
}

void media_grid_clear_selection_state(bool* selected_flags, int selected_capacity, int* selected_count)
{
    if (selected_flags != NULL && selected_capacity > 0)
        memset(selected_flags, 0, (size_t)selected_capacity * sizeof(bool));
    if (selected_count != NULL)
        *selected_count = 0;
}
