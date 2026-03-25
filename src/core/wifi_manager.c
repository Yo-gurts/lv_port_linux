#include "core/wifi_manager.h"
#include "core/param_manager.h"
#include "mlog.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define WIFI_DAEMON_SOCKET "/tmp/aicam_wifi.sock"
#define RESP_SIZE 8192
#define WIFI_STATUS_POLL_INTERVAL_MS 3000

static uint64_t monotonic_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

static int parse_ap_line(char* line, wifi_ap_info_t* ap)
{
    char* fields[8];
    char* p;
    int field_idx = 0;

    if (line == NULL || ap == NULL || strncmp(line, "AP\t", 3) != 0) {
        return -1;
    }

    p = line + 3;
    fields[field_idx++] = p;
    while (*p != '\0' && field_idx < (int)(sizeof(fields) / sizeof(fields[0]))) {
        if (*p == '\t') {
            *p = '\0';
            fields[field_idx++] = p + 1;
        }
        p++;
    }

    if (field_idx < 3) {
        return -1;
    }

    memset(ap, 0, sizeof(*ap));
    strncpy(ap->ssid, fields[0], WIFI_MANAGER_MAX_SSID_LEN - 1);
    ap->signal_level = atoi(fields[1]);
    ap->is_secured = (uint8_t)atoi(fields[2]);
    ap->is_saved = (field_idx >= 4) ? (uint8_t)atoi(fields[3]) : 0;
    ap->is_connected = (field_idx >= 5) ? (uint8_t)atoi(fields[4]) : 0;
    return 0;
}

static int send_cmd(const char* cmd, char* resp, size_t resp_sz)
{
    int fd;
    struct sockaddr_un addr;
    ssize_t n;
    size_t total = 0;
    int got_data = 0;
    fd_set rfds;
    struct timeval tv;

    if (cmd == NULL || resp == NULL || resp_sz < 2) {
        return -1;
    }

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        MLOG_ERR("socket failed: %s", strerror(errno));
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", WIFI_DAEMON_SOCKET);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        MLOG_ERR("connect to %s failed: %s", WIFI_DAEMON_SOCKET, strerror(errno));
        close(fd);
        return -1;
    }

    if (write(fd, cmd, strlen(cmd)) < 0) {
        MLOG_ERR("write cmd failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    /* 持续读取直到服务端关闭连接或短超时，避免只读到响应头。 */
    while (total < resp_sz - 1) {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        tv.tv_sec = got_data ? 0 : 5;
        tv.tv_usec = got_data ? 300 * 1000 : 0;

        n = select(fd + 1, &rfds, NULL, NULL, &tv);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            close(fd);
            return -1;
        }
        if (n == 0) {
            if (got_data) {
                break;
            }
            close(fd);
            return -1;
        }

        n = read(fd, resp + total, resp_sz - 1 - total);
        if (n > 0) {
            total += (size_t)n;
            got_data = 1;
            continue;
        }
        if (n == 0) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        if ((errno == EAGAIN || errno == EWOULDBLOCK) && got_data) {
            break;
        }
        close(fd);
        return -1;
    }
    resp[total] = '\0';

    close(fd);
    return got_data ? 0 : -1;
}

static int map_connect_error(const char* error)
{
    if (error == NULL) {
        return WIFI_CONNECT_ERR_GENERIC;
    }
    if (strcmp(error, "NONE") == 0) {
        return WIFI_CONNECT_OK;
    }
    if (strcmp(error, "CONNECT_AUTH_FAILED") == 0) {
        return WIFI_CONNECT_ERR_AUTH;
    }
    if (strcmp(error, "CONNECT_TIMEOUT") == 0) {
        return WIFI_CONNECT_ERR_TIMEOUT;
    }
    if (strcmp(error, "CONNECT_BUSY") == 0) {
        return WIFI_CONNECT_ERR_BUSY;
    }
    if (strcmp(error, "WIFI_DISABLED") == 0) {
        return WIFI_CONNECT_ERR_DISABLED;
    }
    return WIFI_CONNECT_ERR_GENERIC;
}

