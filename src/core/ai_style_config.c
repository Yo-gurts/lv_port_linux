#include "core/ai_style_config.h"
#include "config.h"
#include "mlog.h"
#include <stdio.h>
#include <string.h>

/* 当前生效的风格表，索引 0 固定为「原图」。 */
static ai_style_entry_t g_styles[AI_STYLE_MAX_COUNT];
static int g_style_count = 0;

/* 去掉字符串首尾空白（空格/制表/换行），原地修改。返回处理后的起始指针。 */
static char* trim(char* s)
{
    char* end;

    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') {
        s++;
    }
    if (*s == '\0') {
        return s;
    }

    end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        *end-- = '\0';
    }
    return s;
}

/* 去掉 value 两端成对的引号（提示词可能带引号），原地修改。 */
static char* strip_quotes(char* s)
{
    size_t len = strlen(s);
    if (len >= 2 && ((s[0] == '"' && s[len - 1] == '"') || (s[0] == '\'' && s[len - 1] == '\''))) {
        s[len - 1] = '\0';
        s++;
    }
    return s;
}

/* 内置第 0 项「原图」，并把风格表重置为仅含它。 */
static void reset_to_original_only(void)
{
    memset(g_styles, 0, sizeof(g_styles));
    snprintf(g_styles[0].name, sizeof(g_styles[0].name), "%s", "原图");
    g_styles[0].prompt[0] = '\0';
    g_styles[0].thumb[0] = '\0';
    g_style_count = 1;
}

/* 解析一个 INI 文件，把其中的风格追加到「原图」之后（不含原图，配置文件不应写原图）。
 * 成功解析出 >=1 项返回解析到的风格数，否则返回 0（保持仅原图，由调用方决定回退）。 */
static int parse_ini(const char* path)
{
    FILE* fp;
    char line[512];
    int count = 1; /* 已含内置原图 */
    int in_section = 0;

    fp = fopen(path, "r");
    if (!fp) {
        return 0;
    }

    while (fgets(line, sizeof(line), fp)) {
        char* p = trim(line);

        if (*p == '\0' || *p == ';' || *p == '#') {
            continue; /* 空行或注释 */
        }

        if (*p == '[') {
            /* 新的一个风格段落 */
            if (count >= AI_STYLE_MAX_COUNT) {
                MLOG_WARN("风格条目超过上限 %d，忽略后续项", AI_STYLE_MAX_COUNT);
                break;
            }
            memset(&g_styles[count], 0, sizeof(g_styles[count]));
            in_section = 1;
            count++;
            continue;
        }

        if (in_section) {
            char* eq = strchr(p, '=');
            char* key;
            char* val;
            ai_style_entry_t* cur = &g_styles[count - 1];

            if (!eq) {
                continue; /* 非 key=value，跳过 */
            }
            *eq = '\0';
            key = trim(p);
            val = strip_quotes(trim(eq + 1));

            if (strcmp(key, "name") == 0) {
                snprintf(cur->name, sizeof(cur->name), "%s", val);
            } else if (strcmp(key, "prompt") == 0) {
                snprintf(cur->prompt, sizeof(cur->prompt), "%s", val);
            } else if (strcmp(key, "thumb") == 0) {
                snprintf(cur->thumb, sizeof(cur->thumb), "%s", val);
            }
        }
    }

    fclose(fp);

    /* 只保留有显示名的风格：把 name 为空的段落剔除（坏行不至于产生空项） */
    {
        int w = 1;
        int r;
        for (r = 1; r < count; r++) {
            if (g_styles[r].name[0] != '\0') {
                if (w != r) {
                    g_styles[w] = g_styles[r];
                }
                w++;
            }
        }
        count = w;
    }

    return (count > 1) ? count : 0;
}

int ai_style_config_load(void)
{
    int n;

    reset_to_original_only();

    /* 优先 /mnt/data 覆盖版 */
    n = parse_ini(AI_STYLE_INI_DATA_PATH);
    if (n > 0) {
        g_style_count = n;
        MLOG_INFO("AI风格表加载自 %s，共 %d 项", AI_STYLE_INI_DATA_PATH, g_style_count);
        return g_style_count;
    }

    /* 回退到 /app/res 基线版 */
    reset_to_original_only();
    n = parse_ini(AI_STYLE_INI_RES_PATH);
    if (n > 0) {
        g_style_count = n;
        MLOG_INFO("AI风格表加载自 %s，共 %d 项", AI_STYLE_INI_RES_PATH, g_style_count);
        return g_style_count;
    }

    /* 都失败：仅内置原图 */
    reset_to_original_only();
    MLOG_WARN("AI风格配置缺失或解析失败，仅保留「原图」一项");
    return g_style_count;
}

const ai_style_entry_t* ai_style_config_get(int index)
{
    if (index < 0 || index >= g_style_count) {
        return NULL;
    }
    return &g_styles[index];
}
