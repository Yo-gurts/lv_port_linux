// #############################################################################
// ! #region 1. 头文件与宏定义
// #############################################################################

#include "core/page_manager.h"
#include "config.h"
#include "mlog.h"
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

static page_manager_t g_page_manager;
static int g_page_manager_inited = 0;

// #endregion
// #############################################################################
// ! #region 4. 内部工具函数（注意用static修饰）
// #############################################################################

static page_manager_t* page_manager_get_instance(void)
{
    if (!g_page_manager_inited) {
        return NULL;
    }
    return &g_page_manager;
}

static int page_manager_switch_to_index(int target_index, int record_history)
{
    page_manager_t* pm = page_manager_get_instance();
    if (!pm || target_index < 0 || target_index >= pm->page_count) {
        return -1;
    }

    if (pm->current_page_index == target_index) {
        return 0;
    }

    int prev_index = pm->current_page_index;
    if (prev_index >= 0) {
        page_t* prev_page = &pm->pages[prev_index];
        if (prev_page->interface && prev_page->interface->hide) {
            prev_page->interface->hide();
        }

        if (record_history && pm->history_count < MAX_PAGES) {
            pm->history[pm->history_count++] = prev_index;
        }
    }

    pm->current_page_index = target_index;

    page_t* target_page = &pm->pages[target_index];
    if (!target_page->is_created) {
        if (target_page->interface && target_page->interface->create) {
            target_page->interface->create();
        }
        target_page->is_created = 1;
    }

    /* 在show之前调用update刷新UI状态 */
    if (target_page->interface && target_page->interface->update) {
        target_page->interface->update();
    }

    if (target_page->interface && target_page->interface->show) {
        target_page->interface->show();
    }

    return 0;
}

// #endregion
// #############################################################################
// ! #region 5. 对外接口函数
// #############################################################################

int page_manager_create(void)
{
    memset(&g_page_manager, 0, sizeof(g_page_manager));
    g_page_manager.current_page_index = -1;
    g_page_manager_inited = 1;
    return 0;
}

void page_manager_destroy(void)
{
    page_manager_t* pm = page_manager_get_instance();
    if (!pm) {
        return;
    }

    for (int i = 0; i < pm->page_count; i++) {
        if (!pm->pages[i].is_created) {
            continue;
        }

        // page_xxx_destory 获取私有数据是通过 current_page_index
        pm->current_page_index = i;
        if (pm->pages[i].interface && pm->pages[i].interface->destroy) {
            pm->pages[i].interface->destroy();
        }
        // Todo: free memory
        pm->pages[i].private_data = NULL;
        pm->pages[i].is_created = 0;
    }
    pm->current_page_index = -1;
    g_page_manager_inited = 0;
}

int page_manager_register(const char* name, page_interface_t* interface, void* user_data)
{
    page_manager_t* pm = page_manager_get_instance();
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

int page_manager_navigate(const char* page_name)
{
    page_manager_t* pm = page_manager_get_instance();
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

    return page_manager_switch_to_index(target_index, 1);
}

int page_manager_back(void)
{
    page_manager_t* pm = page_manager_get_instance();
    if (!pm || pm->history_count == 0) {
        return -1;
    }

    int prev_index = pm->history[--pm->history_count];
    if (prev_index < 0 || prev_index >= pm->page_count) {
        return -1;
    }

    return page_manager_switch_to_index(prev_index, 0);
}

const char* page_manager_get_current(void)
{
    page_manager_t* pm = page_manager_get_instance();
    if (!pm || pm->current_page_index < 0) {
        return NULL;
    }

    return pm->pages[pm->current_page_index].name;
}

lv_obj_t* page_get_root(void)
{
    if (!page_manager_get_instance()) {
        return NULL;
    }
    return lv_screen_active();
}

void* page_get_private_data(void)
{
    page_manager_t* pm = page_manager_get_instance();
    if (!pm || pm->current_page_index < 0) {
        return NULL;
    }

    return pm->pages[pm->current_page_index].private_data;
}

void page_set_private_data(void* data)
{
    page_manager_t* pm = page_manager_get_instance();
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

/* 通用返回按钮回调 - 所有页面可直接作为事件回调使用 */
void page_manager_back_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    MLOG_INFO("Back button clicked");
    page_manager_back();
}

/* 通用右滑返回回调 - 所有页面可直接作为事件回调使用
 * 当检测到滑动时，调用 lv_indev_set_wait_until_release() 避免释放时触发点击 */
void page_manager_swipe_right_cb(lv_event_t* e)
{
    static lv_point_t swipe_start_point = {0};
    static int swipe_start_valid = 0;
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t* indev = lv_indev_get_act();

    if (code == LV_EVENT_PRESSED) {
        if (indev != NULL) {
            lv_indev_get_point(indev, &swipe_start_point);
            swipe_start_valid = 1;
        }
        return;
    }

    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = indev ? lv_indev_get_gesture_dir(indev) : LV_DIR_NONE;
        int from_left_edge = swipe_start_valid && (swipe_start_point.x <= SWIPE_BACK_EDGE_THRESHOLD_PX);
        if (dir == LV_DIR_RIGHT && from_left_edge) {
            MLOG_INFO("Swipe right, back to previous page");
            page_manager_back();
        }
        /* 检测到滑动，忽略后续的点击事件 */
        if (indev) {
            lv_indev_wait_release(indev);
        }
        swipe_start_valid = 0;
    }
}

// #endregion
// #############################################################################
// ! #region 8. 初始化、去初始化、资源管理
// #############################################################################

// #endregion
// #############################################################################
// ! #region 9. 调试与测试
// #############################################################################

// #endregion
