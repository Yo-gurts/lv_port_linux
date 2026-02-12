#ifndef __PAGE_VERSION_INFO_H__
#define __PAGE_VERSION_INFO_H__

#include "core/page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* container; /* 页面容器 */
    lv_obj_t* nav_bar; /* 顶部导航栏 */
    lv_obj_t* info_container; /* 信息列表容器 */
    lv_obj_t* click_label; /* 点击标签，用于检测连点 */
    uint32_t last_click_time; /* 上次点击时间 */
    int click_count; /* 连点计数 */
} page_version_info_data_t;

void page_version_info_create(void);
void page_version_info_destroy(void);
void page_version_info_show(void);
void page_version_info_hide(void);
void page_version_info_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_VERSION_INFO_H__ */
