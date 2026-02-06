cmake_minimum_required(VERSION 3.10)
project(lvgl)

set(CMAKE_C_STANDARD 99)#C99 # lvgl officially support C99 and above
set(CMAKE_CXX_STANDARD 17)#C17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#=======================
# Strict Compiler Flags (for project src only, not lvgl)
#=======================
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(STRICT_C_FLAGS -Wall -Wextra -Wpedantic)
    set(STRICT_C_FLAGS ${STRICT_C_FLAGS} -Werror=return-type -Werror=uninitialized -Werror=pointer-arith)
    set(STRICT_C_FLAGS ${STRICT_C_FLAGS} -Werror=format-security -Werror=format=2)
    set(STRICT_C_FLAGS ${STRICT_C_FLAGS} -Wstrict-prototypes -Wmissing-prototypes)
    set(STRICT_C_FLAGS ${STRICT_C_FLAGS} -Wshadow -Wundef)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(STRICT_C_FLAGS ${STRICT_C_FLAGS} -Wlogical-op)
    endif()
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#=======================
# LVGL Configuration
#=======================
if(NOT DEFINED CONFIG)
	set(CONFIG "SDL")
endif()

if(CONFIG STREQUAL "SDL")
	set(LV_CONF_FILE "lv_conf_sdl.h")
	add_definitions(-DFB_RUN=0)
	find_package(SDL2)
	find_package(SDL2_image)
	include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
elseif(CONFIG STREQUAL "FB")
	set(LV_CONF_FILE "lv_conf_fb.h")
	add_definitions(-DFB_RUN=1)
else()
	message(FATAL_ERROR "Unknown CONFIG: ${CONFIG}")
endif()

# Set LVGL config path
if(CONFIG STREQUAL "SDL")
    set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/lv_conf_sdl.h" CACHE PATH "")
else()
    set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/lv_conf_fb.h" CACHE PATH "")
endif()

add_subdirectory(lvgl)
target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR})

# Add include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.c
    src/mouse_cursor_icon.c
    src/core/page_manager.c
    src/styles/style_common.c
    src/pages/page_home.c
    src/pages/page_photo.c
    src/pages/page_video.c
    src/pages/page_photo_settings.c
    src/font_manager.c
)

add_executable(main ${SOURCES})
if(STRICT_C_FLAGS)
    target_compile_options(main PRIVATE ${STRICT_C_FLAGS})
endif()

# target_link_libraries(main lvgl lvgl::examples lvgl::demos lvgl::thorvg ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${Libdrm_LIBRARIES} m pthread)
target_link_libraries(main lvgl lvgl::examples lvgl::demos lvgl::thorvg m pthread)
if(CONFIG STREQUAL "SDL")
	target_link_libraries(main ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
endif()

# FreeType library for Chinese font support
if(CONFIG STREQUAL "SDL")
    set(FREETYPE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/third_party/freetype/lib/x86_64/libfreetype.a")
    set(FREETYPE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/freetype/freetype2")
    target_include_directories(main PRIVATE ${FREETYPE_INCLUDE})
    target_include_directories(lvgl PRIVATE ${FREETYPE_INCLUDE})
    target_link_libraries(main ${FREETYPE_LIB})
endif()

add_custom_target (run COMMAND ${EXECUTABLE_OUTPUT_PATH}/main DEPENDS main)
