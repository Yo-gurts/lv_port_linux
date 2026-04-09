# AI Camera UI（LVGL）

本项目是运行在 Linux AI 摄像机上的 LVGL UI 应用，面向 Sophgo 平台，同时支持 x86_64 + SDL2 的桌面仿真开发。

- 目标设备：嵌入式 Linux（Sophgo）
- 开发模式：x86_64 SDL2 仿真
- 图形框架：LVGL
- 关键约束：LVGL 单线程调用、页面生命周期管理、输入统一收敛

上游 LVGL Linux 端口说明请参考：
- https://github.com/lvgl/lv_port_linux

----------------------------------------

## 1. 快速开始

### 1.1 获取代码

```bash
git clone <your-repo-url>
cd turnkey
git submodule update --init --recursive
```

### 1.2 常用构建命令

```bash
# SDL（默认，x86_64 仿真）
make

# FB（真机/嵌入式路径）
make CONFIG=FB

# 清理
make clean
```

或直接使用 CMake：

```bash
mkdir -p build/x86_64
cmake -B build/x86_64 -DCONFIG=SDL
cmake --build build/x86_64 -j
```

### 1.3 运行

可执行文件默认输出到：

```text
bin/main
```

----------------------------------------

## 2. 目录结构

```text
include/
  core/                 # 核心模块头文件
  pages/                # 页面头文件
  ui/                   # UI组件头文件

src/
  core/                 # page/key/message/media/param 等管理器
  pages/                # 页面实现（home/photo/video/album/settings...）
  ui/                   # 顶层UI组件（top_notice/status_bar/volume_bar...）
  mock/                 # 仿真实现（SDL 开发使用）
  main/
    simulator/          # 仿真入口
    sophcam/            # 真机入口

docs/
  core/                 # 核心模块设计文档
  ...
```

----------------------------------------

## 3. 核心架构

### 3.1 页面管理（Page Manager）

- 生命周期：`create -> update -> show -> hide -> destroy`
- 导航：`page_manager_navigate("page")`、`page_manager_back()`
- 页面私有数据：`page_set_private_data()` / `page_get_private_data()`

说明：当前切页逻辑中，目标页会先 `update()` 再 `show()`。

### 3.2 输入系统（Key Manager）

- 输入来源：`/dev/input/power-key`、`/dev/input/adc-key2`（真机）+ SDL mouse/keyboard（仿真）
- 统一事件：CLICK/PRESS/RELEASE/LONG_PRESS/LONG_PRESS_REPEAT/LONG_PRESS_3S/LONG_PRESS_3S_RELEASE
- 输入屏蔽位图：TP / power key / adc key / non-camera keys
- 回调模型：支持多订阅者、优先级、可选传播截断

### 3.3 消息与媒体操作

- `message_manager` 对接底层事件总线，统一订阅系统事件
- `media_manager` 提供媒体控制接口（同步 + 异步队列）
- 涉及 UI 刷新的回调统一通过 `lv_async_call` 回 GUI 线程

----------------------------------------

## 4. 开发规范（必读）

1) 所有 LVGL 对象操作必须在 GUI 线程执行。  
2) 不在 LVGL timer 回调里做阻塞任务。  
3) 页面事件注册放在 `create()`，`show()` 只做状态刷新。  
4) 隐藏页面时暂停定时器/动画，避免后台耗时。  
5) 业务日志统一中文（`MLOG_ERR/WARN/INFO/DBG`）。

详见：
- `CLAUDE.md`
- `docs/core/thread_model.md`
- `docs/core/page_manager.md`
- `docs/core/key_manager.md`

----------------------------------------

## 5. 常见问题

### Q1：为什么 SDL 仿真下也要绑定 touch indev 到 key_manager？
为保证 `KEY_INPUT_BLOCK_TP` 在仿真与真机行为一致，SDL mouse 也接入 key_manager 统一控制。

### Q2：格式化/恢复出厂为什么改成异步？
避免 UI 线程阻塞导致卡顿，页面只维护“处理中”状态，结果通过异步回调更新提示。

### Q3：相册和视频相册为什么抽公共模块？
两者是同构九宫格，公共能力抽取可避免双份逻辑长期漂移和修 bug 漏改。
