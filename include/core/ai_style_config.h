#ifndef __AI_STYLE_CONFIG_H__
#define __AI_STYLE_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* AI 风格转换类型表：从 INI 配置文件加载，取代原先的硬编码数组。
 * 第 0 项固定为「原图」（prompt 为空，不做 AI 处理），后续项来自配置文件。 */

#define AI_STYLE_NAME_MAX 32
#define AI_STYLE_PROMPT_MAX 256
#define AI_STYLE_THUMB_MAX 128
#define AI_STYLE_MAX_COUNT 64 /* 风格条目数上限（含原图） */

typedef struct {
    char name[AI_STYLE_NAME_MAX]; /* 显示名 */
    char prompt[AI_STYLE_PROMPT_MAX]; /* 提示词，空串表示不做 AI 处理（如原图） */
    char thumb[AI_STYLE_THUMB_MAX]; /* 缩略图路径（不含 A: 前缀），空串用默认图 */
} ai_style_entry_t;

/* 加载风格表：先内置「原图」，再按优先级读配置文件整体覆盖后续项——
 * 优先 /mnt/data 版（存在且至少一项），否则 /app/res 版，都失败则只剩「原图」。
 * 返回最终条目数（至少为 1）。可多次调用（进页面重读）。 */
int ai_style_config_load(void);

/* 取第 index 项，越界返回 NULL。 */
const ai_style_entry_t* ai_style_config_get(int index);

#ifdef __cplusplus
}
#endif

#endif /* __AI_STYLE_CONFIG_H__ */
