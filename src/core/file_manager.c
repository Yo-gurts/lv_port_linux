#include "core/file_manager.h"

#include "config.h"
#include "filemng.h"
#include "mlog.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char* to_real_path(const char* path)
{
    if (path && path[0] == 'A' && path[1] == ':')
        return path + 2;
    return path;
}

static int to_lv_path(const char* real_path, char* lv_path, size_t lv_size)
{
    if (!real_path || !lv_path || lv_size == 0)
        return -1;

    if (real_path[0] == '/') {
        if (snprintf(lv_path, lv_size, "A:%s", real_path) >= (int)lv_size)
            return -1;
    } else {
        if (snprintf(lv_path, lv_size, "%s", real_path) >= (int)lv_size)
            return -1;
    }
    return 0;
}

static int file_exists_and_valid(const char* path)
{
    struct stat st;

    if (!path)
        return 0;

    if (stat(path, &st) != 0)
        return 0;

    if (!S_ISREG(st.st_mode))
        return 0;

    return st.st_size > 0;
}

static int get_photo_lv_path_by_index(int index, char* out_name, size_t name_size, char* out_path, size_t path_size)
{
    if (!out_name || name_size == 0 || !out_path || path_size == 0)
        return -1;

    if (file_manager_get_photo_name(index, out_name, name_size) != 0)
        return -1;

    if (snprintf(out_path, path_size, "%s%s", PHOTO_ALBUM_IMAGE_PATH, out_name) >= (int)path_size)
        return -1;

    return 0;
}

int file_manager_refresh_photo_list(void)
{
    /* file list is managed by FILEMNG/DTCF in manager layer */
    return 0;
}

int file_manager_get_photo_count(void)
{
    uint32_t cnt = FILEMNG_GetDirFileCnt(0, FILEMNG_DIR_PHOTO);
    if (cnt == (uint32_t)-1)
        return 0;
    return (int)cnt;
}

int file_manager_get_photo_name(int index, char* out_name, size_t out_size)
{
    char filename[FILEMNG_PATH_MAX_LEN] = {0};

    if (!out_name || out_size == 0 || index < 0)
        return -1;

    if (FILEMNG_GetFileNameByFileInx(0, FILEMNG_DIR_PHOTO, (uint32_t)index, &filename, 1) != 0)
        return -1;

    if (snprintf(out_name, out_size, "%s", filename) >= (int)out_size)
        return -1;

    return 0;
}

int file_manager_get_photo_thumbnail_path(int index, char* out_path, size_t out_size)
{
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char src_lv_path[FILE_MANAGER_MAX_PATH_LEN];
    char thumb_small_real[FILE_MANAGER_MAX_PATH_LEN];
    char thumb_large_real[FILE_MANAGER_MAX_PATH_LEN];
    char thumb_small_lv[FILE_MANAGER_MAX_PATH_LEN];
    const char* src_real_path;

    if (!out_path || out_size == 0)
        return -1;

    if (get_photo_lv_path_by_index(index, file_name, sizeof(file_name), src_lv_path, sizeof(src_lv_path)) != 0)
        return -1;

    if (FILEMNG_GetThumbPathByFile(file_name, 0, 0, thumb_small_real, sizeof(thumb_small_real)) != 0)
        return -1;
    if (FILEMNG_GetThumbPathByFile(file_name, 0, 1, thumb_large_real, sizeof(thumb_large_real)) != 0)
        return -1;
    if (to_lv_path(thumb_small_real, thumb_small_lv, sizeof(thumb_small_lv)) != 0)
        return -1;

    if (file_exists_and_valid(thumb_small_real)) {
        if (snprintf(out_path, out_size, "%s", thumb_small_lv) >= (int)out_size)
            return -1;
        return 0;
    }

    src_real_path = to_real_path(src_lv_path);
    if (FILEMNG_ExtractJpegThumb(src_real_path, thumb_small_real, thumb_large_real) == 0 &&
        file_exists_and_valid(thumb_small_real)) {
        if (snprintf(out_path, out_size, "%s", thumb_small_lv) >= (int)out_size)
            return -1;
        return 0;
    }

    if (snprintf(out_path, out_size, "%s", src_lv_path) >= (int)out_size)
        return -1;

    return 0;
}

int file_manager_delete_photo_by_index(int index)
{
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char src_lv_path[FILE_MANAGER_MAX_PATH_LEN];
    char src_real_path[FILE_MANAGER_MAX_PATH_LEN];
    int ret;

    if (get_photo_lv_path_by_index(index, file_name, sizeof(file_name), src_lv_path, sizeof(src_lv_path)) != 0)
        return -1;
    if (snprintf(src_real_path, sizeof(src_real_path), "%s", to_real_path(src_lv_path)) >= (int)sizeof(src_real_path))
        return -1;

    /*
     * 通过 FILEMNG_DelFile 触发 DTCF 索引更新；
     * 若直接 unlink，会导致 FILEMNG_GetDirFileCnt 仍然读到旧计数。
     */
    ret = FILEMNG_DelFile(0, src_real_path);
    if (ret != 0) {
        MLOG_WARN("Delete photo via FILEMNG failed: index=%d file=%s path=%s ret=%d", index, file_name, src_real_path, ret);
        return -1;
    }

    return 0;
}

int file_manager_format_sdcard(void)
{
    MLOG_INFO("file_manager: start format sdcard (mock)");
    usleep(1000 * 1000);
    MLOG_INFO("file_manager: format sdcard done (mock)");
    return 0;
}
