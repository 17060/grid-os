#!/usr/bin/env bash
# Build Android Termux bundles: single .sh launcher + optional zip.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION="${GRID_OS_VERSION:-$(git describe --tags --always 2>/dev/null | sed 's/^v//')}"
OUT_DIR="${OUT_DIR:-$ROOT/dist}"
SAFE_VERSION="$(echo "$VERSION" | tr '/ ' '-')"
OUT_SH="$OUT_DIR/GridOS-${SAFE_VERSION}-Android-Termux.sh"
OUT_ZIP="$OUT_DIR/GridOS-${SAFE_VERSION}-Android-Termux.zip"

echo "Building Android Termux bundles (Grid OS $VERSION)..."
make disk seed-disk >/dev/null

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

cp "$ROOT/build/grid-os.elf" "$ROOT/build/grid.img" "$STAGE/"
PAYLOAD="$STAGE/payload.tar.gz"
tar czf "$PAYLOAD" -C "$STAGE" grid-os.elf grid.img

STUB="$STAGE/stub.sh"
sed "s/__GRIDOS_VERSION__/$VERSION/g" "$ROOT/tools/standalone_termux_stub.sh" > "$STUB"

mkdir -p "$OUT_DIR"
cat "$STUB" "$PAYLOAD" > "$OUT_SH"
chmod +x "$OUT_SH"

ZIP_STAGE="$(mktemp -d)"
cp "$OUT_SH" "$ZIP_STAGE/"
cp "$ROOT/tools/RUN-TERMUX.txt" "$ZIP_STAGE/"
rm -f "$OUT_ZIP"
(
  cd "$ZIP_STAGE"
  zip -qr "$OUT_ZIP" .
)

echo ""
echo "Android Termux standalone script:"
echo "  $OUT_SH"
ls -lh "$OUT_SH"
echo ""
echo "Android Termux zip (script + README):"
echo "  $OUT_ZIP"
ls -lh "$OUT_ZIP"
echo ""
echo "On Android (Termux):"
echo "  pkg install qemu-system-x86"
echo "  chmod +x GridOS-*-Android-Termux.sh && ./GridOS-*-Android-Termux.sh"
