#include "core/page_manager.h"
#include <stdlib.h>
#include <string.h>

#define MAX_PAGES 32

struct page_manager_t {
    page_t pages[MAX_PAGES];
    int page_count;
    int current_page_index;
    int history[MAX_PAGES];
    int history_count;
};

page_manager_t* page_manager_create(void)
{
    page_manager_t* pm = (page_manager_t*)malloc(sizeof(page_manager_t));
    if (!pm) {
        return NULL;
    }

    memset(pm, 0, sizeof(page_manager_t));
    return pm;
}

void page_manager_destroy(page_manager_t* pm)
{
    if (!pm) {
        return;
    }

    for (int i = 0; i < pm->page_count; i++) {
        if (pm->pages[i].interface && pm->pages[i].interface->destroy) {
            pm->pages[i].interface->destroy(pm, pm->pages[i].private_data);
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

    if (pm->current_page_index >= 0) {
        page_t* current_page = &pm->pages[pm->current_page_index];
        if (current_page->interface && current_page->interface->hide) {
            current_page->interface->hide(pm, current_page->private_data);
        }

        if (pm->history_count < MAX_PAGES) {
            pm->history[pm->history_count++] = pm->current_page_index;
        }
    }

    page_t* target_page = &pm->pages[target_index];
    if (target_page->interface && target_page->interface->create) {
        target_page->interface->create(pm, target_page->private_data);
    }

    if (target_page->interface && target_page->interface->show) {
        target_page->interface->show(pm, target_page->private_data);
    }

    pm->current_page_index = target_index;

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

void* page_get_private_data(page_manager_t* pm, const char* key)
{
    if (!pm || !key) {
        return NULL;
    }

    for (int i = 0; i < pm->page_count; i++) {
        if (pm->pages[i].interface == NULL) {
            continue;
        }
        /* Check if this is the current page's private data */
        if (i == pm->current_page_index) {
            /* Return private_data which is used as user_data in registration */
            return pm->pages[i].private_data;
        }
    }

    return NULL;
}

void page_set_private_data(page_manager_t* pm, const char* key, void* data)
{
    (void)key;
    if (!pm) {
        return;
    }

    for (int i = 0; i < pm->page_count; i++) {
        if (pm->pages[i].interface == NULL) {
            continue;
        }
        if (i == pm->current_page_index) {
            pm->pages[i].private_data = data;
            return;
        }
    }
}
