#include "core/wifi_manager.h"
#include "core/param_manager.h"
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

static int g_wifi_enabled = 1;
static int g_connected_index = 0;
static uint8_t g_saved_flags[MOCK_AP_COUNT];
static uint8_t g_saved_flags_initialized = 0;
static int g_scan_id = 0;
static uint8_t g_scan_pending = 0;
static uint8_t g_scan_poll_count = 0;
static int g_connect_state = WIFI_CONNECT_STATE_IDLE;
static int g_connect_error = WIFI_CONNECT_OK;
static char g_connect_ssid[WIFI_MANAGER_MAX_SSID_LEN];

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

static int fill_scan_results(wifi_ap_info_t* out_list, int max_count)
{
    int count;
    int i;
    int j;

    if (out_list == NULL || max_count <= 0) {
        return 0;
    }

    count = MOCK_AP_COUNT;
    if (count > max_count) {
        count = max_count;
    }

    for (i = 0; i < count; i++) {
        memset(&out_list[i], 0, sizeof(out_list[i]));
        strncpy(out_list[i].ssid, g_mock_ap_table[i].ssid, WIFI_MANAGER_MAX_SSID_LEN - 1);
        out_list[i].ssid[WIFI_MANAGER_MAX_SSID_LEN - 1] = '\0';
        out_list[i].signal_level = g_mock_ap_table[i].signal_level;
        out_list[i].is_secured = g_mock_ap_table[i].is_secured;
        out_list[i].is_saved = g_saved_flags[i];
        out_list[i].is_connected = (i == g_connected_index) ? 1 : 0;
    }

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

int wifi_manager_get_status(void)
{
    return g_wifi_enabled;
}

void wifi_manager_poll(void)
{
    int connected = (g_wifi_enabled != 0 && g_connected_index >= 0 && g_connected_index < MOCK_AP_COUNT) ? 1 : 0;
    int signal_dbm = -1;

    if (connected) {
        signal_dbm = g_mock_ap_table[g_connected_index].signal_level - 100;
    }

    (void)param_manager_set(PARAM_ID_WIFI_ENABLED, g_wifi_enabled ? 1 : 0);
    (void)param_manager_set(PARAM_ID_WIFI_CONNECTED, connected);
    (void)param_manager_set(PARAM_ID_WIFI_SIGNAL_DBM, signal_dbm);
}

int wifi_manager_set_enabled(int enabled)
{
    g_wifi_enabled = enabled ? 1 : 0;
    if (g_wifi_enabled == 0) {
        g_connected_index = -1;
        g_scan_pending = 0;
        g_connect_state = WIFI_CONNECT_STATE_IDLE;
        g_connect_error = WIFI_CONNECT_OK;
        g_connect_ssid[0] = '\0';
    }
    return 0;
}

int wifi_manager_start_scan(void)
{
    if (g_wifi_enabled == 0) {
        return -1;
    }

    g_scan_id++;
    g_scan_pending = 1;
    g_scan_poll_count = 0;
    return g_scan_id;
}

int wifi_manager_get_scan_results(wifi_ap_info_t* out_list, int max_count, int expected_scan_id)
{
    ensure_saved_flags_initialized();

    if (g_wifi_enabled == 0) {
        return 0;
    }

    if (expected_scan_id > g_scan_id) {
        return -1;
    }

    if (g_scan_pending != 0) {
        g_scan_poll_count++;
        if (g_scan_poll_count < 2) {
            return -1;
        }
        g_scan_pending = 0;
    }

    return fill_scan_results(out_list, max_count);
}

int wifi_manager_scan(wifi_ap_info_t* out_list, int max_count)
{
    ensure_saved_flags_initialized();

    if (g_wifi_enabled == 0) {
        return 0;
    }

    return fill_scan_results(out_list, max_count);
}

int wifi_manager_connect_async(const char* ssid, const char* password)
{
    int i;

    ensure_saved_flags_initialized();

    if (g_wifi_enabled == 0 || ssid == NULL || ssid[0] == '\0') {
        return (g_wifi_enabled == 0) ? WIFI_CONNECT_ERR_DISABLED : WIFI_CONNECT_ERR_GENERIC;
    }

    g_connect_state = WIFI_CONNECT_STATE_CONNECTING;
    g_connect_error = WIFI_CONNECT_OK;
    strncpy(g_connect_ssid, ssid, WIFI_MANAGER_MAX_SSID_LEN - 1);
    g_connect_ssid[WIFI_MANAGER_MAX_SSID_LEN - 1] = '\0';

    for (i = 0; i < MOCK_AP_COUNT; i++) {
        if (strcmp(ssid, g_mock_ap_table[i].ssid) == 0) {
            if (g_mock_ap_table[i].is_secured && password != NULL && strcmp(password, "wrong") == 0) {
                g_saved_flags[i] = 0;
                if (g_connected_index == i) {
                    g_connected_index = -1;
                }
                g_connect_state = WIFI_CONNECT_STATE_FAILED;
                g_connect_error = WIFI_CONNECT_ERR_AUTH;
                return WIFI_CONNECT_OK;
            }
            g_saved_flags[i] = 1;
            g_connected_index = i;
            g_connect_state = WIFI_CONNECT_STATE_CONNECTED;
            g_connect_error = WIFI_CONNECT_OK;
            return WIFI_CONNECT_OK;
        }
    }

    g_connect_state = WIFI_CONNECT_STATE_FAILED;
    g_connect_error = WIFI_CONNECT_ERR_GENERIC;
    return WIFI_CONNECT_OK;
}

int wifi_manager_get_connect_result(int* out_state, int* out_error, char* out_ssid, int out_ssid_sz)
{
    if (out_state == NULL || out_error == NULL || out_ssid == NULL || out_ssid_sz <= 0) {
        return -1;
    }
    *out_state = g_connect_state;
    *out_error = g_connect_error;
    strncpy(out_ssid, g_connect_ssid, (size_t)out_ssid_sz - 1);
    out_ssid[out_ssid_sz - 1] = '\0';
    return 0;
}

int wifi_manager_connect(const char* ssid, const char* password)
{
    int ret;
    int state;
    int err;
    char out_ssid[WIFI_MANAGER_MAX_SSID_LEN];

    ret = wifi_manager_connect_async(ssid, password);
    if (ret != WIFI_CONNECT_OK) {
        return ret;
    }
    if (wifi_manager_get_connect_result(&state, &err, out_ssid, sizeof(out_ssid)) != 0) {
        return WIFI_CONNECT_ERR_GENERIC;
    }
    if (state == WIFI_CONNECT_STATE_CONNECTED) {
        return WIFI_CONNECT_OK;
    }
    if (state == WIFI_CONNECT_STATE_FAILED) {
        return err;
    }
    return WIFI_CONNECT_ERR_TIMEOUT;
}

int wifi_manager_disconnect(void)
{
    g_connected_index = -1;
    return 0;
}

int wifi_manager_forget(const char* ssid)
{
    int i;

    ensure_saved_flags_initialized();

    if (ssid == NULL || ssid[0] == '\0') {
        return -1;
    }

    for (i = 0; i < MOCK_AP_COUNT; i++) {
        if (strcmp(ssid, g_mock_ap_table[i].ssid) == 0) {
            g_saved_flags[i] = 0;
            if (g_connected_index == i) {
                g_connected_index = -1;
            }
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
