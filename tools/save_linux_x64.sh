#!/usr/bin/env bash
# Create a Linux x64 Grid OS bundle (kernel + seeded disk + source).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION="${GRID_OS_VERSION:-$(git describe --tags --always 2>/dev/null || echo "dev")}"
OUT_DIR="${OUT_DIR:-$ROOT/dist}"
ARCHIVE="$OUT_DIR/grid-os-linux-x64-${VERSION}.tar.gz"

echo "Building Grid OS for Linux x64 bundle..."
make disk seed-disk

mkdir -p "$OUT_DIR"
STAGE="$OUT_DIR/.linux-stage-$$"
rm -rf "$STAGE"
mkdir -p "$STAGE/grid-os"

copy_tree() {
  local item="$1"
  if [[ -e "$ROOT/$item" ]]; then
    cp -R "$ROOT/$item" "$STAGE/grid-os/"
  fi
}

for item in boot kernel user tools docs Makefile linker.ld README.md CHANGELOG.md .gitignore; do
  copy_tree "$item"
done

mkdir -p "$STAGE/grid-os/build"
cp "$ROOT/build/grid-os.bin" "$ROOT/build/grid-os.elf" "$ROOT/build/grid.img" "$STAGE/grid-os/build/"

cp "$ROOT/docs/LINUX.md" "$STAGE/grid-os/LINUX.md"

LAUNCHER="$STAGE/grid-os/GridOS-Linux.sh"
sed "s/__GRIDOS_VERSION__/$VERSION/g" "$ROOT/tools/standalone_linux_stub.sh" > "$LAUNCHER"
chmod +x "$LAUNCHER"

rm -f "$ARCHIVE"
tar -czf "$ARCHIVE" -C "$STAGE" grid-os
rm -rf "$STAGE"

echo ""
echo "Saved Linux x64 bundle:"
echo "  $ARCHIVE"
ls -lh "$ARCHIVE"
