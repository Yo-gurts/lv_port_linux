#ifndef __PAGE_BOOT_SWITCH_TEST_H__
#define __PAGE_BOOT_SWITCH_TEST_H__

#include "core/page_manager.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PAGE_BOOT_SWITCH_TEST_OPTION_COUNT 4

typedef struct {
    lv_obj_t* container;
    lv_obj_t* title_label;
    lv_obj_t* count_btns[PAGE_BOOT_SWITCH_TEST_OPTION_COUNT];
    lv_obj_t* start_btn;
    lv_obj_t* start_label;
    lv_obj_t* progress_label;

    uint32_t target; /* 目标切换轮数 */
    uint32_t done; /* 已完成轮数 */
    uint8_t phase; /* 0=下一步切拍照(此刻在 boot); 1=下一步切 boot(此刻在拍照) */
    uint8_t running;
    uint8_t waiting; /* 1=有一个异步切换在途，等待完成回调 */
    uint8_t stop_pending; /* 1=已请求暂停，待下一个 boot 完成点收尾 */
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
