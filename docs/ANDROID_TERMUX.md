# Grid OS on Android (Termux)

Run **Grid OS 6.7** on Android phones and tablets via [Termux](https://termux.dev/) and QEMU. Grid OS is an x86_64 guest — your ARM device emulates it (expect slower boot than on a PC).

## Requirements

| Requirement | Notes |
|-------------|--------|
| **Termux** | Install from [F-Droid](https://f-droid.org/packages/com.termux/) (recommended) |
| **QEMU** | `pkg install qemu-system-x86` |
| **Storage** | ~50 MB free for cache + bundles |
| **RAM** | 128 MB guest + Termux overhead |

Optional: **Termux:X11** if you want a graphical window instead of the serial console.

## Quick start (standalone launcher)

1. Download from [GitHub Releases](https://github.com/17060/grid-os/releases):
   - `GridOS-*-Android-Termux.sh` — single-file launcher, or
   - `GridOS-*-Android-Termux.zip` — script + quick-start text

2. In Termux:

```bash
pkg update && pkg install qemu-system-x86
cp ~/storage/downloads/GridOS-*-Android-Termux.sh ~/
chmod +x ~/GridOS-*-Android-Termux.sh
~/GridOS-*-Android-Termux.sh
```

3. Use the serial console:
   - `Esc` → `grid>` command line / GridBASIC IDE
   - `basic ide` — GridBASIC editor
   - `basic run /programs/hello.bas`
   - `grid> poweroff` — quit

## Headless vs display

**Default:** headless serial console (`-display none`). Best experience on Termux.

**Graphical window** (advanced):

```bash
# Install Termux:X11 from F-Droid, start the X server app, then:
export GRIDOS_HEADLESS=0
./GridOS-*-Android-Termux.sh
```

HDMI-style resolutions (when display is enabled):

```bash
GRIDOS_MODE=hd ./GridOS-*-Android-Termux.sh    # 1920x1080
GRIDOS_MODE=4k ./GridOS-*-Android-Termux.sh    # 3840x2160
```

## Build from source on-device

Download `grid-os-android-termux-*.zip` (full source bundle) or clone the repo on a PC and transfer.

```bash
pkg install qemu-system-x86 nasm gcc python
cd grid-os
make
make disk seed-disk
make run-headless
```

## Host bridges (AI / BTC / HTTPS)

Bridges run on a **PC** on your network. The guest reaches the host at QEMU gateway `10.0.2.2`.

On your PC (same Wi‑Fi as the phone):

```bash
make ai-bridge      # TCP 8766
make btc-bridge     # TCP 8767
make https-bridge   # TCP 8768
```

In Grid OS: `ai ask hello`, `btc status`, `http get gateway 8768 /`.

## Environment variables

| Variable | Default | Description |
|----------|---------|-------------|
| `GRIDOS_HEADLESS` | `1` | `0` = try SDL display (Termux:X11) |
| `GRIDOS_MODE` | `default` | `hd` or `4k` when display enabled |
| `GRIDOS_CACHE` | `~/.grid-os/standalone-<version>/` | Kernel + disk cache |
| `QEMU` | auto | Path to `qemu-system-x86_64` |

## Build release bundles (on Linux/macOS)

```bash
make release-termux
# → dist/GridOS-*-Android-Termux.sh
# → dist/GridOS-*-Android-Termux.zip
# → dist/grid-os-android-termux-*.zip
```

Upload to GitHub releases:

```bash
gh release upload v6.7 dist/GridOS-*-Android-Termux.* dist/grid-os-android-termux-*.zip
```

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `QEMU not found` | `pkg install qemu-system-x86` |
| Very slow boot | Normal on ARM — use headless; close other apps |
| No network in guest | QEMU user-net only; bridges need PC at `10.0.2.2` |
| Permission denied | `chmod +x GridOS-*.sh` |
| Corrupt standalone | Re-download; check file wasn't truncated |

## Limitations

- No KVM on typical phones — pure CPU emulation
- GUI requires Termux:X11 setup
- Not a native Android app — Termux + shell launcher
- Same x86_64 Grid OS as Linux/Windows/macOS (not an ARM port)

See also: [GETTING_STARTED.md](GETTING_STARTED.md), [COMMANDS.md](COMMANDS.md), [WINDOWS.md](WINDOWS.md).
