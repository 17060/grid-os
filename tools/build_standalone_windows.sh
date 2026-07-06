#!/usr/bin/env bash
# Build a Windows zip bundle: GridOS.bat + kernel + Flynn disk.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION="${GRID_OS_VERSION:-$(git describe --tags --always 2>/dev/null | sed 's/^v//')}"
OUT_DIR="${OUT_DIR:-$ROOT/dist}"
SAFE_VERSION="$(echo "$VERSION" | tr '/ ' '-')"
STAGE="$(mktemp -d)"
ZIP="$OUT_DIR/GridOS-${SAFE_VERSION}-Windows-x64.zip"

echo "Building Windows standalone bundle (Grid OS $VERSION)..."
make disk seed-disk >/dev/null

trap 'rm -rf "$STAGE"' EXIT

cp "$ROOT/build/grid-os.elf" "$ROOT/build/grid.img" "$STAGE/"
cp "$ROOT/tools/RUN-WINDOWS.txt" "$STAGE/"

BAT="$STAGE/GridOS.bat"
sed "s/__GRIDOS_VERSION__/$VERSION/g" "$ROOT/tools/standalone_windows_stub.bat" > "$BAT"

mkdir -p "$OUT_DIR"
rm -f "$ZIP"
(
  cd "$STAGE"
  zip -qr "$ZIP" .
)

echo ""
echo "Windows standalone bundle:"
echo "  $ZIP"
ls -lh "$ZIP"
echo ""
echo "On Windows:"
echo "  1. Install QEMU (winget install SoftwareFreedomConservancy.QEMU)"
echo "  2. Unzip and double-click GridOS.bat"
echo "  3. set GRIDOS_MODE=hd|4k for HDMI window sizes"
