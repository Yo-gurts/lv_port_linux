#include "core/file_manager.h"

#include "config.h"
#include "core/media_manager.h"
#include "filemng.h"
#include "mlog.h"
#include "player.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    char name[FILE_MANAGER_MAX_NAME_LEN];
    int duration_sec;
    int status; /* 0=unknown, 1=success, -1=failed */
} video_duration_cache_entry_t;

static video_duration_cache_entry_t* g_video_duration_cache = NULL;
static int g_video_duration_cache_count = 0;

static int file_manager_is_photo_storage_ready(void)
{
    return FILEMNG_GetStorageStatus() == FILEMNG_STORAGE_STATE_SCAN_COMPLETED;
}

/* 检查照片存储是否就绪（已插入 SD 卡且扫描完成） */
int file_manager_is_storage_ready(void)
{
    return file_manager_is_photo_storage_ready();
}

int file_manager_get_storage_space_bytes(uint64_t* available_bytes)
{
    if (available_bytes == NULL) {
        return -1;
    }

    if (FILEMNG_GetAvailableSizeAfterReserveBytes(available_bytes) != 0) {
        return -1;
    }
    return 0;
}

/* 将 LVGL 路径（A:/...）转换为真实文件系统路径（/...）。 */
static const char* to_real_path(const char* path)
{
    if (path && path[0] == 'A' && path[1] == ':')
        return path + 2;
    return path;
}

/* 将真实路径转换为 LVGL 可识别路径；绝对路径自动补 A: 前缀。 */
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

/* 判断文件是否存在且有效（普通文件且大小大于 0）。 */
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

/* 按路径类型将 real_path 输出为 real/LV 路径。 */
static int export_path_by_type(const char* real_path, file_manager_path_type_t path_type, char* out_path, size_t out_size)
{
    if (!real_path || !out_path || out_size == 0)
        return -1;

    if (path_type == FILE_PATH_REAL) {
        if (snprintf(out_path, out_size, "%s", real_path) >= (int)out_size)
            return -1;
        return 0;
    }

    return to_lv_path(real_path, out_path, out_size);
}

static void clear_video_duration_cache(void)
{
    free(g_video_duration_cache);
    g_video_duration_cache = NULL;
    g_video_duration_cache_count = 0;
}

static int ensure_video_duration_cache_capacity(int required_count)
{
    video_duration_cache_entry_t* new_cache;

    if (required_count <= 0)
        return 0;
    if (g_video_duration_cache_count >= required_count)
        return 0;

    new_cache = (video_duration_cache_entry_t*)realloc(
        g_video_duration_cache, (size_t)required_count * sizeof(video_duration_cache_entry_t));
    if (!new_cache)
        return -1;

    memset(new_cache + g_video_duration_cache_count,
           0,
           (size_t)(required_count - g_video_duration_cache_count) * sizeof(video_duration_cache_entry_t));
    g_video_duration_cache = new_cache;
    g_video_duration_cache_count = required_count;
    return 0;
}

static int probe_video_duration_sec(const char* real_path, int* out_duration_sec)
{
    PLAYER_HANDLE_T player = NULL;
    PLAYER_MEDIA_INFO_S info;
    int ret = -1;

    if (!real_path || !out_duration_sec)
        return -1;

    *out_duration_sec = 0;
    memset(&info, 0, sizeof(info));

    if (PLAYER_Create(&player) != 0 || !player) {
        MLOG_WARN("Probe video duration create player failed: %s", real_path);
        goto cleanup;
    }

    if (PLAYER_SetDataSource(player, real_path) != 0) {
        MLOG_WARN("Probe video duration set source failed: %s", real_path);
        goto cleanup;
    }

    if (PLAYER_GetMediaInfo(player, &info) != 0) {
        MLOG_WARN("Probe video duration get media info failed: %s", real_path);
        goto cleanup;
    }

    *out_duration_sec = info.duration_sec > 0.0 ? (int)(info.duration_sec + 0.5) : 0;
    ret = 0;

cleanup:
    if (player)
        (void)PLAYER_Destroy(&player);
    return ret;
}

