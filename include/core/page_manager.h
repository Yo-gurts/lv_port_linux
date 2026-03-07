#ifndef __PAGE_MANAGER_H__
#define __PAGE_MANAGER_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct page_manager_t page_manager_t;

typedef struct {
    const char* name;
    void (*create)(void);
    void (*destroy)(void);
    void (*show)(void);
    void (*hide)(void);
    void (*update)(void);
} page_interface_t;

typedef struct {
    const char* name;
    lv_obj_t* screen;
    page_interface_t* interface;
    void* private_data;
    int is_created; /* 页面是否已创建 */
} page_t;

int page_manager_create(void);
void page_manager_destroy(void);
int page_manager_register(const char* name, page_interface_t* interface, void* user_data);
int page_manager_navigate(const char* page_name);
int page_manager_back(void);
const char* page_manager_get_current(void);

lv_obj_t* page_get_root(void);
void* page_get_private_data(void);
void page_set_private_data(void* data);

/* 通用事件回调函数 - 所有页面可直接作为事件回调使用 */
void page_manager_back_cb(lv_event_t* e);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_MANAGER_H__ */
