#!/usr/bin/env bash
# Create a Mac Silicon–ready Grid OS bundle (kernel + seeded disk + source).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION="${GRID_OS_VERSION:-$(git describe --tags --always 2>/dev/null || echo "dev")}"
OUT_DIR="${OUT_DIR:-$ROOT/dist}"
ARCHIVE="$OUT_DIR/grid-os-macos-arm64-${VERSION}.tar.gz"

echo "Building Grid OS for Mac Silicon bundle..."
make disk seed-disk

mkdir -p "$OUT_DIR"
STAGE="$OUT_DIR/.mac-silicon-stage-$$"
rm -rf "$STAGE"
mkdir -p "$STAGE/grid-os"

copy_tree() {
  local item="$1"
  if [[ -e "$ROOT/$item" ]]; then
    cp -R "$ROOT/$item" "$STAGE/grid-os/"
  fi
}

# Source + docs
for item in boot kernel user tools docs Makefile linker.ld README.md CHANGELOG.md .gitignore; do
  copy_tree "$item"
done

# Pre-built artifacts (portable x86_64 guest — runs under QEMU on ARM Mac)
mkdir -p "$STAGE/grid-os/build"
cp "$ROOT/build/grid-os.bin" "$ROOT/build/grid-os.elf" "$ROOT/build/grid.img" "$STAGE/grid-os/build/"

# Mac-specific guide at bundle root
cp "$ROOT/docs/MAC_SILICON.md" "$STAGE/grid-os/MAC_SILICON.md"

cat > "$STAGE/grid-os/RUN-MAC.txt" <<'EOF'
Grid OS — Apple Silicon quick boot
==================================

  brew install nasm qemu x86_64-elf-gcc
  cd grid-os
  make run          # or: make run-hd | make run-4k

Pre-built kernel and Flynn disk are in build/.
Rebuild anytime: make disk seed-disk

Read MAC_SILICON.md for full details.
EOF

tar czf "$ARCHIVE" -C "$STAGE" grid-os
rm -rf "$STAGE"

echo ""
echo "Saved Mac Silicon bundle:"
echo "  $ARCHIVE"
ls -lh "$ARCHIVE"
