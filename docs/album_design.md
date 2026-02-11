# 相册模块虚拟列表设计方案

## 1. 背景与问题分析

### 1.1 当前实现问题

```c
// 当前实现（问题代码）
#define GRID_COLS 3
#define GRID_ROWS 3
#define GRID_ITEMS_PER_PAGE 9

for (int page = 0; page < 3; page++) {           // 硬编码3页
    for (int i = 0; i < 9; i++) {               // 每页固定9个
        lv_obj_t* item = lv_obj_create(tile);   // 全部预先创建
        // ...
    }
}
```

**存在问题：**

| 问题 | 影响 |
|------|------|
| 固定布局 | 无法适配不同屏幕尺寸 |
| 预创建所有item | 一万张照片会创建一万个对象，内存爆炸 |
| 固定分页 | 不能像手机相册那样连续滑动 |
| 硬编码参数 | 布局不灵活，难以维护 |

### 1.2 需求分析

- **灵活布局**：根据屏幕尺寸自动计算行列数
- **内存效率**：大量照片时不占用过多内存
- **流畅滑动**：支持连续滑动（非分页），体验如手机相册
- **可扩展性**：易于添加新功能（选择、删除、预览等）

---

## 2. 方案对比

### 方案A：虚拟Tile（Tileview + 动态Tile）

```
┌─────────────────────────────────────┐
│ Tile 0                              │
│ ┌───┐ ┌───┐ ┌───┐                   │
│ │ 0 │ │ 1 │ │ 2 │                   │
│ └───┘ └───┘ └───┘                   │
├─────────────────────────────────────┤
│ Tile 1                              │
│ ┌───┐ ┌───┐ ┌───┐                   │
│ │ 9 │ │10 │ │11 │                   │
│ └───┘ └───┘ └───┘                   │
└─────────────────────────────────────┘
```

**优点：**
- LVGL原生支持，代码改动小
- 分页清晰，适合分类浏览
- 每个Tile独立管理

**缺点：**
- 仍是分页滑动，无法连续
- 大量页面时Tile管理复杂

### 方案B：纯虚拟列表（推荐）

```
┌─────────────────────────────────────┐
│ ┌───┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐      │ ← 可见区域 + 缓冲区
│ │ 0 │ │ 1 │ │ 2 │ │ 3 │ │ 4 │      │
│ └───┘ └───┘ └───┘ └───┘ └───┘      │
│            ... 连续滚动 ...          │
│ ┌───┐ ┌───┐ ┌───┐                  │
│ │ N │ │N+1│ │N+2│                  │
└─────────────────────────────────────┘
```

**优点：**
- 连续滑动，体验最接近手机相册
- 内存最优，只创建可见项
- 灵活性高，布局算法统一

**缺点：**
- 实现复杂度较高
- 需要手动管理item池

### 方案C：简化适配

直接根据屏幕尺寸调整固定布局参数。

**优点：**
- 改动最小
- 易于理解

**缺点：**
- 无法解决大量照片的内存问题
- 仍分页滑动

---

## 3. 方案B详细设计

### 3.1 核心架构

```
┌──────────────────────────────────────────────────────────┐
│                    page_album.c                          │
├──────────────────────────────────────────────────────────┤
│  ┌────────────────────────────────────────────────────┐ │
│  │                    Scroll Container                 │ │ ← lv_obj_t (LV_DIR_VER)
│  │  ┌────────────────────────────────────────────────┐ │ │
│  │  │              Item Pool (复用池)                │ │ │
│  │  │  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐   │ │ │
│  │  │  │ 0  │ │ 1  │ │ 2  │ │ 3  │ │ 4  │ │ 5  │   │ │ │
│  │  │  └────┘ └────┘ └────┘ └────┘ └────┘ └────┘   │ │ │ ← 可见项 + 缓冲区
│  │  └────────────────────────────────────────────────┘ │ │
│  └────────────────────────────────────────────────────┘ │
│                                                          │
│  ┌────────────────────────────────────────────────────┐ │
│  │              Photo Manager (照片管理)               │ │
│  │  - photo_count: 照片总数                            │ │
│  │  - photo_get_path(index): 获取照片路径              │ │
│  │  - photo_get_count(): 获取总数                      │ │
│  └────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────┘
```

### 3.2 数据结构

