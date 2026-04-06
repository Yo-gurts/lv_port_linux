#ifndef __FILTER_PANEL_H__
#define __FILTER_PANEL_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FILTER_PANEL_MAX_COUNT 32

void filter_panel_init(void);
void filter_panel_show(void);
void filter_panel_hide(void);
int filter_panel_is_visible(void);

#ifdef __cplusplus
}
#endif

#endif /* __FILTER_PANEL_H__ */
