#ifndef __PAGE_LOOP_VTEST_H__
#define __PAGE_LOOP_VTEST_H__

#include "core/page_manager.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PAGE_LOOP_VTEST_OPTION_COUNT 3 /* 次数档：1/100/无穷 */
#define PAGE_LOOP_VTEST_VIDEO_RES_COUNT 4 /* 录像分辨率档：4K/2.7K/1080P/720P */

typedef struct {
    lv_obj_t* container;
    lv_obj_t* title_label;
    lv_obj_t* count_btn; /* 左下角：点一下循环切下一个次数 */
    lv_obj_t* count_label;
    lv_obj_t* start_btn; /* 右下角：开始/停止 */
    lv_obj_t* start_label;
    lv_obj_t* progress_label;

    lv_timer_t* rec_timer; /* 录像倒计时定时器：每 1s 一跳刷新 notice 剩余秒，到点触发停止录像 */
    uint8_t rec_left_s; /* 录像剩余秒数（倒计时用） */

    uint8_t count_idx; /* 当前次数在 g_lvt_options 中的下标 */
    uint32_t target; /* 目标轮数；LVT_TARGET_INFINITE 表无穷 */
    uint32_t done; /* 已完成轮数（每跑完一整轮 +1） */
    uint8_t step; /* 当前所处循环步（见 .c 的 lvt_step_t） */
    uint8_t running;
    uint8_t waiting; /* 1=有一个异步操作在途，等待完成回调 */
    uint8_t stop_pending; /* 1=已请求停止，待本轮跑完收尾 */
    uint8_t back_pending; /* 1=收尾后还需退回上一页（运行中点返回） */
    uint8_t auto_sleep_disabled;
} page_loop_vtest_data_t;

void page_loop_vtest_create(void);
void page_loop_vtest_destroy(void);
void page_loop_vtest_show(void);
void page_loop_vtest_hide(void);
void page_loop_vtest_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_LOOP_VTEST_H__ */
