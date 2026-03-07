# Album Design（相册页设计说明）

## 1. 目标

相册页设计目标：

- 在大量照片场景保持稳定内存占用
- 提供连续纵向滚动的九宫格浏览体验
- 支持选择、全选、删除等批量操作
- 提供右侧快速滚动条和滚动进度提示
- 与预览页联动，支持返回后定位到目标照片

## 2. 页面结构

页面由两部分组成：

- 顶部导航栏（普通模式 / 选择模式）
- 内容区（可滚动网格 + 右侧快速滚动条）

普通模式控件：返回、拍照、录像、选择。  
选择模式控件：取消、全选、已选数量、删除。

## 3. 核心数据结构

主要定义位于 `include/pages/page_album.h`：

- `album_layout_config_t`
  - 网格布局参数：`item_width/item_height/gap_x/gap_y/cols/start_x/row_height/visible_rows/pool_rows`
- `album_item_t`
  - 复用池 item：`container/img/select_box/index/photo_index/is_visible`
- `page_album_data_t`
  - 页面对象：导航栏、网格容器、滚动条、遮罩
  - 列表状态：`total_photos/first_visible_row/item_pool/pool_size`
  - 滚动条缓存：`fast_scrollbar_range_max/fast_scrollbar_last_value/syncing_fast_scrollbar`
  - 选择删除状态：`selection_mode/selected_flags/selected_count/deleting_in_progress`
  - 防误触状态：`is_scrolling/last_scroll_end_tick/item_press_scroll_y/item_press_valid`

## 4. 虚拟列表与布局

### 4.1 自动布局

- `calculate_layout()` 根据容器尺寸自动计算列数与可见行数
- 单元固定尺寸：`200x140`，间距 `4x4`
- 复用池行数 = 可见行数 + 缓冲行（`GRID_BUFFER_ROWS=2`）

### 4.2 复用池

- `create_item_pool()` 只创建 `cols * pool_rows` 个 item
- 滚动时通过 `refresh_visible_items()` 复用对象并更新位置/内容
- 避免滚动过程中频繁创建销毁 LVGL 对象

### 4.3 索引与数据来源

- 当前实现中 `display_index` 直接作为 `photo_index` 使用
- 缩略图通过 `file_manager_get_photo_thumbnail_path(photo_index, ...)` 获取
- 展示顺序依赖 `file_manager` 当前照片列表顺序

## 5. 快速滚动条与进度提示

### 5.1 滚动条样式

- 使用 `lv_slider` 作为右侧快速滚动条
- 主轨和指示轨透明，仅保留 knob，扩大触控区域（`FAST_SCROLLBAR_TOUCH_PAD=10`）

### 5.2 同步关系

为匹配现有视觉方向，使用反向映射：

- 列表滚动 -> slider：`slider_value = max_scroll_y - scroll_y`
- slider 拖动 -> 列表滚动：`target_y = max_scroll_y - slider_value`

### 5.3 同步优化

- 通过 `fast_scrollbar_range_max/fast_scrollbar_last_value` 避免重复 set
- 使用 `syncing_fast_scrollbar` 防止列表与 slider 互相回调
- 首次进入后通过 `lv_async_call(sync_fast_scrollbar_deferred_cb, ...)` 做一次延迟同步

### 5.4 进度提示

- `show_fast_scrollbar_progress_notice()` 显示 `x/total`
- `x` 为当前视口底部可见行对应的最后一张照片序号（1-based）
- 使用 `top_notice_show_for(..., 600ms)` 进行短时提示

## 6. 点击、防误触与手势

### 6.1 item 点击行为

- `CLICKED`：普通模式进入预览；选择模式切换选中状态
- `LONG_PRESSED`：进入选择模式并切换当前项选中
- `suppress_next_item_click`：长按后抑制后续一次点击，避免重复触发

### 6.2 滚动防误触策略

为避免九宫格快速滑动被误判为点击：

- item 事件补充 `LV_EVENT_PRESSED`、`LV_EVENT_PRESS_LOST`
- `PRESSED` 记录按下时 `scroll_y`
- `PRESS_LOST` 清理按下状态
- `CLICKED/LONG_PRESSED` 前执行门禁判断，满足任一条件即拦截：
  - `is_scrolling == true`
  - `lv_tick_elaps(last_scroll_end_tick) < 180ms`
  - 按下到抬起期间滚动位移 `>= 8px`

### 6.3 滚动状态维护

- `scroll_event_cb()`：
  - `LV_EVENT_SCROLL_BEGIN`：`is_scrolling = true`
  - `LV_EVENT_SCROLL_END`：`is_scrolling = false`，并记录 `last_scroll_end_tick`
- 页面 `show/hide` 时重置防误触状态，避免跨页面残留

### 6.4 返回手势

- 页面容器注册 `gesture_back` 左边缘右滑返回
- `gesture_back_enable_event_bubble_recursive()` 让子控件事件向页面容器汇聚

## 7. 选择与删除流程

### 7.1 选择模式

- 通过“选择”按钮或 item 长按进入
- 支持单项切换、全选
- 顶栏实时显示“已选择 N 项”

### 7.2 删除流程

`delete_selected_photos()` 的关键步骤：

- 记录删除前 `scroll_y`
- 设置 `deleting_in_progress`，并通过 `key_manager_set_block_non_power()` 屏蔽触控与按键输入（保留电源键）
- 显示阻塞提示与遮罩
- 按索引倒序删除，避免前删后索引变化影响遍历
- 删除完成后恢复输入屏蔽、刷新列表与内容高度
- 退出选择模式，滚动位置尽量保持在删除前附近

## 8. 与预览页联动

- 相册进入预览前调用 `page_photo_preview_set_initial_photo_index(photo_index)`
- 从预览返回时，预览页会设置 `page_album_set_focus_photo_index(...)`
- `page_album_show()` 中读取该索引并计算 `target_scroll_y`，使目标项尽量居中可见

## 9. 生命周期与资源管理

- `create`：创建 UI、计算布局、创建复用池、绑定事件
- `show`：刷新总数与滚动状态，处理预览回跳定位
- `hide`：退出选择态、清理提示与防误触状态
- `destroy`：销毁复用池、释放选中缓存、删除页面容器

## 10. 依赖接口

相册页依赖 `file_manager`：

- `file_manager_refresh_photo_list`
- `file_manager_get_photo_count`
- `file_manager_get_photo_thumbnail_path`
- `file_manager_delete_photo_by_index`

依赖 `key_manager`：

- `key_manager_get_block_non_power`
- `key_manager_set_block_non_power`

## 11. 主要文件

- 页面实现：`src/pages/page_album.c`
- 页面数据结构：`include/pages/page_album.h`
- 预览联动页：`src/pages/page_photo_preview.c`

## 12. 已知边界与后续建议

- 目前 `photo_btn/video_btn` 仅打印日志，后续可接入实际跳转
- 当前布局在 `show` 阶段不重算列数，如分辨率/方向动态变化可补充重算逻辑
- 可继续优化缩略图缓存策略（如 LRU）以降低 I/O 抖动
