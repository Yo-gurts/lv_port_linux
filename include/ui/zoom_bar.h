#ifndef __ZOOM_BAR_H__
#define __ZOOM_BAR_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void zoom_bar_init(void);
void zoom_bar_show(void);
void zoom_bar_hide(void);
int zoom_bar_is_visible(void);
void zoom_bar_set_value(int zoom_value);
void zoom_bar_zoom_in(void);
void zoom_bar_zoom_out(void);

#ifdef __cplusplus
}
#endif

#endif /* __ZOOM_BAR_H__ */
