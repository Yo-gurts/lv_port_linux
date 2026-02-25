# Param Manager 设计文档

## 1. 目标

`param_manager` 负责统一管理系统参数，提供：

- 参数存储与读取
- 参数合法性校验（在 `set` 时）
- 参数变更通知（callback）
- 线程安全更新

---

## 2. 职责边界

`param_manager` 负责：

- 保存参数当前值与默认值
- 对参数写入做范围校验
- 提供统一回调通知参数变化

`param_manager` 不负责：

- UI 控件操作
- 媒体业务动作执行（拍照、模式切换等）
- 页面跳转

---

## 3. 数据模型

- 参数 ID：`param_id_t`
- 参数默认值：`default_values[]`
- 参数规则：`param_rules[]`（`min/max/validate_enabled`）
- 当前值：`current_values[]`

规则说明：

- `param_manager_set()` 校验参数 ID 与取值范围
- `param_manager_get()` 不做值修正，不兜底回退

---

## 4. 回调模型（重点）

当前 callback 是**参数广播模型**，不是“事件类型分发模型”：

- 回调签名：`callback(param_id_t id, int value, void* user_data)`
- 任意参数变化时，所有已注册 callback 都会收到一次通知
- callback 自己根据 `id` 过滤是否关心该参数

即：

- 不是“注册某个参数专属回调”
- 而是“注册全局参数变化监听”

---

## 5. 线程安全与 LVGL 安全

### 5.1 问题

如果后台线程调用 `param_manager_set()`，而 callback 里直接改 LVGL 控件，会触发线程安全问题。

### 5.2 方案

采用“写入线程与回调线程分离”：

- `set/reset_all`：仅更新参数并标记 `pending`
- `poll`：在主线程中派发 callback

这样 callback 运行在 UI 主线程，可安全更新 LVGL。

### 5.3 运行要求

主循环必须周期调用：

```c
while (1) {
    key_manager_poll();
    param_manager_poll();
    lv_timer_handler();
    usleep(5000);
}
```

---

## 6. API 说明

```c
int param_manager_init(void);
void param_manager_deinit(void);

int param_manager_get(param_id_t id);
int param_manager_set(param_id_t id, int value);
int param_manager_get_default(param_id_t id);
void param_manager_reset_all(void);

int param_manager_register_callback(param_change_callback_t callback, void* user_data);
void param_manager_unregister_callback(param_change_callback_t callback);

void param_manager_poll(void);
```

---

## 7. 使用建议

1. 业务层统一走 `param_manager_set()`，不要直接改内部数据。  
2. UI 页面 callback 中先判断 `id`，只处理关心参数。  
3. callback 中不要做重逻辑，保持短小，避免阻塞 UI。  
4. 需要参数与业务动作一致性时，建议由 `media_manager` 编排后再写参。  

---

## 8. 总结

`param_manager` 当前是“参数级广播 + 主线程派发回调”模型：

- `set` 校验并写值
- `get` 只读不修正
- `poll` 统一回调

该模型兼顾了参数一致性与 LVGL 线程安全。
