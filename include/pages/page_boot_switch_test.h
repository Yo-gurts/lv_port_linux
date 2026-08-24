#ifndef __PAGE_BOOT_SWITCH_TEST_H__
#define __PAGE_BOOT_SWITCH_TEST_H__

#include "core/page_manager.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PAGE_BOOT_SWITCH_TEST_OPTION_COUNT 5
#define PAGE_BOOT_SWITCH_TEST_ACTION_COUNT 3

/* 切换动作：两端模式来回切。收尾一律回 boot（回 boot 不计入次数）。 */
typedef enum {
    BST_ACTION_PHOTO_BOOT = 0, /* 拍照 <-> boot */
    BST_ACTION_VIDEO_BOOT, /* 录像 <-> boot */
    BST_ACTION_PHOTO_VIDEO, /* 拍照 <-> 录像（收尾回 boot） */
} bst_action_t;

typedef struct {
    lv_obj_t* container;
    lv_obj_t* title_label;
    lv_obj_t* action_btns[PAGE_BOOT_SWITCH_TEST_ACTION_COUNT];
    lv_obj_t* count_btns[PAGE_BOOT_SWITCH_TEST_OPTION_COUNT];
    lv_obj_t* start_btn;
    lv_obj_t* start_label;
    lv_obj_t* progress_label;

    bst_action_t action; /* 当前选中的切换动作，默认 BST_ACTION_PHOTO_BOOT */
    uint32_t target; /* 目标切换轮数；BST_TARGET_INFINITE 表无穷 */
    uint32_t done; /* 已完成轮数 */
    uint8_t phase; /* 0=下一步切 op_a(此刻在 boot/op_b); 1=下一步切 op_b(此刻在 op_a) */
    uint8_t running;
    uint8_t waiting; /* 1=有一个异步切换在途，等待完成回调 */
    uint8_t finishing; /* 1=正在发收尾回 boot 的半步（不计数） */
    uint8_t stop_pending; /* 1=已请求暂停，待一轮完成点收尾 */
    uint8_t back_pending; /* 1=收尾后还需退回上一页(运行中点返回) */
    uint8_t auto_sleep_disabled;
} page_boot_switch_test_data_t;

void page_boot_switch_test_create(void);
void page_boot_switch_test_destroy(void);
void page_boot_switch_test_show(void);
void page_boot_switch_test_hide(void);
void page_boot_switch_test_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_BOOT_SWITCH_TEST_H__ */
