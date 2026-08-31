---
name: sdl-sim-automation
description: Use when需要在 SDL 模拟器里批量截取 UI 页面截图、用按键仿真翻页/操作页面、遍历页面生成用户手册或 UI 截图素材，或排查「模拟器怎么自动操作/截图」类问题。触发词：抓图、截图、截屏、按键仿真、按键注入、翻页、遍历页面、用户手册、sim_auto、FIFO。
---

# SDL 模拟器自动化：自动抓图 + 按键仿真翻页

## 概述

本仓库（turnkey UI）的 SDL 模拟器内置 `sim_automation` 模块：模拟器运行时创建一个 FIFO 命令通道，
外部脚本向 FIFO 写命令即可**截取无鼠标光标的纯 UI 画面**、**注入物理按键事件翻页**、直接跳转页面。
截图走 LVGL 自己的 draw buffer（与 SDL 窗口合成无关），按键走 `key_manager_inject_key()`
（与 /dev/input 同一状态机）——**产出与真实按键/真实画面一致**。

板端 FB 构建中该模块为空实现（`defined(FB_RUN)&&!FB_RUN` 守卫），不影响出货。

## 何时使用

- 需要 UI 页面截图（手册、评审、问题记录、给客户的页面说明）
- 需要无人值守批量遍历页面（回归截图对比、翻译检查）
- 需要按键驱动的自动化操作（模拟用户翻页/确认/返回）
- **不适用**：需要真实 sensor 画面、触摸轨迹、性能数据——模拟器里这些是 mock。

## 快速开始（三步）

```bash
# ① 起模拟器（无头服务器先起 Xvfb；FIFO 默认 /tmp/sim_auto.fifo）
Xvfb :99 -screen 0 1024x768x24 &            # 已有 X 会话可跳过
cd <turnkey 仓库根>
rm -f /tmp/sim_auto.fifo
DISPLAY=:99 ./bin/main > /tmp/sim_run.log 2>&1 &
sleep 3                                       # 等 "[sim_auto] automation fifo ready"

# ② 驱动它（任意 shell / 脚本）
echo "nav home"                      > /tmp/sim_auto.fifo
echo "key down"                      > /tmp/sim_auto.fifo
echo "screenshot /tmp/pages/home.bmp" > /tmp/sim_auto.fifo

# ③ 收尾
echo "quit" > /tmp/sim_auto.fifo
```

> 截图产物是 **BMP**（SDL2 无 SDL_image，见「常见坑」），要 PNG 用 `convert in.bmp out.png`。

## 命令参考

| 命令 | 作用 | 说明 |
|---|---|---|
| `screenshot [path]` | 截当前画面 | path 省略则 `sim_<时间戳>.bmp` 写到运行目录；内容为 BMP |
| `key <name>` | 完整短按 | 按下 + ~20ms 后抬起 → 产生 PRESS + CLICK |
| `keydown <name>` | 仅按下 | 配合 keyup 可组出长按（长按需按住 >700ms 再 keyup） |
| `keyup <name>` | 仅抬起 | 结束一次按住 |
| `nav <page>` | 页面跳转 | 页面名见 `src/core/ui_common.c` 的 `page_manager_register` 调用 |
| `back` | 返回上一页 | 等效 MENU 返回语义 |
| `quit` | 退出模拟器 | 脚本收尾用 |

按键名（13 个）：`power assistant volup voldown focus camera mode menu up down left right ok`

一行可发一条命令（`\n` 分隔）；命令之间留 0.2~0.5s 让页面切换/动画完成再截。

## 按键翻页速查（常用页面导航路径）

页面真实导航关系（按键走起，与用户操作一致；`nav` 适合跳过路径直达）：

- **主页**（2×3 宫格：拍照/AI 拍照/相册/AI 对话/测试/设置）
  - `up`/`down` 跨行移动选中项，`left`/`right` 同行移动，`ok` 进入
  - 例：home→测试 = `down`+`right`+`ok`；home→设置 = `down`+`right`+`right`+`ok`
