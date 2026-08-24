#ifndef __PAGE_KEY_TOUCH_TEST_H__
#define __PAGE_KEY_TOUCH_TEST_H__

#include "core/page_manager.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PAGE_KEY_TOUCH_TEST_KEY_COUNT 6
#define PAGE_KEY_TOUCH_TEST_MAX_TRACE_DOTS 512

typedef struct {
    uint8_t pressed_once;
    uint8_t pressing_now;
    lv_obj_t* card;
    lv_obj_t* name_label;
    lv_obj_t* state_label;
} page_key_touch_test_key_item_t;

typedef struct {
    lv_obj_t* container;
    lv_obj_t* title_label;
    lv_obj_t* summary_label;
    lv_obj_t* touch_status_label;
    lv_obj_t* touch_area;

    page_key_touch_test_key_item_t key_items[PAGE_KEY_TOUCH_TEST_KEY_COUNT];

    lv_obj_t* trace_dots[PAGE_KEY_TOUCH_TEST_MAX_TRACE_DOTS];
    uint16_t trace_dot_count;
    uint8_t touch_tracking;
    uint8_t touch_passed;
    uint8_t diag_down_passed;
    uint8_t diag_up_passed;
    uint8_t key_callbacks_registered;
    uint8_t auto_sleep_disabled;
    lv_point_t last_touch_local;
    uint32_t touch_move_accum;
} page_key_touch_test_data_t;

void page_key_touch_test_create(void);
void page_key_touch_test_destroy(void);
void page_key_touch_test_show(void);
void page_key_touch_test_hide(void);
void page_key_touch_test_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_KEY_TOUCH_TEST_H__ */
