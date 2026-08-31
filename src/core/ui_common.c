/**
 * @file    ui_common.c
 * @brief   UI 共用主流程实现
 */
#include "ui_common.h"
#include "config.h"
#include "core/font_manager.h"
#include "core/key_manager.h"
#include "core/media_manager.h"
#include "core/message_manager.h"
#include "core/page_manager.h"
#include "core/param_manager.h"
#include "core/power_manager.h"
#include "core/style_manager.h"
#include "core/wifi_manager.h"
#include "lvgl/lvgl.h"
#include "mlog.h"
#include "pages/page_ai_photo.h"
#include "pages/page_ai_photo_settings.h"
#include "pages/page_ai_recognition_preview.h"
#include "pages/page_ai_style_preview.h"
#include "pages/page_album.h"
#include "pages/page_boot_switch_test.h"
#include "pages/page_chat.h"
#include "pages/page_home.h"
#include "pages/page_key_test.h"
#include "pages/page_loop_ptest.h"
#include "pages/page_photo.h"
#include "pages/page_photo_preview.h"
#include "pages/page_photo_resolution_test.h"
#include "pages/page_photo_settings.h"
#include "pages/page_system_settings.h"
#include "pages/page_test.h"
#include "pages/page_touch_test.h"
#include "pages/page_version_info.h"
#include "pages/page_video.h"
#include "pages/page_video_album.h"
#include "pages/page_video_preview.h"
#include "pages/page_video_settings.h"
#include "pages/page_wifi_list.h"
#include "ui/status_bar.h"
#include "ui/top_notice.h"
#include "ui/volume_bar.h"
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
    } else {
        // 将输入设备交给 key manager 管理，统一处理输入屏蔽逻辑
        key_manager_bind_touch_indev(indev);
    }
#endif
}
#elif LV_USE_SDL
#include <SDL2/SDL.h>

static key_id_t sdl_key_to_key_id(SDL_Keycode sym)
{
    switch (sym) {
    case SDLK_UP:
        return KEY_ID_UP;
    case SDLK_DOWN:
        return KEY_ID_DOWN;
    case SDLK_LEFT:
        return KEY_ID_LEFT;
    case SDLK_RIGHT:
        return KEY_ID_RIGHT;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        return KEY_ID_OK;
    case SDLK_SPACE:
        return KEY_ID_CAMERA;
    case SDLK_f:
        return KEY_ID_FOCUS;
    case SDLK_m:
        return KEY_ID_MODE;
    case SDLK_TAB:
        return KEY_ID_MENU;
    case SDLK_p:
        return KEY_ID_POWER;
    case SDLK_i:
        return KEY_ID_ASSISTANT;
    case SDLK_EQUALS:
        return KEY_ID_VOLUME_UP;
    case SDLK_MINUS:
        return KEY_ID_VOLUME_DOWN;
    default:
        return KEY_ID_ANY;
    }
}

static int sdl_key_event_watch(void* userdata, SDL_Event* event)
{
    (void)userdata;
    if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) {
        key_id_t key = sdl_key_to_key_id(event->key.keysym.sym);
        if (key != KEY_ID_ANY && event->key.repeat == 0) {
            int value = (event->type == SDL_KEYDOWN) ? 1 : 0;
            key_manager_inject_key(key, value);
        }
    }
    return 1;
}

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

    /* 不创建 LVGL keyboard indev —— 所有按键通过 SDL_AddEventWatch +
     * key_manager_inject_key 统一处理，避免 LVGL group/focus 路径
     * 与 key_manager 回调产生双重响应冲突。 */

    SDL_AddEventWatch(sdl_key_event_watch, NULL);

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

static page_interface_t ai_style_preview_page_interface = {
    .create = page_ai_style_preview_create,
    .destroy = page_ai_style_preview_destroy,
    .show = page_ai_style_preview_show,
    .hide = page_ai_style_preview_hide,
    .update = page_ai_style_preview_update,
};

static page_interface_t ai_recognition_preview_page_interface = {
    .create = page_ai_recognition_preview_create,
    .destroy = page_ai_recognition_preview_destroy,
    .show = page_ai_recognition_preview_show,
    .hide = page_ai_recognition_preview_hide,
    .update = page_ai_recognition_preview_update,
};

static page_interface_t video_page_interface = {
    .create = page_video_create,
    .destroy = page_video_destroy,
    .show = page_video_show,
    .hide = page_video_hide,
    .update = page_video_update,
};

static page_interface_t video_album_page_interface = {
    .create = page_video_album_create,
    .destroy = page_video_album_destroy,
    .show = page_video_album_show,
    .hide = page_video_album_hide,
    .update = page_video_album_update,
};

