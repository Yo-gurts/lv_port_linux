# Intent 设计说明

## 1. 设计背景

在 UI 工程里，最容易失控的是页面代码不断“长业务逻辑”：

- 页面事件回调里直接调用硬件/系统接口
- 一个点击动作里混入状态切换、输入控制、异步任务、错误处理
- UI 层和业务层强耦合，后续改动牵一发而动全身

为避免这类问题，引入 `intent` 作为 UI 和业务之间的统一语义层。

当前实现位置：

- `include/core/intent.h`
- `src/core/intent.c`

---

## 2. 设计目标

`intent` 的核心目标是两句话：

1. UI 只表达“要做什么”，不表达“怎么做”  
2. 具体实现统一封装在 intent 处理层（后续可接 usecase/manager）

这样做的价值：

- 降低页面代码复杂度
- 便于统一处理流程（前置校验、状态切换、错误码、埋点）
- 便于后续替换底层实现（不同平台/驱动/业务策略）

---

## 3. 分层职责

### 3.1 UI 页面层（`src/pages/*.c`）

只做：

- 组件创建与样式
- 事件监听
- `intent_dispatch(...)`
- 基于结果更新显示（后续接结果订阅）

不做：

- 硬件调用
- 输入设备策略切换
- 复杂流程编排

### 3.2 Intent 层（`src/core/intent.c`）

只做：

- 将语义动作路由到处理分支
- 读取 `param_manager` 中的当前参数（后续接入）
- 调用封装好的 usecase/manager（后续接入）

---

## 4. 为什么“切页面”也要走 Intent

页面切换通常不只是导航行为，经常带有“上下文切换副作用”，例如：

- 模式切换（拍照 -> 录像）
- 输入策略变化（禁触摸、禁部分按键）
- 资源切换（停止拍照相关任务，启动录像相关任务）

如果这些逻辑分散在多个页面回调中，会很难维护。  
把它们放到 `intent`，就可以将“切换到录像”定义成一个完整业务动作。

---

## 5. 示例：切换到录像模式

### 5.1 需求描述

“切换到录像”不是单一步骤，而是一个复合操作：

1. 切换业务模式到 `video`
2. 禁用触摸输入（例如录像前准备阶段）
3. 禁用部分按键输入（例如屏蔽菜单键/返回键）
4. 完成后再允许特定输入
5. 最终进入录像页面

### 5.2 UI 层写法（只发意图）

```c
/* page_photo.c */
static void mode_switch_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    intent_dispatch(INTENT_OPEN_VIDEO_PAGE);
}
```

### 5.3 Intent 层封装（示意）

```c
/* intent.c */
static int handle_intent_open_video_page(void)
{
    /* 1) 从 param_manager 读取当前策略 */
    /* 2) 执行业务切换 */
    /* 3) 设置输入策略：禁用触摸 + 屏蔽部分按键 */
    /* 4) 跳转或触发页面显示 */
    return 0;
}
```

> 关键点：UI 不关心禁哪些按键、何时恢复、失败怎么回滚；这些都由 intent 封装处理。

---

## 6. 与 param_manager 的关系

本工程约定：`intent_dispatch` 只接收 `intent` 枚举值，不携带额外参数。  
运行期所需上下文由 `param_manager` 提供，例如：

- 当前模式
- 电池/存储状态
- 输入策略配置
- 功能开关

这样可以保持 UI 事件接口稳定，减少参数传递和耦合。

---

## 7. 当前最小集合

当前保留的 intent：

- `INTENT_OPEN_VIDEO_PAGE`
- `INTENT_OPEN_PHOTO_PAGE`

后续新增 intent 时，建议遵循：

1. 先定义语义动作（业务视角）
2. 再在 intent 层实现路由和封装
3. UI 页面只增加 `intent_dispatch(...)` 调用

---

## 8. 反例（不推荐）

不推荐在页面回调里直接写：

- 禁用触摸输入
- 修改按键映射
- 启停后台任务
- 调硬件 API

这类实现会导致“同一个动作在多个页面各写一份”，后续修改容易出现行为不一致。

---

## 9. 总结

`intent` 的设计意图不是增加一层“中转代码”，而是把“动作语义”与“实现细节”分离：

- UI：告诉系统“我要切到录像”
- Intent：决定“如何安全地切到录像（含输入策略和业务流程）”

通过这条边界，可以让 UI 保持简单、业务保持集中、演进成本可控。

---

## 10. 推荐落地模板（编码规范）

下面给出一个推荐的 intent 处理模板，适合团队统一风格。

### 10.1 处理阶段约定

建议每个复杂 intent 都按 4 个阶段组织：

1. 校验阶段：检查当前状态是否允许执行
2. 输入策略阶段：设置触摸/按键可用性
3. 业务执行阶段：调用 usecase/manager
4. 收尾阶段：成功确认或失败回滚

### 10.2 示例模板（`INTENT_OPEN_VIDEO_PAGE`）

```c
typedef enum {
    INTENT_OK = 0,
    INTENT_ERR_INVALID_STATE = -1,
    INTENT_ERR_INPUT_POLICY = -2,
    INTENT_ERR_SWITCH_MODE = -3,
} intent_result_t;

static int handle_intent_open_video_page(void)
{
    int ret = INTENT_OK;
    bool touch_disabled = false;
    bool key_limited = false;

    /* 1) 校验阶段 */
    if (!param_manager_can_switch_to_video()) {
        MLOG_WARN("INTENT_OPEN_VIDEO_PAGE rejected: invalid state");
        return INTENT_ERR_INVALID_STATE;
    }

    /* 2) 输入策略阶段 */
    ret = input_policy_disable_touch();
    if (ret != 0) {
        MLOG_ERR("disable touch failed: %d", ret);
        return INTENT_ERR_INPUT_POLICY;
    }
    touch_disabled = true;

    ret = input_policy_limit_keys();
    if (ret != 0) {
        MLOG_ERR("limit keys failed: %d", ret);
        goto rollback;
    }
    key_limited = true;

    /* 3) 业务执行阶段 */
    ret = mode_usecase_switch_to_video();
    if (ret != 0) {
        MLOG_ERR("switch to video failed: %d", ret);
        ret = INTENT_ERR_SWITCH_MODE;
        goto rollback;
    }

    /* 4) 收尾阶段（成功） */
    MLOG_INFO("INTENT_OPEN_VIDEO_PAGE done");
    return INTENT_OK;

rollback:
    /* 失败回滚：保证输入策略可恢复 */
    if (key_limited) {
        input_policy_restore_keys();
    }
    if (touch_disabled) {
        input_policy_enable_touch();
    }
    return ret;
}
```

### 10.3 日志与错误码建议

- 每个 intent 入口打印 1 行“开始日志”
- 每个失败分支打印错误码和关键上下文
- 回滚动作也要记录日志，便于定位“失败后二次异常”
- 返回统一错误码，不直接在 UI 层拼接错误文案

### 10.4 团队约束建议

- UI 页面中禁止出现 `input_policy_*` / `*_usecase_*` / 硬件 API
- 允许 UI 页面调用：`intent_dispatch(...)`
- 复杂 intent 必须有回滚路径
- 新增 intent 时，文档同步更新“语义定义 + 失败策略”