int wifi_manager_get_status(void)
{
    char resp[128];
    int status;
    if (send_cmd("GET_STATUS\n", resp, sizeof(resp)) != 0) {
        MLOG_ERR("GET_STATUS failed");
        return -1;
    }
    if (strncmp(resp, "OK\tSTATUS\t", 10) == 0) {
        status = atoi(resp + 10);
        MLOG_INFO("wifi status: %s", status ? "enabled" : "disabled");
        return status;
    }
    MLOG_ERR("GET_STATUS parse failed: %s", resp);
    return -1;
}

void wifi_manager_poll(void)
{
    static uint64_t last_poll_ms = 0;
    uint64_t now_ms = monotonic_ms();
    char resp[128];
    char* saveptr = NULL;
    char* token;
    int enabled = 0;
    int connected = 0;
    int signal_dbm = -1;

    if (now_ms - last_poll_ms < WIFI_STATUS_POLL_INTERVAL_MS) {
        return;
    }
    last_poll_ms = now_ms;

    if (send_cmd("GET_STATUS\n", resp, sizeof(resp)) != 0) {
        return;
    }

    token = strtok_r(resp, "\t\n", &saveptr);
    if (token == NULL || strcmp(token, "OK") != 0) {
        return;
    }
    token = strtok_r(NULL, "\t\n", &saveptr);
    if (token == NULL || strcmp(token, "STATUS") != 0) {
        return;
    }

    token = strtok_r(NULL, "\t\n", &saveptr);
    if (token != NULL) {
        enabled = atoi(token);
    }
    token = strtok_r(NULL, "\t\n", &saveptr);
    if (token != NULL) {
        connected = atoi(token);
    }
    token = strtok_r(NULL, "\t\n", &saveptr);
    if (token != NULL) {
        signal_dbm = atoi(token);
    }

    if (!enabled) {
        connected = 0;
        signal_dbm = -1;
    }

    (void)param_manager_set(PARAM_ID_WIFI_CONNECTED, connected ? 1 : 0);
    (void)param_manager_set(PARAM_ID_WIFI_SIGNAL_DBM, signal_dbm);
}

int wifi_manager_set_enabled(int enabled)
{
    char resp[128];
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "SET_ENABLED\t%d\n", enabled ? 1 : 0);
    MLOG_INFO("set wifi enabled: %d", enabled);
    if (send_cmd(cmd, resp, sizeof(resp)) != 0) {
        MLOG_ERR("SET_ENABLED failed");
        return -1;
    }
    if (strncmp(resp, "OK\tSTATE", 8) == 0) {
        MLOG_INFO("set wifi enabled success");
        return 0;
    }
    MLOG_ERR("SET_ENABLED failed: %s", resp);
    return -1;
}

/* 启动 WiFi 扫描，返回 scan_id，-1 表示失败 */
int wifi_manager_start_scan(void)
{
    char resp[128];
    int scan_id = -1;

    MLOG_INFO("starting wifi scan...");
    if (send_cmd("SCAN_START\n", resp, sizeof(resp)) != 0) {
        MLOG_ERR("SCAN_START failed");
        return -1;
    }

    if (strncmp(resp, "OK\tSCAN_STARTED\t", 16) == 0) {
        scan_id = atoi(resp + 16);
    }

    if (scan_id < 0) {
        MLOG_ERR("SCAN_START parse failed: %s", resp);
        return -1;
    }

    MLOG_INFO("scan started, scan_id=%d", scan_id);
    return scan_id;
}

/* 获取扫描结果，expected_scan_id 是期望的 scan_id，返回找到的 AP 数量，-1 表示还在扫描中 */
int wifi_manager_get_scan_results(wifi_ap_info_t* out_list, int max_count, int expected_scan_id)
{
    char resp[RESP_SIZE];
    int count = 0;
    char* line;
    int curr_id;

    if (out_list == NULL || max_count <= 0) {
        return 0;
    }

    if (send_cmd("SCAN_GET\n", resp, sizeof(resp)) != 0) {
        return -1; /* 还在扫描中 */
    }

    MLOG_INFO("SCAN_GET response: %s", resp);

    if (strncmp(resp, "OK\tSCAN\t", 8) != 0) {
        return -1; /* 还在扫描中 */
    }

    /* 检查 scan_id 是否匹配 */
    curr_id = atoi(resp + 8);
    if (curr_id < expected_scan_id) {
        return -1; /* 还在扫描中，结果还不是最新的 */
    }

    /* 解析 AP 列表 */
    line = strtok(resp, "\n");
    while (line != NULL && count < max_count) {
        if (parse_ap_line(line, &out_list[count]) == 0) {
            count++;
        }
        line = strtok(NULL, "\n");
    }

    MLOG_INFO("scan completed, found %d APs", count);
    return count;
}