/* 通过照片索引获取文件名与原图路径（按 path_type 输出）。 */
static int get_photo_path_by_index(
    int index,
    char* out_name,
    size_t name_size,
    char* out_path,
    size_t path_size,
    file_manager_path_type_t path_type)
{
    char real_path[FILE_MANAGER_MAX_PATH_LEN];
    const char* image_dir_real;

    if (!out_name || name_size == 0 || !out_path || path_size == 0)
        return -1;

    if (file_manager_get_photo_name(index, out_name, name_size) != 0)
        return -1;

    image_dir_real = to_real_path(PHOTO_ALBUM_IMAGE_PATH);
    if (!image_dir_real)
        return -1;
    if (snprintf(real_path, sizeof(real_path), "%s%s", image_dir_real, out_name) >= (int)sizeof(real_path))
        return -1;
    if (export_path_by_type(real_path, path_type, out_path, path_size) != 0)
        return -1;

    return 0;
}

/* 通过视频索引获取文件名与原路径（按 path_type 输出）。 */
static int get_video_path_by_index(
    int index,
    char* out_name,
    size_t name_size,
    char* out_path,
    size_t path_size,
    file_manager_path_type_t path_type)
{
    char real_path[FILE_MANAGER_MAX_PATH_LEN];
    const char* video_dir_real;

    if (!out_name || name_size == 0 || !out_path || path_size == 0)
        return -1;

    if (file_manager_get_video_name(index, out_name, name_size) != 0)
        return -1;

    video_dir_real = to_real_path(VIDEO_ALBUM_VIDEO_PATH);
    if (!video_dir_real)
        return -1;
    if (snprintf(real_path, sizeof(real_path), "%s%s", video_dir_real, out_name) >= (int)sizeof(real_path))
        return -1;
    if (export_path_by_type(real_path, path_type, out_path, path_size) != 0)
        return -1;

    return 0;
}

typedef enum {
    PHOTO_DERIVED_TYPE_THUMBNAIL = 0,
    PHOTO_DERIVED_TYPE_SUBPIC = 1,
} photo_derived_type_t;

/* 统一获取派生图片路径（缩略图/子图），按类型处理提取与回退策略。 */
static int get_photo_derived_path(
    int index,
    photo_derived_type_t derived_type,
    char* out_path,
    size_t out_size,
    file_manager_path_type_t path_type)
{
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char src_real_path[FILE_MANAGER_MAX_PATH_LEN];
    char thumbnail_real_path[FILE_MANAGER_MAX_PATH_LEN];
    char subpic_real_path[FILE_MANAGER_MAX_PATH_LEN];
    const char* target_real_path;

    if (!out_path || out_size == 0)
        return -1;

    if (get_photo_path_by_index(
            index, file_name, sizeof(file_name), src_real_path, sizeof(src_real_path), FILE_PATH_REAL) != 0) {
        return -1;
    }

    if (FILEMNG_GetThumbPathByFile(file_name, 0, 0, thumbnail_real_path, sizeof(thumbnail_real_path)) != 0)
        return -1;
    if (FILEMNG_GetThumbPathByFile(file_name, 0, 1, subpic_real_path, sizeof(subpic_real_path)) != 0)
        return -1;

    target_real_path = (derived_type == PHOTO_DERIVED_TYPE_SUBPIC) ? subpic_real_path : thumbnail_real_path;
    if (file_exists_and_valid(target_real_path)) {
        if (export_path_by_type(target_real_path, path_type, out_path, out_size) != 0)
            return -1;
        return 0;
    }

    if (FILEMNG_ExtractJpegThumb(src_real_path, thumbnail_real_path, subpic_real_path) == 0 &&
        file_exists_and_valid(target_real_path)) {
        if (export_path_by_type(target_real_path, path_type, out_path, out_size) != 0)
            return -1;
        return 0;
    }

    if (derived_type == PHOTO_DERIVED_TYPE_THUMBNAIL) {
        if (export_path_by_type(src_real_path, path_type, out_path, out_size) != 0)
            return -1;
        return 0;
    }

    return -1;
}

/* 刷新照片列表（真实环境由 FILEMNG/DTCF 维护，此处仅占位返回成功）。 */
int file_manager_refresh_photo_list(void)
{
    /* file list is managed by FILEMNG/DTCF in manager layer */
    return 0;
}

/* 获取照片总数。 */
int file_manager_get_photo_count(void)
{
    if (!file_manager_is_photo_storage_ready())
        return 0;

    uint32_t cnt = FILEMNG_GetDirFileCnt(0, FILEMNG_DIR_PHOTO);
    if (cnt == (uint32_t)-1)
        return 0;
    return (int)cnt;
}

