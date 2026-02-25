# Media Manager 设计文档

## 1. 背景与目标

当前 UI 页面会直接处理部分媒体逻辑与参数写入，存在两个问题：

- UI 与业务逻辑耦合，难以复用到其他项目
- 耗时操作（模式切换、格式化等）缺少统一的过程状态管理

目标：

- `media_manager` 成为**非 UI 业务编排层**
- `param_manager` 成为**参数存储与校验层**
- UI 只负责展示与交互，不直接承担业务流程

---

## 2. 职责边界

### 2.1 media_manager 负责

- 对外提供统一媒体操作入口（`op + args`）
- 操作合法性判断（状态、前置条件、互斥关系）
- 调用底层能力（消息总线、驱动、mode 管理）
- 管理异步任务生命周期（开始/进度/完成/失败）
- 在“操作成功”时，统一触发参数更新（通过 `param_manager`）

### 2.2 media_manager 不负责

- 页面切换、弹窗、动画、loading、toast
- 任何 LVGL 控件创建/更新

### 2.3 param_manager 负责

- 参数值保存与读取
- `set` 时合法性校验（范围/枚举）
- 参数变更通知（回调/事件）

### 2.4 param_manager 不负责

- 具体业务动作执行（拍照、切模式、格式化等）
- 操作流程状态管理（进行中/失败重试）

---

## 3. 两者关系（核心规则）

规则 1：UI 不直接执行业务动作，只调用 `media_manager_execute(...)`。  
规则 2：参数变更由 `media_manager` 统一写入 `param_manager`（推荐），保证“动作结果”和“参数状态”一致。  
规则 3：`param_manager` 是 `media_manager` 的依赖，不反向依赖。  
规则 4：UI 可以订阅 `param_manager` 变化来刷新显示，但不反推业务流程。

---

## 4. 操作模型建议

### 4.1 同步类操作

例如：设置白平衡、ISO、曝光、质量、人脸检测开关。

- UI -> `media_manager_execute(op, args)`
- `media_manager` 校验并执行
- 成功后 `param_manager_set(...)`
- UI 通过参数更新刷新

### 4.2 异步类操作

例如：模式切换、前后摄切换、格式化、恢复出厂、拍照触发。

建议增加异步接口与事件：

- `media_manager_execute_async(op, args, req_id)`
- `MEDIA_EVENT_START(req_id, op)`
- `MEDIA_EVENT_PROGRESS(req_id, op, progress)`
- `MEDIA_EVENT_DONE(req_id, op, result)`

UI 仅订阅事件：

- `START` -> 显示“处理中”
- `PROGRESS` -> 更新进度
- `DONE` -> 成功/失败提示

---

## 5. 参数更新策略建议

### 5.1 推荐策略：成功后落参

用于会触发底层行为的设置（例如切模式相关分辨率）：

- 底层执行成功后再写入 `param_manager`
- 避免 UI 先看到参数变更，但底层实际失败

### 5.2 可选策略：先落参后回滚

用于需要“立即回显”的场景：

- 先写参数并通知 UI
- 失败时回滚旧值并发失败事件

默认建议使用 5.1，逻辑更稳。

---

## 6. 推荐目录依赖

- `core/media_manager.*` 依赖：
  - `core/param_manager.h`
  - `core/message_manager.h`（或底层适配层）
- `pages/*` 依赖：
  - `core/media_manager.h`
  - `core/param_manager.h`（只读/订阅）

禁止：

- `media_manager` 依赖任何 `pages/*` 或 `ui/*`

---

## 7. 迁移路线

1. 保持当前 `media_manager_execute(op, args)` 接口稳定。  
2. 页面中的“设置写参”逐步替换为调用 `media_manager`。  
3. 为耗时操作补齐异步事件机制。  
4. UI 统一改为“调用 media_manager + 订阅状态事件/参数变化”。  
5. 完成后，页面中不再出现业务直连逻辑。

---

## 8. 总结

`media_manager` 定位为“非 UI 业务编排层”，`param_manager` 定位为“参数存储校验层”。  
两者配合后，可同时满足：

- 跨项目复用
- UI 与业务解耦
- 耗时流程可视化反馈
- 参数与业务结果一致性
