#!/usr/bin/env bash
# Build a single standalone .command file for macOS Apple Silicon.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION="${GRID_OS_VERSION:-$(git describe --tags --always 2>/dev/null | sed 's/^v//')}"
OUT_DIR="${OUT_DIR:-$ROOT/dist}"
SAFE_VERSION="$(echo "$VERSION" | tr '/ ' '-')"
OUT_FILE="$OUT_DIR/GridOS-${SAFE_VERSION}-macOS-AppleSilicon.command"

echo "Building standalone Mac file (Grid OS $VERSION)..."
make disk seed-disk >/dev/null

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

cp "$ROOT/build/grid-os.elf" "$ROOT/build/grid.img" "$STAGE/"
PAYLOAD="$STAGE/payload.tar.gz"
tar czf "$PAYLOAD" -C "$STAGE" grid-os.elf grid.img

STUB="$STAGE/stub.command"
sed "s/__GRIDOS_VERSION__/$VERSION/g" "$ROOT/tools/standalone_mac_stub.sh" > "$STUB"

mkdir -p "$OUT_DIR"
cat "$STUB" "$PAYLOAD" > "$OUT_FILE"
chmod +x "$OUT_FILE"

echo ""
echo "Standalone Mac file:"
echo "  $OUT_FILE"
ls -lh "$OUT_FILE"
echo ""
echo "On your Mac:"
echo "  1. brew install qemu"
echo "  2. Double-click GridOS-*.command  (or run from Terminal)"
echo "  3. GRIDOS_MODE=hd|4k ./GridOS-*.command  for HDMI sizes"
