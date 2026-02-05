# Page Manager 设计文档

## 1. 概述

Page Manager 是一个轻量级的页面管理系统，专门为嵌入式 LVGL 应用设计。它提供了一种简洁的方式来管理多个页面（page）的生命周期、页面切换和导航历史，解决了传统直接操作 LVGL 屏幕时遇到的页面切换冲突和状态管理混乱的问题。

### 设计目标

- **页面隔离**：每个页面拥有独立的 UI 容器，避免页面切换时 UI 元素冲突
- **生命周期管理**：自动管理页面的创建、显示、隐藏和销毁
- **导航历史**：支持页面返回功能，保存访问历史
- **资源节约**：页面隐藏时暂停定时器等资源密集型操作
- **简洁接口**：提供清晰的 API，易于理解和使用

## 2. 核心概念

### 2.1 页面接口 (page_interface_t)

每个页面需要实现一组标准的生命周期回调函数：

```c
typedef struct {
    const char* name;                    // 页面名称
    void (*create)(page_manager_t* pm);  // 创建页面（首次访问时调用）
    void (*destroy)(page_manager_t* pm); // 销毁页面（应用退出或页面注销时调用）
    void (*show)(page_manager_t* pm);    // 显示页面（页面获得焦点时调用）
    void (*hide)(page_manager_t* pm);    // 隐藏页面（页面失去焦点时调用）
    void (*update)(page_manager_t* pm);  // 更新页面（需要刷新UI时调用）
} page_interface_t;
```

### 2.2 页面实例 (page_t)

```c
typedef struct {
    const char* name;           // 页面名称
    lv_obj_t* screen;           // 页面根对象（预留）
    page_interface_t* interface;// 页面接口实现
    void* private_data;         // 页面私有数据
    int is_created;             // 页面是否已创建标志
} page_t;
```

### 2.3 页面管理器 (page_manager_t)

```c
struct page_manager_t {
    page_t pages[MAX_PAGES];       // 页面数组
    int page_count;                // 已注册页面数量
    int current_page_index;        // 当前页面索引
    int history[MAX_PAGES];        // 访问历史栈
    int history_count;             // 历史记录数量
};
```

## 3. 页面生命周期

### 3.1 状态转换图

```
  [未创建] --create()--> [已创建隐藏] --show()--> [激活显示]
        |                      |                       |
        |                      |                       |
        |                      +-------hide()----------+
        |                                        |
        |                                        v
        +-------destroy()-----------------> [已销毁]
```

### 3.2 生命周期详解

| 阶段 | 回调函数 | 说明 |
|------|----------|------|
| **create** | `create()` | 首次访问页面时调用，用于创建 UI 元素和初始化数据 |
| **show** | `show()` | 页面获得焦点时调用，显示 UI 并恢复定时器等资源 |
| **hide** | `hide()` | 页面失去焦点时调用，隐藏 UI 并暂停资源消耗操作 |
| **destroy** | `destroy()` | 页面被注销或应用退出时调用，释放所有资源 |

### 3.3 生命周期管理示例

```c
void page_home_show(page_manager_t* pm)
{
    home data = page_get_private_data(pm);
    if (!data || !data_page_data_t*->container) {
        return;
    }

    // 显示 UI
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);

    // 恢复定时器
    if (data->timer) {
        lv_timer_resume(data->timer);
    }
}

void page_home_hide(page_manager_t* pm)
{
    home_page_data_t* data = page_get_private_data(pm);
    if (!data || !data->container) {
        return;
    }

    // 隐藏 UI
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);

    // 暂停定时器，节约资源
    if (data->timer) {
        lv_timer_pause(data->timer);
    }
}
```

## 4. API 参考

### 4.1 创建和销毁

```c
// 创建页面管理器
page_manager_t* page_manager_create(void);

// 销毁页面管理器（会自动销毁所有已注册的页面）
void page_manager_destroy(page_manager_t* pm);
```

### 4.2 页面注册

```c
// 注册页面到管理器
// @param pm        页面管理器
// @param name      页面名称（唯一标识）
// @param interface 页面接口实现
// @param user_data 私有数据指针
int page_manager_register(page_manager_t* pm, const char* name,
                          page_interface_t* interface, void* user_data);
```

### 4.3 页面导航