/* 同步扫描接口，阻塞等待结果 */
int wifi_manager_scan(wifi_ap_info_t* out_list, int max_count)
{
    char resp[RESP_SIZE];
    int scan_id = -1;
    int count = 0;
    int i;

    if (out_list == NULL || max_count <= 0) {
        return 0;
    }

    MLOG_INFO("start wifi scan...");
    /* 启动扫描 */
    if (send_cmd("SCAN_START\n", resp, sizeof(resp)) != 0) {
        MLOG_ERR("SCAN_START failed");
        return 0;
    }

    /* 解析 scan_id */
    if (strncmp(resp, "OK\tSCAN_STARTED\t", 16) == 0) {
        scan_id = atoi(resp + 16);
    }

    if (scan_id < 0) {
        MLOG_ERR("SCAN_START parse failed");
        return 0;
    }

    MLOG_INFO("scan started, scan_id=%d, waiting results...", scan_id);

    /* 等待扫描结果，最多 10 次轮询 */
    for (i = 0; i < 10; i++) {
        usleep(1000000); /* 等待 1 秒 */
        if (send_cmd("SCAN_GET\n", resp, sizeof(resp)) != 0) {
            continue;
        }
        /* 检查 scan_id 是否匹配 */
        if (strncmp(resp, "OK\tSCAN\t", 8) == 0) {
            int curr_id = atoi(resp + 8);
            if (curr_id >= scan_id) {
                break;
            }
        }
    }

    if (i >= 10) {
        MLOG_WARN("scan timeout, no results");
    } else {
        MLOG_INFO("scan completed in %d seconds, found %d APs", i + 1, count);
    }

    /* 解析 AP 列表 */
    char* line = strtok(resp, "\n");
    while (line != NULL && count < max_count) {
        if (parse_ap_line(line, &out_list[count]) == 0) {
            count++;
        }
        line = strtok(NULL, "\n");
    }

    return count;
}

int wifi_manager_connect_async(const char* ssid, const char* password)
{
    char resp[128];
    char cmd[256];

    if (ssid == NULL || ssid[0] == '\0') {
        return WIFI_CONNECT_ERR_GENERIC;
    }
    if (password == NULL) {
        password = "";
    }

    MLOG_INFO("start async connect to SSID: %s", ssid);
    snprintf(cmd, sizeof(cmd), "CONNECT\t%s\t%s\n", ssid, password);
    if (send_cmd(cmd, resp, sizeof(resp)) != 0) {
        MLOG_ERR("CONNECT failed");
        return WIFI_CONNECT_ERR_GENERIC;
    }

    if (strncmp(resp, "OK\tCONNECTING", 13) == 0 || strncmp(resp, "OK\tCONNECTED", 12) == 0) {
        return WIFI_CONNECT_OK;
    }
    if (strncmp(resp, "ERR\tWIFI_DISABLED", 17) == 0) {
        return WIFI_CONNECT_ERR_DISABLED;
    }
    if (strncmp(resp, "ERR\tCONNECT_BUSY", 16) == 0) {
        return WIFI_CONNECT_ERR_BUSY;
    }
    if (strncmp(resp, "ERR\tCONNECT_AUTH_FAILED", 23) == 0) {
        return WIFI_CONNECT_ERR_AUTH;
    }
    if (strncmp(resp, "ERR\tCONNECT_TIMEOUT", 19) == 0) {
        return WIFI_CONNECT_ERR_TIMEOUT;
    }

    MLOG_ERR("connect start failed: %s", resp);
    return WIFI_CONNECT_ERR_GENERIC;
}

