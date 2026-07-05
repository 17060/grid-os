#!/bin/bash
# Grid OS — standalone launcher for macOS Apple Silicon (single-file).
# Double-click this .command file, or: ./GridOS-macOS-AppleSilicon.command
#
# Requires: brew install qemu
# Optional: GRIDOS_MODE=hd|4k|default  GRIDOS_HEADLESS=1
set -euo pipefail

VERSION="__GRIDOS_VERSION__"
QEMU="${QEMU:-qemu-system-x86_64}"

find_qemu() {
  if command -v "$QEMU" >/dev/null 2>&1; then
    return 0
  fi
  for candidate in \
    /opt/homebrew/bin/qemu-system-x86_64 \
    /usr/local/bin/qemu-system-x86_64; do
    if [[ -x "$candidate" ]]; then
      QEMU="$candidate"
      return 0
    fi
  done
  return 1
}

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "This standalone file is for macOS. On Linux use: make run"
  exit 1
fi

if ! find_qemu; then
  echo "QEMU not found. Install with:  brew install qemu"
  osascript -e 'display alert "Grid OS needs QEMU" message "Run in Terminal:\nbrew install qemu"' 2>/dev/null || true
  read -r -p "Press Enter to close..."
  exit 1
fi

SCRIPT="$0"
while [[ -L "$SCRIPT" ]]; do
  SCRIPT="$(readlink "$SCRIPT")"
done

CACHE="${GRIDOS_CACHE:-$HOME/.grid-os/standalone-$VERSION}"
mkdir -p "$CACHE"

MARKER_LINE="$(grep -an '^__GRIDOS_PAYLOAD__$' "$SCRIPT" | head -1 | cut -d: -f1)"
if [[ -z "$MARKER_LINE" ]]; then
  echo "Corrupt standalone file (missing payload marker)."
  read -r -p "Press Enter to close..."
  exit 1
fi

PAYLOAD="$CACHE/.payload.stamp"
STAMP="$(tail -n +"$((MARKER_LINE + 1))" "$SCRIPT" | shasum -a 256 | awk '{print $1}')"
if [[ ! -f "$CACHE/grid-os.elf" || ! -f "$CACHE/grid.img" || "$(cat "$PAYLOAD" 2>/dev/null || true)" != "$STAMP" ]]; then
  echo "Extracting Grid OS $VERSION..."
  tail -n +"$((MARKER_LINE + 1))" "$SCRIPT" | tar xzf - -C "$CACHE"
  echo "$STAMP" > "$PAYLOAD"
fi

KERNEL="$CACHE/grid-os.elf"
DISK="$CACHE/grid.img"
MODE="${GRIDOS_MODE:-default}"
DISPLAY_ARGS=(-display cocoa,zoom-to-fit=on)
VGA_ARGS=()
NAME="Grid OS $VERSION"

case "$MODE" in
  hd|HD)
    VGA_ARGS=(-device VGA,xres=1920,yres=1080,edid=on)
    NAME="Grid OS — HDMI HD (1920x1080)"
    ;;
  4k|4K)
    VGA_ARGS=(-device VGA,xres=3840,yres=2160,edid=on)
    NAME="Grid OS — HDMI 4K (3840x2160)"
    ;;
esac

echo "Starting Grid OS $VERSION (Flynn's Grid)..."
echo "  Esc → grid> shell in GridBASIC IDE"
echo "  grid> poweroff to quit"
echo ""

QEMU_ARGS=(
  -machine q35,acpi=off
  -cpu qemu64
  -m 128M
  -kernel "$KERNEL"
  -drive "if=none,id=grid0,file=$DISK,format=raw"
  -device virtio-blk-pci,drive=grid0
  -netdev user,id=net0
  -device virtio-net-pci,netdev=net0
  -serial stdio
  -no-reboot
  -device isa-debug-exit,iobase=0xf4,iosize=0x04
  -name "$NAME"
)

if [[ "${GRIDOS_HEADLESS:-}" == "1" ]]; then
  QEMU_ARGS+=(-display none)
else
  QEMU_ARGS+=("${DISPLAY_ARGS[@]}")
fi
QEMU_ARGS+=("${VGA_ARGS[@]}")

exec "$QEMU" "${QEMU_ARGS[@]}"

__GRIDOS_PAYLOAD__
