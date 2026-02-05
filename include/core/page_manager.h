#ifndef __PAGE_MANAGER_H__
#define __PAGE_MANAGER_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct page_manager_t page_manager_t;

typedef struct {
    const char* name;
    void (*create)(page_manager_t* pm);
    void (*destroy)(page_manager_t* pm);
    void (*show)(page_manager_t* pm);
    void (*hide)(page_manager_t* pm);
    void (*update)(page_manager_t* pm);
} page_interface_t;

typedef struct {
    const char* name;
    lv_obj_t* screen;
    page_interface_t* interface;
    void* private_data;
    int is_created; /* 页面是否已创建 */
} page_t;

page_manager_t* page_manager_create(void);
void page_manager_destroy(page_manager_t* pm);
int page_manager_register(page_manager_t* pm, const char* name, page_interface_t* interface, void* user_data);
int page_manager_navigate(page_manager_t* pm, const char* page_name);
int page_manager_back(page_manager_t* pm);
const char* page_manager_get_current(page_manager_t* pm);

lv_obj_t* page_get_root(page_manager_t* pm);
void* page_get_private_data(page_manager_t* pm);
void page_set_private_data(page_manager_t* pm, void* data);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_MANAGER_H__ */
