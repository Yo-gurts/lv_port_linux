#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "core/page_manager.h"
#include "styles/style_common.h"
#include "styles/style_fonts.h"
#include "pages/page_home.h"

static const char *getenv_default(const char *name, const char *dflt)
{
    return getenv(name) ? : dflt;
}

#if LV_USE_LINUX_FBDEV
static void lv_linux_disp_init(void)
{
    const char *device = getenv_default("LV_LINUX_FBDEV_DEVICE", "/dev/fb0");
    lv_display_t * disp = lv_linux_fbdev_create();

    lv_linux_fbdev_set_file(disp, device);
}
#elif LV_USE_LINUX_DRM
static void lv_linux_disp_init(void)
{
    const char *device = getenv_default("LV_LINUX_DRM_CARD", "/dev/dri/card0");
    lv_display_t * disp = lv_linux_drm_create();

    lv_linux_drm_set_file(disp, device, -1);
}
#elif LV_USE_SDL
static void lv_linux_disp_init(void)
{
    const int width = atoi(getenv("LV_SDL_VIDEO_WIDTH") ? : "800");
    const int height = atoi(getenv("LV_SDL_VIDEO_HEIGHT") ? : "480");

    lv_group_set_default(lv_group_create());

    lv_display_t * disp = lv_sdl_window_create(width, height);

    lv_indev_t * mouse = lv_sdl_mouse_create();
    lv_indev_set_group(mouse, lv_group_get_default());
    lv_indev_set_display(mouse, disp);
    lv_display_set_default(disp);

    LV_IMAGE_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
    lv_obj_t * cursor_obj;
    cursor_obj = lv_image_create(lv_screen_active()); /*Create an image object for the cursor */
    lv_image_set_src(cursor_obj, &mouse_cursor_icon); /*Set the image source*/
    lv_indev_set_cursor(mouse, cursor_obj);           /*Connect the image  object to the driver*/

    lv_indev_t * mousewheel = lv_sdl_mousewheel_create();
    lv_indev_set_display(mousewheel, disp);
    lv_indev_set_group(mousewheel, lv_group_get_default());

    lv_indev_t * kb = lv_sdl_keyboard_create();
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

int main(void)
{
    lv_init();

    /*Linux display device init*/
    lv_linux_disp_init();

    /* Initialize styles */
    style_common_init();
    style_fonts_init();

    /* Create page manager */
    page_manager_t *pm = page_manager_create();
    if (!pm) {
        printf("Failed to create page manager\n");
        return -1;
    }

    /* Register pages */
    page_manager_register(pm, "home", &home_page_interface, NULL);

    /* Navigate to home page */
    page_manager_navigate(pm, "home");

    /*Handle LVGL tasks*/
    while(1) {
        lv_timer_handler();
        usleep(5000);
    }

    page_manager_destroy(pm);
    return 0;
}
