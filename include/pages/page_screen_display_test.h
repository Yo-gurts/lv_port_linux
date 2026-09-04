#ifndef __PAGE_SCREEN_DISPLAY_TEST_H__
#define __PAGE_SCREEN_DISPLAY_TEST_H__

#include "core/page_manager.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PAGE_SCREEN_DISPLAY_TEST_COLOR_COUNT 7
#define SCREEN_DISPLAY_TEST_INTERVAL_MS 100 /* 每种颜色显示时长 */

typedef struct {
    lv_obj_t* container;
    lv_obj_t* title_label;
    lv_obj_t* start_btn; /* 右下角：开始/停止，运行中常驻色块之上可点击 */
    lv_obj_t* start_label;
    lv_obj_t* count_label; /* 底部居中：切换次数显示 */
    lv_obj_t* color_view; /* 运行中纯色视图，填充顶栏以下区域 */
    lv_timer_t* timer; /* 100ms 换色定时器 */
    uint8_t running;
    uint8_t color_idx; /* 当前颜色在颜色表中的下标 */
    uint64_t switch_count; /* 已切换颜色次数 */
    uint8_t auto_sleep_disabled;
} page_screen_display_test_data_t;

void page_screen_display_test_create(void);
void page_screen_display_test_destroy(void);
void page_screen_display_test_show(void);
void page_screen_display_test_hide(void);
void page_screen_display_test_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_SCREEN_DISPLAY_TEST_H__ */
