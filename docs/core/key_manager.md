# Key Manager 设计文档

## 1. 概述

Key Manager 统一处理物理按键与触摸屏蔽策略，负责：
- 输入设备轮询（power-key / adc-key2）
- 按键事件状态机（按下、抬起、短按、长按、连发、3秒长按）
- 回调注册/注销（支持多订阅者）
- 输入屏蔽位图管理（TP、Power、ADC、非拍照键）

默认内置行为：
- 音量键在 CLICK / LONG_PRESS / LONG_PRESS_REPEAT 上执行音量增减

## 2. 按键与事件模型

### 2.1 按键枚举
- `KEY_ID_POWER`
- `KEY_ID_AI`
- `KEY_ID_VOLUME_UP`
- `KEY_ID_VOLUME_DOWN`
- `KEY_ID_FOCUS`
- `KEY_ID_CAMERA`
- `KEY_ID_ANY`（通配）

### 2.2 事件枚举
- `KEY_EVENT_CLICK`
- `KEY_EVENT_PRESS`
- `KEY_EVENT_RELEASE`
- `KEY_EVENT_LONG_PRESS`
- `KEY_EVENT_LONG_PRESS_3S`
- `KEY_EVENT_LONG_PRESS_3S_RELEASE`
- `KEY_EVENT_LONG_PRESS_REPEAT`
- `KEY_EVENT_ANY`（通配）

说明：业务文档若只写“5类事件”已过时，当前实现包含 PRESS/RELEASE 与 3S 相关事件。

## 3. 回调分发机制

## 3.1 匹配路径
一次事件按如下顺序匹配：
1) `key + event`
2) `key + ANY_EVENT`
3) `ANY_KEY + event`
4) `ANY_KEY + ANY_EVENT`

## 3.2 多订阅者与优先级
同一个 bucket 支持多个回调：
- `priority` 越大越先执行
- `stop_propagation=1` 时执行后中断后续回调

接口：
```c
int key_manager_register_callback_with_priority(
    key_id_t key,
    key_event_type_t event_type,
    key_event_callback_t callback,
    void* user_data,
    int priority,
    uint8_t stop_propagation);
```

兼容接口：
```c
int key_manager_register_callback(key_id_t key, key_event_type_t event_type,
    key_event_callback_t callback, void* user_data);
```
等价于 `priority=0, stop_propagation=0`。

## 3.3 注销规则
`key_manager_unregister_callback(...)` 按 `(key,event,callback,user_data)` 精确匹配注销。

## 4. 输入屏蔽策略

屏蔽位图：
- `KEY_INPUT_BLOCK_TP`
- `KEY_INPUT_BLOCK_POWER_KEY`
- `KEY_INPUT_BLOCK_ADC_KEY2`
- `KEY_INPUT_BLOCK_NON_CAMERA_KEYS`

调用：
```c
void key_manager_set_block_non_power(uint8_t block_mask);
uint8_t key_manager_get_block_non_power(void);
```

行为约束：
- 切到屏蔽态会清空对应按键状态，避免解除后补发旧 click/long 事件
- TP 屏蔽通过 `lv_indev_enable(indev, false)` 生效
- SDL 模式下 mouse indev 也绑定到 key_manager，保证仿真与真机一致

## 5. 线程与调用时序

- key_manager 本身不创建 UI 线程外回调线程
- `key_manager_poll()` 在 GUI 主循环周期调用
- 回调中如涉及 UI 操作，默认处于 GUI 线程上下文

典型主循环：
```c
while (1) {
    key_manager_poll();
    lv_timer_handler();
    usleep(5000);
}
```

## 6. 常见误区

1) 误区：同一个 key+event 只能注册一个回调。  
   现状：已支持多回调，按优先级执行。  

2) 误区：可“直接重新注册覆盖”默认行为。  
   现状：不会覆盖已有回调；若需替换行为，可先注销旧回调或使用更高优先级并截断传播。  

3) 误区：仅屏蔽 TP 就能阻断所有输入。  
   现状：物理按键仍会生效，需结合 ADC/POWER 屏蔽位。

## 7. 相关文件

- `include/core/key_manager.h`
- `src/core/key_manager.c`
- `src/core/ui_common.c`（SDL/FB 输入绑定）
