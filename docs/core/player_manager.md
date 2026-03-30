# Player Manager 设计文档

## 1. 背景与目标

`video_preview` 页面原先维护了本地“假播放进度”，UI 与播放底层耦合较重，存在以下问题：

- 页面直接承担播放状态管理，难以复用
- 真实播放能力（`PLAYER_SERVICE`）无法在页面间统一管理
- 模拟器和真机行为难以保持一致

目标：

- 新增 `player_manager` 作为**非 UI 播放编排层**
- 页面仅负责交互与展示，播放动作统一由 manager 执行
- 在真机使用 `PLAYER_SERVICE`，在 SDL 使用 mock 实现

---

## 2. 职责边界

### 2.1 player_manager 负责

- 统一封装视频播放操作：`prepare/play/pause/stop/seek`
- 管理播放上下文状态（是否已准备、是否暂停、总时长）
- 对外提供播放进度查询接口
- 屏蔽真机与模拟器实现差异

### 2.2 player_manager 不负责

- LVGL 控件创建/更新
- 页面切换、手势返回、动画
- 自动息屏策略（由页面继续通过 `power_manager` 控制）

---

## 3. 相关文件

- 头文件：`include/core/player_manager.h`
- 真机实现：`src/core/player_manager.c`
- mock 实现：`src/mock/player_manager.c`
- 页面接入：`src/pages/page_video_preview.c`

---

## 4. 对外接口

```c
int player_manager_init(void);
void player_manager_deinit(void);

int player_manager_prepare(const char* video_path);
int player_manager_play(void);
int player_manager_pause(void);
int player_manager_stop(void);
int player_manager_seek_sec(int sec);

int player_manager_get_progress(int* current_sec, int* total_sec);
int player_manager_is_paused(int* out_paused);
```

返回约定：

- `0`：成功
- `< 0`：失败（参数非法或底层状态异常）

---

## 5. 运行机制

### 5.1 prepare 阶段（切换视频后不自动播放）

`player_manager_prepare(video_path)` 在真机流程为：

1. `PLAYER_SERVICE_SetInput(...)`
2. `PLAYER_SERVICE_Play(...)`
3. `PLAYER_SERVICE_TouchSeekPause(..., 0)`

该流程保证页面切换视频后停在首帧，不自动播放。

### 5.2 play / pause

- `player_manager_play()`：恢复音频并开始播放
- `player_manager_pause()`：暂停当前播放

### 5.3 seek

- 播放中：调用 `PLAYER_SERVICE_Seek`
- 暂停中：调用 `PLAYER_SERVICE_TouchSeekPause`（seek 后保持暂停画面）

### 5.4 进度获取

- 真机：`player_manager_get_progress()` 使用 `PLAYER_SERVICE_SeekTime` 获取当前位置，并维护总时长
- mock：使用内部时钟模拟播放进度，保证页面逻辑可联调

---

## 6. 页面接入约定（video_preview）

`page_video_preview` 侧行为：

1. `show` 时按当前索引调用 `player_manager_prepare`。
2. 切换上一条/下一条时：
   - 继续显示 `video_large` 封面图
   - 重新 `prepare`
   - 保持暂停态（不自动播放）
3. 用户点击播放按钮后才调用 `player_manager_play`。
4. 进度条释放时调用 `player_manager_seek_sec`。
5. `back/hide/destroy` 时统一调用 `player_manager_stop`。

---

## 7. 异常与降级策略

- 视频路径获取失败：页面保持封面显示，播放操作返回失败
- `prepare/play/pause/seek` 失败：记录日志并保持页面处于安全状态（默认暂停）
- 时长不可得：页面可使用已有兜底时长展示，不阻塞播放流程

---

## 8. 构建接入

在 `CMakeLists.txt` 中按平台选择实现：

- `FB`：`src/core/player_manager.c`
- `SDL`：`src/mock/player_manager.c`

保证同一页面代码在真机/模拟器都可编译运行。

---

## 9. 后续扩展建议

- 增加事件回调接口（播放完成、打开失败、进度事件）
- 将错误码细分为可恢复/不可恢复类型
- 支持更多播放参数（倍速、循环、静音模式）
