#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FILE_MANAGER_MAX_NAME_LEN 128
#define FILE_MANAGER_MAX_PATH_LEN 256

/* 格式化 SD 卡（mock: 仅延时模拟处理） */
int file_manager_format_sdcard(void);

int file_manager_refresh_photo_list(void);
int file_manager_get_photo_count(void);
int file_manager_get_photo_name(int index, char* out_name, size_t out_size);
int file_manager_get_photo_path(int index, char* out_path, size_t out_size);
int file_manager_get_photo_thumbnail_path(int index, char* out_path, size_t out_size);
int file_manager_get_photo_subpic_path(int index, char* out_path, size_t out_size);
int file_manager_delete_photo_by_index(int index);

#ifdef __cplusplus
}
#endif

#endif /* __FILE_MANAGER_H__ */
