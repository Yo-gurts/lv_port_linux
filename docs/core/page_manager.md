# Page Manager 设计文档

## 1. 概述

Page Manager 是一个轻量级页面管理系统，用于统一管理 LVGL 页面生命周期、页面跳转和历史返回。

当前实现采用**模块内单例**：
- 仅在 `src/core/page_manager.c` 内部维护一个全局 `page_manager_t` 实例
- 外部代码不再传递 `pm` 指针
- 所有页面生命周期函数与导航 API 都是无参（或仅保留页面名参数）

## 2. 核心数据结构

### 2.1 页面接口 `page_interface_t`

```c
typedef struct {
    const char* name;
    void (*create)(void);
    void (*destroy)(void);
    void (*show)(void);
    void (*hide)(void);
    void (*update)(void);
} page_interface_t;
```

说明：
- 页面回调全部无参
- 页面内部若需访问自身私有数据，通过 `page_get_private_data()` 获取

### 2.2 页面实例 `page_t`

```c
typedef struct {
    const char* name;
    lv_obj_t* screen;
    page_interface_t* interface;
    void* private_data;
    int is_created;
} page_t;
```

### 2.3 页面管理器 `page_manager_t`

`page_manager_t` 结构体仍在头文件中前置声明，真实实例只存在于 `page_manager.c` 内部。

## 3. 生命周期与切换流程

页面切换时序：
1. `page_manager_navigate("target")`
2. 当前页面 `hide()`（如果存在）
3. 记录历史栈
4. 首次进入目标页面时执行 `create()`
5. 目标页面执行 `show()`

返回流程：
1. `page_manager_back()`
2. 历史栈弹出上一个页面
3. 当前页面 `hide()`
4. 上一个页面 `show()`

销毁流程：
- `page_manager_destroy()` 会遍历所有已创建页面并调用各自 `destroy()`

## 4. API 参考

### 4.1 创建与销毁

```c
int page_manager_create(void);
void page_manager_destroy(void);
```

### 4.2 页面注册与导航

```c
int page_manager_register(const char* name, page_interface_t* interface, void* user_data);
int page_manager_navigate(const char* page_name);
int page_manager_back(void);
const char* page_manager_get_current(void);
```

### 4.3 页面私有数据

```c
void* page_get_private_data(void);
void page_set_private_data(void* data);
```

### 4.4 通用事件回调

```c
void page_manager_back_cb(lv_event_t* e);
void page_manager_swipe_right_cb(lv_event_t* e);
```

说明：
- 这两个回调内部直接调用单例 Page Manager
- 不依赖 `lv_event_get_user_data(e)` 里的 `pm`

## 5. 使用示例

### 5.1 初始化与注册页面

```c
int ui_main(void)
{
    page_manager_create();

    page_manager_register("home", &home_page_interface, NULL);
    page_manager_register("photo", &photo_page_interface, NULL);

    page_manager_navigate("home");
    return 0;
}
```

### 5.2 页面实现（无参生命周期）

```c
void page_home_create(void)
{
    home_page_data_t* data = malloc(sizeof(*data));
    memset(data, 0, sizeof(*data));

    data->container = lv_obj_create(lv_screen_active());
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);

    page_set_private_data(data);
}

void page_home_show(void)
{
    home_page_data_t* data = page_get_private_data();
    if (!data || !data->container) return;

    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}

void page_home_hide(void)
{
    home_page_data_t* data = page_get_private_data();
    if (!data || !data->container) return;

    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
}
```

### 5.3 事件回调中跳转

```c
static void photo_button_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_manager_navigate("photo");
}

static void back_button_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    page_manager_back();
}
```

### 5.4 手势返回绑定

```c
lv_obj_clear_flag(data->container, LV_OBJ_FLAG_GESTURE_BUBBLE);
lv_obj_add_event_cb(data->container, page_manager_swipe_right_cb, LV_EVENT_GESTURE, NULL);
```

## 6. 设计要点

1. 页面必须使用独立容器，避免控件重叠。
2. 页面私有状态统一放入 `page_set_private_data()`。
3. `show()/hide()` 负责资源启停（定时器、动画、后台任务）。
4. 页面回调不要依赖外部 `pm` 指针。

## 7. 常见问题

### Q1: 为什么不再传 `pm` 参数？

A: 当前工程只存在一个 Page Manager 实例，改为模块内单例后，接口更简洁，页面代码更少样板逻辑。

### Q2: 事件回调里还需要传 `pm` 到 `user_data` 吗？

A: 不需要。`page_manager_back_cb` / `page_manager_swipe_right_cb` 已直接访问单例。

### Q3: 如何获取当前页面的上下文数据？

A: 在对应页面生命周期内通过 `page_get_private_data()` 读取当前页面私有数据。

## 8. 相关文件

- `include/core/page_manager.h`
- `src/core/page_manager.c`
- `src/pages/*.c`
