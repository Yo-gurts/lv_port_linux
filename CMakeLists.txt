cmake_minimum_required(VERSION 3.10)
project(lvgl)

set(CMAKE_C_STANDARD 99)#C99 # lvgl officially support C99 and above
set(CMAKE_CXX_STANDARD 17)#C17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(lvgl)
target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR})

add_executable(main src/main.c src/mouse_cursor_icon.c)

# 增加 include 路径
include_directories(${PROJECT_SOURCE_DIR}/src/include)

# 相关源文件
file(GLOB GUIGUIDER_SRCS
    ${PROJECT_SOURCE_DIR}/src/*.c
)

# 将 guiguider_ui 源文件加入 main 目标
set_property(TARGET main APPEND PROPERTY SOURCES ${GUIGUIDER_SRCS})

# include(${CMAKE_CURRENT_LIST_DIR}/lvgl/tests/FindLibDRM.cmake)
# include_directories(${Libdrm_INCLUDE_DIRS})

if(NOT DEFINED CONFIG)
	set(CONFIG "SDL")
endif()

if(CONFIG STREQUAL "SDL")
	add_definitions(-DFB_RUN=0)
	find_package(SDL2)
	find_package(SDL2_image)
	include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
elseif(CONFIG STREQUAL "FB")
	add_definitions(-DFB_RUN=1)
else()
	message(FATAL_ERROR "Unknown CONFIG: ${CONFIG}")
endif()

# target_link_libraries(main lvgl lvgl::examples lvgl::demos lvgl::thorvg ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${Libdrm_LIBRARIES} m pthread)
target_link_libraries(main lvgl lvgl::examples lvgl::demos lvgl::thorvg m pthread)
if(CONFIG STREQUAL "SDL")
	target_link_libraries(main ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
endif()
add_custom_target (run COMMAND ${EXECUTABLE_OUTPUT_PATH}/main DEPENDS main)
