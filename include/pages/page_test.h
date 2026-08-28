#ifndef __PAGE_TEST_H__
#define __PAGE_TEST_H__

#include "core/page_manager.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PAGE_TEST_ITEM_COUNT 3

typedef struct {
    lv_obj_t* container;
    lv_obj_t* title_label;
    lv_obj_t* items[PAGE_TEST_ITEM_COUNT]; /* 测试项列表，下标即选中序号 */
    int selected_index; /* 上下键当前选中项 */
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
