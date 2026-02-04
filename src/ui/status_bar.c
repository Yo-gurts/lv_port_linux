#include "ui/status_bar.h"
#include <stdlib.h>
#include <string.h>

#define MAX_ITEMS 10

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
    sb->item_count = 0;
    sb->height = 30;
    sb->bg_color = lv_color_hex(0x2C3E50);
    sb->text_color = lv_color_hex(0xFFFFFF);

    sb->bar_obj = lv_obj_create(parent);
    lv_obj_set_size(sb->bar_obj, lv_pct(100), sb->height);
    lv_obj_set_style_bg_color(sb->bar_obj, sb->bg_color, LV_PART_MAIN);

    sb->left_container = lv_obj_create(sb->bar_obj);
    lv_obj_set_size(sb->left_container, lv_pct(50), lv_pct(100));
    lv_obj_set_layout(sb->left_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sb->left_container, LV_FLEX_FLOW_ROW);
    lv_obj_align(sb->left_container, LV_ALIGN_LEFT_MID, 0, 0);

    sb->right_container = lv_obj_create(sb->bar_obj);
    lv_obj_set_size(sb->right_container, lv_pct(50), lv_pct(100));
    lv_obj_set_layout(sb->right_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sb->right_container, LV_FLEX_FLOW_ROW);
    lv_obj_align(sb->right_container, LV_ALIGN_RIGHT_MID, 0, 0);

    return sb;
}

void status_bar_destroy(status_bar_t* sb)
{
    if (!sb) {
        return;
    }

    for (int i = 0; i < sb->item_count; i++) {
        if (sb->item_widgets[i]) {
            lv_obj_del(sb->item_widgets[i]);
            sb->item_widgets[i] = NULL;
        }
    }

    if (sb->right_container) {
        lv_obj_del(sb->right_container);
        sb->right_container = NULL;
    }

    if (sb->left_container) {
        lv_obj_del(sb->left_container);
        sb->left_container = NULL;
    }

    if (sb->bar_obj) {
        lv_obj_del(sb->bar_obj);
        sb->bar_obj = NULL;
    }

    free(sb);
}

int status_bar_add_item(status_bar_t* sb, status_bar_item_t* item)
{
    if (!sb || !item || sb->item_count >= MAX_ITEMS) {
        return -1;
    }

    int index = sb->item_count;
    memcpy(&sb->items[index], item, sizeof(status_bar_item_t));

    if (item->type == STATUS_BAR_ITEM_TYPE_TEXT) {
        sb->item_widgets[index] = lv_label_create(sb->left_container);
        lv_label_set_text(sb->item_widgets[index], item->text);
        lv_obj_set_style_text_color(sb->item_widgets[index], sb->text_color, LV_PART_MAIN);
    } else if (item->type == STATUS_BAR_ITEM_TYPE_ICON) {
        sb->item_widgets[index] = lv_img_create(sb->right_container);
        lv_img_set_src(sb->item_widgets[index], item->icon_data);
    } else if (item->type == STATUS_BAR_ITEM_TYPE_PROGRESS) {
        sb->item_widgets[index] = lv_bar_create(sb->right_container);
        lv_bar_set_value(sb->item_widgets[index], item->progress, LV_ANIM_OFF);
    }

    sb->item_count++;

    return 0;
}

int status_bar_remove_item(status_bar_t* sb, int index)
{
    if (!sb || index < 0 || index >= sb->item_count) {
        return -1;
    }

    if (sb->item_widgets[index]) {
        lv_obj_del(sb->item_widgets[index]);
        sb->item_widgets[index] = NULL;
    }

    for (int i = index; i < sb->item_count - 1; i++) {
        memcpy(&sb->items[i], &sb->items[i + 1], sizeof(status_bar_item_t));
        sb->item_widgets[i] = sb->item_widgets[i + 1];
    }

    sb->item_count--;

    return 0;
}

void status_bar_clear(status_bar_t* sb)
{
    if (!sb) {
        return;
    }

    for (int i = 0; i < sb->item_count; i++) {
        if (sb->item_widgets[i]) {
            lv_obj_del(sb->item_widgets[i]);
            sb->item_widgets[i] = NULL;
        }
    }

    sb->item_count = 0;
}

void status_bar_set_item_text(status_bar_t* sb, int index, const char* text)
{
    if (!sb || index < 0 || index >= sb->item_count) {
        return;
    }

    sb->items[index].text = text;

    if (sb->items[index].type == STATUS_BAR_ITEM_TYPE_TEXT && sb->item_widgets[index]) {
        lv_label_set_text(sb->item_widgets[index], text);
    }
}

void status_bar_set_item_progress(status_bar_t* sb, int index, int progress)
{
    if (!sb || index < 0 || index >= sb->item_count) {
        return;
    }

    sb->items[index].progress = progress;

    if (sb->items[index].type == STATUS_BAR_ITEM_TYPE_PROGRESS && sb->item_widgets[index]) {
        lv_bar_set_value(sb->item_widgets[index], progress, LV_ANIM_OFF);
    }
}

void status_bar_set_item_visible(status_bar_t* sb, int index, bool visible)
{
    if (!sb || index < 0 || index >= sb->item_count) {
        return;
    }

    sb->items[index].visible = visible;

    if (sb->item_widgets[index]) {
        if (visible) {
            lv_obj_clear_flag(sb->item_widgets[index], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(sb->item_widgets[index], LV_OBJ_FLAG_HIDDEN);
        }
    }
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

    for (int i = 0; i < sb->item_count; i++) {
        if (sb->items[i].type == STATUS_BAR_ITEM_TYPE_TEXT && sb->item_widgets[i]) {
            lv_obj_set_style_text_color(sb->item_widgets[i], color, LV_PART_MAIN);
        }
    }
}

void status_bar_refresh(status_bar_t* sb)
{
    if (!sb) {
        return;
    }

    for (int i = 0; i < sb->item_count; i++) {
        if (sb->items[i].type == STATUS_BAR_ITEM_TYPE_TEXT && sb->item_widgets[i]) {
            lv_label_set_text(sb->item_widgets[i], sb->items[i].text);
        } else if (sb->items[i].type == STATUS_BAR_ITEM_TYPE_PROGRESS && sb->item_widgets[i]) {
            lv_bar_set_value(sb->item_widgets[i], sb->items[i].progress, LV_ANIM_OFF);
        }
    }
}
