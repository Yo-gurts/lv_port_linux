# CLAUDE.md

本文档为 Claude Code (claude.ai/code) 在本项目中工作时提供指导。

## 项目概述

这是一个基于 LVGL 的 UI 应用程序，运行于 Linux 系统的 AI 摄像机。项目面向嵌入式设备（Sophgo 芯片），并支持在 x86_64 平台上使用 SDL2 进行开发。

## 编译命令

```bash
# 使用 SDL 编译（默认，用于 x86_64 开发）
make

# 使用 framebuffer/DRM 编译（用于嵌入式设备）
make CONFIG=FB

# 清理编译
make clean
```

**直接使用 CMake 编译：**
```bash
mkdir build/x86_64 && cd build/x86_64
cmake -DCONFIG=SDL ..
make -j
```

## 架构

### 页面管理器系统

应用程序使用自定义的**页面管理器**（`src/core/page_manager.c`）来管理 UI 页面：

- **生命周期**：`create()` → `show()` → `hide()` → `destroy()`
- **导航**：`page_manager_navigate("page_name")` 和 `page_manager_back()`
- **页面**：首页、拍照、录像、拍照设置、录像设置、相册、WiFi 列表、系统设置、AI 对话、AI 拍照
- 每个页面通过 `page_set_private_data()/page_get_private_data()` 存储私有数据

### 目录结构

```
src/
├── core/                    # 核心模块
│   ├── page_manager.c      # 页面生命周期和导航管理
│   ├── font_manager.c      # FreeType 中文字体管理
│   ├── style_manager.c     # 样式管理
│   ├── key_manager.c       # 按键管理
│   ├── message_manager.c   # 消息管理
│   ├── media_manager.c     # 媒体管理
│   ├── param_manager.c     # 参数管理
│   ├── ui_common.c         # UI 公共组件
│   ├── top_notice.c        # 顶部通知组件
│   └── framebuffer_manager.c # Framebuffer 管理
├── pages/                   # 页面模块
│   ├── page_home.c         # 首页
│   ├── page_photo.c        # 拍照模式
│   ├── page_video.c        # 录像模式
│   ├── page_photo_settings.c   # 拍照设置
│   ├── page_video_settings.c   # 录像设置
│   ├── page_album.c        # 相册
│   ├── page_wifi_list.c    # WiFi 列表
│   ├── page_system_settings.c  # 系统设置
│   ├── page_chat.c         # AI 对话
│   ├── page_ai_photo.c     # AI 拍照
│   ├── page_ai_photo_settings.c # AI 拍照设置
│   └── page_version_info.c # 版本信息
├── ui/                      # UI 组件
│   ├── volume_bar.c        # 音量条
│   ├── filter_panel.c      # 滤镜面板
│   └── top_notice.c        # 顶部通知
├── utils/                   # 工具函数
├── mock/                    # 模拟模块（用于开发测试）
│   ├── wifi_manager.c
│   ├── message_manager.c
│   ├── media_manager.c
│   └── file_manager.c
└── main/                    # 程序入口
    ├── sophcam/             # 真机入口
    │   └── uiapp_start.c
    └── simulator/           # 模拟器入口
        ├── uiapp_start.c
        ├── main_simulator.c
        └── mouse_cursor_icon.c
```

### 关键组件

- **LVGL**：图形库（在 `lvgl/` 子模块中）
- **FreeType**：中文字体渲染（在 `third_party/freetype/` 中）
- **SDL2**：x86 开发的显示后端
- **MLOG**：基于 Syslog 的日志系统，支持多级别（MLOG_ERR, MLOG_INFO, MLOG_DBG 等）

<<<<<<< HEAD
## 代码规范
=======
- Use Chinese commit messages following existing history style (e.g. `feat: ...`, `refactor(ui): ...`).
- Keep the commit body compact and readable; avoid too many blank lines between items.
- Commit body must use real line breaks. Do not write literal `\n` in message text.
- Recommended command format:

```bash
git commit -m "fix(module): 简短标题" -m $'- 要点1\n- 要点2\n- 要点3'
```

- Wrong example (will keep `\n` as plain text):

```bash
git commit -m "fix(module): 简短标题" -m "- 要点1\n- 要点2"
```
>>>>>>> docs(claude): 补充提交信息换行规范