/* 按索引获取照片文件名。 */
int file_manager_get_photo_name(int index, char* out_name, size_t out_size)
{
    char filename[FILEMNG_PATH_MAX_LEN] = {0};

    if (!out_name || out_size == 0 || index < 0)
        return -1;

    if (!file_manager_is_photo_storage_ready())
        return -1;

    if (FILEMNG_GetFileNameByFileInx(0, FILEMNG_DIR_PHOTO, (uint32_t)index, &filename, 1) != 0)
        return -1;

    if (snprintf(out_name, out_size, "%s", filename) >= (int)out_size)
        return -1;

    return 0;
}

/* 按索引获取原图路径。 */
int file_manager_get_photo_path(int index, char* out_path, size_t out_size, file_manager_path_type_t path_type)
{
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char real_path[FILE_MANAGER_MAX_PATH_LEN];
    const char* image_dir_real;

    if (!out_path || out_size == 0)
        return -1;

    if (file_manager_get_photo_name(index, file_name, sizeof(file_name)) != 0)
        return -1;

    image_dir_real = to_real_path(PHOTO_ALBUM_IMAGE_PATH);
    if (!image_dir_real)
        return -1;
    if (snprintf(real_path, sizeof(real_path), "%s%s", image_dir_real, file_name) >= (int)sizeof(real_path))
        return -1;
    if (export_path_by_type(real_path, path_type, out_path, out_size) != 0)
        return -1;

    return 0;
}

/* 按索引获取缩略图路径。 */
int file_manager_get_photo_thumbnail_path(int index, char* out_path, size_t out_size, file_manager_path_type_t path_type)
{
    return get_photo_derived_path(index, PHOTO_DERIVED_TYPE_THUMBNAIL, out_path, out_size, path_type);
}

/* 按索引获取子图路径（photo_large）。 */
int file_manager_get_photo_subpic_path(int index, char* out_path, size_t out_size, file_manager_path_type_t path_type)
{
    return get_photo_derived_path(index, PHOTO_DERIVED_TYPE_SUBPIC, out_path, out_size, path_type);
}

