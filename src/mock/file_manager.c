#include "core/file_manager.h"

#include "config.h"
#include "mlog.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef COMPONENTS_THUMBNAIL_EXTRACTOR_ON
#include "thumbnail_extractor.h"
#endif

typedef struct {
    char** names;
    int count;
    int capacity;
} photo_list_t;

static photo_list_t g_photo_list = {0};

typedef enum {
    PHOTO_DERIVED_TYPE_THUMBNAIL = 0,
    PHOTO_DERIVED_TYPE_SUBPIC = 1,
} photo_derived_type_t;

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

/* 判断文件名是否为 JPEG 扩展名。 */
static int is_jpeg_file(const char* name)
{
    const char* dot;

    if (!name)
        return 0;

    dot = strrchr(name, '.');
    if (!dot)
        return 0;

    return (strcasecmp(dot, ".jpg") == 0 || strcasecmp(dot, ".jpeg") == 0);
}

/* 照片文件名字典序比较函数，用于排序。 */
static int photo_name_cmp(const void* a, const void* b)
{
    const char* lhs = *(const char* const*)a;
    const char* rhs = *(const char* const*)b;

    return strcmp(rhs, lhs); /* 降序排序，最新的文件在前 */
}

/* 释放照片列表内存。 */
static void free_photo_list(photo_list_t* list)
{
    int i;

    if (!list || !list->names)
        return;

    for (i = 0; i < list->count; ++i)
        free(list->names[i]);

    free(list->names);
    list->names = NULL;
    list->count = 0;
    list->capacity = 0;
}

/* 追加一个照片文件名到列表，必要时扩容。 */
static int append_photo_name(photo_list_t* list, const char* name)
{
    char** new_names;
    char* dup_name;
    int new_capacity;

    if (!list || !name)
        return -1;

    if (list->count >= list->capacity) {
        new_capacity = (list->capacity == 0) ? 64 : (list->capacity * 2);
        new_names = (char**)realloc(list->names, (size_t)new_capacity * sizeof(char*));
        if (!new_names)
            return -1;

        list->names = new_names;
        list->capacity = new_capacity;
    }

    dup_name = strdup(name);
    if (!dup_name)
        return -1;

    list->names[list->count++] = dup_name;
    return 0;
}

/* 递归确保目录存在。 */
static int ensure_dir_recursive(const char* dir_path)
{
    char tmp[FILE_MANAGER_MAX_PATH_LEN];
    size_t len;
    char* p;

    if (!dir_path)
        return -1;

    len = strlen(dir_path);
    if (len == 0 || len >= sizeof(tmp))
        return -1;

    memcpy(tmp, dir_path, len + 1);

    if (tmp[len - 1] == '/')
        tmp[len - 1] = '\0';

    for (p = tmp + 1; *p != '\0'; ++p) {
        if (*p != '/')
            continue;

        *p = '\0';
        if (mkdir(tmp, 0775) != 0 && errno != EEXIST)
            return -1;
        *p = '/';
    }

    if (mkdir(tmp, 0775) != 0 && errno != EEXIST)
        return -1;

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

/* 判断路径是否为普通文件。 */
static int is_regular_file_path(const char* path)
{
    struct stat st;

    if (!path)
        return 0;

    if (stat(path, &st) != 0)
        return 0;

    return S_ISREG(st.st_mode);
}

/* 提取文件主名（去掉扩展名）。 */
static void get_photo_basename(const char* name, char* out_base, size_t out_size)
{
    const char* dot;
    size_t len;

    if (!out_base || out_size == 0)
        return;

    out_base[0] = '\0';

    if (!name)
        return;

    dot = strrchr(name, '.');
    len = dot ? (size_t)(dot - name) : strlen(name);

    if (len >= out_size)
        len = out_size - 1;

    memcpy(out_base, name, len);
    out_base[len] = '\0';
}

/* 通过扫描文件系统刷新照片列表。 */
static int refresh_photo_list_by_fs(const char* real_dir)
{
    char full_path[FILE_MANAGER_MAX_PATH_LEN];
    DIR* dir;
    struct dirent* ent;
    photo_list_t new_list = {0};

    if (!real_dir)
        return -1;

    dir = opendir(real_dir);
    if (!dir) {
        MLOG_WARN("FS scan open photo dir failed: %s errno=%d", real_dir, errno);
        return -1;
    }

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.')
            continue;

        if (!is_jpeg_file(ent->d_name))
            continue;

        if (snprintf(full_path, sizeof(full_path), "%s%s", real_dir, ent->d_name) >= (int)sizeof(full_path))
            continue;

        if (!is_regular_file_path(full_path))
            continue;

        if (append_photo_name(&new_list, ent->d_name) != 0) {
            free_photo_list(&new_list);
            closedir(dir);
            return -1;
        }
    }

    closedir(dir);

    if (new_list.count > 1)
        qsort(new_list.names, (size_t)new_list.count, sizeof(char*), photo_name_cmp);

    free_photo_list(&g_photo_list);
    g_photo_list = new_list;
    return 0;
}

