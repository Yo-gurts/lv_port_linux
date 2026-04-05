# photo_preview 滑动交互设计说明

## 1. 目标

在 `page_photo_preview` 内统一处理以下交互：

- 照片左右滑动跟手预览
- 松手后切换或撤销
- 左/右边缘滑动返回上一页

避免依赖全局 `gesture_back` 的 `LV_EVENT_GESTURE` 抢占，保证预览页拖拽连续性。

## 2. 核心方案

采用“双图层 + 跟手位移”方案：

- `current_slide/current_image`：当前照片
- `target_slide/target_image`：目标照片（下一张或上一张）

`PRESSING` 阶段实时更新两个图层的 `x`，形成跟手效果；
`RELEASED` 阶段决定“提交切换”或“撤销回弹”。

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

## 4. 边缘返回规则

返回也在本页统一处理，规则与原 `gesture_back` 语义一致：

- 左边缘起滑 + 向右 => 返回
- 右边缘起滑 + 向左 => 返回

边缘阈值沿用 `SWIPE_BACK_EDGE_THRESHOLD_PX`。

## 5. 事件模型

在 `container` 监听：

- `LV_EVENT_PRESSED`
- `LV_EVENT_PRESSING`
- `LV_EVENT_RELEASED`
- `LV_EVENT_PRESS_LOST`

同时为 `image_area` 子树开启 `LV_OBJ_FLAG_EVENT_BUBBLE`，确保按在图片上时事件可冒泡到 `container`。

## 6. 状态字段

`page_photo_preview_data_t` 新增关键字段：

- 视图层：`current_slide/current_image`、`target_slide/target_image`
- 拖拽：`drag_start_x`、`drag_last_x`、`drag_offset_x`
- 方向：`drag_direction`、`drag_last_move_dir`
- 目标索引：`drag_target_index`
- 动画：`swipe_anim_running`、`swipe_commit_on_anim_end`

## 7. 与 gesture_back 的关系

本页不再接入 `gesture_back_register_events` / `gesture_back_set_left_edge_swipe_cb`。

原因：`gesture_back` 的 `LV_EVENT_GESTURE` 流程会调用 `lv_indev_wait_release`，可能打断本页连续 `PRESSING` 跟手逻辑。
