# Power Manager 设计文档

## 1. 概述

`power_manager` 负责 UI 侧电源相关行为，当前覆盖：

- 自动息屏（无操作超时）
- 输入唤醒亮屏（按键 / 触摸）
- 电源键短按背光切换
- 电源键长按 3 秒分阶段关机

模块定位是“策略层”，不管理页面 UI 细节；页面通过接口控制“临时禁用自动息屏”。

## 2. 相关文件

- 头文件：`include/core/power_manager.h`
- 核心实现：`src/core/power_manager.c`
- mock 实现：`src/mock/power_manager.c`

## 3. 对外接口

```c
int power_manager_init(void);
void power_manager_deinit(void);
void power_manager_poll(void);

void power_manager_mark_activity(void);
void power_manager_disable_auto_sleep(void);
void power_manager_enable_auto_sleep(void);

typedef int (*power_manager_shutdown_prepare_cb_t)(void* user_data);
void power_manager_register_shutdown_prepare_cb(power_manager_shutdown_prepare_cb_t cb, void* user_data);
void power_manager_unregister_shutdown_prepare_cb(power_manager_shutdown_prepare_cb_t cb, void* user_data);
```

说明：

- `power_manager_poll()` 需要在主循环周期调用。
- `disable/enable_auto_sleep` 是“临时屏蔽自动息屏”，常用于拍照/录像页面。
- 自动息屏总开关不在 `power_manager` 内维护，统一走 `param_manager`：
  - 参数项：`PARAM_ID_AUTO_SLEEP`（0=关闭，1=开启）

## 4. 自动息屏策略

### 4.1 超时阈值

- 配置宏：`POWER_MANAGER_SCREEN_IDLE_TIMEOUT_MS`
- 超时条件：`now_ms - g_last_activity_ms >= threshold`

### 4.2 生效条件（必须同时满足）

- 屏幕当前为亮屏
- 临时屏蔽计数 `g_disable_auto_sleep_depth == 0`
- 系统总开关 `PARAM_ID_AUTO_SLEEP == 1`

否则即使到达超时也不会自动息屏。

### 4.3 优先级

从高到低：

1. 系统设置总开关（`PARAM_ID_AUTO_SLEEP`）
2. 页面临时禁用（`disable/enable_auto_sleep`）
3. 空闲超时自动息屏逻辑

即：系统总开关关闭时，自动息屏完全失效；但电源键短按熄屏仍有效。

## 5. 输入唤醒策略

当屏幕处于熄屏状态时：

- 任意非电源键输入会先唤醒亮屏；
- 触摸按下边沿会唤醒亮屏；
- 电源键短按由专门路径处理：
  - 熄屏时：先亮屏并结束本次 click 语义；
  - 亮屏时：执行短按背光切换（拍照/录像页面除外）。

## 6. 电源键行为

### 6.1 短按（`KEY_EVENT_CLICK`）

- 若当前页面是 `photo` 或 `video`：忽略短按背光切换；
- 否则切换背光开关（亮 <-> 灭）；
- 若当前已熄屏，短按仅唤醒亮屏，不立刻再切回熄屏。

### 6.2 长按 3 秒（`KEY_EVENT_LONG_PRESS_3S`）

- 阶段 1：进入“假关机”（先熄屏）
- 调用可选的 `shutdown_prepare_cb`，例如录像页先停录
- 设置等待标记，等待按键释放

### 6.3 长按释放（`KEY_EVENT_LONG_PRESS_3S_RELEASE`）

- 阶段 2：执行正式关机
- 当前实现：`sync(); reboot(RB_POWER_OFF);`

## 7. 与系统设置页联动

系统设置页“自动息屏”开关直接读写 `PARAM_ID_AUTO_SLEEP`：

- 打开：`param_manager_set(PARAM_ID_AUTO_SLEEP, 1)`
- 关闭：`param_manager_set(PARAM_ID_AUTO_SLEEP, 0)`

页面显示通过参数回调和 show 刷新同步，避免状态陈旧。

## 8. 轮询与时序

建议主循环顺序：

```c
key_manager_poll();
power_manager_poll();
param_manager_poll();
lv_timer_handler();
```

这样可保证：

- 本轮输入先更新
- 电源策略再判定（唤醒/息屏）
- 参数回调最后派发到 UI

## 9. 设计约束

- `power_manager` 不保存系统自动息屏总开关，统一以 `param_manager` 为单一数据源。
- 避免“通配按键回调”和“电源键专用回调”重复处理同一语义，防止背光被反向切回。
- 日志统一使用中文，便于板端定位问题。
