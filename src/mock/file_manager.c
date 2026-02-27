#include "core/file_manager.h"
#include "mlog.h"
#include <unistd.h>

int file_manager_format_sdcard(void)
{
    MLOG_INFO("file_manager: start format sdcard (mock)");
    usleep(1000 * 1000);
    MLOG_INFO("file_manager: format sdcard done (mock)");
    return 0;
}
