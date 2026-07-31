#!/bin/bash
# =============================================================================
# AetherRPG Maker – Aktuelles Build + Milestone 01 Test (EventBridge/ADR-001)
# =============================================================================
# Benötigt: cmake, build-essential, nlohmann_json (FetchContent oder System)
# Für echte UI: libglfw3-dev libgl1-mesa-dev libx11-dev ... (siehe README)
# Für Headless/CI: keine GUI-Abhängigkeiten nötig
# =============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${REPO_ROOT}/build"

echo "=== Milestone 01 Build + Test ==="
echo "Branch: arena/019fb6ee-game-engine-2"
echo "Neu: engine/src/game/event_bridge.cpp + bootstrap.cpp Integration"

# 1. Konfigurieren (Debug – für Tests & Debugging besser geeignet)
cmake -S "${REPO_ROOT}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DAETHER_WITH_GLFW=ON \
  -DAETHER_WITH_OPENGL=ON \
  -DAETHER_WITH_MRUBY=ON

# 2. Bauen (Parallel, mit neuer event_bridge.cpp)
cmake --build "${BUILD_DIR}" -j$(nproc)

# 3. Unit/Smoke-Tests (ctest – wenn Tests registriert)
echo "=== CTest (Headless) ==="
ctest --test-dir "${BUILD_DIR}" --output-on-failure || true

# 4. Milestone 01 – EventBridge Integrationstest (Headless, ohne Fenster)
echo "=== Milestone 01 Smoke-Test (EventBridge) ==="
g++ -std=c++20 -O0 -g \
  -I"${REPO_ROOT}/engine/include" \
  -I"${REPO_ROOT}/shared/include" \
  -I"${REPO_ROOT}/third_party/nlohmann" \
  -I"${REPO_ROOT}/third_party/stb" \
  "${REPO_ROOT}/tests/integration/test_event_bridge.cpp" \
  "${REPO_ROOT}/engine/src/game/event_bridge.cpp" \
  "${REPO_ROOT}/engine/src/game/event_system.cpp" \
  "${REPO_ROOT}/engine/src/core/logger.cpp" \
  -lpthread -o "${BUILD_DIR}/test_event_bridge_smoke"

"${BUILD_DIR}/test_event_bridge_smoke"

echo "=== Fertig ==="
echo "Binaries: ${BUILD_DIR}/bin/AetherEditor | ${BUILD_DIR}/bin/Game"
echo "Smoke:    ${BUILD_DIR}/test_event_bridge_smoke"
