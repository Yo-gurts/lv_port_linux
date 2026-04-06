# Image Process Manager 设计文档

## 1. 目标

`image_process_manager` 为 AI 图片处理提供统一的异步执行能力，当前一期聚焦“图生图风格化”。

核心目标：

- UI 不阻塞：AI 处理在后台线程执行
- 页面可轮询状态：`IDLE/RUNNING/SUCCESS/FAILED`
- 结果可回读：输出原图路径与展示路径（优先缩略图/子图）
- 统一文件命名策略：沿用 `_AI0001` 递增规则

---

## 2. 职责边界

`image_process_manager` 负责：

- AI 风格化任务排队与执行（单任务模型）
- 线程与状态机管理
- 输出文件名生成
- 结果文件入索引、缩略图/子图提取
- 对外提供任务状态与结果路径接口

`image_process_manager` 不负责：

- 页面控件创建/销毁
- 按键注册与页面导航
- 提示词选择 UI

---

## 3. 模块结构

- 头文件：`include/core/image_process_manager.h`
- 实现：`src/core/image_process_manager.c`
- 调用方（当前）：`src/pages/page_ai_style_preview.c`

对外接口：

```c
int image_process_manager_init(void);
void image_process_manager_deinit(void);
int image_process_manager_is_supported(void);

int image_process_manager_start_style(const char* input_real_path, const char* prompt);
image_process_state_t image_process_manager_get_state(void);
int image_process_manager_get_error(void);
int image_process_manager_get_result_path(char* out_path, size_t out_size, int lv_path);

void image_process_manager_reset(void);
```

---

## 4. 状态机与线程模型

### 4.1 状态定义

- `IMAGE_PROCESS_STATE_IDLE`
- `IMAGE_PROCESS_STATE_RUNNING`
- `IMAGE_PROCESS_STATE_SUCCESS`
- `IMAGE_PROCESS_STATE_FAILED`

### 4.2 线程模型

- 管理器初始化时创建一个常驻 worker 线程。
- 主线程通过 `start_style()` 投递任务并返回，不等待 AI 完成。
- worker 线程使用 `pthread_cond_wait` 等待任务，收到任务后执行：
  1. 生成输出路径
  2. 调用 AI SDK（FB 构建）
  3. 结果入库并提取缩略图/子图
  4. 回写状态与错误码

### 4.3 并发约束

- 单任务串行：`RUNNING` 时再次 `start_style()` 直接拒绝。
- 通过 `mutex + cond` 保护共享状态，避免页面线程与 worker 线程竞争。

---

## 5. 关键实现策略

### 5.1 输出命名规则

输入文件：

- `xxx.jpg` -> `xxx_AI0001.jpg`
- `xxx_AI0002.jpg` -> `xxx_AI0003.jpg`

实现方式：

- 解析 basename 和扩展名
- 识别是否已有 `_AIdddd` 后缀
- 在 `PHOTO_ALBUM_IMAGE_PATH` 下 `access()` 递增找下一个可用号

### 5.2 缩略图与展示路径

处理成功后：

1. `FILEMNG_AddFile(0, output_real_path)` 更新文件索引
2. `FILEMNG_GetThumbPathByFile` 获取缩略图与子图目标路径
3. `FILEMNG_ExtractJpegThumb` 生成缩略图/子图
4. 展示路径优先级：`subpic -> thumbnail -> output`

`get_result_path(..., lv_path=1)` 返回可直接给 `lv_img_set_src()` 使用的 `A:` 路径。

### 5.3 FB/SDL 差异

- `FB`：启用真实 `img2img` 调用（AIsdk）
- `SDL`：返回不支持（用于保持本地模拟可编译，不执行真实 AI）

---

## 6. 与页面协作（以 ai_style_preview 为例）

页面侧建议流程：

1. `show` 时初始化/复位 manager，注册 AI 按键
2. 选中风格后按 AI 键：
   - 调用 `start_style(input_real_path, prompt)`
   - 显示 loading（转圈 + “正在处理中，请稍后。”）
3. 使用 `lv_timer` 周期轮询 `get_state()`
4. `SUCCESS`：取结果路径更新预览图
5. `FAILED`：读取错误码并提示
6. `hide/destroy`：注销按键、停止轮询

---

## 7. 错误处理约定

- `start_style()` 返回非 0：表示任务未启动（参数错误、运行中、或不支持）
- `FAILED` 状态时通过 `get_error()` 获取底层错误码
- `reset()` 仅在非 `RUNNING` 状态下清理结果与错误码，避免中途覆写状态

---

## 8. 后续扩展建议

1. 扩展任务类型：对象识别、拍照翻译等统一并入同一 manager。
2. 增加取消能力：支持页面退出时中断长耗时任务。
3. 增加任务队列：支持串行排队多个任务而不是直接拒绝。
4. 增加统一错误码映射层：页面提示可国际化、可读性更高。
