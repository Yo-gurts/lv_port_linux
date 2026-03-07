# gesture_back 设计说明

## 1. 背景

页面边缘滑动返回在交互上要求稳定、可预期，不能和普通点击冲突。

当前实现抽离为 `gesture_back` 模块，页面在 `create()`/`show()` 分别调用：

```c
/* create() */
gesture_back_register_events(data->container);

/* show() */
gesture_back_set_left_edge_swipe_cb(data->container, page_manager_back_cb);
```

## 2. 要解决的问题

边缘滑动返回主要有两类误触风险：

1. 慢速拖动误触发  
触摸拖得很慢时，用户意图可能不是“返回”。

2. 先点击，再滑动误触发  
用户先在页面点击了一次（例如点按钮），随后再滑动，如果仍沿用旧的按下状态，可能被误判为右滑返回。

第二类是本次重点：时间窗不仅用于过滤慢速拖动，更用于切断“先点后滑”的事件串联误判。

## 3. 核心策略

`gesture_back` 在 `LV_EVENT_PRESSED` 与 `LV_EVENT_GESTURE` 之间做联合判定：

- 必须是同一个活跃页面容器
- 必须从边缘起滑（`SWIPE_BACK_EDGE_THRESHOLD_PX`）
  - 左边缘右滑
  - 右边缘左滑
- 必须在时间窗内完成（`SWIPE_BACK_PRESS_GESTURE_MAX_MS`）

只有同时满足以上条件才触发返回回调。

另外，事件冒泡策略采用：

- 页面容器：`clear LV_OBJ_FLAG_EVENT_BUBBLE`
- 子孙控件：`add LV_OBJ_FLAG_EVENT_BUBBLE`

目的：让子控件按压事件可以汇聚到页面容器用于手势判定，但不继续向更上层冒泡。

## 4. 时间窗设计要点

### 4.1 判定方式

- `PRESSED` 时记录：
  - 起点坐标
  - 起始 tick
  - 目标容器
- `GESTURE` 时校验：
  - `elapsed = lv_tick_elaps(start_tick)`
  - `elapsed <= SWIPE_BACK_PRESS_GESTURE_MAX_MS`

### 4.2 设计收益

- 过滤慢速拖动导致的误返回
- 防止“先点击一下，再去滑动”被错误识别成返回
- 页面切换后通过 `show()` 重绑当前回调，避免跨页面状态污染

## 5. 当前实现形态（单实例）

模块只维护一个“当前活跃页面”的回调，不做页面缓存表：

- `create()` 注册固定事件回调（`gesture_back_event_cb`）
- 进入页面（`show`）时仅调用 `gesture_back_set_left_edge_swipe_cb(...)` 刷新当前回调
- 事件回调中只处理当前活跃容器
- `show()` 阶段不注册事件回调（避免重复挂载）

当前仅注册两个事件：

- `LV_EVENT_PRESSED`
- `LV_EVENT_GESTURE`

这能保持逻辑简单，并符合当前页面生命周期。

## 6. 配置项

位于 `include/config.h`：

- `SWIPE_BACK_EDGE_THRESHOLD_PX`：边缘起滑阈值（左/右边缘）
- `SWIPE_BACK_PRESS_GESTURE_MAX_MS`：`PRESSED -> GESTURE` 最大时间窗