/* 按索引删除照片文件（通过 FILEMNG 触发索引更新）。 */
int file_manager_delete_photo_by_index(int index)
{
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char src_real_path[FILE_MANAGER_MAX_PATH_LEN];
    int ret;

    if (get_photo_path_by_index(
            index, file_name, sizeof(file_name), src_real_path, sizeof(src_real_path), FILE_PATH_REAL) != 0) {
        return -1;
    }

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

/* 格式化 SD 卡。 */
int file_manager_format_sdcard(void)
{
    int ret;

    if (FILEMNG_GetStorageStatus() == FILEMNG_STORAGE_STATE_NOT_AVAILABLE) {
        MLOG_WARN("file_manager: format sdcard skipped, storage unavailable");
        return -1;
    }

    MLOG_INFO("file_manager: start format sdcard");
    ret = media_manager_execute(MEDIA_OP_FORMAT_STORAGE, 0);
    if (ret != MEDIA_MANAGER_OK) {
        MLOG_ERR("file_manager: format sdcard failed: ret=%d", ret);
        return -1;
    }

    clear_video_duration_cache();
    MLOG_INFO("file_manager: format sdcard done");
    return 0;
}

/* 刷新视频列表（真实环境由 FILEMNG/DTCF 维护，此处仅占位返回成功）。 */
int file_manager_refresh_video_list(void)
{
    clear_video_duration_cache();
    return 0;
}

/* 获取视频总数（当前真机接口未接入，先返回 0）。 */
int file_manager_get_video_count(void)
{
    uint32_t cnt;

    if (!file_manager_is_photo_storage_ready())
        return 0;

    cnt = FILEMNG_GetDirFileCnt(0, FILEMNG_DIR_NORMAL);
    if (cnt == (uint32_t)-1)
        return 0;
    return (int)cnt;
}

/* 按索引获取视频文件名 */
int file_manager_get_video_name(int index, char* out_name, size_t out_size)
{
    char filename[FILEMNG_PATH_MAX_LEN] = { 0 };

    if (!out_name || out_size == 0 || index < 0)
        return -1;
    if (!file_manager_is_photo_storage_ready())
        return -1;

    if (FILEMNG_GetFileNameByFileInx(0, FILEMNG_DIR_NORMAL, (uint32_t)index, &filename, 1) != 0)
        return -1;

    if (snprintf(out_name, out_size, "%s", filename) >= (int)out_size)
        return -1;
    return 0;
}

/* 按索引获取视频路径 */
int file_manager_get_video_path(int index, char* out_path, size_t out_size, file_manager_path_type_t path_type)
{
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char real_path[FILE_MANAGER_MAX_PATH_LEN];
    const char* video_dir_real;

    if (!out_path || out_size == 0)
        return -1;

    if (file_manager_get_video_name(index, file_name, sizeof(file_name)) != 0)
        return -1;

    video_dir_real = to_real_path(VIDEO_ALBUM_VIDEO_PATH);
    if (!video_dir_real)
        return -1;
    if (snprintf(real_path, sizeof(real_path), "%s%s", video_dir_real, file_name) >= (int)sizeof(real_path))
        return -1;
    if (export_path_by_type(real_path, path_type, out_path, out_size) != 0)
        return -1;

    return 0;
}

/* 按索引获取视频缩略图路径（当前真机接口未接入）。 */
int file_manager_get_video_thumbnail_path(int index, char* out_path, size_t out_size, file_manager_path_type_t path_type)
{
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char thumb_real_path[FILE_MANAGER_MAX_PATH_LEN];

    if (!out_path || out_size == 0)
        return -1;

    if (file_manager_get_video_name(index, file_name, sizeof(file_name)) != 0)
        return -1;

    /* is_video=1, thumb_type=0(小图) */
    if (FILEMNG_GetThumbPathByFile(file_name, 1, 0, thumb_real_path, sizeof(thumb_real_path)) != 0)
        return -1;

    if (!file_exists_and_valid(thumb_real_path))
        return -1;

    return export_path_by_type(thumb_real_path, path_type, out_path, out_size);
}

/* 按索引获取视频大图路径（video_large）。 */
int file_manager_get_video_subpic_path(int index, char* out_path, size_t out_size, file_manager_path_type_t path_type)
{
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char subpic_real_path[FILE_MANAGER_MAX_PATH_LEN];

    if (!out_path || out_size == 0)
        return -1;

    if (file_manager_get_video_name(index, file_name, sizeof(file_name)) != 0)
        return -1;

    /* is_video=1, thumb_type=1(大图) */
    if (FILEMNG_GetThumbPathByFile(file_name, 1, 1, subpic_real_path, sizeof(subpic_real_path)) != 0)
        return -1;

    if (!file_exists_and_valid(subpic_real_path))
        return -1;

    return export_path_by_type(subpic_real_path, path_type, out_path, out_size);
}

/* 按索引获取视频时长（秒）。 */
int file_manager_get_video_duration_sec(int index, int* out_duration_sec)
{
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char real_path[FILE_MANAGER_MAX_PATH_LEN];
    video_duration_cache_entry_t* cache_entry = NULL;
    int video_count;

    if (!out_duration_sec)
        return -1;
    if (index < 0)
        return -1;

    *out_duration_sec = 0;
    if (!file_manager_is_photo_storage_ready())
        return -1;

    video_count = file_manager_get_video_count();
    if (index >= video_count)
        return -1;

    if (file_manager_get_video_name(index, file_name, sizeof(file_name)) != 0)
        return -1;

    if (ensure_video_duration_cache_capacity(video_count) == 0)
        cache_entry = &g_video_duration_cache[index];

    if (cache_entry && cache_entry->status != 0 && strcmp(cache_entry->name, file_name) == 0) {
        *out_duration_sec = cache_entry->duration_sec;
        return cache_entry->status > 0 ? 0 : -1;
    }

    if (get_video_path_by_index(
            index, file_name, sizeof(file_name), real_path, sizeof(real_path), FILE_PATH_REAL)
        != 0) {
        return -1;
    }

    if (probe_video_duration_sec(real_path, out_duration_sec) != 0) {
        if (cache_entry) {
            snprintf(cache_entry->name, sizeof(cache_entry->name), "%s", file_name);
            cache_entry->duration_sec = 0;
            cache_entry->status = -1;
        }
        return -1;
    }

    if (cache_entry) {
        snprintf(cache_entry->name, sizeof(cache_entry->name), "%s", file_name);
        cache_entry->duration_sec = *out_duration_sec;
        cache_entry->status = 1;
    }

    return 0;
}

int file_manager_delete_video_by_index(int index)
{
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char src_real_path[FILE_MANAGER_MAX_PATH_LEN];
    int ret;

    if (get_video_path_by_index(
            index, file_name, sizeof(file_name), src_real_path, sizeof(src_real_path), FILE_PATH_REAL)
        != 0) {
        return -1;
    }

    ret = FILEMNG_DelFile(0, src_real_path);
    if (ret != 0) {
        MLOG_WARN("Delete video via FILEMNG failed: index=%d file=%s path=%s ret=%d", index, file_name, src_real_path, ret);
        return -1;
    }

    clear_video_duration_cache();
    return 0;
}
