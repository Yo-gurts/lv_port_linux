# Message Manager 设计文档

## 1. 目的与范围

`message_manager` 是 `turnkey` UI 层的统一消息组件，负责两类能力：

- 向 `MODEMNG` 发送消息（异步 / 同步超时）；
- 订阅 `EVENTHUB` 事件并进行统一分发。

本文档覆盖当前实现行为，不讨论未来扩展方案细节。

## 2. 模块职责

- 对外提供稳定接口：`create/destroy/send_async/send_sync_timeout`；
- 维护单个 in-flight 请求上下文，避免回包匹配冲突；
- 在事件回调中先处理请求回包，再处理业务事件分发；
- 提供统一错误码语义，业务结果透传 `evt->s32Result`。

## 3. 对外接口

头文件：`include/core/message_manager.h`

- `int32_t message_manager_create(void);`
- `void message_manager_destroy(void);`
- `int32_t message_manager_send_async(const MESSAGE_S* msg, message_manager_result_cb_t cb);`
- `int32_t message_manager_send_sync_timeout(const MESSAGE_S* msg, uint32_t timeout_ms);`

回调类型：

- `typedef int32_t (*message_manager_result_cb_t)(EVENT_S* evt);`

## 4. 错误码约定

- `0`：成功；
- `MESSAGE_MANAGER_EINVAL`：参数非法；
- `MESSAGE_MANAGER_EBUSY`：已有未完成请求；
- `MESSAGE_MANAGER_ESEND`：调用 `MODEMNG_SendMessage` 失败；
- `MESSAGE_MANAGER_ETIMEOUT`：同步等待超时；
- `MESSAGE_MANAGER_ESTATE`：未初始化或内部状态异常。

同步接口在收到匹配回包后，返回值为 `evt->s32Result`（业务结果透传）。

## 5. 核心数据结构

实现文件：`src/core/message_manager.c`

- `message_context_t g_msg_ctx`
- 字段：
  - `MESSAGE_S msg`：当前 in-flight 请求副本；
  - `bool msg_processed`：当前请求是否已完成；
  - `message_manager_result_cb_t result_cb`：回包处理回调；
  - `pthread_mutex_t msg_mutex`：上下文互斥锁。

同步等待状态：

- `pthread_cond_t g_sync_cond`
- `int32_t g_sync_result`
- `bool g_sync_done`

订阅状态：

- `EVENTHUB_SUBSCRIBER_S* g_subscriber_desc`
- `MW_PTR g_subscriber_hdl`
- `bool g_msgmgr_created`

## 6. 并发模型

- 全局仅允许一个 in-flight 请求；
- `send_async` 与 `send_sync_timeout` 互斥（共享 `g_msg_ctx.msg_mutex`）；
- 事件线程由 `EVENTHUB` 决定，回调路径必须轻量、不可阻塞；
- 同步等待通过条件变量阻塞调用线程，不阻塞事件线程。

## 7. 关键流程

### 7.1 初始化流程

1. `message_manager_create` 将 `g_msgmgr_created` 置为 `true`；
2. 调用 `message_manager_subscribe`；
3. `message_manager_subscribe` 创建 subscriber 并订阅主题列表；
4. 订阅回调统一指向 `message_manager_event_cb`。

### 7.2 异步发送流程

1. 校验 `msg` 与 manager 状态；
2. 加锁检查是否 busy；
3. 填充 in-flight 上下文并登记 `result_cb`；
4. 调用 `MODEMNG_SendMessage`；
5. 失败则回滚上下文并返回 `MESSAGE_MANAGER_ESEND`。

### 7.3 同步发送流程（超时）

1. 校验入参与状态；
2. 加锁检查 busy；
3. 重置 `g_sync_done/g_sync_result`；
4. 将 `result_cb` 设置为内部回调 `message_manager_sync_result_proc`；
5. 发送消息后进入 `pthread_cond_timedwait`；
6. 超时返回 `MESSAGE_MANAGER_ETIMEOUT`；
7. 收到回包后返回 `g_sync_result`（即 `evt->s32Result`）。

### 7.4 事件回调处理流程

`message_manager_event_cb` 顺序固定：

1. 先加锁执行 `message_manager_process_result_locked`：
   - 按 `(topic,arg1,arg2)` 匹配回包；
   - 命中后调用 `result_cb` 并置 `msg_processed=true`；
2. 解锁后调用 `message_manager_dispatch_event`：
   - 对已关注主题做统一日志与后续业务扩展入口。

## 8. 已订阅主题

当前实现订阅以下类别主题：

- 卡状态：`EVENT_MODEMNG_CARD_*`
- 模式状态：`EVENT_MODEMNG_MODEOPEN/MODECLOSE/MODESWITCH/RESET/SETTING`
- 录像状态：`EVENT_MODEMNG_RECODER_*`
- UI 触摸：`EVENT_UI_TOUCH`（通过 `EVENTHUB_RegisterTopic` 注册）

实际列表以 `message_manager_subscribe` 中 `topics[]` 为准。

## 9. 生命周期与资源回收

- `message_manager_destroy` 会：
  - 将上下文标记为可发送；
  - 清理回调；
  - 唤醒可能阻塞的同步等待线程；
  - 销毁 subscriber 并重置全局状态。

建议在 UI 应用生命周期中成对调用：

- 启动阶段：`message_manager_create`
- 退出阶段：`message_manager_destroy`

## 10. 日志与可观测性

- 默认分发路径对关键主题打印 `MLOG_DBG`；
- 主题订阅失败打印 `MLOG_WARN` 并累计失败状态；
- 建议联调时重点关注：`EBUSY`、`ESEND`、`ETIMEOUT` 发生频率。

## 11. 设计边界

- 当前不支持多请求并发匹配；
- 回包匹配条件固定为 `(topic,arg1,arg2)`；
- `message_manager_dispatch_event` 目前以统一日志为主，业务处理可在此扩展。
