#include "core/file_manager.h"

#include "config.h"
#include "dtcf.h"
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

#include "thumbnail_extractor.h"

typedef struct {
    char** names;
    int count;
    int capacity;
} photo_list_t;

static photo_list_t g_photo_list = {0};
static void* g_dtcf_handle = NULL;
static char g_dtcf_main_dir[DTCF_PATH_MAX_LEN];
static char g_dtcf_photo_dir[DTCF_PATH_MAX_LEN];

static const char* to_real_path(const char* path)
{
    if (path && path[0] == 'A' && path[1] == ':')
        return path + 2;
    return path;
}

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

static int32_t dtcf_photo_selector(const struct dirent* d)
{
    if (!d)
        return 0;

    if (d->d_name[0] == '.')
        return 0;

    return is_jpeg_file(d->d_name) ? 1 : 0;
}

static int32_t dtcf_photo_compare(const struct dirent** a, const struct dirent** b)
{
    if (!a || !b || !(*a) || !(*b))
        return 0;

    return strcmp((*a)->d_name, (*b)->d_name);
}

static int photo_name_cmp(const void* a, const void* b)
{
    const char* lhs = *(const char* const*)a;
    const char* rhs = *(const char* const*)b;

    return strcmp(lhs, rhs);
}

static int split_photo_path_for_dtcf(const char* photo_dir_real, char* out_main_dir, size_t main_size, char* out_photo_dir,
                                     size_t photo_size)
{
    char tmp[FILE_MANAGER_MAX_PATH_LEN];
    char* last_sep;
    size_t dir_len;

    if (!photo_dir_real || !out_main_dir || !out_photo_dir || main_size == 0 || photo_size == 0)
        return -1;

    if (snprintf(tmp, sizeof(tmp), "%s", photo_dir_real) >= (int)sizeof(tmp))
        return -1;

    dir_len = strlen(tmp);
    while (dir_len > 1 && tmp[dir_len - 1] == '/') {
        tmp[dir_len - 1] = '\0';
        --dir_len;
    }

    last_sep = strrchr(tmp, '/');
    if (!last_sep || *(last_sep + 1) == '\0')
        return -1;

    if (snprintf(out_photo_dir, photo_size, "%s", last_sep + 1) >= (int)photo_size)
        return -1;

    *last_sep = '\0';
    if (snprintf(out_main_dir, main_size, "%s", tmp) >= (int)main_size)
        return -1;

    return 0;
}

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

static int is_regular_file_path(const char* path)
{
    struct stat st;

    if (!path)
        return 0;

    if (stat(path, &st) != 0)
        return 0;

    return S_ISREG(st.st_mode);
}

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

