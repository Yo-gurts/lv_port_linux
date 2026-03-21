#include "ui/wifi_icon_helper.h"

#include "config.h"

const char* wifi_icon_helper_get_path(int connected, int signal_dbm)
{
    if (connected != 1) {
        return "A:" RES_ICON_PATH "/wifi-off.png";
    }

    if (signal_dbm <= -75) {
        return "A:" RES_ICON_PATH "/wifi-1.png";
    }
    if (signal_dbm <= -60) {
        return "A:" RES_ICON_PATH "/wifi-2.png";
    }

    return "A:" RES_ICON_PATH "/wifi.png";
}