#ifdef COMPONENTS_THUMBNAIL_EXTRACTOR_ON
/* 缩略图提取函数前置声明（启用提取器时有效）。 */
static int extract_thumbnail_file(const char* src_path, const char* dst_path);
#endif

/* 统一获取派生图片路径（缩略图/子图），并按策略回退。 */
static int get_photo_derived_lv_path(
    int index,
    photo_derived_type_t derived_type,
    char* out_path,
    size_t out_size,
    file_manager_path_type_t path_type)
{
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char base_name[FILE_MANAGER_MAX_NAME_LEN];
    char src_real_path[FILE_MANAGER_MAX_PATH_LEN];
    char target_real_path_buf[FILE_MANAGER_MAX_PATH_LEN];
    const char* target_real_path;
    const char* image_dir_real;
    const char* target_dir_lv;
    const char* target_dir_log_name;
    int fallback_to_thumbnail;

    if (!out_path || out_size == 0)
        return -1;

    if (file_manager_get_photo_name(index, file_name, sizeof(file_name)) != 0)
        return -1;

    get_photo_basename(file_name, base_name, sizeof(base_name));

    if (derived_type == PHOTO_DERIVED_TYPE_THUMBNAIL) {
        target_dir_lv = PHOTO_ALBUM_IMAGE_THUMB_PATH;
        target_dir_log_name = "thumbnail";
        fallback_to_thumbnail = 0;
    } else {
        target_dir_lv = PHOTO_ALBUM_IMAGE_SUBPIC_PATH;
        target_dir_log_name = "subpic";
        fallback_to_thumbnail = 1;
    }

    if (snprintf(target_real_path_buf, sizeof(target_real_path_buf), "%s%s.jpg", to_real_path(target_dir_lv), base_name) >=
        (int)sizeof(target_real_path_buf)) {
        return -1;
    }
    target_real_path = target_real_path_buf;

    image_dir_real = to_real_path(PHOTO_ALBUM_IMAGE_PATH);
    if (!image_dir_real)
        return -1;
    if (snprintf(src_real_path, sizeof(src_real_path), "%s%s", image_dir_real, file_name) >= (int)sizeof(src_real_path))
        return -1;

    if (file_exists_and_valid(target_real_path)) {
        if (export_path_by_type(target_real_path, path_type, out_path, out_size) != 0)
            return -1;
        return 0;
    }

    if (ensure_dir_recursive(to_real_path(target_dir_lv)) != 0)
        MLOG_WARN("Create %s dir failed: %s", target_dir_log_name, target_dir_lv);

#ifdef COMPONENTS_THUMBNAIL_EXTRACTOR_ON
    if (extract_thumbnail_file(src_real_path, target_real_path) == 0 && file_exists_and_valid(target_real_path)) {
        if (export_path_by_type(target_real_path, path_type, out_path, out_size) != 0)
            return -1;
        return 0;
    }
#endif

    if (fallback_to_thumbnail)
        return file_manager_get_photo_thumbnail_path(index, out_path, out_size, path_type);

    if (export_path_by_type(src_real_path, path_type, out_path, out_size) != 0)
        return -1;

    return 0;
}

#ifdef COMPONENTS_THUMBNAIL_EXTRACTOR_ON
/* 从原图提取缩略图并写入目标文件。 */
static int extract_thumbnail_file(const char* src_path, const char* dst_path)
{
    THUMBNAIL_EXTRACTOR_HANDLE_T extractor = NULL;
    THUMBNAIL_PACKET_S packet = {0};
    FILE* fp = NULL;
    int ret = -1;

    if (!src_path || !dst_path)
        return -1;

    if (THUMBNAIL_EXTRACTOR_Create(&extractor) != 0)
        return -1;

    if (THUMBNAIL_EXTRACTOR_GetThumbnailByType(extractor, src_path, &packet, 0) != 0)
        goto cleanup;

    fp = fopen(dst_path, "wb");
    if (!fp)
        goto cleanup;

    if (fwrite(packet.data, 1, packet.size, fp) != packet.size)
        goto cleanup;

    ret = 0;

cleanup:
    if (fp)
        fclose(fp);
    if (packet.data)
        THUMBNAIL_EXTRACTOR_ClearPacket(&packet);
    if (extractor)
        THUMBNAIL_EXTRACTOR_Destroy(&extractor);

    return ret;
}
#endif

