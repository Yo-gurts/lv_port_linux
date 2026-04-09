# 输入系统设计文档

## 1. 设计目标

统一触摸、物理键、输入屏蔽与手势返回行为，避免各页面各自实现导致冲突。

目标：
- 输入事件从一个入口收敛（key_manager + gesture_back）
- 可按场景屏蔽输入（比如模式切换、格式化处理中）
- 手势返回行为跨页面一致

## 2. 组成模块

### 2.1 key_manager
负责物理键采集、状态机、事件分发、TP 屏蔽控制。

### 2.2 gesture_back
负责页面容器上的左边缘右滑返回判定。

### 2.3 页面层
负责绑定/解绑页面级回调，管理页面特有输入门禁（例如相册误触保护）。

## 3. 事件链路

1) 底层输入设备事件进入 key_manager
2) key_manager 转换为业务事件并按规则分发（key+event / any 组合）
3) 页面按需消费事件
4) 页面容器上的手势事件由 gesture_back 判定后触发 `page_manager_back`

## 4. 屏蔽位图策略

位定义：
- `KEY_INPUT_BLOCK_TP`
- `KEY_INPUT_BLOCK_POWER_KEY`
- `KEY_INPUT_BLOCK_ADC_KEY2`
- `KEY_INPUT_BLOCK_NON_CAMERA_KEYS`

常见场景：
- 模式切换：`TP + ADC` 临时屏蔽
- 页面危险操作处理中：按需屏蔽非必要输入
- 手势冲突页面：配合页面内门禁避免误触

## 5. 手势返回约束

推荐规则：
- 事件只在 `create()` 注册一次，不在 `show()` 重复注册
- 页面容器 clear `GESTURE_BUBBLE`
- 子控件 add `EVENT_BUBBLE`，让事件向页面容器汇聚
- 右滑返回需同时满足：左边缘起滑 + 同目标 + 时间窗

## 6. 仿真与真机一致性

- FB 路径：触摸 indev 绑定 key_manager
- SDL 路径：mouse indev 也绑定 key_manager

这样 `KEY_INPUT_BLOCK_TP` 在仿真/真机行为一致，联调结果更可信。

## 7. 页面级误触门禁（以相册九宫格为例）

- 滚动中拦截 item 点击
- 滚动结束后冷却窗口
- 按下到抬起位移阈值判定

作用：防止快速滑动尾段被误识别为点击。

## 8. 扩展建议

- 按键音建议作为独立横切模块（sound_feedback_manager），挂在 key_manager dispatch 后
- 对需要埋点的输入事件，可通过 ANY_KEY + ANY_EVENT 高优先级订阅实现全局观察
