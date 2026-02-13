/**
 * @file    ui_main.c
 * @brief   板端入口，提供 ui_main() 接口供外部调用
 */
#include "ui_main.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/page_manager.h"
#include "core/style_manager.h"
#include "lvgl/lvgl.h"
#include "mlog.h"
#include "pages/page_ai_photo.h"
#include "pages/page_ai_photo_settings.h"
#include "pages/page_album.h"
#include "pages/page_chat.h"
#include "pages/page_home.h"
#include "pages/page_photo.h"
#include "pages/page_photo_settings.h"
#include "pages/page_system_settings.h"
#include "pages/page_version_info.h"
#include "pages/page_video.h"
#include "pages/page_video_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#if LV_USE_LINUX_FBDEV
#include "core/framebuffer_manager.h"
#include "lvgl/src/drivers/evdev/lv_evdev.h"

static void lv_linux_disp_init(void)
{
    const char* device = FB_DEV_NAME;
    lv_display_t* disp = lv_linux_fbdev_create();

    lv_linux_fbdev_set_file(disp, device);
    lv_display_set_resolution(disp, H_RES, V_RES);
    // 使用 ARGB，默认 XRGB 会有页面残留（与LVGL版本相关）
    lv_display_set_color_format(disp, 0x10); // 0x10 ARGB, 0x11 XRGB
    /* screen 设置为全透明，以便显示底层画面 */
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_TRANSP, LV_PART_MAIN);

    framebuffer_manager_t* fb_mgr = framebuffer_manager_create(FB_DEV_NAME, disp);
    if (!fb_mgr) {
        MLOG_ERR("Failed to create framebuffer manager");
    }

    /* Initialize touchscreen input device */
#if LV_USE_EVDEV
    lv_indev_t* indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, TOUCH_PANEL_EVENT_PATH);
    if (!indev) {
        MLOG_ERR("Failed to create touchscreen input: %s", TOUCH_PANEL_EVENT_PATH);
    }
#endif
}
#elif LV_USE_SDL
static void lv_linux_disp_init(void)
{
    const int width = H_RES;
    const int height = V_RES;

    lv_group_set_default(lv_group_create());

    lv_display_t* disp = lv_sdl_window_create(width, height);

    lv_indev_t* mouse = lv_sdl_mouse_create();
    lv_indev_set_group(mouse, lv_group_get_default());
    lv_indev_set_display(mouse, disp);
    lv_display_set_default(disp);
    // 使用 ARGB，默认 XRGB 会有页面残留（与LVGL版本相关）
    lv_display_set_color_format(disp, 0x10); // 0x10 ARGB, 0x11 XRGB

    LV_IMAGE_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
    lv_obj_t* cursor_obj;
    cursor_obj = lv_image_create(lv_screen_active()); /*Create an image object for the cursor */
    lv_image_set_src(cursor_obj, &mouse_cursor_icon); /*Set the image source*/
    lv_indev_set_cursor(mouse, cursor_obj); /*Connect the image  object to the driver*/

    lv_indev_t* mousewheel = lv_sdl_mousewheel_create();
    lv_indev_set_display(mousewheel, disp);
    lv_indev_set_group(mousewheel, lv_group_get_default());

    lv_indev_t* kb = lv_sdl_keyboard_create();
    lv_indev_set_display(kb, disp);
    lv_indev_set_group(kb, lv_group_get_default());

    // return disp;
}
#else
#error Unsupported configuration
#endif

static page_interface_t home_page_interface = {
    .create = page_home_create,
    .destroy = page_home_destroy,
    .show = page_home_show,
    .hide = page_home_hide,
    .update = page_home_update,
};

static page_interface_t photo_page_interface = {
    .create = page_photo_create,
    .destroy = page_photo_destroy,
    .show = page_photo_show,
    .hide = page_photo_hide,
    .update = page_photo_update,
};

static page_interface_t ai_photo_page_interface = {
    .create = page_ai_photo_create,
    .destroy = page_ai_photo_destroy,
    .show = page_ai_photo_show,
    .hide = page_ai_photo_hide,
    .update = page_ai_photo_update,
};

static page_interface_t video_page_interface = {
    .create = page_video_create,
    .destroy = page_video_destroy,
    .show = page_video_show,
    .hide = page_video_hide,
    .update = page_video_update,
};

static page_interface_t photo_settings_page_interface = {
    .create = page_photo_settings_create,
    .destroy = page_photo_settings_destroy,
    .show = page_photo_settings_show,
    .hide = page_photo_settings_hide,
    .update = page_photo_settings_update,
};

static page_interface_t video_settings_page_interface = {
    .create = page_video_settings_create,
    .destroy = page_video_settings_destroy,
    .show = page_video_settings_show,
    .hide = page_video_settings_hide,
    .update = page_video_settings_update,
};

static page_interface_t system_settings_page_interface = {
    .create = page_system_settings_create,
    .destroy = page_system_settings_destroy,
    .show = page_system_settings_show,
    .hide = page_system_settings_hide,
    .update = page_system_settings_update,
};

static page_interface_t version_info_page_interface = {
    .create = page_version_info_create,
    .destroy = page_version_info_destroy,
    .show = page_version_info_show,
    .hide = page_version_info_hide,
    .update = page_version_info_update,
};

static page_interface_t ai_photo_settings_page_interface = {
    .create = page_ai_photo_settings_create,
    .destroy = page_ai_photo_settings_destroy,
    .show = page_ai_photo_settings_show,
    .hide = page_ai_photo_settings_hide,
    .update = page_ai_photo_settings_update,
};

static page_interface_t chat_page_interface = {
    .create = page_chat_create,
    .destroy = page_chat_destroy,
    .show = page_chat_show,
    .hide = page_chat_hide,
    .update = page_chat_update,
};

static page_interface_t album_page_interface = {
    .create = page_album_create,
    .destroy = page_album_destroy,
    .show = page_album_show,
    .hide = page_album_hide,
    .update = page_album_update,
};

int ui_main(void)
{
    lv_init();

    /* Linux display device init */
    lv_linux_disp_init();

    /* Initialize fonts */
    font_manager_init();

    /* Initialize styles */
    style_common_init();

    /* Create page manager */
    if (page_manager_create() != 0) {
        MLOG_ERR("Failed to create page manager");
        return -1;
    }

    /* Register pages */
    page_manager_register("home", &home_page_interface, NULL);
    page_manager_register("photo", &photo_page_interface, NULL);
    page_manager_register("ai_photo", &ai_photo_page_interface, NULL);
    page_manager_register("video", &video_page_interface, NULL);
    page_manager_register("photo_settings", &photo_settings_page_interface, NULL);
    page_manager_register("video_settings", &video_settings_page_interface, NULL);
    page_manager_register("system_settings", &system_settings_page_interface, NULL);
    page_manager_register("version_info", &version_info_page_interface, NULL);
    page_manager_register("ai_photo_settings", &ai_photo_settings_page_interface, NULL);
    page_manager_register("chat", &chat_page_interface, NULL);
    page_manager_register("album", &album_page_interface, NULL);

    /* Navigate to home page */
    page_manager_navigate("home");

    /* Handle LVGL tasks */
    while (1) {
        lv_timer_handler();
        usleep(5000);
    }

    page_manager_destroy();
    return 0;
}