```c
// src/pages/page_album.h

#pragma once

#include <stdint.h>
#include <lvgl.h>

/* 相册配置 */
typedef struct {
    int item_width;       // 单项宽度
    int item_height;      // 单项高度
    int gap_x;            // 水平间距
    int gap_y;            // 垂直间距
    int cols;             // 列数（自动计算）
    int buffer_count;     // 缓冲区item数量（通常为 cols * 2）
} album_config_t;

/* 虚拟列表item数据结构 */
typedef struct {
    int index;            // 照片索引
    lv_obj_t* container;  // 容器对象
    lv_obj_t* img;        // 图片对象
    lv_obj_t* label;      // 索引标签
    bool is_visible;      // 是否可见
} album_item_t;

/* 页面私有数据 */
typedef struct {
    lv_obj_t* container;           // 页面容器
    lv_obj_t* scroll_container;   // 滚动容器
    album_config_t config;         // 配置参数
    int total_photos;             // 照片总数
    album_item_t* item_pool;      // item复用池
    int pool_size;                // 池大小（可见+缓冲）
    int first_visible_index;      // 第一个可见项的索引
} page_album_data_t;
```

### 3.3 核心算法

#### 3.3.1 布局参数自动计算

```c
/**
 * @brief 根据屏幕宽度自动计算列数和起始位置
 */
static void calculate_layout(album_config_t* config)
{
    int available_width = H_RES;
    int gap = config->gap_x;

    // 计算能容纳多少列
    // (available_width - gap) / (item_width + gap) >= cols
    // cols * (item_width + gap) - gap <= available_width
    int cols = (available_width + gap) / (config->item_width + gap);
    if (cols < 1) cols = 1;

    config->cols = cols;

    // 计算实际使用的宽度（用于居中）
    int used_width = cols * config->item_width + (cols - 1) * gap;
    config->start_x = (H_RES - used_width) / 2;
}
```

#### 3.3.2 滚动位置到索引的映射

```c
/**
 * @brief 根据滚动位置计算可见范围
 */
static void update_visible_range(page_album_data_t* data, int scroll_y)
{
    int item_height = data->config.item_height + data->config.gap_y;
    int gap = data->config.gap_y;

    // 计算第一个可见项的索引
    int first_index = (scroll_y + gap) / item_height;
    if (first_index < 0) first_index = 0;

    // 计算最后一个可见项的索引
    int visible_count = V_RES / item_height + 1;  // 可见数量 + 1缓冲
    int last_index = first_index + visible_count;

    // 限制范围
    if (last_index > data->total_photos) {
        last_index = data->total_photos;
    }

    // 更新状态
    data->first_visible_index = first_index;
    data->last_visible_index = last_index;
}
```

#### 3.3.3 Item复用机制

```c
/**
 * @brief 刷新可见区域的item
 */
static void refresh_visible_items(page_album_data_t* data)
{
    int first = data->first_visible_index;
    int last = data->last_visible_index;

    // 遍历池中所有item
    for (int i = 0; i < data->pool_size; i++) {
        album_item_t* item = &data->item_pool[i];
        int item_index = first + i;

        if (item_index < last) {
            // 该位置应该显示item
            if (!item->is_visible) {
                // 重新激活隐藏的item
                lv_obj_clear_flag(item->container, LV_OBJ_FLAG_HIDDEN);
                item->is_visible = true;
            }

            // 更新内容（如果是新显示的）
            if (item->index != item_index) {
                item->index = item_index;
                update_item_content(item, item_index);
            }

            // 更新位置
            update_item_position(item, item_index);
        } else {
            // 该位置不需要显示，隐藏
            if (item->is_visible) {
                lv_obj_add_flag(item->container, LV_OBJ_FLAG_HIDDEN);
                item->is_visible = false;
            }
        }
    }
}
```

#### 3.3.4 滚动事件处理

```c
/**
 * @brief 滚动事件回调
 */
static void scroll_event_cb(lv_event_t* e)
{
    page_album_data_t* data = (page_album_data_t*)lv_event_get_user_data(e);
    if (!data) return;

    lv_obj_t* container = lv_event_get_current_target(e);
    int scroll_y = lv_obj_get_scroll_y(container);

    // 重新计算可见范围
    update_visible_range(data, scroll_y);

    // 刷新item
    refresh_visible_items(data);
}
```

### 3.4 Item创建与初始化

