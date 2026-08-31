/**
 * @file    sim_automation.h
 * @brief   SDL 模拟器自动化接口：纯 UI 截图 + 按键注入（仅仿真构建，FB_RUN=0）
 *
 * 用途：在 SDL 模拟器中批量截取纯 UI 画面（无鼠标光标）、
 *       并以按键注入方式驱动页面导航，用于生成用户手册截图。
 *
 * 使用方式（外部脚本经 FIFO 驱动）：
 *   SIM_AUTO_FIFO=/tmp/sim_auto.fifo ./bin/main
 *   echo "screenshot /path/xxx.png" > /tmp/sim_auto.fifo
 *   echo "key up"        > /tmp/sim_auto.fifo   # 按下并抬起（产生 CLICK）
 *   echo "keydown ok"    > /tmp/sim_auto.fifo   # 仅按下（产生 PRESS，长按场景）
 *   echo "keyup ok"      > /tmp/sim_auto.fifo   # 仅抬起
 *   echo "nav home"      > /tmp/sim_auto.fifo   # 直接页面跳转（辅助用）
 *   echo "back"          > /tmp/sim_auto.fifo   # 返回上一页
 *   echo "quit"          > /tmp/sim_auto.fifo   # 退出模拟器
 *
 * 说明：
 *   - key_manager_inject_key 与 /dev/input 共用同一状态机，
 *     注入产生的事件流与物理按键完全一致（PRESS/CLICK/LONG_PRESS...）。
 *   - 截图直接读 LVGL 显示 draw buffer（ARGB8888 direct 模式），
 *     与 SDL 窗口/合成器无关，天然不含鼠标光标。
 */

#ifndef __SIM_AUTOMATION_H__
#define __SIM_AUTOMATION_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(FB_RUN) && !FB_RUN

#define SIM_AUTO_DEFAULT_FIFO "/tmp/sim_auto.fifo"

/* 初始化自动化通道（读环境变量 SIM_AUTO_FIFO，默认 /tmp/sim_auto.fifo）。
 * 返回 0 表示成功；无 FIFO 环境时静默失败（返回 -1），不影响正常运行。 */
int sim_auto_init(void);

/* 主循环中周期调用：消费 FIFO 命令（非阻塞）。 */
void sim_auto_poll(void);

/* 反初始化，清理 FIFO 资源。 */
void sim_auto_deinit(void);

/* 把当前 LVGL 默认显示的画面写入 PNG（无鼠标光标）。
 * path 为空时用时间戳命名写到当前目录。返回 0 成功。 */
int sim_auto_screenshot(const char* path);

#else /* 板端 FB 构建一律为空实现 */

static inline int sim_auto_init(void) { return -1; }
static inline void sim_auto_poll(void) { }
static inline void sim_auto_deinit(void) { }
static inline int sim_auto_screenshot(const char* path)
{
    (void)path;
    return -1;
}

#endif /* defined(FB_RUN) && !FB_RUN */

#ifdef __cplusplus
}
#endif

#endif /* __SIM_AUTOMATION_H__ */
