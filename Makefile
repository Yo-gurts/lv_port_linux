CONFIG ?= SDL
BUILD_DIR ?= build/x86_64
CMAKE_BUILD_TYPE ?= Release
PLATFORM ?= x86_64

TOOLCHAIN_FILE =
ifeq ($(PLATFORM), musl_arm32)
	TOOLCHAIN_FILE = toolchains/arm-none-linux-musleabihf.cmake
	BUILD_DIR = build/musl_arm32
else ifeq ($(PLATFORM), glibc_arm32)
	TOOLCHAIN_FILE = toolchains/arm-linux-gnueabihf.cmake
	BUILD_DIR = build/glibc_arm32
else ifeq ($(PLATFORM), x86_64)
	TOOLCHAIN_FILE = toolchains/x86_64-linux-gnu.cmake
	BUILD_DIR = build/x86_64
endif

.PHONY: all clean help

all:
	cmake -B $(BUILD_DIR) -DCONFIG=$(CONFIG) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN_FILE) -DPROJECT_PATH="$(CURDIR)"
	cmake --build $(BUILD_DIR) -j

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Usage: make [target] [CONFIG=SDL|FB] [PLATFORM=x86_64|musl_arm32|glibc_arm32]"
	@echo "Targets:"
	@echo "  all        Build the project (default, CONFIG=SDL or CONFIG=FB, PLATFORM=x86_64)"
	@echo "  clean      Remove build directory"
	@echo "  help       Show this help message"
	@echo "Examples:"
	@echo "  make                # Build with SDL for x86_64 (default)"
	@echo "  make CONFIG=FB      # Build with framebuffer/evdev support for x86_64"
	@echo "  make PLATFORM=musl_arm32 # Build for musl_arm32"
	@echo "  make clean          # Clean build files"
