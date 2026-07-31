#!/usr/bin/env bash
set -euo pipefail

echo "=== AetherRPG Maker – Release Build ==="
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$ROOT/build-release"
OUT_DIR="$ROOT/dist/AetherRPG-Maker-Release"

echo "Cleaning previous release build..."
rm -rf "$BUILD_DIR" "$OUT_DIR"

mkdir -p "$BUILD_DIR" "$OUT_DIR"

echo "Configuring Release build (64-bit, optimized)..."
cmake -S "$ROOT" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DAETHER_BUILD_EDITOR=ON \
    -DAETHER_BUILD_RUNTIME=ON \
    -DAETHER_BUILD_TESTS=OFF \
    -DAETHER_WARNINGS_AS_ERRORS=OFF \
    -DAETHER_WITH_GLFW=ON \
    -DAETHER_WITH_OPENGL=ON \
    -DAETHER_WITH_MINIAUDIO=ON \
    -DAETHER_WITH_STB=ON \
    -DAETHER_WITH_MRUBY=OFF \
    -G "Unix Makefiles" || cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release

echo "Building..."
cmake --build "$BUILD_DIR" --config Release -j"$(nproc || echo 4)"

echo "Collecting release package..."

# Binaries
mkdir -p "$OUT_DIR/bin"
cp "$BUILD_DIR/bin/AetherEditor" "$OUT_DIR/bin/" 2>/dev/null || true
cp "$BUILD_DIR/bin/Game" "$OUT_DIR/bin/" 2>/dev/null || true

# On Windows the names would be .exe
for f in AetherEditor Game; do
    if [ -f "$BUILD_DIR/bin/${f}.exe" ]; then
        cp "$BUILD_DIR/bin/${f}.exe" "$OUT_DIR/bin/"
    fi
done

# Assets & Templates
cp -r "$ROOT/assets" "$OUT_DIR/" 2>/dev/null || mkdir -p "$OUT_DIR/assets"
cp -r "$ROOT/templates" "$OUT_DIR/"
cp -r "$ROOT/plugins" "$OUT_DIR/"
cp -r "$ROOT/samples" "$OUT_DIR/" 2>/dev/null || true

# Documentation
cp "$ROOT/README.md" "$OUT_DIR/"
cp -r "$ROOT/docs/user" "$OUT_DIR/docs/" 2>/dev/null || mkdir -p "$OUT_DIR/docs"

# License
cp "$ROOT/LICENSE" "$OUT_DIR/" 2>/dev/null || true

echo "Creating portable package structure..."

# Make a ready-to-use "New Game" starter
mkdir -p "$OUT_DIR/NewGameTemplate"
cp -r "$ROOT/templates/empty_project/"* "$OUT_DIR/NewGameTemplate/" 2>/dev/null || true

echo ""
echo "=== RELEASE BUILD COMPLETE ==="
echo "Release package: $OUT_DIR"
echo ""
echo "To create a game:"
echo "  1. cd $OUT_DIR/bin"
echo "  2. ./AetherEditor --gui --new /path/to/MyFirstRPG"
echo "  3. Open the project, use Karte tab, drag objects"
echo "  4. Testspiel (F5)"
echo "  5. Export"
echo ""
echo "Run the exported Game directly."
