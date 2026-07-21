# --- CUSTOMIZE YOUR GAME NAMES HERE ---
NAME ?= mini_game
TITLE ?= Mini Game

# --- DEFAULT BUILD SETTING (Debug or Release) ---
BUILD ?= Debug
BUILD_DIR = build/$(BUILD)

# --- Asset Packing Configurations ---
ASSETS_DIR         := assets
ASSETS_SCRIPT_SRC  := toolchain/src/pack_assets.cr
ASSETS_SCRIPTS     := $(ASSETS_SCRIPT_SRC) $(shell find toolchain/src/export -type f 2>/dev/null)

# Granular source lists
FONT_SRCS  := $(shell find $(ASSETS_DIR)/fonts -type f 2>/dev/null)
IMAGE_SRCS := $(shell find $(ASSETS_DIR)/images -type f 2>/dev/null)
MUSIC_SRCS := $(shell find $(ASSETS_DIR)/music -type f 2>/dev/null)

# Granular stamps
STAMP_DIR  := build
FONT_STAMP  := $(STAMP_DIR)/.fonts.stamp
IMAGE_STAMP := $(STAMP_DIR)/.images.stamp
MUSIC_STAMP := $(STAMP_DIR)/.music.stamp

.PHONY: all \
	assets fonts images music \
	build run clean clean-assets \
  build-release run-release clean-release release \
  config reconfig reset

# DEFAULT WORKFLOW
all: build run

# Master assets target updates everything conditionally
assets: $(FONT_STAMP) $(IMAGE_STAMP) $(MUSIC_STAMP)

# 1. Fonts pipeline (only runs if font files or exporter scripts change)
$(FONT_STAMP): $(FONT_SRCS) $(ASSETS_SCRIPTS)
	@mkdir -p $(STAMP_DIR)
	@if [ -n "$(FONT_SRCS)" ]; then \
		echo "--- Font updates detected! Re-packing fonts ---"; \
		crystal run $(ASSETS_SCRIPT_SRC) -- --only=fonts; \
	fi
	@touch $(FONT_STAMP)

# 2. Images pipeline (only runs if image files or exporter scripts change)
$(IMAGE_STAMP): $(IMAGE_SRCS) $(ASSETS_SCRIPTS)
	@mkdir -p $(STAMP_DIR)
	@if [ -n "$(IMAGE_SRCS)" ]; then \
		echo "--- Image updates detected! Re-packing images ---"; \
		crystal run $(ASSETS_SCRIPT_SRC) -- --only=images; \
	fi
	@touch $(IMAGE_STAMP)

# 3. Music pipeline (only runs if music files or exporter scripts change)
$(MUSIC_STAMP): $(MUSIC_SRCS) $(ASSETS_SCRIPTS)
	@mkdir -p $(STAMP_DIR)
	@if [ -n "$(MUSIC_SRCS)" ]; then \
		echo "--- Music updates detected! Re-packing music ---"; \
		crystal run $(ASSETS_SCRIPT_SRC) -- --only=music; \
	fi
	@touch $(MUSIC_STAMP)

# Individual manual shortcuts if you ever want them explicitly
fonts: $(FONT_STAMP)
images: $(IMAGE_STAMP)
music: $(MUSIC_STAMP)

build: assets
	@echo "--- Compiling [$(NAME) | Mode: $(BUILD)] ---"
	@if [ ! -d "$(BUILD_DIR)" ]; then \
		cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD) -DGAME_BIN=$(NAME) -DGAME_TITLE="$(TITLE)"; \
	fi
	@cmake --build $(BUILD_DIR) -j

run:
	@echo "--- Running [$(NAME) | Mode: $(BUILD)] ---"
	@./$(BUILD_DIR)/$(NAME)

clean:
	@echo "--- Cleaning [$(BUILD)] ---"
	@rm -rf $(BUILD_DIR)

clean-assets:
	@echo "--- Cleaning Asset Stamps ---"
	@rm -f $(FONT_STAMP) $(IMAGE_STAMP) $(MUSIC_STAMP)

# --- RELEASE SHORTCUTS ---
build-release:
	@$(MAKE) build BUILD=Release

run-release:
	@$(MAKE) run BUILD=Release

clean-release:
	@$(MAKE) clean BUILD=Release

release:
	@$(MAKE) BUILD=Release build-release run-release

# --- CMAKE CONFIG SHORTCUTS ---
config:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD) -DGAME_BIN=$(NAME) -DGAME_TITLE="$(TITLE)"

reconfig:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD) -DGAME_BIN=$(NAME) -DGAME_TITLE="$(TITLE)" --fresh

reset: clean reconfig
