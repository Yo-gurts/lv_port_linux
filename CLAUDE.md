# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an LVGL-based UI application for an AI camera/dashcam running on Linux. The project targets embedded devices (Sophgo chipsets) and supports development on x86_64 using SDL2.

## Build Commands

```bash
# Build with SDL (default, for x86_64 development)
make

# Build with framebuffer/DRM (for embedded devices)
make CONFIG=FB

# Build for ARM platforms
make PLATFORM=musl_arm32   # Build with musl toolchain
make PLATFORM=glibc_arm32  # Build with glibc toolchain

# Clean build
make clean

# Run the application
make run
```

**CMake direct build:**
```bash
mkdir build/x86_64 && cd build/x86_64
cmake -DCONFIG=SDL ..
make -j
```

## Architecture

### Page Manager System

The app uses a custom **Page Manager** (`src/core/page_manager.c`) for managing UI pages:

- **Lifecycle**: `create()` → `show()` → `hide()` → `destroy()`
- **Navigation**: `page_manager_navigate(pm, "page_name")` and `page_manager_back(pm)`
- **Pages**: Home, Photo, Video, Photo Settings
- Each page stores private data via `page_set_private_data()/page_get_private_data()`

### Directory Structure

```
src/
├── main.c              # Entry point, initializes LVGL and registers pages
├── core/
│   └── page_manager.c # Page lifecycle and navigation management
├── pages/
│   ├── page_home.c           # Home page with grid of function icons
│   ├── page_photo.c          # Photo capture mode
│   ├── page_video.c          # Video recording mode
│   └── page_photo_settings.c # Photo settings configuration
├── styles/
│   └── style_common.c  # Reusable LVGL styles
└── font_manager.c       # FreeType-based Chinese font support
```

### Key Components

- **LVGL**: Graphics library (in `lvgl/` submodule)
- **FreeType**: Chinese font rendering (in `third_party/freetype/`)
- **SDL2**: Display backend for x86 development
- **MLOG**: Syslog-based logging with levels (MLOG_ERR, MLOG_INFO, MLOG_DBG, etc.)

## Code Style

- **Format**: `clang-format-12` with WebKit style (`.clang-format`)
- **Auto-format on commit**: Pre-commit hook in `.git/hooks/` applies clang-format-diff-12

## LVGL Development Guidelines

Follow these rules from `Contributing.md` to ensure stability and performance:

1. **Avoid threads** - Use LVGL timers (`lv_timer_create()`) instead
2. **Single-thread LVGL** - All LVGL calls must be in the GUI thread
3. **Reuse objects** - Avoid frequent create/destroy; use hide/show
4. **Static resources** - Load fonts/images statically when possible
5. **Short timer callbacks** - Never block in timer callbacks
6. **Limit animations** - Excessive animations increase CPU usage
7. **Minimize style changes** - Pre-define styles, avoid runtime changes
8. **Use virtual lists** - For large lists/images (tileview, list, table)
9. **Pause resources on hide** - Stop timers/animations when page is hidden
10. **Watch memory** - Configure `LV_MEM_SIZE` appropriately

### Naming Conventions

- Controls: `lv_label_title`, `lv_btn_ok`
- Styles: `style_screen_bg`, `style_btn_pressed`
- Event handlers: `event_handler_btn_ok()`
- Comments: Chinese comments must describe control purpose and event logic

### Page Code Structure

Follow this 9-section structure (see `Contributing.md`):
1. Headers & macros
2. Data structures
3. Global variables & declarations
4. Internal static helpers
5. External interfaces
6. Thread functions
7. Event callbacks (keys, gestures, timers)
8. Init/destroy/resource management
9. Debug & testing

## Configuration

- **SDL backend**: `lv_conf_sdl.h` (default, for x86 dev)
- **Framebuffer backend**: `lv_conf_fb.h` (for embedded)
- Environment variables: `LV_SDL_VIDEO_WIDTH`, `LV_SDL_VIDEO_HEIGHT`

## Icon Resources

- Sources: https://icons.getbootstrap.com/, https://www.flaticon.com/
- Size: 45x45 PNG format
- Location: `res/` directory
- Batch resize: `mogrify -resize 45x45 *.png`
