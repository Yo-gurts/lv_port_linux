# Page Manager 设计文档

## 1. 概述

Page Manager 用于统一管理页面生命周期、页面跳转与返回栈。
当前实现为模块内单例（`src/core/page_manager.c` 内部 `g_page_manager`）。

关键点：
- 页面回调均为无参函数
- 页面私有数据通过 `page_set_private_data()/page_get_private_data()` 读写
- 页面切换时目标页执行顺序为：`create(首次) -> update -> show`

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

## 3. 生命周期与切换流程

### 3.1 导航到新页面

`page_manager_navigate("target")`：
1) 当前页 `hide()`（如果存在）
2) 当前页索引入历史栈
3) 目标页首次进入则执行 `create()`
4) 执行目标页 `update()`
5) 执行目标页 `show()`

### 3.2 返回上一页

`page_manager_back()`：
1) 从历史栈弹出上一页索引
2) 当前页 `hide()`
3) 上一页 `update()`
4) 上一页 `show()`

### 3.3 销毁

`page_manager_destroy()` 会遍历所有已创建页面，切换 `current_page_index` 后逐页调用 `destroy()`。

## 4. API 参考

```c
int page_manager_create(void);
void page_manager_destroy(void);
int page_manager_register(const char* name, page_interface_t* interface, void* user_data);
int page_manager_navigate(const char* page_name);
int page_manager_back(void);
const char* page_manager_get_current(void);

void* page_get_private_data(void);
void page_set_private_data(void* data);

void page_manager_back_cb(lv_event_t* e);
```

说明：
- 当前对外仅提供 `page_manager_back_cb` 通用事件回调
- 文档中曾出现的 `page_manager_swipe_right_cb` 已不在现有接口内

## 5. 使用建议

1) 事件回调注册放在 `create()`，不要在 `show()` 重复注册。  
2) `show()` 做状态刷新，`hide()` 负责停表/停动画。  
3) 页面私有状态统一放在 `private_data`，避免跨页静态变量耦合。  
4) 页面跳转失败时要有日志（页面名、当前页、错误码）。

## 6. 与手势返回的关系

页面右滑返回由 `gesture_back` 模块负责手势判定，判定成功后通常调用 `page_manager_back_cb`（或直接 `page_manager_back`）。

建议遵循：
- 页面容器 `clear EVENT_BUBBLE`
- 子控件递归 `add EVENT_BUBBLE`
- 手势事件只注册一次，避免重复挂载

## 7. 相关文件

- `include/core/page_manager.h`
- `src/core/page_manager.c`
- `src/pages/*.c`
