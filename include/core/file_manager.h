#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* 格式化 SD 卡（mock: 仅延时模拟处理） */
int file_manager_format_sdcard(void);

#ifdef __cplusplus
}
#endif

#endif /* __FILE_MANAGER_H__ */
