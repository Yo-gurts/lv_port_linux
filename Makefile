CONFIG ?= SDL
BUILD_DIR ?= build
CMAKE_BUILD_TYPE ?= Release

.PHONY: all clean help

all:
	cmake -B $(BUILD_DIR) -DCONFIG=$(CONFIG) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE)
	cmake --build $(BUILD_DIR) -j

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Usage: make [target] [CONFIG=SDL|FB]"
	@echo "Targets:"
	@echo "  all        Build the project (default, CONFIG=SDL or CONFIG=FB)"
	@echo "  clean      Remove build directory"
	@echo "  help       Show this help message"
	@echo "Examples:"
	@echo "  make                # Build with SDL (default)"
	@echo "  make CONFIG=FB      # Build with framebuffer/evdev support"
	@echo "  make clean          # Clean build files"

