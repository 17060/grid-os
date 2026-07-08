# Grid OS 4K Display

Grid OS can drive a **native 3840×2160×32** linear framebuffer via **Bochs VBE** when running under QEMU with the HDMI 4K profile. The **GridBASIC IDE** renders into this framebuffer with scaled 8×16 fonts (80×25 logical cells).

## Launch 4K + IDE

```bash
make disk seed-disk
make run-4k
```

Boot sequence:

1. Grid OS detects Bochs VBE and switches to **3840×2160**
2. GridBASIC IDE opens with **`/programs/4k-ide-demo.bas`** loaded
3. Press **Esc `:run`** to execute the demo, or edit and explore

## Display modes

| Command | Resolution | Notes |
|---------|------------|-------|
| `make run` | Window default | VGA text 80×25, host scales window |
| `make run-hd` | 1920×1080 window | EDID HD, scaled VGA |
| `make run-4k` | **3840×2160 window** | **Native framebuffer + IDE** |
| `make run-headless` | Serial only | No display; VBE skipped if unavailable |

## How it works

| Layer | Behavior |
|-------|----------|
| **Guest** | `kernel/vbe.c` programs Bochs DISPI, maps LFB at `0xE0000000` |
| **Console / IDE** | `console.c` draws 80×25 cells at 48×86 pixels each |
| **Host (QEMU)** | `-device VGA,xres=3840,yres=2160,edid=on` + 32 MB VRAM |
| **Fallback** | If VBE is absent, console uses classic VGA text at `0xB8000` |

## Demo program

`/programs/4k-ide-demo.bas` — prints 4K status, identity, capabilities, and sample `GRID.PLOT` lines.

```text
grid> basic run /programs/4k-ide-demo.bas
```

## Requirements

- `qemu-system-x86_64` with standard VGA/Bochs VBE
- **128 MB** guest RAM (default)
- ~**32 MB** framebuffer mapped at `0xE0000000`

On macOS, `tools/qemu_hdmi_4k.sh` attempts to resize the QEMU cocoa window to full 4K (may need Accessibility permission for `osascript`).

## Related

- [GETTING_STARTED.md](GETTING_STARTED.md) — first boot
- [docs/wiki/encyclopedia/grid-statements.md](wiki/encyclopedia/grid-statements.md) — `GRID.PLOT`, `GRID.LINE`, `GRID.CIRCLE`