```c
/**
 * @brief 创建item池
 */
static int create_item_pool(page_album_data_t* data)
{
    int cols = data->config.cols;
    int rows = V_RES / (data->config.item_height + data->config.gap_y) + 2;

    data->pool_size = cols * rows + cols * 2;  // 可见 + 缓冲区
    data->item_pool = malloc(sizeof(album_item_t) * data->pool_size);

    // 创建所有item对象（初始化为隐藏）
    for (int i = 0; i < data->pool_size; i++) {
        album_item_t* item = &data->item_pool[i];
        item->index = -1;
        item->is_visible = false;

        // 创建容器
        item->container = lv_obj_create(data->scroll_container);
        lv_obj_set_size(item->container, data->config.item_width, data->config.item_height);
        lv_obj_add_style(item->container, &style_album_item, LV_PART_MAIN);
        lv_obj_clear_flag(item->container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(item->container, LV_OBJ_FLAG_HIDDEN);  // 初始隐藏

        // 创建图片
        item->img = lv_img_create(item->container);
        lv_obj_set_size(item->img, data->config.item_width, data->config.item_height);
        lv_obj_center(item->img);

        // 创建标签
        item->label = lv_label_create(item->container);
        lv_obj_center(item->label);
    }

    return 0;
}

/**
 * @brief 更新item内容
 */
static void update_item_content(album_item_t* item, int index)
{
    // 更新索引标签
    lv_label_set_text_fmt(item->label, "%d", index + 1);

    // TODO: 加载实际图片
    // photo_manager_load_thumbnail(index, item->img);
}
```

### 3.5 滚动容器总高度设置

```c
/**
 * @brief 设置滚动容器的总高度
 * @note 总高度 = 照片总数 * (item高度 + 间距) - 最后一个间距
 */
static void update_scroll_height(page_album_data_t* data)
{
    int item_height = data->config.item_height + data->config.gap_y;
    int cols = data->config.cols;

    // 计算总行数
    int total_rows = (data->total_photos + cols - 1) / cols;

    // 计算总高度
    int total_height = total_rows * item_height - data->config.gap_y;

    lv_obj_set_height(data->scroll_container, total_height);
}
```

---

## 4. 接口设计

### 4.1 照片管理器接口（待实现）

```c
// src/core/photo_manager.h

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化照片管理器
 */
int photo_manager_init(const char* photo_dir);

/**
 * @brief 获取照片总数
 */
int photo_manager_get_count(void);

/**
 * @brief 根据索引获取照片路径
 * @param index 照片索引 (0-based)
 * @return 照片路径字符串（内部缓存，使用后不要free）
 */
const char* photo_manager_get_path(int index);

/**
 * @brief 加载缩略图
 * @param index 照片索引
 * @param img_obj LVGL图片对象
 */
void photo_manager_load_thumbnail(int index, lv_obj_t* img_obj);

/**
 * @brief 卸载缩略图（释放内存）
 */
void photo_manager_unload_thumbnail(int index);

/**
 * @brief 刷新照片列表
 */
void photo_manager_refresh(void);

/**
 * @brief 释放照片管理器
 */
void photo_manager_deinit(void);

#ifdef __cplusplus
}
#endif
```

---

## 5. 实现步骤

### Phase 1: 基础框架
- [ ] 更新 `page_album.h` 数据结构
- [ ] 实现布局参数计算
- [ ] 创建item池

### Phase 2: 核心逻辑
- [ ] 实现滚动位置映射
- [ ] 实现item复用机制
- [ ] 实现滚动事件处理

### Phase 3: 照片集成
- [ ] 实现照片管理器（简化版）
- [ ] 集成缩略图加载
- [ ] 处理图片异步加载

### Phase 4: 优化完善
- [ ] 添加加载动画
- [ ] 添加下拉刷新
- [ ] 添加图片点击预览
- [ ] 添加选择/删除功能

---

## 6. 内存分析

### 当前方案（预创建所有item）

```
一万张照片：
├── 10000 * container (64B)     = 640 KB
├── 10000 * img (64B)           = 640 KB
├── 10000 * label (64B)         = 640 KB
├── 总计 ≈ 2 MB+（仅对象头，实际更多）
└── 实际内存占用可能达到 50-100 MB
```

### 虚拟列表方案

```
一万张照片，屏幕显示12个，池大小20：
├── 20 * container (64B)       = 1.28 KB
├── 20 * img (64B)             = 1.28 KB
├── 20 * label (64B)           = 1.28 KB
├── 复用池结构                  ≈ 5 KB
├── 总计 ≈ 10 KB（对象占用）
└── 实际内存占用 < 1 MB
```

---

## 7. 注意事项

1. **线程安全**：如果从文件系统读取照片，需要在主线程操作LVGL对象
2. **异步加载**：大图加载需要异步处理，避免阻塞UI
3. **缓存策略**：缩略图需要合理的缓存和淘汰策略
4. **边界处理**：滚动到顶部/底部时的边界检查
5. **动画优化**：禁用item切换时的动画，减少CPU占用

---

## 8. 参考资料

- [LVGL Tileview](https://docs.lvgl.io/master/widgets/extra(tileview.html))
- [LVGL Virtual List](https://docs.lvgl.io/master/widgets/core(list.html))
- [LVGL Layout](https://docs.lvgl.io/master/layouts/)
- Android RecyclerView 实现原理
- iOS UICollectionView 实现原理
