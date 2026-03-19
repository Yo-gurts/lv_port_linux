#include "core/wifi_manager.h"
#include "mlog.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define WIFI_DAEMON_SOCKET "/tmp/aicam_wifi.sock"
#define RESP_SIZE 8192

static int send_cmd(const char* cmd, char* resp, size_t resp_sz)
{
    int fd;
    struct sockaddr_un addr;
    ssize_t n;
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

    /* 使用 select 等待响应，设置超时 */
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    if (select(fd + 1, &rfds, NULL, NULL, &tv) <= 0) {
        close(fd);
        return -1;
    }

    n = read(fd, resp, resp_sz - 1);
    if (n > 0) {
        resp[n] = '\0';
    } else {
        resp[0] = '\0';
    }

    close(fd);
    return 0;
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

    if (out_list == NULL || max_count <= 0) {
        return 0;
    }

    if (send_cmd("SCAN_GET\n", resp, sizeof(resp)) != 0) {
        return -1; /* 还在扫描中 */
    }

    MLOG_INFO("SCAN_GET response: %s", resp);

    /* 检查是否有 AP 结果 */
    if (strstr(resp, "AP\t") == NULL) {
        return -1; /* 没有 AP 结果，继续等待 */
    }

    /* 检查 scan_id 是否匹配 */
    if (strncmp(resp, "OK\tSCAN\t", 8) == 0) {
        int curr_id = atoi(resp + 8);
        if (curr_id < expected_scan_id) {
            return -1; /* 还在扫描中，结果还不是最新的 */
        }
    } else {
        return -1; /* 还在扫描中 */
    }

    /* 解析 AP 列表 */
    char* line = strtok(resp, "\n");
    while (line != NULL && count < max_count) {
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

            if (field_idx >= 5) {
                memset(&out_list[count], 0, sizeof(out_list[count]));
                strncpy(out_list[count].ssid, fields[0], WIFI_MANAGER_MAX_SSID_LEN - 1);
                out_list[count].signal_level = atoi(fields[1]);
                out_list[count].is_secured = (uint8_t)atoi(fields[2]);
                out_list[count].is_saved = (uint8_t)atoi(fields[3]);
                out_list[count].is_connected = (uint8_t)atoi(fields[4]);
                count++;
            }
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

            if (field_idx >= 5) {
                memset(&out_list[count], 0, sizeof(out_list[count]));
                strncpy(out_list[count].ssid, fields[0], WIFI_MANAGER_MAX_SSID_LEN - 1);
                out_list[count].signal_level = atoi(fields[1]);
                out_list[count].is_secured = (uint8_t)atoi(fields[2]);
                out_list[count].is_saved = (uint8_t)atoi(fields[3]);
                out_list[count].is_connected = (uint8_t)atoi(fields[4]);
                count++;
            }
        }
        line = strtok(NULL, "\n");
    }

    return count;
}

int wifi_manager_connect(const char* ssid, const char* password)
{
    char resp[128];
    char cmd[256];

    if (ssid == NULL || ssid[0] == '\0') {
        return -1;
    }

    if (password == NULL) {
        password = "";
    }

    MLOG_INFO("connecting to SSID: %s", ssid);
    snprintf(cmd, sizeof(cmd), "CONNECT\t%s\t%s\n", ssid, password);
    if (send_cmd(cmd, resp, sizeof(resp)) != 0) {
        MLOG_ERR("CONNECT failed");
        return -1;
    }

    if (strncmp(resp, "OK\tCONNECTED", 12) == 0) {
        MLOG_INFO("connected to %s success", ssid);
        return 0;
    }

    MLOG_ERR("connect to %s failed: %s", ssid, resp);
    return -1;
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
