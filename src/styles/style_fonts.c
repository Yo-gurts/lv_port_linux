#include "styles/style_fonts.h"
#include <stdio.h>

lv_style_t style_font_12;
lv_style_t style_font_14;
lv_style_t style_font_16;
lv_style_t style_font_18;
lv_style_t style_font_20;
lv_style_t style_font_22;
lv_style_t style_font_24;
lv_style_t style_font_28;
lv_style_t style_font_30;
lv_style_t style_font_34;

void style_fonts_init(void)
{
    printf("Initializing font styles\n");
    
    lv_style_t* style_list[] = { &style_font_12, &style_font_14, &style_font_16, &style_font_18, &style_font_20,
        &style_font_22, &style_font_24, &style_font_28, &style_font_30, &style_font_34 };
    
    for (size_t i = 0; i < sizeof(style_list) / sizeof(style_list[0]); i++) {
        lv_style_init(style_list[i]);
        lv_style_set_opa(style_list[i], LV_OPA_COVER);
        lv_style_set_text_color(style_list[i], lv_color_white());
        lv_style_set_text_align(style_list[i], LV_TEXT_ALIGN_CENTER);
    }
}
