#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "config.h"

static const char *getenv_default(const char *name, const char *dflt)
{
    return getenv(name) ?: dflt;
}

#if LV_USE_LINUX_FBDEV
#include <linux/fb.h>

typedef struct
{
    const char *devname;
    lv_color_format_t color_format;
#if LV_LINUX_FBDEV_BSD
    struct bsd_fb_var_info vinfo;
    struct bsd_fb_fix_info finfo;
#else
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
#endif /* LV_LINUX_FBDEV_BSD */
#if LV_LINUX_FBDEV_MMAP
    char *fbp;
#endif
    uint8_t *rotated_buf;
    size_t rotated_buf_size;
    long int screensize;
    int fbfd;
    bool force_refresh;
} lv_linux_fb_t;

static void write_to_fb(lv_linux_fb_t *dsc, uint32_t fb_pos, const void *data, size_t sz)
{
#if LV_LINUX_FBDEV_MMAP
    uint8_t *fbp = (uint8_t *)dsc->fbp;
    lv_memcpy(&fbp[fb_pos], data, sz);
#else
    if(pwrite(dsc->fbfd, data, sz, fb_pos) < 0) LV_LOG_ERROR("write failed: %d", errno);
#endif
}

static void flush_cb_pandisplay(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p)
{
    lv_linux_fb_t *dsc = lv_display_get_driver_data(disp);

#if LV_LINUX_FBDEV_MMAP
    if(dsc->fbp == NULL) {
        lv_display_flush_ready(disp);
        return;
    }
#endif

    int32_t w            = lv_area_get_width(area);
    int32_t h            = lv_area_get_height(area);
    lv_color_format_t cf = lv_display_get_color_format(disp);
    uint32_t px_size     = lv_color_format_get_size(cf);

    lv_area_t rotated_area;
    lv_display_rotation_t rotation = lv_display_get_rotation(disp);

    /* Not all framebuffer kernel drivers support hardware rotation, so we need to handle it in software here */
    if(rotation != LV_DISPLAY_ROTATION_0 && LV_LINUX_FBDEV_RENDER_MODE == LV_DISPLAY_RENDER_MODE_PARTIAL) {
        /* (Re)allocate temporary buffer if needed */
        size_t buf_size = w * h * px_size;
        if(!dsc->rotated_buf || dsc->rotated_buf_size != buf_size) {
            dsc->rotated_buf      = realloc(dsc->rotated_buf, buf_size);
            dsc->rotated_buf_size = buf_size;
        }

        /* Rotate the pixel buffer */
        uint32_t w_stride = lv_draw_buf_width_to_stride(w, cf);
        uint32_t h_stride = lv_draw_buf_width_to_stride(h, cf);

        switch(rotation) {
            case LV_DISPLAY_ROTATION_0: break;
            case LV_DISPLAY_ROTATION_90:
                lv_draw_sw_rotate(color_p, dsc->rotated_buf, w, h, w_stride, h_stride, rotation, cf);
                break;
            case LV_DISPLAY_ROTATION_180:
                lv_draw_sw_rotate(color_p, dsc->rotated_buf, w, h, w_stride, w_stride, rotation, cf);
                break;
            case LV_DISPLAY_ROTATION_270:
                lv_draw_sw_rotate(color_p, dsc->rotated_buf, w, h, w_stride, h_stride, rotation, cf);
                break;
        }
        color_p = dsc->rotated_buf;

        /* Rotate the area */
        rotated_area = *area;
        lv_display_rotate_area(disp, &rotated_area);
        area = &rotated_area;

        if(rotation != LV_DISPLAY_ROTATION_180) {
            w = lv_area_get_width(area);
            h = lv_area_get_height(area);
        }
    }

    /* Ensure that we're within the framebuffer's bounds */
    if(area->x2 < 0 || area->y2 < 0 || area->x1 > (int32_t)dsc->vinfo.xres - 1 ||
       area->y1 > (int32_t)dsc->vinfo.yres - 1) {
        lv_display_flush_ready(disp);
        return;
    }

    uint32_t fb_pos =
        (area->x1 + dsc->vinfo.xoffset) * px_size + (area->y1 + dsc->vinfo.yoffset) * dsc->finfo.line_length;

    int32_t y;
    if(LV_LINUX_FBDEV_RENDER_MODE == LV_DISPLAY_RENDER_MODE_DIRECT) {
        uint32_t color_pos = area->x1 * px_size + area->y1 * lv_display_get_horizontal_resolution(disp) * px_size;

        for(y = area->y1; y <= area->y2; y++) {
            write_to_fb(dsc, fb_pos, &color_p[color_pos], w * px_size);
            fb_pos += dsc->finfo.line_length;
            color_pos += lv_display_get_horizontal_resolution(disp) * px_size;
        }
    } else {
        w = lv_area_get_width(area);
        for(y = area->y1; y <= area->y2; y++) {
            write_to_fb(dsc, fb_pos, color_p, w * px_size);
            fb_pos += dsc->finfo.line_length;
            color_p += w * px_size;
        }
    }

    if(dsc->force_refresh) {
        dsc->vinfo.activate |= FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;
        if(ioctl(dsc->fbfd, FBIOPUT_VSCREENINFO, &(dsc->vinfo)) == -1) {
            perror("Error setting var screen info");
        }
    }

    if(ioctl(dsc->fbfd, FBIOPAN_DISPLAY, &(dsc->vinfo)) < 0) {
        perror("FBIOPAN_DISPLAY failed!\n");
    }

    lv_display_flush_ready(disp);
}

