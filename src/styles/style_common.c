#include "styles/style_common.h"
#include <stdio.h>

lv_style_t style_common_cont_top;
lv_style_t style_common_main_bg;
lv_style_t style_common_btn_back;
lv_style_t style_common_label_back;
lv_style_t style_focus_orange;

void style_common_init(void)
{
    printf("Initializing common styles\n");
    
    lv_style_init(&style_common_cont_top);
    lv_style_set_border_width(&style_common_cont_top, 0);
    lv_style_set_radius(&style_common_cont_top, 0);
    lv_style_set_bg_opa(&style_common_cont_top, LV_OPA_TRANSP);
    lv_style_set_bg_color(&style_common_cont_top, lv_color_black());
    lv_style_set_bg_grad_dir(&style_common_cont_top, LV_GRAD_DIR_NONE);
    lv_style_set_pad_top(&style_common_cont_top, 0);
    lv_style_set_pad_bottom(&style_common_cont_top, 0);
    lv_style_set_pad_left(&style_common_cont_top, 0);
    lv_style_set_pad_right(&style_common_cont_top, 0);
    lv_style_set_shadow_width(&style_common_cont_top, 0);
    
    lv_style_init(&style_common_main_bg);
    lv_style_set_bg_opa(&style_common_main_bg, LV_OPA_COVER);
    lv_style_set_bg_color(&style_common_main_bg, lv_color_hex(0x000000));
    
    lv_style_init(&style_common_btn_back);
    lv_style_set_bg_opa(&style_common_btn_back, LV_OPA_COVER);
    lv_style_set_bg_color(&style_common_btn_back, lv_color_hex(0x171717));
    lv_style_set_border_width(&style_common_btn_back, 0);
    lv_style_set_radius(&style_common_btn_back, 20);
    lv_style_set_shadow_width(&style_common_btn_back, 0);
    lv_style_set_text_color(&style_common_btn_back, lv_color_hex(0x1A1A1A));
    lv_style_set_text_align(&style_common_btn_back, LV_TEXT_ALIGN_CENTER);
    lv_style_set_pad_top(&style_common_btn_back, 0);
    lv_style_set_pad_bottom(&style_common_btn_back, 0);
    lv_style_set_pad_left(&style_common_btn_back, 0);
    lv_style_set_pad_right(&style_common_btn_back, 0);
    
    lv_style_init(&style_common_label_back);
    lv_style_set_text_color(&style_common_label_back, lv_color_hex(0xFFFFFF));
    lv_style_set_text_align(&style_common_label_back, LV_TEXT_ALIGN_CENTER);
    
    lv_style_init(&style_focus_orange);
    lv_style_set_outline_color(&style_focus_orange, lv_color_hex(0xF09F20));
    lv_style_set_outline_opa(&style_focus_orange, LV_OPA_COVER);
}
