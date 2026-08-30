#ifndef __PAGE_KEY_TEST_H__
#define __PAGE_KEY_TEST_H__

#include "core/page_manager.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PAGE_KEY_TEST_KEY_COUNT 13

typedef struct {
    uint8_t pressed_once;
    uint8_t pressing_now;
    lv_obj_t* btn;
} page_key_test_key_item_t;

typedef struct {
    lv_obj_t* container;
    lv_obj_t* title_label;
    lv_obj_t* summary_label;

    page_key_test_key_item_t key_items[PAGE_KEY_TEST_KEY_COUNT];

    uint8_t key_callbacks_registered;
    uint8_t auto_sleep_disabled;
} page_key_test_data_t;

void page_key_test_create(void);
void page_key_test_destroy(void);
void page_key_test_show(void);
void page_key_test_hide(void);
void page_key_test_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_KEY_TEST_H__ */
