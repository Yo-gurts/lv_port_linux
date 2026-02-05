#include "ui/status_bar.h"
#include "config.h"
#include "font_manager.h"
#include "mlog.h"
#include <stdlib.h>
#include <string.h>

static const char* get_battery_icon_path(int level)
{
    if (level < 0) {
        level = 0;
    }
    if (level >= 100) {
        return "A:" RES_ICON_PATH "/battery100%.png";
    } else if (level >= 66) {
        return "A:" RES_ICON_PATH "/battery66%.png";
    } else if (level >= 33) {
        return "A:" RES_ICON_PATH "/battery33%.png";
    } else {
        return "A:" RES_ICON_PATH "/battery33%.png";
    }
}

status_bar_t* status_bar_create(lv_obj_t* parent)
{
    if (!parent) {
        return NULL;
    }

    status_bar_t* sb = (status_bar_t*)malloc(sizeof(status_bar_t));
    if (!sb) {
        return NULL;
    }

    memset(sb, 0, sizeof(status_bar_t));

    sb->parent = parent;
    sb->height = 50;
    sb->bg_color = lv_color_hex(0x2C3E50);
    sb->text_color = lv_color_hex(0xFFFFFF);

    /* Main bar object */
    sb->bar_obj = lv_obj_create(parent);
    lv_obj_set_size(sb->bar_obj, lv_pct(100), sb->height);
    lv_obj_set_style_bg_opa(sb->bar_obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(sb->bar_obj, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(sb->bar_obj, LV_SCROLLBAR_MODE_OFF);

    /* Time label - center */
    sb->time_label = lv_label_create(sb->bar_obj);
    lv_label_set_text(sb->time_label, "0000-00-00 00:00:00");
    lv_obj_add_style(sb->time_label, &ttf_font_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(sb->time_label, lv_color_black(), LV_PART_MAIN);
    lv_obj_align(sb->time_label, LV_ALIGN_CENTER, 0, 0);

    /* Wifi icon - right */
    sb->wifi_icon = lv_label_create(sb->bar_obj);
    lv_label_set_text(sb->wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(sb->wifi_icon, lv_color_black(), LV_PART_MAIN);
    lv_obj_align(sb->wifi_icon, LV_ALIGN_RIGHT_MID, -60, 0);

    /* Battery icon - right */
    sb->battery_icon = lv_img_create(sb->bar_obj);
    lv_img_set_src(sb->battery_icon, get_battery_icon_path(100));
    lv_obj_align(sb->battery_icon, LV_ALIGN_RIGHT_MID, -10, 0);

    return sb;
}

void status_bar_destroy(status_bar_t* sb)
{
    if (!sb) {
        return;
    }

    if (sb->battery_icon) {
        lv_obj_del(sb->battery_icon);
        sb->battery_icon = NULL;
    }

    if (sb->wifi_icon) {
        lv_obj_del(sb->wifi_icon);
        sb->wifi_icon = NULL;
    }

    if (sb->time_label) {
        lv_obj_del(sb->time_label);
        sb->time_label = NULL;
    }

    if (sb->bar_obj) {
        lv_obj_del(sb->bar_obj);
        sb->bar_obj = NULL;
    }

    free(sb);
}

void status_bar_set_height(status_bar_t* sb, int height)
{
    if (!sb) {
        return;
    }

    sb->height = height;

    if (sb->bar_obj) {
        lv_obj_set_height(sb->bar_obj, height);
    }
}

int status_bar_get_height(status_bar_t* sb)
{
    if (!sb) {
        return 0;
    }

    return sb->height;
}

void status_bar_set_bg_color(status_bar_t* sb, lv_color_t color)
{
    if (!sb) {
        return;
    }

    sb->bg_color = color;

    if (sb->bar_obj) {
        lv_obj_set_style_bg_color(sb->bar_obj, color, LV_PART_MAIN);
    }
}

void status_bar_set_text_color(status_bar_t* sb, lv_color_t color)
{
    if (!sb) {
        return;
    }

    sb->text_color = color;

    if (sb->time_label) {
        lv_obj_set_style_text_color(sb->time_label, color, LV_PART_MAIN);
    }
    if (sb->wifi_icon) {
        lv_obj_set_style_text_color(sb->wifi_icon, color, LV_PART_MAIN);
    }
}

void status_bar_set_time(status_bar_t* sb, const char* time_str)
{
    if (!sb || !sb->time_label) {
        return;
    }

    if (time_str) {
        lv_label_set_text(sb->time_label, time_str);
    }
}

void status_bar_set_wifi_icon(status_bar_t* sb, int level)
{
    if (!sb || !sb->wifi_icon) {
        return;
    }

    /* WiFi signal level: 0-3 */
    const char* wifi_symbols[] = { " .  ", " .. ", "... ", LV_SYMBOL_WIFI };
    int idx = level < 0 ? 0 : (level > 3 ? 3 : level);
    lv_label_set_text(sb->wifi_icon, wifi_symbols[idx]);
}

void status_bar_set_battery_icon(status_bar_t* sb, int level)
{
    if (!sb || !sb->battery_icon) {
        return;
    }

    lv_img_set_src(sb->battery_icon, get_battery_icon_path(level));
}
