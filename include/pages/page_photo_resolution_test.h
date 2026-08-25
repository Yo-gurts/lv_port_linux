#ifndef __PAGE_PHOTO_RESOLUTION_TEST_H__
#define __PAGE_PHOTO_RESOLUTION_TEST_H__

#include "core/page_manager.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PAGE_PHOTO_RES_TEST_OPTION_COUNT 4 /* 次数档：100/1000/10000/无穷 */
#define PAGE_PHOTO_RES_RESOLUTION_COUNT 5 /* 拍照分辨率档：8M/12M/24M/48M/64M */

typedef struct {
    lv_obj_t* container;
    lv_obj_t* title_label;
    lv_obj_t* count_btn; /* 左下角：点一下循环切下一个次数 */
    lv_obj_t* count_label;
    lv_obj_t* start_btn; /* 右下角：开始/暂停 */
    lv_obj_t* start_label;
    lv_obj_t* progress_label;

    lv_timer_t* step_timer; /* 两次分辨率切换之间的 500ms 单次延时定时器 */

    uint8_t count_idx; /* 当前次数在 g_prt_options 中的下标 */
    uint32_t target; /* 目标切换次数；PRT_TARGET_INFINITE 表无穷 */
    uint32_t done; /* 已完成切换次数（每切一档 +1） */
    uint8_t res_idx; /* 下一个要切到的分辨率档位下标 0..4 */
    uint8_t running;
    uint8_t waiting; /* 1=有一个异步操作在途，等待完成回调 */
    uint8_t entering; /* 1=正在异步进拍照模式（尚未开始切分辨率） */
    uint8_t finishing; /* 1=正在异步切回 boot 收尾；2=收尾且保留满进度 */
    uint8_t stop_pending; /* 1=已请求暂停，待一次切换完成点收尾 */
    uint8_t back_pending; /* 1=收尾后还需退回上一页(运行中点返回) */
    uint8_t auto_sleep_disabled;
} page_photo_resolution_test_data_t;

void page_photo_resolution_test_create(void);
void page_photo_resolution_test_destroy(void);
void page_photo_resolution_test_show(void);
void page_photo_resolution_test_hide(void);
void page_photo_resolution_test_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_PHOTO_RESOLUTION_TEST_H__ */
