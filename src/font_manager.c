#include "font_manager.h"
#include "config.h"
#include "lvgl/src/libs/freetype/lv_freetype.h"
#include "mlog.h"
#include <stdlib.h>

/* 中文字体样式 */
lv_style_t ttf_font_12;
lv_style_t ttf_font_14;
lv_style_t ttf_font_16;
lv_style_t ttf_font_18;
lv_style_t ttf_font_20;
lv_style_t ttf_font_22;
lv_style_t ttf_font_24;
lv_style_t ttf_font_26;
lv_style_t ttf_font_28;
lv_style_t ttf_font_30;
lv_style_t ttf_font_34;

static void init_font_style(lv_style_t* style, int size, lv_color_t color)
{
    lv_font_t* font = lv_freetype_font_create(CHINESE_FONT_PATH,
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
        size,
        LV_FREETYPE_FONT_STYLE_NORMAL);
    if (!font) {
        MLOG_ERR("Failed to create font size: %d", size);
        return;
    }

    lv_style_init(style);
    lv_style_set_opa(style, LV_OPA_COVER);
    lv_style_set_text_color(style, color);
    lv_style_set_text_opa(style, 255);
    lv_style_set_text_align(style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_font(style, font);
}

int font_manager_init(void)
{
    MLOG_INFO("Loading Chinese font from: %s", CHINESE_FONT_PATH);

    lv_color_t white = lv_color_white();

    init_font_style(&ttf_font_12, 12, white);
    init_font_style(&ttf_font_14, 14, white);
    init_font_style(&ttf_font_16, 16, white);
    init_font_style(&ttf_font_18, 18, white);
    init_font_style(&ttf_font_20, 20, white);
    init_font_style(&ttf_font_22, 22, white);
    init_font_style(&ttf_font_24, 24, white);
    init_font_style(&ttf_font_26, 26, white);
    init_font_style(&ttf_font_28, 28, white);
    init_font_style(&ttf_font_30, 30, white);
    init_font_style(&ttf_font_34, 34, white);

    MLOG_INFO("Chinese font loaded successfully");
    return 0;
}
