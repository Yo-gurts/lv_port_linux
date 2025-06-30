# 定义颜色变量
GREEN := \033[1;32m
BLUE := \033[1;34m
YELLOW := \033[1;33m
NC := \033[0m   # No Color

CONFIG ?= SDL
BUILD_DIR ?= build/x86_64
CMAKE_BUILD_TYPE ?= Debug
PLATFORM ?= x86_64

TOOLCHAIN_FILE =
ifeq ($(PLATFORM), musl_arm32)
	TOOLCHAIN_FILE = toolchains/arm-none-linux-musleabihf.cmake
	BUILD_DIR = build/musl_arm32
else ifeq ($(PLATFORM), glibc_arm32)
	TOOLCHAIN_FILE = toolchains/arm-none-linux-gnueabihf.cmake
	BUILD_DIR = build/glibc_arm32
else ifeq ($(PLATFORM), x86_64)
	TOOLCHAIN_FILE = toolchains/x86_64-linux-gnu.cmake
	BUILD_DIR = build/x86_64
endif

.PHONY: all clean help

all: lv_conf_link
	cmake -B $(BUILD_DIR) -DCONFIG=$(CONFIG) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN_FILE)
	cmake --build $(BUILD_DIR) -j

lv_conf_link:
ifeq ($(CONFIG), FB)
	ln -sf config/lv_conf_fb.h lv_conf.h
else
	ln -sf config/lv_conf_sdl.h lv_conf.h
endif

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo
	@echo "$(GREEN)Usage: make [target] [CONFIG=SDL|FB] [PLATFORM=x86_64|musl_arm32|glibc_arm32]$(NC)"
	@echo
	@echo "$(BLUE)Targets:$(NC)"
	@echo "  all        Build the project (default, CONFIG=SDL or CONFIG=FB, PLATFORM=x86_64)"
	@echo "  clean      Remove build directory"
	@echo "  help       Show this help message"
	@echo
	@echo "$(BLUE)Examples:$(NC)"
	@echo "  make                $(YELLOW)# Build with SDL for x86_64 (default)$(NC)"
	@echo "  make CONFIG=FB      $(YELLOW)# Build with framebuffer/evdev support for x86_64$(NC)"
	@echo "  make PLATFORM=musl_arm32 $(YELLOW)# Build for musl_arm32$(NC)"
	@echo "  make clean          $(YELLOW)# Clean build files$(NC)"
	@echo "  make CONFIG=FB PLATFORM=glibc_arm32"
	@echo "  make all CONFIG=FB PLATFORM=glibc_arm32 CMAKE_BUILD_TYPE=Release"
	@echo
