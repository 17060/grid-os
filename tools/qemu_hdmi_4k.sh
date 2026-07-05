#!/usr/bin/env bash
# Launch Grid OS in HDMI 4K mode (3840x2160 QEMU window, scaled VGA text).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

DISK_IMAGE="${DISK_IMAGE:-build/grid.img}"
TARGET="${TARGET:-build/grid-os.bin}"

QEMU="${QEMU:-qemu-system-x86_64}"
QEMU_MACHINE="${QEMU_MACHINE:-q35,acpi=off}"
QEMU_CPU="${QEMU_CPU:-qemu64}"
QEMU_RAM="${QEMU_RAM:-128M}"

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
    -device VGA,xres=3840,yres=2160,edid=on \
    -display cocoa,zoom-to-fit=on \
    -name "Grid OS — HDMI 4K (3840x2160)" \
    -no-reboot \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 &
QPID=$!

# Best-effort: resize the cocoa window to 3840x2160 (needs Accessibility for osascript).
sleep 1.5
osascript <<'APPLESCRIPT' 2>/dev/null || true
tell application "System Events"
    repeat with p in (every process whose name contains "qemu")
        repeat with w in (every window of p)
            try
                set position of w to {50, 50}
                set size of w to {3840, 2160}
            end try
        end repeat
    end repeat
end tell
APPLESCRIPT

wait "$QPID"
