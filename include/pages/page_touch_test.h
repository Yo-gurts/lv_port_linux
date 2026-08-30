#ifndef __PAGE_TOUCH_TEST_H__
#define __PAGE_TOUCH_TEST_H__

#include "core/page_manager.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PAGE_TOUCH_TEST_MAX_TRACE_DOTS 512

typedef struct {
    lv_obj_t* container;
    lv_obj_t* title_label;
    lv_obj_t* summary_label;
    lv_obj_t* touch_area;
    lv_obj_t* hint; /* 分步提示文案 */

    lv_obj_t* trace_dots[PAGE_TOUCH_TEST_MAX_TRACE_DOTS];
    uint16_t trace_dot_count;

    /* 每条对角线引导组各包成一个透明 container，整体显隐：
     * 组内含 中线实线点 + 两侧 ±BAND 边界虚线 + 中点方向箭头。
     * 左对角(↘)默认显示、进 stage1 隐藏；右对角(↗)默认隐藏、进 stage1 显示。 */
    lv_obj_t* guide_down_grp;
    lv_obj_t* guide_up_grp;

    uint8_t touch_tracking;
    uint8_t touch_passed;
    uint8_t stage; /* 0=等左对角线(↘) 1=左过等右对角线(↗) 2=全部通过 */
    uint8_t stroke_in_band; /* 本笔至今是否全程在当前目标线容差带内 */
    uint8_t stroke_start_ok; /* 本笔起点是否落在当前目标线的起角距端区 */
    uint8_t auto_sleep_disabled;
    uint8_t menu_key_registered; /* MENU 键返回回调是否已注册 */
    lv_point_t last_touch_local;
} page_touch_test_data_t;

void page_touch_test_create(void);
void page_touch_test_destroy(void);
void page_touch_test_show(void);
void page_touch_test_hide(void);
void page_touch_test_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_TOUCH_TEST_H__ */
