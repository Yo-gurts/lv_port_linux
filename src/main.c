#include "config.h"
#include "core/page_manager.h"
#include "font_manager.h"
#include "lvgl/lvgl.h"
#include "mlog.h"
#include "pages/page_home.h"
#include "pages/page_photo.h"
#include "pages/page_photo_settings.h"
#include "pages/page_video.h"
#include "styles/style_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static const char* getenv_default(const char* name, const char* dflt)
{
    const char* env = getenv(name);
    return env ? env : dflt;
}

#if LV_USE_LINUX_FBDEV
static void lv_linux_disp_init(void)
{
    const char* device = getenv_default("LV_LINUX_FBDEV_DEVICE", FB_DEV_NAME);
    lv_display_t* disp = lv_linux_fbdev_create();

    lv_linux_fbdev_set_file(disp, device);
}
#elif LV_USE_LINUX_DRM
static void lv_linux_disp_init(void)
{
    const char* device = getenv_default("LV_LINUX_DRM_CARD", "/dev/dri/card0");
    lv_display_t* disp = lv_linux_drm_create();

    lv_linux_drm_set_file(disp, device, -1);
}
#elif LV_USE_SDL
static void lv_linux_disp_init(void)
{
    const int width = atoi(getenv_default("LV_SDL_VIDEO_WIDTH", "800"));
    const int height = atoi(getenv_default("LV_SDL_VIDEO_HEIGHT", "480"));

    lv_group_set_default(lv_group_create());

    lv_display_t* disp = lv_sdl_window_create(width, height);

    lv_indev_t* mouse = lv_sdl_mouse_create();
    lv_indev_set_group(mouse, lv_group_get_default());
    lv_indev_set_display(mouse, disp);
    lv_display_set_default(disp);

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

int main(void)
{
    lv_init();

    /*Linux display device init*/
    lv_linux_disp_init();

    /* Initialize fonts */
    font_manager_init();

    /* Initialize styles */
    style_common_init();

    /* Create page manager */
    page_manager_t* pm = page_manager_create();
    if (!pm) {
        MLOG_ERR("Failed to create page manager");
        return -1;
    }

    /* Register pages */
    page_manager_register(pm, "home", &home_page_interface, NULL);
    page_manager_register(pm, "photo", &photo_page_interface, NULL);
    page_manager_register(pm, "video", &video_page_interface, NULL);
    page_manager_register(pm, "photo_settings", &photo_settings_page_interface, NULL);

    /* Navigate to home page */
    page_manager_navigate(pm, "home");

    /*Handle LVGL tasks*/
    while (1) {
        lv_timer_handler();
        usleep(5000);
    }

    page_manager_destroy(pm);
    return 0;
}