static int extract_thumbnail_file(const char* src_path, const char* dst_path)
{
    THUMBNAIL_EXTRACTOR_HANDLE_T extractor = NULL;
    THUMBNAIL_PACKET_S packet = {0};
    FILE* fp = NULL;
    size_t expected_size = 0;
    int ret = -1;

    if (!src_path || !dst_path) {
        MLOG_WARN("Thumbnail extract invalid args: src=%p dst=%p", src_path, dst_path);
        return -1;
    }

    if (THUMBNAIL_EXTRACTOR_Create(&extractor) != 0) {
        MLOG_WARN("Thumbnail extract create failed: src=%s", src_path);
        return -1;
    }

    if (THUMBNAIL_EXTRACTOR_GetThumbnailByType(extractor, src_path, &packet, 0) != 0) {
        MLOG_WARN("Thumbnail extract payload failed: src=%s", src_path);
        goto cleanup;
    }

    fp = fopen(dst_path, "wb");
    if (!fp) {
        MLOG_WARN("Thumbnail extract open dst failed: dst=%s errno=%d", dst_path, errno);
        goto cleanup;
    }

    if (packet.size <= 0) {
        MLOG_WARN("Thumbnail extract empty payload: src=%s size=%d", src_path, packet.size);
        goto cleanup;
    }

    expected_size = (size_t)packet.size;
    if (fwrite(packet.data, 1, expected_size, fp) != expected_size) {
        MLOG_WARN("Thumbnail extract write failed: dst=%s expected=%zu errno=%d", dst_path, expected_size, errno);
        goto cleanup;
    }

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

int file_manager_refresh_photo_list(void)
{
    const char* real_dir = to_real_path(PHOTO_ALBUM_IMAGE_PATH);
    photo_list_t new_list = {0};
    DTCF_PARAM_S param;
    char dirs[1][DTCF_PATH_MAX_LEN];
    uint32_t cnt = 0;
    uint32_t start = 0;
    char names[32][DTCF_PATH_MAX_LEN];

    if (!real_dir) {
        MLOG_WARN("file_manager_refresh_photo_list real_dir is null");
        return -1;
    }

    if (refresh_photo_list_by_fs(real_dir) == 0) {
        MLOG_INFO("file_manager_refresh_photo_list done by FS scan: total=%d", g_photo_list.count);
        return 0;
    }

    MLOG_WARN("file_manager_refresh_photo_list FS scan failed, fallback to DTCF");
    MLOG_INFO("file_manager_refresh_photo_list start: real_dir=%s handle=%p", real_dir, g_dtcf_handle);

    if (!g_dtcf_handle) {
        memset(&param, 0, sizeof(param));

        if (split_photo_path_for_dtcf(real_dir, g_dtcf_main_dir, sizeof(g_dtcf_main_dir), g_dtcf_photo_dir,
                                      sizeof(g_dtcf_photo_dir)) != 0) {
            MLOG_WARN("split_photo_path_for_dtcf failed: real_dir=%s", real_dir);
            free_photo_list(&g_photo_list);
            return -1;
        }
        MLOG_INFO("DTCF path split: main_dir=%s photo_dir=%s", g_dtcf_main_dir, g_dtcf_photo_dir);

        if (snprintf(param.main_dir, sizeof(param.main_dir), "%s", g_dtcf_main_dir) >= (int)sizeof(param.main_dir)) {
            MLOG_WARN("param.main_dir overflow: main_dir=%s", g_dtcf_main_dir);
            free_photo_list(&g_photo_list);
            return -1;
        }

        param.file_selector_cb = dtcf_photo_selector;
        param.file_compare_cb = dtcf_photo_compare;

        MLOG_INFO("DTCF_Init begin: main_dir=%s", param.main_dir);
        if (DTCF_Init(&g_dtcf_handle, &param) != 0) {
            MLOG_WARN("DTCF_Init failed: main_dir=%s", param.main_dir);
            g_dtcf_handle = NULL;
            free_photo_list(&g_photo_list);
            return -1;
        }
        MLOG_INFO("DTCF_Init done: handle=%p", g_dtcf_handle);

        memset(dirs, 0, sizeof(dirs));
        snprintf(dirs[0], sizeof(dirs[0]), "%s", g_dtcf_photo_dir);
        MLOG_INFO("DTCF_CreateDir begin: dir=%s", dirs[0]);
        if (DTCF_CreateDir(g_dtcf_handle, dirs, 1, DTCF_DIR_MODE) != 0) {
            MLOG_WARN("DTCF_CreateDir failed: dir=%s", dirs[0]);
            DTCF_Deinit(g_dtcf_handle);
            g_dtcf_handle = NULL;
            free_photo_list(&g_photo_list);
            return -1;
        }
        MLOG_INFO("DTCF_CreateDir done: dir=%s", dirs[0]);
    }

    MLOG_INFO("DTCF_Scan begin: handle=%p", g_dtcf_handle);
    if (DTCF_Scan(g_dtcf_handle) != 0) {
        MLOG_WARN("DTCF_Scan failed: handle=%p", g_dtcf_handle);
        free_photo_list(&g_photo_list);
        return -1;
    }
    MLOG_INFO("DTCF_Scan done");

    MLOG_INFO("DTCF_GetDirFileCnt begin: dir=%s", g_dtcf_photo_dir);
    if (DTCF_GetDirFileCnt(g_dtcf_handle, g_dtcf_photo_dir, &cnt) != 0) {
        MLOG_WARN("DTCF_GetDirFileCnt failed: dir=%s", g_dtcf_photo_dir);
        free_photo_list(&g_photo_list);
        return -1;
    }
    MLOG_INFO("DTCF_GetDirFileCnt done: cnt=%u", cnt);

    while (start < cnt) {
        uint32_t batch = cnt - start;
        uint32_t i;

        if (batch > 32)
            batch = 32;

        memset(names, 0, sizeof(names));
        MLOG_INFO("DTCF_GetFileNameByInx begin: start=%u batch=%u", start, batch);
        if (DTCF_GetFileNameByInx(g_dtcf_handle, g_dtcf_photo_dir, names, batch, start) != 0) {
            MLOG_WARN("DTCF_GetFileNameByInx failed: start=%u batch=%u", start, batch);
            free_photo_list(&new_list);
            return -1;
        }
        MLOG_INFO("DTCF_GetFileNameByInx done: start=%u batch=%u", start, batch);

        for (i = 0; i < batch; ++i) {
            if (names[i][0] == '\0')
                continue;

            if (append_photo_name(&new_list, names[i]) != 0) {
                free_photo_list(&new_list);
                return -1;
            }
        }

        start += batch;
    }

    free_photo_list(&g_photo_list);
    g_photo_list = new_list;
    MLOG_INFO("file_manager_refresh_photo_list done: total=%d", g_photo_list.count);
    return 0;
}

int file_manager_get_photo_count(void)
{
    return g_photo_list.count;
}

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

int file_manager_get_photo_thumbnail_path(int index, char* out_path, size_t out_size)
{
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char base_name[FILE_MANAGER_MAX_NAME_LEN];
    char src_lv_path[FILE_MANAGER_MAX_PATH_LEN];
    char dst_lv_path[FILE_MANAGER_MAX_PATH_LEN];
    const char* dst_real_path;

    if (!out_path || out_size == 0)
        return -1;

    if (file_manager_get_photo_name(index, file_name, sizeof(file_name)) != 0)
        return -1;

    get_photo_basename(file_name, base_name, sizeof(base_name));

    if (snprintf(dst_lv_path, sizeof(dst_lv_path), "%s%s.jpg", PHOTO_ALBUM_IMAGE_THUMB_PATH, base_name) >=
        (int)sizeof(dst_lv_path)) {
        return -1;
    }

    dst_real_path = to_real_path(dst_lv_path);

    if (file_exists_and_valid(dst_real_path)) {
        MLOG_INFO("Thumbnail cache hit: index=%d file=%s thumb=%s", index, file_name, dst_lv_path);
        if (snprintf(out_path, out_size, "%s", dst_lv_path) >= (int)out_size)
            return -1;
        return 0;
    }

    if (ensure_dir_recursive(to_real_path(PHOTO_ALBUM_IMAGE_THUMB_PATH)) != 0)
        MLOG_WARN("Create thumbnail dir failed: %s", PHOTO_ALBUM_IMAGE_THUMB_PATH);

    if (snprintf(src_lv_path, sizeof(src_lv_path), "%s%s", PHOTO_ALBUM_IMAGE_PATH, file_name) >=
        (int)sizeof(src_lv_path)) {
        return -1;
    }

    {
        const char* src_real_path = to_real_path(src_lv_path);
        MLOG_INFO("Thumbnail extract start: index=%d file=%s src=%s dst=%s", index, file_name, src_real_path,
                  dst_real_path);
        if (extract_thumbnail_file(src_real_path, dst_real_path) == 0 && file_exists_and_valid(dst_real_path)) {
            MLOG_INFO("Thumbnail extract success: index=%d file=%s thumb=%s", index, file_name, dst_lv_path);
            if (snprintf(out_path, out_size, "%s", dst_lv_path) >= (int)out_size)
                return -1;
            return 0;
        }
        MLOG_WARN("Thumbnail extract failed, fallback source image: index=%d file=%s src=%s", index, file_name,
                  src_lv_path);
    }

    if (snprintf(out_path, out_size, "%s", src_lv_path) >= (int)out_size)
        return -1;

    return 0;
}

int file_manager_delete_photo_by_index(int index)
{
    char file_name[FILE_MANAGER_MAX_NAME_LEN];
    char base_name[FILE_MANAGER_MAX_NAME_LEN];
    char src_lv_path[FILE_MANAGER_MAX_PATH_LEN];
    char thumb_lv_path[FILE_MANAGER_MAX_PATH_LEN];
    const char* src_real_path;
    const char* thumb_real_path;
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

    removed_name = g_photo_list.names[index];
    move_count = g_photo_list.count - index - 1;
    if (move_count > 0)
        memmove(&g_photo_list.names[index], &g_photo_list.names[index + 1], (size_t)move_count * sizeof(char*));
    g_photo_list.count--;
    free(removed_name);

    MLOG_INFO("Delete photo success: index=%d file=%s remain=%d", index, file_name, g_photo_list.count);
    return 0;
}

int file_manager_format_sdcard(void)
{
    MLOG_INFO("file_manager: start format sdcard (mock)");
    usleep(1000 * 1000);
    MLOG_INFO("file_manager: format sdcard done (mock)");
    return 0;
}
