#include "core/wifi_manager.h"
#include <string.h>

typedef struct {
    const char* ssid;
    int signal_level;
    uint8_t is_secured;
    uint8_t is_saved;
} mock_ap_config_t;

static const mock_ap_config_t g_mock_ap_table[] = {
    { "DashCam-Office-5G", 92, 1, 1 },
    { "DashCam-Guest", 78, 0, 0 },
    { "Home-WiFi", 66, 1, 1 },
    { "CoffeeShop_Free", 54, 0, 0 },
    { "Phone-Hotspot", 43, 1, 0 },
    { "Studio-AP", 88, 1, 1 },
    { "MeetingRoom-WiFi", 72, 1, 0 },
    { "CameraLab", 61, 1, 1 },
    { "Open-Test-Net", 49, 0, 0 },
    { "IoT-Device-Net", 37, 1, 0 },
};

#define MOCK_AP_COUNT ((int)(sizeof(g_mock_ap_table) / sizeof(g_mock_ap_table[0])))

static int g_connected_index = 0;
static uint8_t g_saved_flags[MOCK_AP_COUNT];
static uint8_t g_saved_flags_initialized = 0;

static void ensure_saved_flags_initialized(void)
{
    int i;

    if (g_saved_flags_initialized) {
        return;
    }

    for (i = 0; i < MOCK_AP_COUNT; i++) {
        g_saved_flags[i] = g_mock_ap_table[i].is_saved;
    }
    g_saved_flags_initialized = 1;
}

int wifi_manager_scan(wifi_ap_info_t* out_list, int max_count)
{
    int count;
    int i;
    int j;

    if (out_list == NULL || max_count <= 0) {
        return 0;
    }
    ensure_saved_flags_initialized();

    count = MOCK_AP_COUNT;
    if (count > max_count) {
        count = max_count;
    }

    for (i = 0; i < count; i++) {
        memset(&out_list[i], 0, sizeof(out_list[i]));
        strncpy(out_list[i].ssid, g_mock_ap_table[i].ssid, WIFI_MANAGER_MAX_SSID_LEN - 1);
        out_list[i].signal_level = g_mock_ap_table[i].signal_level;
        out_list[i].is_secured = g_mock_ap_table[i].is_secured;
        out_list[i].is_saved = g_saved_flags[i];
        out_list[i].is_connected = (i == g_connected_index) ? 1 : 0;
    }

    /* 按信号强度从高到低排序返回。 */
    for (i = 0; i < count - 1; i++) {
        for (j = i + 1; j < count; j++) {
            if (out_list[j].signal_level > out_list[i].signal_level) {
                wifi_ap_info_t tmp = out_list[i];
                out_list[i] = out_list[j];
                out_list[j] = tmp;
            }
        }
    }

    return count;
}

int wifi_manager_connect(const char* ssid, const char* password)
{
    int i;
    ensure_saved_flags_initialized();

    if (ssid == NULL || ssid[0] == '\0') {
        return -1;
    }

    for (i = 0; i < MOCK_AP_COUNT; i++) {
        if (strcmp(ssid, g_mock_ap_table[i].ssid) == 0) {
            if (password != NULL && password[0] != '\0') {
                g_saved_flags[i] = 1;
            }
            g_connected_index = i;
            return 0;
        }
    }

    return -1;
}

const char* wifi_manager_get_connected_ssid(void)
{
    if (g_connected_index < 0 || g_connected_index >= MOCK_AP_COUNT) {
        return "未连接";
    }
    return g_mock_ap_table[g_connected_index].ssid;
}
