#include "core/key_manager.h"
#include "lvgl/lvgl.h"
#include "mlog.h"
#include "ui/volume_bar.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define KEY_MANAGER_DEFAULT_LONG_PRESS_MS 700U
#define KEY_MANAGER_DEFAULT_REPEAT_MS 200U
#define KEY_MANAGER_LONG_PRESS_3S_MS 3000U
#define KEY_MANAGER_ANY_BUCKET KEY_ID_BUTT
#define KEY_MANAGER_EVENT_ANY_BUCKET KEY_EVENT_BUTT

typedef struct {
    const char* path;
    int fd;
} key_input_device_t;

typedef struct {
    uint8_t pressed;
    uint8_t long_fired;
    uint8_t long_3s_fired;
    uint64_t pressed_ms;
    uint64_t next_repeat_ms;
} key_state_t;

typedef struct {
    key_event_callback_t callback;
    void* user_data;
    uint8_t valid;
} key_callback_entry_t;

static key_input_device_t g_devices[] = {
    { "/dev/input/power-key", -1 },
    { "/dev/input/adc-key1", -1 },
    { "/dev/input/adc-key2", -1 },
};

static key_state_t g_key_states[KEY_ID_BUTT];
/* 回调映射表：
 * 第一维: key bucket（0..KEY_ID_BUTT-1 + ANY_BUCKET）
 * 第二维: event bucket（0..KEY_EVENT_BUTT-1 + EVENT_ANY_BUCKET）
 * 每个 bucket 仅保存一个回调入口。 */
static key_callback_entry_t g_callback_map[KEY_ID_BUTT + 1][KEY_EVENT_BUTT + 1];
static uint8_t g_inited = 0;
static uint8_t g_input_block_mask = 0;
static uint32_t g_long_press_ms = KEY_MANAGER_DEFAULT_LONG_PRESS_MS;
static uint32_t g_repeat_ms = KEY_MANAGER_DEFAULT_REPEAT_MS;
static lv_indev_t* g_touch_indev = NULL;

#define ENUM_CASE(x) \
    case x:          \
        return #x

static const char* key_manager_key_to_string(key_id_t key)
{
    switch (key) {
        ENUM_CASE(KEY_ID_POWER);
        ENUM_CASE(KEY_ID_ASSISTANT);
        ENUM_CASE(KEY_ID_VOLUME_UP);
        ENUM_CASE(KEY_ID_VOLUME_DOWN);
        ENUM_CASE(KEY_ID_FOCUS);
        ENUM_CASE(KEY_ID_CAMERA);
        ENUM_CASE(KEY_ID_MODE);
        ENUM_CASE(KEY_ID_MENU);
        ENUM_CASE(KEY_ID_UP);
        ENUM_CASE(KEY_ID_DOWN);
        ENUM_CASE(KEY_ID_LEFT);
        ENUM_CASE(KEY_ID_RIGHT);
        ENUM_CASE(KEY_ID_OK);
        ENUM_CASE(KEY_ID_ANY);
    default:
        return "UNKNOWN";
    }
}

static const char* key_manager_event_to_string(key_event_type_t event_type)
{
    switch (event_type) {
        ENUM_CASE(KEY_EVENT_CLICK);
        ENUM_CASE(KEY_EVENT_PRESS);
        ENUM_CASE(KEY_EVENT_RELEASE);
        ENUM_CASE(KEY_EVENT_LONG_PRESS);
        ENUM_CASE(KEY_EVENT_LONG_PRESS_REPEAT);
        ENUM_CASE(KEY_EVENT_LONG_PRESS_3S);
        ENUM_CASE(KEY_EVENT_LONG_PRESS_3S_RELEASE);
        ENUM_CASE(KEY_EVENT_ANY);
    default:
        return "UNKNOWN";
    }
}

#undef ENUM_CASE

static int key_manager_is_valid_key(key_id_t key)
{
    return (key >= KEY_ID_POWER && key < KEY_ID_BUTT) || key == KEY_ID_ANY;
}

static int key_manager_is_valid_event(key_event_type_t event_type)
{
    return (event_type >= KEY_EVENT_CLICK && event_type < KEY_EVENT_BUTT) || event_type == KEY_EVENT_ANY;
}

static int key_manager_key_bucket(key_id_t key)
{
    /* KEY_ID_ANY 映射到最后一个 bucket。 */
    return (key == KEY_ID_ANY) ? KEY_MANAGER_ANY_BUCKET : (int)key;
}

static int key_manager_event_bucket(key_event_type_t event_type)
{
    /* KEY_EVENT_ANY 映射到最后一个 bucket。 */
    return (event_type == KEY_EVENT_ANY) ? KEY_MANAGER_EVENT_ANY_BUCKET : (int)event_type;
}

