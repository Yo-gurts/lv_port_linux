#include "core/intent.h"
#include "mlog.h"

static int handle_intent_open_video_page(void)
{
    /* TODO: get required runtime settings from param_manager and route to usecase. */
    MLOG_INFO("Handle INTENT_OPEN_VIDEO_PAGE (placeholder)");
    return 0;
}

static int handle_intent_open_photo_page(void)
{
    /* TODO: get required runtime settings from param_manager and route to usecase. */
    MLOG_INFO("Handle INTENT_OPEN_PHOTO_PAGE (placeholder)");
    return 0;
}

const char* intent_type_to_string(intent_type_t type)
{
#define ENUM_CASE(x) \
    case x:          \
        return #x
    switch (type) {
        ENUM_CASE(INTENT_OPEN_VIDEO_PAGE);
        ENUM_CASE(INTENT_OPEN_PHOTO_PAGE);
    default:
        return "INTENT_UNKNOWN";
    }
#undef ENUM_CASE
}

int intent_dispatch(intent_type_t type)
{
    MLOG_INFO("Dispatch intent: %s", intent_type_to_string(type));

    switch (type) {
    case INTENT_OPEN_VIDEO_PAGE:
        return handle_intent_open_video_page();
    case INTENT_OPEN_PHOTO_PAGE:
        return handle_intent_open_photo_page();
    default:
        MLOG_WARN("Unhandled intent: %s", intent_type_to_string(type));
        return -1;
    }
}