```c
// 跳转到指定页面
int page_manager_navigate(page_manager_t* pm, const char* page_name);

// 返回上一页
int page_manager_back(page_manager_t* pm);

// 获取当前页面名称
const char* page_manager_get_current(page_manager_t* pm);
```

### 4.4 私有数据管理

```c
// 获取当前页面的私有数据
void* page_get_private_data(page_manager_t* pm);

// 设置当前页面的私有数据
void page_set_private_data(page_manager_t* pm, void* data);
```

## 5. 实践示例：Home 和 Photo 页面

### 5.1 注册页面

```c
// 在应用初始化时注册页面
void app_init(page_manager_t* pm)
{
    static page_interface_t home_interface = {
        .name = "home",
        .create = page_home_create,
        .destroy = page_home_destroy,
        .show = page_home_show,
        .hide = page_home_hide,
        .update = page_home_update,
    };

    static page_interface_t photo_interface = {
        .name = "photo",
        .create = page_photo_create,
        .destroy = page_photo_destroy,
        .show = page_photo_show,
        .hide = page_photo_hide,
        .update = page_photo_update,
    };

    page_manager_register(pm, "home", &home_interface, NULL);
    page_manager_register(pm, "photo", &photo_interface, NULL);

    // 跳转到首页
    page_manager_navigate(pm, "home");
}
```

### 5.2 Home 页面实现

Home 页面是应用的入口页面，包含一个 2x3 的功能图标网格。

**页面数据结构**：
```c
typedef struct {
    lv_obj_t* container;        // 页面容器
    lv_obj_t* grid_container;   // 图标网格容器
    lv_obj_t* time_label;       // 时间标签
    lv_obj_t* wifi_icon;        // WiFi 图标
    lv_obj_t* battery_icon;     // 电池图标
    lv_timer_t* timer;          // 时间更新定时器
} home_page_data_t;
```

**创建页面**：
```c
void page_home_create(page_manager_t* pm)
{
    home_page_data_t* data = (home_page_data_t*)malloc(sizeof(home_page_data_t));
    memset(data, 0, sizeof(home_page_data_t));

    // 创建页面容器（关键：使用容器隔离）
    data->container = lv_obj_create(lv_screen_active());
    lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);

    // 创建状态栏和时间标签
    data->time_label = lv_label_create(data->container);
    data->wifi_icon = lv_img_create(data->container);
    data->battery_icon = lv_img_create(data->container);

    // 创建网格容器
    data->grid_container = lv_obj_create(data->container);
    lv_obj_set_layout(data->grid_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(data->grid_container, LV_FLEX_FLOW_ROW_WRAP);

    // 添加功能按钮（点击跳转到对应页面）
    create_icon_button(pm, data->grid_container, LV_SYMBOL_IMAGE, "AI拍照",
        LV_ALIGN_TOP_LEFT, 0, 0, photo_button_cb, NULL);

    // 创建定时器（用于更新时间显示）
    data->timer = lv_timer_create(home_update_timer_cb, 1000, data);

    // 保存私有数据
    page_set_private_data(pm, data);
}
```

**按钮点击跳转到 Photo 页面**：
```c
static void photo_button_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    page_manager_navigate(pm, "photo");  // 跳转到 photo 页面
}
```

### 5.3 Photo 页面实现

Photo 页面是一个拍照/录像模式切换的相机预览页面，支持滑动手势返回。

**页面数据结构**：
```c
typedef struct {
    lv_obj_t* container;        // 页面容器
    lv_obj_t* top_bar;          // 顶部状态栏
    lv_obj_t* bottom_bar;       // 底部工具栏
    lv_obj_t* mode_btn;         // 拍照/录像切换按钮
    lv_obj_t* menu_btn;         // 菜单按钮（返回首页）
    int is_video_mode;          // 是否录像模式
} page_photo_data_t;
```

**菜单按钮返回首页**：
```c
static void menu_back_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    page_manager_back(pm);  // 返回上一页
}
```

**滑动手势返回**：
```c
static void gesture_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {  // 从左往右滑
            page_manager_t* pm = lv_event_get_user_data(e);
            page_manager_back(pm);
        }
    }
}

// 在 create 中注册手势回调
void page_photo_create(page_manager_t* pm)
{
    // ...
    data->container = lv_obj_create(lv_screen_active());
    lv_obj_add_flag(data->container, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(data->container, gesture_cb, LV_EVENT_GESTURE, pm);
}
```

### 5.4 页面切换流程

