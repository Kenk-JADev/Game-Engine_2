# =============================================================================
# AetherRPG Maker – Convenience Makefile
# =============================================================================

.PHONY: all release debug clean run-editor run-game new-game help

BUILD_DIR ?= build
RELEASE_DIR ?= build-release
DIST_DIR ?= dist/AetherRPG-Maker-Release

all: debug

# --- Debug build (development) ---
debug:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR) -j$(shell nproc 2>/dev/null || echo 4)

# --- Release build (what the user wants) ---
release:
	@echo "=== Building RELEASE ==="
	cmake -S . -B $(RELEASE_DIR) -DCMAKE_BUILD_TYPE=Release \
		-DAETHER_BUILD_EDITOR=ON \
		-DAETHER_BUILD_RUNTIME=ON \
		-DAETHER_BUILD_TESTS=OFF \
		-DAETHER_WITH_GLFW=ON \
		-DAETHER_WITH_OPENGL=ON \
		-DAETHER_WITH_MINIAUDIO=ON \
		-DAETHER_WITH_STB=ON
	cmake --build $(RELEASE_DIR) -j$(shell nproc 2>/dev/null || echo 4)
	@echo "Release binaries in $(RELEASE_DIR)/bin/"

# --- Create full portable release package ---
package: release
	@chmod +x tools/build_release.sh
	./tools/build_release.sh

# --- Clean everything ---
clean:
	rm -rf $(BUILD_DIR) $(RELEASE_DIR) dist/

# --- Run Editor (after debug build) ---
run-editor:
	./$(BUILD_DIR)/bin/AetherEditor --gui

# --- Run Runtime with demo ---
run-game:
	./$(BUILD_DIR)/bin/Game --project samples/demo_project

# --- Create a new game project quickly ---
new-game:
	@if [ -z "$(PROJECT)" ]; then \
		echo "Usage: make new-game PROJECT=/path/to/MyRPG"; \
		exit 1; \
	fi
	@mkdir -p $(PROJECT)
	@cp -r templates/empty_project/* $(PROJECT)/ 2>/dev/null || true
	@echo "New project created at $(PROJECT)"
	@echo "Now run: ./$(BUILD_DIR)/bin/AetherEditor --gui --project $(PROJECT)"

help:
	@echo "AetherRPG Maker Makefile"
	@echo ""
	@echo "  make release     - Build optimized Release (Editor + Game)"
	@echo "  make package     - Build Release + create portable package in dist/"
	@echo "  make debug       - Development build"
	@echo "  make run-editor  - Start Editor (debug build)"
	@echo "  make run-game    - Start demo in Runtime"
	@echo "  make new-game PROJECT=/path/to/MyGame"
	@echo "  make clean"