static uint8_t key_manager_mask_normalize(uint8_t mask)
{
    return (uint8_t)(mask & (KEY_INPUT_BLOCK_TP | KEY_INPUT_BLOCK_POWER_KEY | KEY_INPUT_BLOCK_ADC_KEY2 | KEY_INPUT_BLOCK_NON_CAMERA_KEYS));
}

static void key_manager_apply_touch_block(uint8_t block_mask)
{
    if (g_touch_indev == NULL) {
        return;
    }
    lv_indev_enable(g_touch_indev, (block_mask & KEY_INPUT_BLOCK_TP) == 0);
}

/* 清空按键状态，避免后续产生遗留 click/long_press。 */
static void key_manager_clear_key_states(uint8_t include_power, uint8_t include_non_power)
{
    int i;

    if (include_power) {
        memset(&g_key_states[KEY_ID_POWER], 0, sizeof(g_key_states[KEY_ID_POWER]));
    }
    if (include_non_power) {
        for (i = KEY_ID_POWER + 1; i < KEY_ID_BUTT; i++) {
            memset(&g_key_states[i], 0, sizeof(g_key_states[i]));
        }
    }
}

static uint8_t key_manager_is_key_blocked(key_id_t key)
{
    if (key == KEY_ID_POWER) {
        return (g_input_block_mask & KEY_INPUT_BLOCK_POWER_KEY) != 0;
    }
    if ((g_input_block_mask & KEY_INPUT_BLOCK_ADC_KEY2) != 0) {
        return 1;
    }
    if ((g_input_block_mask & KEY_INPUT_BLOCK_NON_CAMERA_KEYS) != 0 && key != KEY_ID_CAMERA) {
        return 1;
    }
    return 0;
}

/* 获取单调时钟毫秒时间戳，用于长按与连发判定。 */
static uint64_t key_manager_now_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

/* 将 Linux input key code 映射为业务按键枚举。 */
static key_id_t key_manager_map_code_to_key(int code)
{
    switch (code) {
    case KEY_POWER:
        return KEY_ID_POWER;
    case KEY_MODE:
        return KEY_ID_MODE;
    case KEY_MENU:
        return KEY_ID_MENU;
    case KEY_UP:
        return KEY_ID_UP;
    case KEY_DOWN:
        return KEY_ID_DOWN;
    case KEY_LEFT:
        return KEY_ID_LEFT;
    case KEY_RIGHT:
        return KEY_ID_RIGHT;
    case KEY_OK:
        return KEY_ID_OK;
    case KEY_ASSISTANT:
        return KEY_ID_ASSISTANT;
    case KEY_VOLUMEUP:
        return KEY_ID_VOLUME_UP;
    case KEY_VOLUMEDOWN:
        return KEY_ID_VOLUME_DOWN;
    case KEY_CAMERA_FOCUS:
        return KEY_ID_FOCUS;
    case KEY_CAMERA:
        return KEY_ID_CAMERA;
    default:
        return KEY_ID_ANY;
    }
}

/* 向所有匹配的监听者分发按键事件。 */
static void key_manager_dispatch(key_id_t key, key_event_type_t event_type)
{
    int key_buckets[2];
    int event_buckets[2];
    int kb;
    int eb;

    MLOG_INFO("key event: key=%s(%d), event=%s(%d)",
        key_manager_key_to_string(key), key,
        key_manager_event_to_string(event_type), event_type);

    key_buckets[0] = key_manager_key_bucket(key);
    key_buckets[1] = KEY_MANAGER_ANY_BUCKET;
    event_buckets[0] = key_manager_event_bucket(event_type);
    event_buckets[1] = KEY_MANAGER_EVENT_ANY_BUCKET;

    for (kb = 0; kb < 2; kb++) {
        for (eb = 0; eb < 2; eb++) {
            /* 按 4 条路径分发：
             * 1) key + event
             * 2) key + ANY_EVENT
             * 3) ANY_KEY + event
             * 4) ANY_KEY + ANY_EVENT */
            key_callback_entry_t* entry = &g_callback_map[key_buckets[kb]][event_buckets[eb]];
            if (entry->valid && entry->callback != NULL) {
                entry->callback(key, event_type, entry->user_data);
            }
        }
    }
}

/* 处理按键按下/抬起原始值，更新状态并产生 click 事件。 */
static void key_manager_handle_key_value(key_id_t key, int value, uint64_t now_ms)
{
    key_state_t* state;

    if (key < 0 || key >= KEY_ID_BUTT) {
        return;
    }
    if (key_manager_is_key_blocked(key)) {
        memset(&g_key_states[key], 0, sizeof(g_key_states[key]));
        return;
    }

    state = &g_key_states[key];
    if (value == 1) {
        if (state->pressed) {
            return;
        }
        state->pressed = 1;
        state->long_fired = 0;
        state->long_3s_fired = 0;
        state->pressed_ms = now_ms;
        state->next_repeat_ms = now_ms + g_long_press_ms;
        key_manager_dispatch(key, KEY_EVENT_PRESS);
    } else if (value == 0) {
        if (!state->pressed) {
            return;
        }
        key_manager_dispatch(key, KEY_EVENT_RELEASE);
        if (state->long_3s_fired) {
            key_manager_dispatch(key, KEY_EVENT_LONG_PRESS_3S_RELEASE);
        }
        if (!state->long_fired) {
            key_manager_dispatch(key, KEY_EVENT_CLICK);
        }
        memset(state, 0, sizeof(*state));
    }
}