- **格式**：`clang-format-12` 配合 WebKit 风格（`.clang-format`）
- **提交时自动格式化**：`.git/hooks/` 中的预提交钩子会应用 clang-format-diff-12

## Git 提交信息

### 格式结构

```
type(scope): 简要描述

- 详细变更点1
- 详细变更点2
- 详细变更点3
```

### 说明

- **第一行**：`type(scope): description`
  - `type`：类型前缀，如 `feat`（新功能）、`fix`（bug 修复）、`refactor`（重构）、`style`（样式）、`chore`（构建/工具）等
  - `scope`：影响范围（可选），如 `system_settings`、`ui`、`photo`、`media` 等
  - `description`：简要描述，用中文，不超过 50 字

- **正文**：详细变更列表
  - 以 `-` 开头，每行一个变更点
  - 描述具体做了什么（不要描述"如何做"）
  - 按逻辑顺序排列（先 UI/接口，后逻辑/状态管理）

### 示例

```
feat(system_settings): 增加格式化/恢复出厂确认弹框与处理提示

- 系统设置页新增格式化与恢复出厂确认弹框，分别展示数据删除/参数重置风险说明
- 确认后先更新 top_notice 为处理中提示，处理完成后展示成功/失败结果提示
- 新增 file_manager_format_sdcard() mock 接口，内部延时1s模拟格式化处理
- param_manager 增加 param_manager_factory_reset() 接口，延时1s后重置全部参数
- page_system_settings 增加待处理动作与状态管理，防止处理中重复触发
```

### 注意事项

- 使用中文提交信息
- 保持提交内容紧凑易读；避免条目之间留过多空行
- 如果改动很简单（1-2点），可以省略列表，直接在第一行描述
- 提交正文必须使用真实换行，不要把 `\n` 当普通字符写入正文
- 推荐命令：`git commit -m "fix(module): 简要描述" -m $'- 变更点1\n- 变更点2\n- 变更点3'`
- 错误示例：`git commit -m "fix(module): 简要描述" -m "- 变更点1\n- 变更点2"`（会把 `\n` 原样写进提交信息）

## LVGL 开发指南

请遵循 `Contributing.md` 中的以下规则以确保稳定性和性能：

1. **避免使用线程** - 改用 LVGL 定时器（`lv_timer_create()`）
2. **单线程 LVGL** - 所有 LVGL 调用必须在 GUI 线程中执行
3. **复用对象** - 避免频繁创建/销毁；使用显示/隐藏
4. **静态资源** - 尽可能静态加载字体/图片
5. **定时器回调要简短** - 切勿在定时器回调中阻塞
6. **限制动画** - 过多动画会增加 CPU 消耗
7. **最小化样式更改** - 预定义样式，避免运行时更改
8. **使用虚拟列表** - 用于大型列表/图片（tileview、list、table）
9. **页面隐藏时暂停资源** - 隐藏页面时停止定时器/动画
10. **关注内存** - 合理配置 `LV_MEM_SIZE`

### 命名约定

- 控件：`lv_label_title`、`lv_btn_ok`
- 样式：`style_screen_bg`、`style_btn_pressed`
- 事件处理函数：`event_handler_btn_ok()`
- 注释：中文注释必须描述控件用途和事件逻辑

### 页面代码结构

遵循以下 9 部分结构（参见 `Contributing.md`）：
1. 头文件与宏定义
2. 数据结构
3. 全局变量与声明
4. 内部静态辅助函数
5. 外部接口
6. 线程函数
7. 事件回调（按键、手势、定时器）
8. 初始化/销毁/资源管理
9. 调试与测试

## 配置

- **SDL 后端**：`include/lv_conf_sdl.h`（默认，用于 x86 开发）
- **Framebuffer 后端**：`include/lv_conf_fb.h`（用于嵌入式设备）

## 图标资源

- 来源：https://icons.getbootstrap.com/、https://www.flaticon.com/
- 格式：PNG（45x45）和 SVG
- 位置：`res/icons/`（PNG）、`res/svg/`（SVG）
- 字体：`res/fonts/HarmonyOS_Sans_SC_Regular.ttf`
- 批量调整尺寸：`mogrify -resize 45x45 res/icons/*.png`
