#!/bin/bash
# Compile-Skript für Headless-Smoke-Test (ADR-001 / Milestone 01)
# Benötigt: g++ mit C++20, nlohmann/json (im Repo unter third_party/)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

INCLUDES="-I${REPO_ROOT}/engine/include -I${REPO_ROOT}/shared/include -I${REPO_ROOT}/third_party/nlohmann -I${REPO_ROOT}/third_party/stb"

echo "=== Kompiliere EventBridge-Smoke-Test ==="
g++ -std=c++20 -O0 -g ${INCLUDES} \
    "${SCRIPT_DIR}/test_event_bridge.cpp" \
    "${REPO_ROOT}/engine/src/game/event_bridge.cpp" \
    "${REPO_ROOT}/engine/src/game/event_system.cpp" \
    "${REPO_ROOT}/engine/src/core/logger.cpp" \
    -lpthread -o "${SCRIPT_DIR}/test_event_bridge_smoke"

echo "=== Kompilation erfolgreich ==="
echo "=== Starte Smoke-Test ==="
"${SCRIPT_DIR}/test_event_bridge_smoke"
echo "=== Smoke-Test abgeschlossen ==="
