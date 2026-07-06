#!/usr/bin/env bash
# Create an Android Termux Grid OS bundle (kernel + seeded disk + source).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION="${GRID_OS_VERSION:-$(git describe --tags --always 2>/dev/null || echo "dev")}"
OUT_DIR="${OUT_DIR:-$ROOT/dist}"
ARCHIVE="$OUT_DIR/grid-os-android-termux-${VERSION}.zip"

echo "Building Grid OS for Android Termux bundle..."
make disk seed-disk

mkdir -p "$OUT_DIR"
STAGE="$OUT_DIR/.termux-stage-$$"
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

cp "$ROOT/docs/ANDROID_TERMUX.md" "$STAGE/grid-os/ANDROID_TERMUX.md"
cp "$ROOT/tools/RUN-TERMUX.txt" "$STAGE/grid-os/RUN-TERMUX.txt"

LAUNCHER="$STAGE/grid-os/GridOS-Termux.sh"
sed "s/__GRIDOS_VERSION__/$VERSION/g" "$ROOT/tools/standalone_termux_stub.sh" > "$LAUNCHER"
chmod +x "$LAUNCHER"

rm -f "$ARCHIVE"
(
  cd "$STAGE"
  zip -qr "$ARCHIVE" grid-os
)
rm -rf "$STAGE"

echo ""
echo "Saved Android Termux bundle:"
echo "  $ARCHIVE"
ls -lh "$ARCHIVE"
