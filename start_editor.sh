#!/usr/bin/env bash
# AetherRPG Maker - Linux Quick Launcher
# For the easiest experience download the ready package from Releases.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Try common locations after building from source
for candidate in \
    "$SCRIPT_DIR/build-release/bin/AetherEditor" \
    "$SCRIPT_DIR/build/bin/AetherEditor" \
    "$SCRIPT_DIR/bin/AetherEditor"
do
    if [ -x "$candidate" ]; then
        cd "$(dirname "$candidate")"
        exec ./AetherEditor --gui "$@"
    fi
done

echo ""
echo "AetherRPG Maker Editor not found."
echo ""
echo "Recommended (no terminal commands needed):"
echo "  1. Go to https://github.com/Kenk-JADev/Game-Engine_2/releases"
echo "  2. Download the latest AetherRPG-Maker-Linux-x64.tar.gz"
echo "  3. Extract and run ./start_editor.sh"
echo ""
exit 1