static void lv_linux_disp_init(void)
{
    const char *device = getenv_default("LV_LINUX_FBDEV_DEVICE", FB_DEV_NAME);
    lv_display_t *disp = lv_linux_fbdev_create();

    lv_display_set_flush_cb(disp, flush_cb_pandisplay);
    lv_linux_fbdev_set_file(disp, device);
    lv_display_set_resolution(disp, H_RES, V_RES);

    // 支持触摸屏 TP evdev
    // lv_indev_t *touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, TOUCH_PANEL_EVENT_PATH);
    // lv_indev_set_display(touch, disp);
}
#elif LV_USE_LINUX_DRM
static void lv_linux_disp_init(void)
{
    const char *device = getenv_default("LV_LINUX_DRM_CARD", "/dev/dri/card0");
    lv_display_t *disp = lv_linux_drm_create();

    lv_linux_drm_set_file(disp, device, -1);
}
#elif LV_USE_SDL
static void lv_linux_disp_init(void)
{
    const int width  = H_RES;
    const int height = V_RES;

    lv_group_set_default(lv_group_create());

    lv_display_t *disp = lv_sdl_window_create(width, height);

    lv_indev_t *mouse = lv_sdl_mouse_create();
    lv_indev_set_group(mouse, lv_group_get_default());
    lv_indev_set_display(mouse, disp);
    lv_display_set_default(disp);

    LV_IMAGE_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
    lv_obj_t *cursor_obj;
    cursor_obj = lv_image_create(lv_screen_active()); /*Create an image object for the cursor */
    lv_image_set_src(cursor_obj, &mouse_cursor_icon); /*Set the image source*/
    lv_indev_set_cursor(mouse, cursor_obj);           /*Connect the image  object to the driver*/

    lv_indev_t *mousewheel = lv_sdl_mousewheel_create();
    lv_indev_set_display(mousewheel, disp);
    lv_indev_set_group(mousewheel, lv_group_get_default());

    lv_indev_t *kb = lv_sdl_keyboard_create();
    lv_indev_set_display(kb, disp);
    lv_indev_set_group(kb, lv_group_get_default());

    // return disp;
}
#else
#error Unsupported configuration
#endif

int main(void)
{
    lv_init();

    /*Linux display device init*/
    lv_linux_disp_init();

    /*Create a Demo*/
    lv_demo_widgets();
    lv_demo_widgets_start_slideshow();

    /*Handle LVGL tasks*/
    while(1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}