当用户点击 Home 页面的 "AI拍照" 按钮时：

```
1. photo_button_cb() 被调用
2. page_manager_navigate(pm, "photo") 被调用
3. page_manager 内部执行：
   a. 调用 home 页面的 hide() -> Home 页面隐藏
   b. 将 home 页面索引压入历史栈
   c. 检查 photo 页面是否已创建
   d. 如果未创建，调用 photo 页面的 create() -> Photo 页面创建
   e. 调用 photo 页面的 show() -> Photo 页面显示
4. Photo 页面现在处于激活状态
```

当用户点击 Photo 页面的菜单按钮时：

```
1. menu_back_cb() 被调用
2. page_manager_back(pm) 被调用
3. page_manager 内部执行：
   a. 从历史栈弹出上一个页面索引 (home)
   b. 调用 photo 页面的 hide() -> Photo 页面隐藏
   c. 调用 home 页面的 show() -> Home 页面显示
4. Home 页面恢复激活状态
```

## 6. 设计要点

### 6.1 容器隔离

每个页面必须创建自己的容器，所有 UI 元素都添加到这个容器中：

```c
// 错误做法：直接添加到屏幕
lv_obj_create(lv_screen_active());

// 正确做法：创建页面容器
data->container = lv_obj_create(lv_screen_active());
lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
// 所有子元素都添加到 container
```

### 6.2 私有数据管理

页面使用 `page_set_private_data()` 和 `page_get_private_data()` 来存储和获取页面私有数据：

```c
// 在 create 中保存
page_set_private_data(pm, data);

// 在 show/hide/destroy 中获取
home_page_data_t* data = page_get_private_data(pm);
```

### 6.3 资源管理

- **定时器**：在 `hide()` 时暂停，在 `show()` 时恢复
- **动画**：在 `hide()` 时暂停或停止
- **后台任务**：在 `hide()` 时暂停

```c
void page_home_hide(page_manager_t* pm)
{
    // ...
    if (data->timer) {
        lv_timer_pause(data->timer);  // 暂停定时器
    }
}
```

### 6.4 历史栈限制

历史栈最大记录 `MAX_PAGES (32)` 个页面，超出时会丢弃最老的记录。

## 7. 扩展建议

### 7.1 添加页面过渡动画

可以在 `page_manager_navigate()` 中添加淡入淡出效果：

```c
// 在 show() 中添加淡入动画
void page_photo_show(page_manager_t* pm)
{
    // ...
    lv_obj_set_style_opa(data->container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(data->container, LV_OBJ_FLAG_HIDDEN);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_duration(&a, 300);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}
```

### 7.2 支持页面参数传递

可以扩展 `page_manager_navigate()` 来支持传递页面参数：

```c
int page_manager_navigate(page_manager_t* pm, const char* page_name, void* params);

// 在 create 中接收参数
void page_photo_create(page_manager_t* pm, void* params)
{
    if (params) {
        page_photo_params_t* p = (page_photo_params_t*)params;
        // 使用参数初始化页面
    }
}
```

### 7.3 添加页面事件通知

可以引入事件系统，让页面能够响应系统事件：

```c
typedef enum {
    PAGE_EVENT_SHOW,
    PAGE_EVENT_HIDE,
    PAGE_EVENT_LOW_MEMORY,
    PAGE_EVENT_WAKE_UP,
} page_event_t;

typedef void (*page_event_cb_t)(page_event_t event, void* data);
```

## 8. 常见问题

### Q: 页面切换后 UI 元素重叠怎么办？

A: 确保每个页面都创建了自己的容器，且容器设置了正确的样式：
```c
lv_obj_add_style(data->container, &style_page_container, LV_PART_MAIN);
```

### Q: 定时器在页面隐藏后仍在运行怎么办？

A: 在 `hide()` 回调中暂停定时器：
```c
if (data->timer) {
    lv_timer_pause(data->timer);
}
```

### Q: 如何实现页面返回时刷新数据？

A: 在 `show()` 回调中更新数据：
```c
void page_home_show(page_manager_t* pm)
{
    // 立即更新时间
    if (data->timer) {
        home_update_timer_cb(data->timer);
    }
}
```

## 9. 文件结构

```
include/
└── core/
    └── page_manager.h        # 页面管理器头文件

src/
└── core/
    └── page_manager.c        # 页面管理器实现

docs/
└── core/
    └── page_manager.md       # 本设计文档
```