static page_interface_t video_preview_page_interface = {
    .create = page_video_preview_create,
    .destroy = page_video_preview_destroy,
    .show = page_video_preview_show,
    .hide = page_video_preview_hide,
    .update = page_video_preview_update,
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

static page_interface_t wifi_list_page_interface = {
    .create = page_wifi_list_create,
    .destroy = page_wifi_list_destroy,
    .show = page_wifi_list_show,
    .hide = page_wifi_list_hide,
    .update = page_wifi_list_update,
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

static page_interface_t photo_preview_page_interface = {
    .create = page_photo_preview_create,
    .destroy = page_photo_preview_destroy,
    .show = page_photo_preview_show,
    .hide = page_photo_preview_hide,
    .update = page_photo_preview_update,
};

static page_interface_t test_page_interface = {
    .create = page_test_create,
    .destroy = page_test_destroy,
    .show = page_test_show,
    .hide = page_test_hide,
    .update = page_test_update,
};

static page_interface_t key_test_page_interface = {
    .create = page_key_test_create,
    .destroy = page_key_test_destroy,
    .show = page_key_test_show,
    .hide = page_key_test_hide,
    .update = page_key_test_update,
};

static page_interface_t touch_test_page_interface = {
    .create = page_touch_test_create,
    .destroy = page_touch_test_destroy,
    .show = page_touch_test_show,
    .hide = page_touch_test_hide,
    .update = page_touch_test_update,
};

static page_interface_t boot_switch_test_page_interface = {
    .create = page_boot_switch_test_create,
    .destroy = page_boot_switch_test_destroy,
    .show = page_boot_switch_test_show,
    .hide = page_boot_switch_test_hide,
    .update = page_boot_switch_test_update,
};

static page_interface_t photo_resolution_test_page_interface = {
    .create = page_photo_resolution_test_create,
    .destroy = page_photo_resolution_test_destroy,
    .show = page_photo_resolution_test_show,
    .hide = page_photo_resolution_test_hide,
    .update = page_photo_resolution_test_update,
};

static page_interface_t loop_ptest_page_interface = {
    .create = page_loop_ptest_create,
    .destroy = page_loop_ptest_destroy,
    .show = page_loop_ptest_show,
    .hide = page_loop_ptest_hide,
    .update = page_loop_ptest_update,
};

int32_t ui_main(void)
{
    lv_init();

    /* Linux display device init */
    lv_linux_disp_init();

    /* Initialize fonts */
    font_manager_init();

    /* Initialize styles */
    style_common_init();

    /* Initialize param manager */
    param_manager_init();

#if defined(FB_RUN) && !FB_RUN
    /* SDL 仿真默认模拟 SD 卡常驻，避免相册入口因未就绪被直接拦截。
     * 注意板级构建不定义 FB_RUN，`#if !FB_RUN` 会恒真导致本行漏进板端，
     * 必须用 defined(FB_RUN) 守卫（与 message_manager.h 宏隔离同款写法）。 */
    (void)param_manager_set(PARAM_ID_SD_READY, SD_READY_TRUE);
#endif

    /* Initialize volume bar */
    volume_bar_init();
    top_notice_init();
    status_bar_init();

    /* Initialize key manager */
    key_manager_init();

    /* Initialize power manager */
    power_manager_init();

    /* Create page manager */
    if (page_manager_create() != 0) {
        MLOG_ERR("Failed to create page manager");
        return -1;
    }

    /* Register pages */
    page_manager_register("home", &home_page_interface, NULL);
    page_manager_register("photo", &photo_page_interface, NULL);
    page_manager_register("ai_photo", &ai_photo_page_interface, NULL);
    page_manager_register("ai_style_preview", &ai_style_preview_page_interface, NULL);
    page_manager_register("ai_recognition_preview", &ai_recognition_preview_page_interface, NULL);
    page_manager_register("video", &video_page_interface, NULL);
    page_manager_register("video_album", &video_album_page_interface, NULL);
    page_manager_register("video_preview", &video_preview_page_interface, NULL);
    page_manager_register("photo_settings", &photo_settings_page_interface, NULL);
    page_manager_register("video_settings", &video_settings_page_interface, NULL);
    page_manager_register("system_settings", &system_settings_page_interface, NULL);
    page_manager_register("version_info", &version_info_page_interface, NULL);
    page_manager_register("wifi_list", &wifi_list_page_interface, NULL);
    page_manager_register("ai_photo_settings", &ai_photo_settings_page_interface, NULL);
    page_manager_register("chat", &chat_page_interface, NULL);
    page_manager_register("album", &album_page_interface, NULL);
    page_manager_register("photo_preview", &photo_preview_page_interface, NULL);
    page_manager_register("test", &test_page_interface, NULL);
    page_manager_register("key_test", &key_test_page_interface, NULL);
    page_manager_register("touch_test", &touch_test_page_interface, NULL);
    page_manager_register("boot_switch_test", &boot_switch_test_page_interface, NULL);
    page_manager_register("photo_resolution_test", &photo_resolution_test_page_interface, NULL);
    page_manager_register("loop_ptest", &loop_ptest_page_interface, NULL);

    /* Navigate to home page */
    page_manager_navigate("home");

    /* Show volume bar for test */
    // volume_bar_show();
    // MLOG_INFO("Volume bar test shown");

    /* Handle LVGL tasks */
    while (1) {
        key_manager_poll();
        power_manager_poll();
        wifi_manager_poll();
        param_manager_poll();
        media_manager_poll();
        message_manager_poll();
        lv_timer_handler();
        usleep(5000);
    }

    power_manager_deinit();
    key_manager_deinit();
    page_manager_destroy();
    return 0;
}
