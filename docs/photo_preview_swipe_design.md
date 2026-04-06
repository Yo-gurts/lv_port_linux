# photo_preview 滑动交互设计说明

## 1. 目标

在 `page_photo_preview` 内统一处理以下交互：

- 照片左右滑动跟手预览
- 松手后切换或撤销
- 左/右边缘滑动返回上一页

在保留本页翻页跟手体验的同时，复用全局 `gesture_back` 处理边缘返回提示与触发。

## 2. 核心方案

采用“双图层 + 跟手位移”方案：

- `current_slide/current_image`：当前照片
- `target_slide/target_image`：目标照片（下一张或上一张）

`PRESSING` 阶段实时更新两个图层的 `x`，形成跟手效果；
`RELEASED` 阶段决定“提交切换”或“撤销回弹”。

对于“边缘起点”手势（命中 `SWIPE_BACK_EDGE_THRESHOLD_PX`）进入返回模式：

- 只跟踪方向用于返回判定
- 不加载 `target_image`
- 不显示目标页预览
- 返回提示图标与最终返回触发由 `gesture_back` 统一处理

## 3. 方向判定规则

### 3.1 初始方向

当总位移超过 `PREVIEW_DRAG_DEADZONE_PX` 后，锁定初始方向：

- 向左：`PREVIEW_DRAG_DIR_NEXT`
- 向右：`PREVIEW_DRAG_DIR_PREV`

### 3.2 最后方向

在每次 `PRESSING` 里，基于当前点与上一次点的差值记录最近移动方向：

- `move_step_x < 0` => `NEXT`
- `move_step_x > 0` => `PREV`

### 3.3 抬手提交条件

抬手时仅比较“最后方向”和“初始方向”：

- 一致：提交切换
- 不一致：撤销本次滑动（回弹）

这可覆盖“先左滑后右滑”场景，满足用户撤销意图。

## 4. 边缘返回规则（由 gesture_back 统一）

返回规则由 `gesture_back` 统一执行：

- 左边缘起滑 + 向右 + 距离达到阈值 + 抬手 => 返回
- 右边缘起滑 + 向左 + 距离达到阈值 + 抬手 => 返回

本页仅负责在边缘起点时禁用翻页提交：

- 不加载 `target_image`
- 不提交翻页动画
- 直接复位本页翻页状态，避免与 `gesture_back` 竞争

## 5. 事件模型

在 `container` 监听：

- `LV_EVENT_PRESSED`
- `LV_EVENT_PRESSING`
- `LV_EVENT_RELEASED`
- `LV_EVENT_PRESS_LOST`

同时接入 `gesture_back`：

- `gesture_back_register_events(data->container)`
- `gesture_back_enable_event_bubble_recursive(data->container)`
- `gesture_back_set_left_edge_swipe_cb(data->container, back_btn_cb)`

生命周期约束：

- `show()` 重新调用 `gesture_back_set_left_edge_swipe_cb`，抢回活跃容器
- `hide()` 调用 `gesture_back_clear_active_swipe_cb(data->container)` 主动安全解绑

## 6. 状态字段

`page_photo_preview_data_t` 新增关键字段：

- 视图层：`current_slide/current_image`、`target_slide/target_image`
- 拖拽：`drag_start_x`、`drag_last_x`、`drag_offset_x`
- 方向：`drag_direction`、`drag_last_move_dir`
- 目标索引：`drag_target_index`
- 动画：`swipe_anim_running`、`swipe_commit_on_anim_end`

## 7. 与 gesture_back 的关系

本页已接入并复用 `gesture_back`，职责划分如下：

- `page_photo_preview`：负责翻页跟手、方向一致性判定、翻页动画提交/撤销
- `gesture_back`：负责边缘返回图标提示、返回阈值判定、抬手触发返回

当前 `gesture_back` 基于 `PRESSED/PRESSING/RELEASED/PRESS_LOST`，不依赖 `LV_EVENT_GESTURE`，不会调用 `lv_indev_wait_release` 打断本页跟手。