/* 处理长按阈值与长按连发事件。 */
static void key_manager_process_hold_state(uint64_t now_ms)
{
    int i;

    for (i = 0; i < KEY_ID_BUTT; i++) {
        key_state_t* state = &g_key_states[i];
        if (key_manager_is_key_blocked((key_id_t)i)) {
            continue;
        }

        if (!state->pressed) {
            continue;
        }

        if (!state->long_3s_fired && now_ms >= state->pressed_ms + KEY_MANAGER_LONG_PRESS_3S_MS) {
            state->long_3s_fired = 1;
            key_manager_dispatch((key_id_t)i, KEY_EVENT_LONG_PRESS_3S);
        }

        if (!state->long_fired) {
            if (now_ms >= state->pressed_ms + g_long_press_ms) {
                state->long_fired = 1;
                key_manager_dispatch((key_id_t)i, KEY_EVENT_LONG_PRESS);
                state->next_repeat_ms = now_ms + g_repeat_ms;
            }
            continue;
        }

        if (g_repeat_ms == 0) {
            continue;
        }

        while (now_ms >= state->next_repeat_ms) {
            key_manager_dispatch((key_id_t)i, KEY_EVENT_LONG_PRESS_REPEAT);
            state->next_repeat_ms += g_repeat_ms;
        }
    }
}

