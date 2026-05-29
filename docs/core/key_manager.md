# Key Manager 设计文档

## 1. 概述

Key Manager 用于统一管理物理按键输入，提供：

- 多输入设备采集（`/dev/input/power-key`、`/dev/input/adc-key2`）
- 按键事件抽象（短按、长按、长按连发）
- 按键回调注册/注销
- 输入源按位屏蔽能力（TP / power-key / adc-key2）

`key_manager_init()` 内置注册了音量键默认行为：

- `KEY_ID_VOLUME_UP`：每次事件音量 `+10%`
- `KEY_ID_VOLUME_DOWN`：每次事件音量 `-10%`
- 生效事件：`KEY_EVENT_CLICK`、`KEY_EVENT_LONG_PRESS`、`KEY_EVENT_LONG_PRESS_REPEAT`

当前实现采用**主线程轮询**模型，不额外创建线程，避免 LVGL 线程安全问题。

## 2. 按键来源与映射

### 2.1 输入设备

- `power_key`：`/dev/input/power-key`
- 其他键：`/dev/input/adc-key2`

### 2.2 Linux key code 映射

- `KEY_POWER` -> `KEY_ID_POWER`
- `KEY_ASSISTANT(207)` -> `KEY_ID_ASSISTANT`
- `KEY_VOLUMEUP(115)` -> `KEY_ID_VOLUME_UP`
- `KEY_VOLUMEDOWN(114)` -> `KEY_ID_VOLUME_DOWN`
- `KEY_CAMERA_FOCUS(528)` -> `KEY_ID_FOCUS`
- `KEY_CAMERA(212)` -> `KEY_ID_CAMERA`

## 3. 事件模型

支持五类业务事件：

- `KEY_EVENT_CLICK`：短按（按下后在长按阈值前抬起）
- `KEY_EVENT_LONG_PRESS`：达到长按阈值触发一次，默认700ms
- `KEY_EVENT_LONG_PRESS_3S`：按下持续达到 3 秒触发一次（支持所有按键）
- `KEY_EVENT_LONG_PRESS_3S_RELEASE`：触发过 3 秒长按后，抬起时触发一次
- `KEY_EVENT_LONG_PRESS_REPEAT`：长按后按固定周期持续触发

默认参数：

- 长按阈值：`700ms`
- 连发间隔：`200ms`
- `LONG_PRESS_3S` 阈值：固定 `3000ms`

可通过接口动态调整：

- `key_manager_set_long_press_ms()`
- `key_manager_set_repeat_ms()`

## 4. 回调分发设计

### 4.1 二维映射表

回调存储使用二维表：

- 第一维：`key bucket`（`KEY_ID_POWER..KEY_ID_CAMERA` + `KEY_ID_ANY`）
- 第二维：`event bucket`（`CLICK/LONG/REPEAT` + `KEY_EVENT_ANY`）

即：`callback_map[key_bucket][event_bucket]`

说明：

- `KEY_EVENT_BUTT` 是事件枚举的边界值（上界），不是可注册事件。
- `KEY_EVENT_ANY` 是可注册的通配事件（语义上与 `BUTT` 不同）。
- 实现中为了使用数组索引，会把 `KEY_EVENT_ANY(-1)` 映射到最后一个槽位（索引 `KEY_EVENT_BUTT`）。
- 因此“ANY bucket 使用 BUTT 索引”只是内部存储映射，不代表 `ANY == BUTT`。

### 4.2 分发规则

一次按键事件会按 4 条路径分发：

1. `key + event`
2. `key + ANY_EVENT`
3. `ANY_KEY + event`
4. `ANY_KEY + ANY_EVENT`

### 4.3 注册限制

当前每个 `key+event` 仅允许注册一个回调，避免分发顺序不确定。

## 5. 屏蔽策略（位图）

提供 `key_manager_set_block_non_power(uint8_t block_mask)`，参数为位图：

- `KEY_INPUT_BLOCK_TP`：屏蔽 TP 输入（通过 `lv_indev_enable(indev, false)`）
- `KEY_INPUT_BLOCK_POWER_KEY`：屏蔽 `power-key`（`KEY_ID_POWER`）
- `KEY_INPUT_BLOCK_ADC_KEY2`：屏蔽 `adc-key2`（AI/音量/对焦/拍照等）

支持按位组合：

- 仅屏蔽 TP：`KEY_INPUT_BLOCK_TP`
- 仅屏蔽 ADC：`KEY_INPUT_BLOCK_ADC_KEY2`
- 屏蔽 TP + ADC：`KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_ADC_KEY2`

为避免“补发旧事件”，在屏蔽状态切换到开启时会清空对应按键状态机（按下状态、长按计时、连发计时）。

## 6. 线程与时序

### 6.1 为什么不单独起线程

- 工程约束：尽量避免线程，优先主线程处理 UI 相关输入
- LVGL 非线程安全，线程化后仍需回主线程操作 UI，复杂度更高
- 当前实现使用 `poll(..., 0)` + 非阻塞 `read`，不会阻塞 `lv_timer_handler()`

### 6.2 调用位置

在 `ui_main()` 主循环每帧调用：

```c
while (1) {
    key_manager_poll();
    lv_timer_handler();
    usleep(5000);
}
```

## 7. API 参考

```c
int key_manager_init(void);
void key_manager_deinit(void);
void key_manager_bind_touch_indev(struct _lv_indev_t* indev);
void key_manager_poll(void);

int key_manager_register_callback(key_id_t key, key_event_type_t event_type,
    key_event_callback_t callback, void* user_data);
int key_manager_unregister_callback(key_id_t key, key_event_type_t event_type,
    key_event_callback_t callback, void* user_data);

void key_manager_set_long_press_ms(uint32_t long_press_ms);
void key_manager_set_repeat_ms(uint32_t repeat_ms);

/* KEY_INPUT_BLOCK_TP / KEY_INPUT_BLOCK_POWER_KEY / KEY_INPUT_BLOCK_ADC_KEY2 */
void key_manager_set_block_non_power(uint8_t block_mask);
uint8_t key_manager_get_block_non_power(void);
```

## 8. 使用示例

```c
static void on_key_event(key_id_t key, key_event_type_t type, void* user_data)
{
    LV_UNUSED(user_data);
    if (key == KEY_ID_VOLUME_UP && type == KEY_EVENT_CLICK) {
        /* do something */
    }
}

void app_key_init(void)
{
    key_manager_init();
    key_manager_register_callback(KEY_ID_ANY, KEY_EVENT_LONG_PRESS, on_key_event, NULL);
}
```

说明：

- 音量键（`KEY_ID_VOLUME_UP/DOWN`）默认逻辑已在 `key_manager_init()` 中注册。
- 若业务要覆盖默认音量行为，可在初始化后对对应 `key+event` 重新注册或先注销再注册。

## 9. 相关文件

- `include/core/key_manager.h`
- `src/core/key_manager.c`
- `src/ui_main.c`
