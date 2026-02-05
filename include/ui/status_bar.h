#ifndef __STATUS_BAR_H__
#define __STATUS_BAR_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* parent;
    lv_obj_t* bar_obj;
    int height;
    lv_color_t bg_color;
    lv_color_t text_color;
    /* Status widgets */
    lv_obj_t* time_label;
    lv_obj_t* wifi_icon;
    lv_obj_t* battery_icon;
} status_bar_t;

status_bar_t* status_bar_create(lv_obj_t* parent);
void status_bar_destroy(status_bar_t* sb);

void status_bar_set_height(status_bar_t* sb, int height);
int status_bar_get_height(status_bar_t* sb);

void status_bar_set_bg_color(status_bar_t* sb, lv_color_t color);
void status_bar_set_text_color(status_bar_t* sb, lv_color_t color);

void status_bar_set_time(status_bar_t* sb, const char* time_str);
void status_bar_set_wifi_icon(status_bar_t* sb, int level);
void status_bar_set_battery_icon(status_bar_t* sb, int level);

#ifdef __cplusplus
}
#endif

#endif
