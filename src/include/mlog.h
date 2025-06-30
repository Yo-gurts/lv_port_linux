/*
 * HEADER ONLY LOG
 */

#ifndef __MLOG_H
#define __MLOG_H

#include <string.h>

#define MLOG_LEVEL_NONE 0  // 无日志输出
#define MLOG_LEVEL_ERROR 1 // 错误日志
#define MLOG_LEVEL_WARN 2  // 警告日志
#define MLOG_LEVEL_INFO 3  // 信息日志
#define MLOG_LEVEL_DEBUG 4 // 调试日志

#define NONE "\e[0m"
#define RED "\e[1;31m"
#define YELLOW "\e[1;33m"
#define BLUE "\e[0;34m"
#define GREEN "\e[0;32m"

// 设置当前日志级别
#define MLOG_LEVEL MLOG_LEVEL_INFO

/*
 * 支持类似 uboot 的 DEBUG 宏控制：
 * 如果全局 MLOG_LEVEL <= MLOG_LEVEL_DEBUG，且文件内定义了 DEBUG，则该文件输出 DEBUG 日志
 */
#if(MLOG_LEVEL < MLOG_LEVEL_DEBUG)
#ifdef DEBUG
#undef MLOG_LEVEL
#define MLOG_LEVEL MLOG_LEVEL_DEBUG
#endif
#endif
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define MLOG(level, level_str, fmt, ...)                                                                               \
    do {                                                                                                               \
        if(level <= MLOG_LEVEL) {                                                                                      \
            printf(level_str " [%s:%d %s] " fmt, __FILENAME__, __LINE__, __func__, ##__VA_ARGS__);                     \
        }                                                                                                              \
    } while(0)

/* 只对日志级别加上颜色 */
// #define MLOG_ERROR(fmt, ...) MLOG(MLOG_LEVEL_ERROR, RED "[ERR]" NONE, fmt, ##__VA_ARGS__)
// #define MLOG_WARN(fmt, ...)  MLOG(MLOG_LEVEL_WARN, YELLOW "[WARN]" NONE, fmt, ##__VA_ARGS__)
// #define MLOG_INFO(fmt, ...)  MLOG(MLOG_LEVEL_INFO, BLUE "[INFO]" NONE, fmt, ##__VA_ARGS__)
// #define MLOG_DEBUG(fmt, ...) MLOG(MLOG_LEVEL_DEBUG, "[DBG]", fmt, ##__VA_ARGS__)

/* 所有日志行都上颜色 */
#define MLOG_ERR(fmt, ...) MLOG(MLOG_LEVEL_ERROR, RED "[ERR]", fmt NONE, ##__VA_ARGS__)
#define MLOG_WARN(fmt, ...) MLOG(MLOG_LEVEL_WARN, YELLOW "[WARN]", fmt NONE, ##__VA_ARGS__)
#define MLOG_INFO(fmt, ...) MLOG(MLOG_LEVEL_INFO, BLUE "[INFO]", fmt NONE, ##__VA_ARGS__)
#define MLOG_DBG(fmt, ...) MLOG(MLOG_LEVEL_DEBUG, GREEN "[DBG]", fmt NONE, ##__VA_ARGS__)

#endif
