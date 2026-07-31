#!/usr/bin/env bash
set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [ $# -lt 1 ]; then
  echo "Usage: $0 /path/to/MyNewRPG"
  exit 1
fi
TARGET="$1"
mkdir -p "$TARGET"
cp -r "$ROOT/templates/empty_project/"* "$TARGET/"
echo "New AetherRPG project created at: $TARGET"
echo "Now run: cd $ROOT/build/bin && ./AetherEditor --gui --project $TARGET"
