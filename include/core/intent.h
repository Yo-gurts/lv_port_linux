#ifndef __CORE_INTENT_H__
#define __CORE_INTENT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    INTENT_OPEN_VIDEO_PAGE = 0,
    INTENT_OPEN_PHOTO_PAGE,
} intent_type_t;

const char* intent_type_to_string(intent_type_t type);
int intent_dispatch(intent_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* __CORE_INTENT_H__ */
