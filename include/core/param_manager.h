#ifndef __PARAM_MANAGER_H__
#define __PARAM_MANAGER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 参数ID枚举 - 与photo_settings的configs数组顺序对应 */
typedef enum {
    PARAM_ID_RESOLUTION = 0, /* 照片分辨率 */
    PARAM_ID_WHITE_BALANCE, /* 白平衡（照片/视频共享） */
    PARAM_ID_ISO, /* 感光度（照片/视频共享） */
    PARAM_ID_EXPOSURE, /* 曝光设置（照片/视频共享） */
    PARAM_ID_QUALITY, /* 画质 */
    PARAM_ID_FACE_DETECTION, /* 人脸检测 */
    PARAM_ID_SMILE_CAPTURE, /* 笑脸抓拍 */
    PARAM_ID_VIDEO_RESOLUTION, /* 视频分辨率 */
    PARAM_ID_AI_MODE, /* AI功能模式：风格变换/AI识万物/拍照翻译 */
    PARAM_ID_VOLUME, /* 音量 (0-100) */
    PARAM_ID_BUTT,
    PARAM_ID_NONE = -1 /* 无效ID，不同步到param_manager */
} param_id_t;

typedef enum {
    PHOTO_RESOLUTION_8M = 0,
    PHOTO_RESOLUTION_12M,
    PHOTO_RESOLUTION_24M,
    PHOTO_RESOLUTION_48M,
    PHOTO_RESOLUTION_64M,
    PHOTO_RESOLUTION_BUTT
} photo_resolution_t;

typedef enum {
    WHITE_BALANCE_AUTO = 0,
    WHITE_BALANCE_SUNNY,
    WHITE_BALANCE_CLOUDY,
    WHITE_BALANCE_INCANDESCENT,
    WHITE_BALANCE_FLUORESCENT,
    WHITE_BALANCE_BUTT
} white_balance_t;

typedef enum {
    ISO_AUTO = 0,
    ISO_100,
    ISO_400,
    ISO_800,
    ISO_1600,
    ISO_3200,
    ISO_BUTT
} iso_t;

typedef enum {
    EXPOSURE_EV_NEG_2_0 = 0,
    EXPOSURE_EV_NEG_1_5,
    EXPOSURE_EV_NEG_1_0,
    EXPOSURE_EV_NEG_0_5,
    EXPOSURE_EV_0,
    EXPOSURE_EV_POS_0_5,
    EXPOSURE_EV_POS_1_0,
    EXPOSURE_EV_POS_1_5,
    EXPOSURE_EV_POS_2_0,
    EXPOSURE_BUTT
} exposure_t;

typedef enum {
    QUALITY_SUPER = 0,
    QUALITY_HIGH,
    QUALITY_NORMAL,
    QUALITY_BUTT
} quality_t;

typedef enum {
    VIDEO_RESOLUTION_4K = 0,
    VIDEO_RESOLUTION_2_7K,
    VIDEO_RESOLUTION_1080P,
    VIDEO_RESOLUTION_720P,
    VIDEO_RESOLUTION_BUTT
} video_resolution_t;

typedef enum {
    AI_MODE_STYLE_TRANSFER = 0,
    AI_MODE_OBJECT_RECOGNITION,
    AI_MODE_TRANSLATION,
    AI_MODE_BUTT
} ai_mode_t;

/* 参数管理器初始化 */
int param_manager_init(void);

/* 参数管理器反初始化 */
void param_manager_deinit(void);

/* 在主线程周期调用，派发参数变化回调（适合回调里操作 LVGL）。 */
void param_manager_poll(void);

/* 获取参数值（返回索引） */
int param_manager_get(param_id_t id);

/* 设置参数值（传入索引） */
int param_manager_set(param_id_t id, int value);

/* 获取参数默认值 */
int param_manager_get_default(param_id_t id);

/* 重置所有参数到默认值 */
void param_manager_reset_all(void);

/* 参数变化回调类型 */
typedef void (*param_change_callback_t)(param_id_t id, int value, void* user_data);

/* 注册参数变化回调 */
int param_manager_register_callback(param_change_callback_t callback, void* user_data);

/* 注销参数变化回调 */
void param_manager_unregister_callback(param_change_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* __PARAM_MANAGER_H__ */