int key_manager_init(void)
{
    size_t i;
    int opened = 0;

    if (g_inited) {
        return 0;
    }

    memset(g_key_states, 0, sizeof(g_key_states));
    memset(g_callback_map, 0, sizeof(g_callback_map));
    g_input_block_mask = 0;
    key_manager_apply_touch_block(g_input_block_mask);

    for (i = 0; i < sizeof(g_devices) / sizeof(g_devices[0]); i++) {
        g_devices[i].fd = open(g_devices[i].path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (g_devices[i].fd < 0) {
            MLOG_WARN("open input device failed: %s, errno=%d", g_devices[i].path, errno);
            continue;
        }
        opened++;
        MLOG_INFO("key_manager opened input device: %s", g_devices[i].path);
    }

    g_inited = 1;
    if (opened == 0) {
        MLOG_WARN("key_manager initialized but no input device is available");
    }
    return 0;
}

void key_manager_deinit(void)
{
    size_t i;

    if (!g_inited) {
        return;
    }

    for (i = 0; i < sizeof(g_devices) / sizeof(g_devices[0]); i++) {
        if (g_devices[i].fd >= 0) {
            close(g_devices[i].fd);
            g_devices[i].fd = -1;
        }
    }
    memset(g_key_states, 0, sizeof(g_key_states));
    memset(g_callback_map, 0, sizeof(g_callback_map));
    g_input_block_mask = 0;
    g_touch_indev = NULL;
    g_inited = 0;
}

/* 轮询所有输入设备，并驱动按键状态机。 */
void key_manager_poll(void)
{
    size_t i;
    struct pollfd pfds[sizeof(g_devices) / sizeof(g_devices[0])];
    nfds_t nfds = 0;
    uint64_t now_ms;

    if (!g_inited) {
        return;
    }

    for (i = 0; i < sizeof(g_devices) / sizeof(g_devices[0]); i++) {
        if (g_devices[i].fd < 0) {
            continue;
        }
        pfds[nfds].fd = g_devices[i].fd;
        pfds[nfds].events = POLLIN;
        pfds[nfds].revents = 0;
        nfds++;
    }

    if (nfds > 0) {
        int poll_ret = poll(pfds, nfds, 0);
        if (poll_ret > 0) {
            for (i = 0; i < (size_t)nfds; i++) {
                if ((pfds[i].revents & POLLIN) == 0) {
                    continue;
                }
                while (1) {
                    struct input_event ev;
                    ssize_t rd = read(pfds[i].fd, &ev, sizeof(ev));
                    if (rd == (ssize_t)sizeof(ev)) {
                        if (ev.type == EV_KEY) {
                            key_id_t key = key_manager_map_code_to_key(ev.code);
                            if (key != KEY_ID_ANY) {
                                key_manager_handle_key_value(key, ev.value, key_manager_now_ms());
                            }
                        }
                        continue;
                    }
                    if (rd < 0) {
#if EAGAIN == EWOULDBLOCK
                        if (errno == EAGAIN) {
                            break;
                        }
#else
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
#endif
                    }
                    if (rd <= 0) {
                        break;
                    }
                }
            }
        }
    }

    now_ms = key_manager_now_ms();
    key_manager_process_hold_state(now_ms);
}

/* 按“按键+事件”维度注册回调。 */
int key_manager_register_callback(key_id_t key, key_event_type_t event_type, key_event_callback_t callback, void* user_data)
{
    int key_bucket;
    int event_bucket;
    key_callback_entry_t* entry;

    if (callback == NULL) {
        return -1;
    }
    if (!key_manager_is_valid_key(key) || !key_manager_is_valid_event(event_type)) {
        return -1;
    }
    key_bucket = key_manager_key_bucket(key);
    event_bucket = key_manager_event_bucket(event_type);
    entry = &g_callback_map[key_bucket][event_bucket];
    /* 当前策略：同一个 key+event 只允许一个回调，避免分发顺序不确定。 */
    if (entry->valid) {
        MLOG_WARN("key_manager register callback failed: key=%d event=%d already has callback", key, event_type);
        return -1;
    }
    entry->callback = callback;
    entry->user_data = user_data;
    entry->valid = 1;
    return 0;
}

/* 注销一条精确匹配的回调记录。 */
int key_manager_unregister_callback(key_id_t key, key_event_type_t event_type, key_event_callback_t callback, void* user_data)
{
    int key_bucket;
    int event_bucket;
    key_callback_entry_t* entry;

    if (callback == NULL) {
        return -1;
    }
    if (!key_manager_is_valid_key(key) || !key_manager_is_valid_event(event_type)) {
        return -1;
    }
    key_bucket = key_manager_key_bucket(key);
    event_bucket = key_manager_event_bucket(event_type);
    entry = &g_callback_map[key_bucket][event_bucket];
    if (!entry->valid) {
        return -1;
    }
    if (entry->callback != callback || entry->user_data != user_data) {
        return -1;
    }
    memset(entry, 0, sizeof(*entry));
    return 0;
}

/* 设置长按阈值，0 表示保持当前值。 */
void key_manager_set_long_press_ms(uint32_t long_press_ms)
{
    if (long_press_ms == 0) {
        return;
    }
    g_long_press_ms = long_press_ms;
}

/* 设置长按连发间隔，0 表示关闭连发。 */
void key_manager_set_repeat_ms(uint32_t repeat_ms)
{
    g_repeat_ms = repeat_ms;
}

void key_manager_bind_touch_indev(struct _lv_indev_t* indev)
{
    g_touch_indev = (lv_indev_t*)indev;
    key_manager_apply_touch_block(g_input_block_mask);
}

void key_manager_set_block_non_power(uint8_t block_mask)
{
    uint8_t old_mask = g_input_block_mask;
    uint8_t new_mask = key_manager_mask_normalize(block_mask);
    uint8_t block_power_new = (new_mask & KEY_INPUT_BLOCK_POWER_KEY) != 0;
    uint8_t block_power_old = (old_mask & KEY_INPUT_BLOCK_POWER_KEY) != 0;
    uint8_t block_adc_new = (new_mask & KEY_INPUT_BLOCK_ADC_KEY2) != 0;
    uint8_t block_adc_old = (old_mask & KEY_INPUT_BLOCK_ADC_KEY2) != 0;
    uint8_t block_non_camera_new = (new_mask & KEY_INPUT_BLOCK_NON_CAMERA_KEYS) != 0;
    uint8_t block_non_camera_old = (old_mask & KEY_INPUT_BLOCK_NON_CAMERA_KEYS) != 0;

    if (old_mask == new_mask) {
        return;
    }
    g_input_block_mask = new_mask;

    if (!block_power_old && block_power_new) {
        key_manager_clear_key_states(1, 0);
    }
    if (!block_adc_old && block_adc_new) {
        key_manager_clear_key_states(0, 1);
    }
    if (!block_non_camera_old && block_non_camera_new) {
        memset(&g_key_states[KEY_ID_ASSISTANT], 0, sizeof(g_key_states[KEY_ID_ASSISTANT]));
        memset(&g_key_states[KEY_ID_VOLUME_UP], 0, sizeof(g_key_states[KEY_ID_VOLUME_UP]));
        memset(&g_key_states[KEY_ID_VOLUME_DOWN], 0, sizeof(g_key_states[KEY_ID_VOLUME_DOWN]));
        memset(&g_key_states[KEY_ID_FOCUS], 0, sizeof(g_key_states[KEY_ID_FOCUS]));
    }

    key_manager_apply_touch_block(new_mask);
}

uint8_t key_manager_get_block_non_power(void)
{
    return g_input_block_mask;
}
