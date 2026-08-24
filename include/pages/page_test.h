#ifndef __PAGE_TEST_H__
#define __PAGE_TEST_H__

#include "core/page_manager.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* container;
    lv_obj_t* title_label;
    lv_obj_t* key_touch_item;
    lv_obj_t* boot_switch_item;
} page_test_data_t;

void page_test_create(void);
void page_test_destroy(void);
void page_test_show(void);
void page_test_hide(void);
void page_test_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_TEST_H__ */