int wifi_manager_get_connect_result(int* out_state, int* out_error, char* out_ssid, int out_ssid_sz)
{
    char resp[256];
    char* saveptr = NULL;
    char* token;
    int state;
    const char* error = "NONE";
    const char* ssid = "";

    if (out_state == NULL || out_error == NULL || out_ssid == NULL || out_ssid_sz <= 0) {
        return -1;
    }

    if (send_cmd("GET_CONNECT_RESULT\n", resp, sizeof(resp)) != 0) {
        return -1;
    }

    token = strtok_r(resp, "\t\n", &saveptr);
    if (token == NULL || strcmp(token, "OK") != 0) {
        return -1;
    }
    token = strtok_r(NULL, "\t\n", &saveptr);
    if (token == NULL || strcmp(token, "CONNECT_RESULT") != 0) {
        return -1;
    }
    token = strtok_r(NULL, "\t\n", &saveptr);
    if (token == NULL) {
        return -1;
    }
    state = atoi(token);

    token = strtok_r(NULL, "\t\n", &saveptr);
    if (token != NULL) {
        error = token;
    }
    token = strtok_r(NULL, "\t\n", &saveptr);
    if (token != NULL) {
        ssid = token;
    }

    *out_state = state;
    *out_error = map_connect_error(error);
    strncpy(out_ssid, ssid, (size_t)out_ssid_sz - 1);
    out_ssid[out_ssid_sz - 1] = '\0';
    return 0;
}

int wifi_manager_connect(const char* ssid, const char* password)
{
    int ret;
    int i;
    int state = WIFI_CONNECT_STATE_IDLE;
    int error = WIFI_CONNECT_ERR_GENERIC;
    char result_ssid[WIFI_MANAGER_MAX_SSID_LEN];

    ret = wifi_manager_connect_async(ssid, password);
    if (ret != WIFI_CONNECT_OK) {
        return ret;
    }

    for (i = 0; i < 60; i++) { /* up to ~12s */
        if (wifi_manager_get_connect_result(&state, &error, result_ssid, sizeof(result_ssid)) != 0) {
            usleep(200 * 1000);
            continue;
        }
        if (state == WIFI_CONNECT_STATE_CONNECTED) {
            return WIFI_CONNECT_OK;
        }
        if (state == WIFI_CONNECT_STATE_FAILED) {
            return error;
        }
        usleep(200 * 1000);
    }

    return WIFI_CONNECT_ERR_TIMEOUT;
}

int wifi_manager_disconnect(void)
{
    char resp[128];
    MLOG_INFO("disconnecting wifi...");
    if (send_cmd("DISCONNECT\n", resp, sizeof(resp)) != 0) {
        MLOG_ERR("DISCONNECT failed");
        return -1;
    }
    if (strncmp(resp, "OK\tDISCONNECTED", 14) == 0) {
        MLOG_INFO("disconnected success");
        return 0;
    }
    MLOG_ERR("disconnect failed: %s", resp);
    return -1;
}

int wifi_manager_forget(const char* ssid)
{
    char resp[128];
    char cmd[128];

    if (ssid == NULL || ssid[0] == '\0') {
        return -1;
    }

    MLOG_INFO("forget SSID: %s", ssid);
    snprintf(cmd, sizeof(cmd), "FORGET\t%s\n", ssid);
    if (send_cmd(cmd, resp, sizeof(resp)) != 0) {
        MLOG_ERR("FORGET failed");
        return -1;
    }

    if (strncmp(resp, "OK\tFORGOT", 9) == 0) {
        MLOG_INFO("forget %s success", ssid);
        return 0;
    }

    MLOG_ERR("forget %s failed: %s", ssid, resp);
    return -1;
}

const char* wifi_manager_get_connected_ssid(void)
{
    static char connected_ssid[WIFI_MANAGER_MAX_SSID_LEN] = { 0 };
    char resp[RESP_SIZE];

    if (send_cmd("SCAN_GET\n", resp, sizeof(resp)) != 0) {
        return "未连接";
    }

    /* 查找已连接的 AP */
    char* line = strtok(resp, "\n");
    while (line != NULL) {
        if (strncmp(line, "AP\t", 3) == 0) {
            char* fields[6];
            char* p = line + 3;
            int field_idx = 0;

            fields[field_idx++] = p;
            while (*p && field_idx < 6) {
                if (*p == '\t') {
                    *p = '\0';
                    fields[field_idx++] = p + 1;
                }
                p++;
            }

            if (field_idx >= 5 && atoi(fields[4]) == 1) {
                strncpy(connected_ssid, fields[0], WIFI_MANAGER_MAX_SSID_LEN - 1);
                return connected_ssid;
            }
        }
        line = strtok(NULL, "\n");
    }

    return "未连接";
}
