#!/usr/bin/env bash
# Launch Grid OS in HDMI HD mode (1920x1080 QEMU window, scaled VGA text).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

DISK_IMAGE="${DISK_IMAGE:-build/grid.img}"
TARGET="${TARGET:-build/grid-os.bin}"

QEMU="${QEMU:-qemu-system-x86_64}"
QEMU_MACHINE="${QEMU_MACHINE:-q35,acpi=off}"
QEMU_CPU="${QEMU_CPU:-qemu64}"
QEMU_RAM="${QEMU_RAM:-128M}"

case "$(uname -s 2>/dev/null)" in
Darwin)
  QEMU_DISPLAY="-display cocoa,zoom-to-fit=on"
  ;;
*)
  QEMU_DISPLAY="-display gtk,zoom-to-fit=on"
  ;;
esac

exec "$QEMU" \
    -machine "$QEMU_MACHINE" \
    -cpu "$QEMU_CPU" \
    -m "$QEMU_RAM" \
    -kernel "$TARGET" \
    -drive "if=none,id=grid0,file=$DISK_IMAGE,format=raw" \
    -device virtio-blk-pci,drive=grid0 \
    -netdev user,id=net0 \
    -device virtio-net-pci,netdev=net0 \
    -serial stdio \
    -device VGA,xres=1920,yres=1080,edid=on \
    $QEMU_DISPLAY \
    -name "Grid OS — HDMI HD (1920x1080)" \
    -no-reboot \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 &
QPID=$!

if [ "$(uname -s 2>/dev/null)" = "Darwin" ]; then
  sleep 1.5
  osascript <<'APPLESCRIPT' 2>/dev/null || true
tell application "System Events"
    repeat with p in (every process whose name contains "qemu")
        repeat with w in (every window of p)
            try
                set position of w to {50, 50}
                set size of w to {1920, 1080}
            end try
        end repeat
    end repeat
end tell
APPLESCRIPT
fi

wait "$QPID"
