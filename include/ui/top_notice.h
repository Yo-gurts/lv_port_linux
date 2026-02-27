#ifndef __TOP_NOTICE_H__
#define __TOP_NOTICE_H__

#include "lvgl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TOP_NOTICE_TYPE_INFO = 0,
    TOP_NOTICE_TYPE_SUCCESS,
    TOP_NOTICE_TYPE_WARNING,
    TOP_NOTICE_TYPE_ERROR,
    TOP_NOTICE_TYPE_BLOCKING
} top_notice_type_t;

void top_notice_init(void);
void top_notice_show(const char* text, top_notice_type_t type);
void top_notice_show_for(const char* text, top_notice_type_t type, uint32_t duration_ms);
void top_notice_update(const char* text, top_notice_type_t type);
void top_notice_hide(void);

#ifdef __cplusplus
}
#endif

#endif /* __TOP_NOTICE_H__ */
