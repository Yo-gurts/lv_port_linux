#ifndef __STATUS_BAR_H__
#define __STATUS_BAR_H__

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STATUS_BAR_ITEM_TYPE_NONE = 0,
    STATUS_BAR_ITEM_TYPE_TEXT,
    STATUS_BAR_ITEM_TYPE_ICON,
    STATUS_BAR_ITEM_TYPE_PROGRESS,
    STATUS_BAR_ITEM_TYPE_MAX
} status_bar_item_type_t;

typedef struct {
    const char* text;
    const void* icon_data;
    status_bar_item_type_t type;
    int progress;
    bool visible;
    int width;
} status_bar_item_t;

typedef struct {
    lv_obj_t* parent;
    status_bar_item_t items[10];
    int item_count;
    lv_obj_t* bar_obj;
    lv_obj_t* left_container;
    lv_obj_t* right_container;
    lv_obj_t* item_widgets[10];
    int height;
    lv_color_t bg_color;
    lv_color_t text_color;
} status_bar_t;

status_bar_t* status_bar_create(lv_obj_t* parent);
void status_bar_destroy(status_bar_t* sb);

int status_bar_add_item(status_bar_t* sb, status_bar_item_t* item);
int status_bar_remove_item(status_bar_t* sb, int index);
void status_bar_clear(status_bar_t* sb);

void status_bar_set_item_text(status_bar_t* sb, int index, const char* text);
void status_bar_set_item_progress(status_bar_t* sb, int index, int progress);
void status_bar_set_item_visible(status_bar_t* sb, int index, bool visible);

void status_bar_set_height(status_bar_t* sb, int height);
int status_bar_get_height(status_bar_t* sb);

void status_bar_set_bg_color(status_bar_t* sb, lv_color_t color);
void status_bar_set_text_color(status_bar_t* sb, lv_color_t color);

void status_bar_refresh(status_bar_t* sb);

#ifdef __cplusplus
}
#endif

#endif
