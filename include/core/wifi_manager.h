#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_MANAGER_MAX_SSID_LEN 64
#define WIFI_CONNECT_OK 0
#define WIFI_CONNECT_ERR_GENERIC -1
#define WIFI_CONNECT_ERR_AUTH -2
#define WIFI_CONNECT_ERR_TIMEOUT -3
#define WIFI_CONNECT_ERR_BUSY -4
#define WIFI_CONNECT_ERR_DISABLED -5

#define WIFI_CONNECT_STATE_IDLE 0
#define WIFI_CONNECT_STATE_CONNECTING 1
#define WIFI_CONNECT_STATE_CONNECTED 2
#define WIFI_CONNECT_STATE_FAILED 3

typedef struct {
    char ssid[WIFI_MANAGER_MAX_SSID_LEN];
    int signal_level; /* 0~100 */
    uint8_t is_secured;
    uint8_t is_saved;
    uint8_t is_connected;
} wifi_ap_info_t;

int wifi_manager_get_status(void);
int wifi_manager_set_enabled(int enabled);
int wifi_manager_scan(wifi_ap_info_t* out_list, int max_count);
int wifi_manager_start_scan(void);
int wifi_manager_get_scan_results(wifi_ap_info_t* out_list, int max_count, int expected_scan_id);
void wifi_manager_poll(void);
int wifi_manager_connect(const char* ssid, const char* password);
int wifi_manager_connect_async(const char* ssid, const char* password);
int wifi_manager_get_connect_result(int* out_state, int* out_error, char* out_ssid, int out_ssid_sz);
int wifi_manager_disconnect(void);
int wifi_manager_forget(const char* ssid);
const char* wifi_manager_get_connected_ssid(void);

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_MANAGER_H__ */
