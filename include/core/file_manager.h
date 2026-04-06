#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FILE_MANAGER_MAX_NAME_LEN 128
#define FILE_MANAGER_MAX_PATH_LEN 256

typedef enum {
    FILE_PATH_REAL = 0,
    FILE_PATH_LV = 1,
} file_manager_path_type_t;

/* 格式化 SD 卡。 */
int file_manager_format_sdcard(void);

/* 检查照片存储是否就绪（已插入 SD 卡且扫描完成） */
int file_manager_is_storage_ready(void);
int file_manager_get_storage_space_bytes(uint64_t* available_bytes);
int file_manager_get_remaining_photo_count(int resolution_index, int quality_index, uint32_t* out_count);
int file_manager_get_remaining_video_seconds(int video_resolution_index, uint32_t* out_seconds);

int file_manager_refresh_photo_list(void);
int file_manager_get_photo_count(void);
int file_manager_get_photo_name(int index, char* out_name, size_t out_size);
int file_manager_get_photo_path(int index, char* out_path, size_t out_size, file_manager_path_type_t path_type);
int file_manager_get_photo_thumbnail_path(int index, char* out_path, size_t out_size, file_manager_path_type_t path_type);
int file_manager_get_photo_subpic_path(int index, char* out_path, size_t out_size, file_manager_path_type_t path_type);
int file_manager_delete_photo_by_index(int index);

int file_manager_refresh_video_list(void);
int file_manager_get_video_count(void);
int file_manager_get_video_name(int index, char* out_name, size_t out_size);
int file_manager_get_video_path(int index, char* out_path, size_t out_size, file_manager_path_type_t path_type);
int file_manager_get_video_thumbnail_path(int index, char* out_path, size_t out_size, file_manager_path_type_t path_type);
int file_manager_get_video_subpic_path(int index, char* out_path, size_t out_size, file_manager_path_type_t path_type);
int file_manager_get_video_duration_sec(int index, int* out_duration_sec);
int file_manager_delete_video_by_index(int index);

#ifdef __cplusplus
}
#endif

#endif /* __FILE_MANAGER_H__ */