/* 刷新照片列表（mock 通过目录扫描实现）。 */
int file_manager_refresh_photo_list(void)
{
    const char* real_dir = to_real_path(PHOTO_ALBUM_IMAGE_PATH);

    if (!real_dir) {
        MLOG_WARN("file_manager_refresh_photo_list real_dir is null");
        return -1;
    }

    if (refresh_photo_list_by_fs(real_dir) == 0) {
        MLOG_INFO("file_manager_refresh_photo_list done by FS scan: total=%d", g_photo_list.count);
        return 0;
    }

    free_photo_list(&g_photo_list);
    return -1;
}

/* 获取照片总数。 */
int file_manager_get_photo_count(void)
{
    return g_photo_list.count;
}

/* 按索引获取照片文件名。 */
int file_manager_get_photo_name(int index, char* out_name, size_t out_size)
{
    if (!out_name || out_size == 0)
        return -1;

    if (index < 0 || index >= g_photo_list.count)
        return -1;

    if (snprintf(out_name, out_size, "%s", g_photo_list.names[index]) >= (int)out_size)
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
    return get_photo_derived_lv_path(index, PHOTO_DERIVED_TYPE_THUMBNAIL, out_path, out_size, path_type);
}

/* 按索引获取子图路径（photo_large）。 */
int file_manager_get_photo_subpic_path(int index, char* out_path, size_t out_size, file_manager_path_type_t path_type)
{
    return get_photo_derived_lv_path(index, PHOTO_DERIVED_TYPE_SUBPIC, out_path, out_size, path_type);
}

/* 按索引删除原图及其派生图，并同步更新内存列表。 */
int file_manager_delete_photo_by_index(int index)
{
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char base_name[FILE_MANAGER_MAX_NAME_LEN];
    char src_lv_path[FILE_MANAGER_MAX_PATH_LEN];
    char thumb_lv_path[FILE_MANAGER_MAX_PATH_LEN];
    char subpic_lv_path[FILE_MANAGER_MAX_PATH_LEN];
    const char* src_real_path;
    const char* thumb_real_path;
    const char* subpic_real_path;
    char* removed_name;
    int move_count;

    if (file_manager_get_photo_name(index, file_name, sizeof(file_name)) != 0)
        return -1;

    if (snprintf(src_lv_path, sizeof(src_lv_path), "%s%s", PHOTO_ALBUM_IMAGE_PATH, file_name) >= (int)sizeof(src_lv_path))
        return -1;

    src_real_path = to_real_path(src_lv_path);
    if (unlink(src_real_path) != 0 && errno != ENOENT) {
        MLOG_WARN("Delete photo failed: index=%d file=%s path=%s errno=%d", index, file_name, src_real_path, errno);
        return -1;
    }

    get_photo_basename(file_name, base_name, sizeof(base_name));
    if (snprintf(thumb_lv_path, sizeof(thumb_lv_path), "%s%s.jpg", PHOTO_ALBUM_IMAGE_THUMB_PATH, base_name) <
        (int)sizeof(thumb_lv_path)) {
        thumb_real_path = to_real_path(thumb_lv_path);
        if (unlink(thumb_real_path) != 0 && errno != ENOENT)
            MLOG_WARN("Delete thumbnail failed: file=%s path=%s errno=%d", file_name, thumb_real_path, errno);
    }
    if (snprintf(subpic_lv_path, sizeof(subpic_lv_path), "%s%s.jpg", PHOTO_ALBUM_IMAGE_SUBPIC_PATH, base_name) <
        (int)sizeof(subpic_lv_path)) {
        subpic_real_path = to_real_path(subpic_lv_path);
        if (unlink(subpic_real_path) != 0 && errno != ENOENT)
            MLOG_WARN("Delete subpic failed: file=%s path=%s errno=%d", file_name, subpic_real_path, errno);
    }

    removed_name = g_photo_list.names[index];
    move_count = g_photo_list.count - index - 1;
    if (move_count > 0)
        memmove(&g_photo_list.names[index], &g_photo_list.names[index + 1], (size_t)move_count * sizeof(char*));
    g_photo_list.count--;
    free(removed_name);

    MLOG_INFO("Delete photo success: index=%d file=%s remain=%d", index, file_name, g_photo_list.count);
    return 0;
}

/* 格式化 SD 卡（当前为 mock 延时实现）。 */
int file_manager_format_sdcard(void)
{
    MLOG_INFO("file_manager: start format sdcard (mock)");
    usleep(1000 * 1000);
    MLOG_INFO("file_manager: format sdcard done (mock)");
    return 0;
}

/* 检查照片存储是否就绪（mock: 检查照片目录是否存在） */
int file_manager_is_storage_ready(void)
{
    const char* real_dir = to_real_path(PHOTO_ALBUM_IMAGE_PATH);
    struct stat st;

    if (!real_dir)
        return 0;

    /* 检查目录是否存在 */
    if (stat(real_dir, &st) != 0)
        return 0;

    return S_ISDIR(st.st_mode) ? 1 : 0;
}
