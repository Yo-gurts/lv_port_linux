CONFIG ?= SDL
CMAKE_BUILD_TYPE ?= Release

TOOLCHAIN_FILE =
BUILD_DIR =
ifeq ($(CONFIG), FB)
	TOOLCHAIN_FILE = toolchains/arm-none-linux-musleabihf.cmake
	BUILD_DIR = build/musl_arm32
else
	BUILD_DIR = build/x86_64
endif

.PHONY: all clean help

all:
	cmake -B $(BUILD_DIR) -DCONFIG=$(CONFIG) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN_FILE) -DPROJECT_PATH="$(CURDIR)"
	cmake --build $(BUILD_DIR) -j

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Usage: make [CONFIG=SDL|FB]"
	@echo "Targets:"
	@echo "  all        Build the project"
	@echo "  clean      Clean build files"
	@echo "Examples:"
	@echo "  make                # SDL (x86_64)"
	@echo "  make CONFIG=FB     # Framebuffer (musl_arm32)"
	@echo "  make CONFIG=SDL    # SDL (x86_64)"
