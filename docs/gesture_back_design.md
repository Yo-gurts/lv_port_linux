# gesture_back 设计说明

## 1. 目标

`gesture_back` 提供统一的边缘滑动返回能力，并增加类似手机的可视化提示：

- 滑动过程有跟手提示（top layer）
- 仅在抬手时触发返回
- 左右边缘都支持（左边缘右滑 / 右边缘左滑）

## 2. 页面接入方式

页面在 `create()` 阶段注册事件，在 `show()` 阶段激活当前页面回调，在 `hide()` 阶段安全解绑：

```c
/* create() */
gesture_back_register_events(data->container);
gesture_back_enable_event_bubble_recursive(data->container);

/* show() */
gesture_back_set_left_edge_swipe_cb(data->container, page_manager_back_cb);

/* hide() */
gesture_back_clear_active_swipe_cb(data->container);
```

说明：

- 仅“需要返回手势”的页面接入（例如 Home 页面不接入）
- 模块采用单实例活跃页策略，`show()` 时重绑活跃容器与回调
- `gesture_back_clear_active_swipe_cb` 只会在“当前活跃容器匹配”时解绑，避免误清空其他页面绑定

## 3. 事件与状态机

当前实现事件：

- `LV_EVENT_PRESSED`
- `LV_EVENT_PRESSING`
- `LV_EVENT_RELEASED`
- `LV_EVENT_PRESS_LOST`

状态流程：

1. `PRESSED`  
记录起点坐标与目标容器，判定是否命中左右边缘。

2. `PRESSING`  
计算拖动距离，驱动提示层跟手移动，并更新“是否达到触发阈值”。

3. `RELEASED`  
仅当满足“边缘起滑 + 方向正确 + 达到阈值”时触发返回回调。  
未达阈值则只收起提示，不返回。

4. `PRESS_LOST`  
仅清理状态，不触发返回。

## 4. 触发规则

返回触发必须同时满足：

- 当前事件来自活跃页面容器
- 起点位于边缘阈值内（`SWIPE_BACK_EDGE_THRESHOLD_PX`）
- 方向正确：
  - 左边缘起滑，向右拖动
  - 右边缘起滑，向左拖动
- 拖动距离达到内部阈值（当前实现：`SWIPE_BACK_TRIGGER_DISTANCE_PX = 90`）
- 在 `RELEASED` 才真正调用回调

备注：当前版本不再使用“按下到抬手的总时长”限制；达到距离阈值后可停顿再松手，仍会触发返回。

## 5. UI 提示层设计

提示层挂载在 `lv_layer_top()`，不依附页面容器，避免页面内容被一起拖动。

视觉形态：

- 仅显示返回图标（`res/icons/back.png`），无额外背景框
- 根据左右边缘方向切换图标旋转方向
- 达到触发阈值时图标全不透明；未达到时 50% 透明

跟手行为：

- 图标沿边缘内侧做有限位移（由拖动距离线性映射）
- 位置带边界钳制，确保不越界裁切

## 6. 事件冒泡与滚动约束

为保证手势稳定性：

- 页面根容器关闭可滚动：`lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE)`
- 页面根容器不向上冒泡：`clear LV_OBJ_FLAG_EVENT_BUBBLE`
- 子对象递归开启事件冒泡：`add LV_OBJ_FLAG_EVENT_BUBBLE`
- 关闭 `GESTURE_BUBBLE`，避免手势上抛到更高层造成干扰

## 7. 关键参数

外部配置（`include/config.h`）：

- `SWIPE_BACK_EDGE_THRESHOLD_PX`：边缘命中阈值

模块内部参数（`src/ui/gesture_back.c`）：

- `SWIPE_BACK_TRIGGER_DISTANCE_PX`：返回触发距离阈值
- `SWIPE_HINT_EDGE_OFFSET_PX`：贴边偏移
- `SWIPE_HINT_MAX_SLIDE_PX`：最大跟手位移
- `SWIPE_HINT_VISUAL_MARGIN_PX`：图标位置边界留白

## 8. 已知限制

- 当前提示层为单实例；同一时刻仅服务当前活跃页面
- 接入页面需要在 `show()` 重绑活跃容器；建议在 `hide()` 主动安全解绑
