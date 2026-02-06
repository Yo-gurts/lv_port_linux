// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "core/page_manager.h"
#include <stdlib.h>
#include <string.h>

#define MAX_PAGES 32

// #endregion
// #############################################################################
// ! #region 2. 数据结构定义
// #############################################################################

struct page_manager_t {
    page_t pages[MAX_PAGES];
    int page_count;
    int current_page_index;
    int history[MAX_PAGES];
    int history_count;
};

// #endregion
// #############################################################################
// ! #region 3. 全局变量 & 函数声明
// #############################################################################

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

// #endregion
// #############################################################################
// ! #region 5. 对外接口函数
// #############################################################################

page_manager_t* page_manager_create(void)
{
    page_manager_t* pm = (page_manager_t*)malloc(sizeof(page_manager_t));
    if (!pm) {
        return NULL;
    }

    memset(pm, 0, sizeof(page_manager_t));
    pm->current_page_index = -1;
    return pm;
}

void page_manager_destroy(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    for (int i = 0; i < pm->page_count; i++) {
        if (pm->pages[i].interface && pm->pages[i].interface->destroy) {
            pm->pages[i].interface->destroy(pm);
        }
    }

    free(pm);
}

int page_manager_register(page_manager_t* pm, const char* name, page_interface_t* interface, void* user_data)
{
    if (!pm || !name || !interface) {
        return -1;
    }

    if (pm->page_count >= MAX_PAGES) {
        return -1;
    }

    page_t* page = &pm->pages[pm->page_count];
    page->name = name;
    page->interface = interface;
    page->private_data = user_data;
    page->screen = NULL;

    pm->page_count++;

    return 0;
}

int page_manager_navigate(page_manager_t* pm, const char* page_name)
{
    if (!pm || !page_name) {
        return -1;
    }

    int target_index = -1;
    for (int i = 0; i < pm->page_count; i++) {
        if (strcmp(pm->pages[i].name, page_name) == 0) {
            target_index = i;
            break;
        }
    }

    if (target_index == -1) {
        return -1;
    }

    /* 如果目标页是当前页，不做任何操作 */
    if (pm->current_page_index == target_index) {
        return 0;
    }

    /* 先隐藏并记录上一个页面 */
    int prev_index = pm->current_page_index;
    if (prev_index >= 0) {
        page_t* prev_page = &pm->pages[prev_index];
        if (prev_page->interface && prev_page->interface->hide) {
            prev_page->interface->hide(pm);
        }

        /* 记录历史 */
        if (pm->history_count < MAX_PAGES) {
            pm->history[pm->history_count++] = prev_index;
        }
    }

    /* 设置当前页面索引 */
    pm->current_page_index = target_index;

    page_t* target_page = &pm->pages[target_index];

    /* 首次访问，创建页面 */
    if (!target_page->is_created) {
        if (target_page->interface && target_page->interface->create) {
            target_page->interface->create(pm);
        }
        target_page->is_created = 1;
    }

    /* 显示目标页面 */
    if (target_page->interface && target_page->interface->show) {
        target_page->interface->show(pm);
    }

    return 0;
}

int page_manager_back(page_manager_t* pm)
{
    if (!pm || pm->history_count == 0) {
        return -1;
    }

    int prev_index = pm->history[--pm->history_count];
    const char* prev_name = pm->pages[prev_index].name;

    return page_manager_navigate(pm, prev_name);
}

const char* page_manager_get_current(page_manager_t* pm)
{
    if (!pm || pm->current_page_index < 0) {
        return NULL;
    }

    return pm->pages[pm->current_page_index].name;
}

lv_obj_t* page_get_root(page_manager_t* pm)
{
    if (!pm) {
        return NULL;
    }

    return lv_screen_active();
}

void* page_get_private_data(page_manager_t* pm)
{
    if (!pm || pm->current_page_index < 0) {
        return NULL;
    }

    return pm->pages[pm->current_page_index].private_data;
}

void page_set_private_data(page_manager_t* pm, void* data)
{
    if (!pm || pm->current_page_index < 0) {
        return;
    }

    pm->pages[pm->current_page_index].private_data = data;
}

// #endregion
// #############################################################################
// ! #region 6. 线程处理函数
// #############################################################################

// #endregion
// #############################################################################
// ! #region 7. 按键、手势、定时器 等事件回调函数
// #############################################################################

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
