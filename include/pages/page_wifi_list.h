#ifndef __PAGE_WIFI_LIST_H__
#define __PAGE_WIFI_LIST_H__

#include "core/page_manager.h"
#include "core/wifi_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_LIST_MAX_AP_COUNT 32

typedef struct {
    lv_obj_t* container;
    lv_obj_t* nav_bar;
    lv_obj_t* wifi_switch;
    lv_obj_t* refresh_btn;
    lv_obj_t* notice_popup;
    lv_obj_t* notice_label;
    lv_obj_t* wifi_list;
    lv_obj_t* password_modal_mask;
    lv_obj_t* password_panel;
    lv_obj_t* password_ssid_label;
    lv_obj_t* password_ta;
    lv_obj_t* password_cancel_btn;
    lv_obj_t* password_confirm_btn;
    lv_obj_t* password_kb;
    lv_timer_t* notice_timer;
    uint8_t wifi_enabled;
    char pending_ssid[WIFI_MANAGER_MAX_SSID_LEN];
    wifi_ap_info_t scan_results[WIFI_LIST_MAX_AP_COUNT];
    int scan_count;
} page_wifi_list_data_t;

void page_wifi_list_create(void);
void page_wifi_list_destroy(void);
void page_wifi_list_show(void);
void page_wifi_list_hide(void);
void page_wifi_list_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_WIFI_LIST_H__ */
