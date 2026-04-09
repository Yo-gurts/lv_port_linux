# 线程模型与可调用线程约束

## 1. 目标

本文档用于明确：
- 哪些模块回调可能不在 GUI 线程
- 哪些 API 只能在 GUI 线程调用
- 如何把异步结果安全切回 GUI 线程

本项目核心原则：LVGL 单线程调用。

## 2. 线程角色

### 2.1 GUI 主线程
由 `ui_main()` 主循环驱动：
- `key_manager_poll()`
- `power_manager_poll()`
- `wifi_manager_poll()`
- `param_manager_poll()`
- `lv_timer_handler()`

所有 LVGL 对象创建/删除/样式/文本更新必须在该线程执行。

### 2.2 EventHub 回调线程
`message_manager` 订阅回调配置为 `sync=false`，事件分发回调不保证在 GUI 线程。

结论：`message_manager` 路径中若需要刷新 UI，必须通过 `lv_async_call` 切回 GUI 线程。

### 2.3 媒体异步工作线程
`media_manager_execute_async()` 使用内部队列 + worker 线程执行耗时媒体操作。

结论：异步完成回调通过 `lv_async_call` 在 GUI 线程触发，页面回调可安全更新 UI。

## 3. GUI线程专属 API（非穷举）

以下函数必须仅在 GUI 线程触发：
- `top_notice_show*()` / `top_notice_hide()`
- `status_bar_*()` 中涉及对象更新的接口
- 任何 `lv_obj_*`、`lv_label_*`、`lv_img_*`、`lv_timer_*` 调用
- 页面 `create/show/hide/destroy/update` 中所有 UI 逻辑

## 4. 非GUI线程可执行的操作

- 纯数据计算
- 参数读写（不直接触发 LVGL）
- 底层 message/media 同步等待

但若后续需要 UI 反馈，必须异步投递回 GUI 线程。

## 5. 标准回主线程模板

```c
static void ui_async_cb(void* user_data)
{
    // 这里是 GUI 线程
    // 安全操作 LVGL 对象
}

void worker_or_event_thread_fn(...)
{
    // 非 GUI 线程
    (void)lv_async_call(ui_async_cb, user_data);
}
```

## 6. 反例（禁止）

1) 在 EventHub 回调里直接 `lv_label_set_text`。  
2) 在 `lv_timer` 回调里执行 1s+ 阻塞调用。  
3) 在后台线程直接删除页面对象。  

## 7. 现有实现对应关系

- `message_manager`：SD 卡通知/状态栏刷新通过 `lv_async_call` 派发
- `status_bar`：参数回调改为 `lv_async_call` 刷新
- `page_system_settings`：格式化/恢复出厂改为 `media_manager_execute_async`
- `media_manager`：worker 完成后统一回 GUI 线程执行页面回调

## 8. 排障建议

当出现“偶发 UI 崩溃/随机绘制异常”时，优先排查：
1) 是否有非 GUI 线程直接调用 LVGL API
2) 是否有定时器回调中的阻塞逻辑
3) 是否存在页面销毁后仍异步访问旧对象