- **拍照 ↔ 录像**：拍照页 `mode` 切录像（无历史跳转），录像页 `mode` 切回
- **设置页（列表型）**：`up`/`down` 选中项循环移动，`ok` 进入/修改，`menu` 返回
- **相册 ↔ 视频相册**：相册页 `mode` 互切
- 需要新页面的按键映射：查对应 `src/pages/page_*.c` 的 `key_manager_register_callback(...)` 注册表

## 批量遍历模板

```bash
#!/bin/bash
FIFO=/tmp/sim_auto.fifo
OUT=./shots; mkdir -p "$OUT"
send()  { echo "$1" > "$FIFO"; }
snap()  { send "screenshot $OUT/$1.bmp"; sleep 0.4; }
keys()  { for k in "$@"; do send "key $k"; sleep 0.25; done; }

send "nav home"; sleep 0.6
snap 01_home
keys ok; sleep 0.4; snap 02_photo          # 进拍照
keys menu; sleep 0.4; snap 03_photo_settings
# ... 按导航图继续；动画多的页面把 sleep 加到 0.6~1.0
send "quit"
```

完整可运行参考（23 页遍历）：`notes/dc309-turnkey-sdl-screenshot-and-manual/drive_pages.sh`
（在 cvitek_agent 仓库；本仓库外部的配套产物）。

## 实现位置（改功能时看这里）

- `include/core/sim_automation.h` / `src/core/sim_automation.c` —— 模块本体（FIFO 解析、截图、注入）
- `src/core/ui_common.c` —— `ui_main()` 里 `sim_auto_init()` + 主循环首行 `sim_auto_poll()`
- 截图原理：隐藏 `lv_indev_get_cursor()` 光标对象 → `lv_refr_now()` → 读
  `lv_display_get_buf_active()->data`（ARGB8888 direct）→ `SDL_SaveBMP`
- 环境变量 `SIM_AUTO_FIFO` 可覆盖默认 FIFO 路径（多实例并行时用）

## 常见坑

| 坑 | 现象 | 处理 |
|---|---|---|
| FIFO 路径打错 | 命令"发出去没反应" | `echo` 到不存在的路径不会报错；先 `ls -l /tmp/sim_auto.fifo`，或看 `/tmp/sim_run.log` 里 ready 行 |
| 截图时机太早 | 截到上一页/半截动画 | `key` 后 sleep ≥0.25s，页面切换后 ≥0.5s 再 screenshot |
| 以为产物是 PNG | `.bmp` 后缀写 `.png` 内容仍是 BMP | 统一 `.bmp` 命名 + `convert` 转 PNG |
| 长按没生效 | `keydown` 后立刻 `keyup` 只算短按 | 长按阈值 700ms：keydown → sleep 1 → keyup |
| 模拟器起不来 | SDL 报 display 相关错误 | 无头环境没起 Xvfb，或 DISPLAY 指错；`Xvfb :99` 后用 `DISPLAY=:99` |
| 想注入未知按键名 | 日志 `unknown key name` | 名单只有上文 13 个；新按键先在 `key_manager.h` 的 `key_id_t` 加，再补 `g_key_names[]` |
| 改了代码没生效 | 行为不变 | `make -j && 重启 ./bin/main`（模块编进主程序，无热加载） |

## 约束

- 截图/注入仅在 SDL 仿真构建（`make`，CONFIG=SDL 默认）可用；`make CONFIG=FB` 板端构建全空实现
- 一次只消费一行的简单命令协议，无回执；确认执行结果靠日志（`[sim_auto]` 前缀）与截图本身
- 不要在板端代码路径引用 `sim_auto_*` 以外的新头文件——保持 mock/板端隔离（同 `message_manager.h` 宏隔离纪律）
