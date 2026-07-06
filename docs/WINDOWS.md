# Grid OS on Windows (x64)

Grid OS is an **x86_64 guest** — it runs inside **QEMU** on your PC. Windows hosts use native QEMU to emulate the Grid; no WSL required for boot.

## Quick start

1. **Install QEMU**

   ```powershell
   winget install SoftwareFreedomConservancy.QEMU
   ```

   Or [Chocolatey](https://community.chocolatey.org/): `choco install qemu`  
   Or [Scoop](https://scoop.sh/): `scoop install qemu`

2. **Download a release bundle** from [GitHub Releases](https://github.com/17060/grid-os/releases):

   | Bundle | Use case |
   |--------|----------|
   | `GridOS-*-Windows-x64.zip` | Unzip and double-click `GridOS.bat` |
   | `grid-os-windows-x64-*.zip` | Full source + prebuilt `build/` |

3. **Unzip** and double-click **`GridOS.bat`**.

4. In the GridBASIC IDE, press **Esc** for the `grid>` shell. Type **`poweroff`** to quit.

## Display modes

Set environment variables before launching (cmd.exe):

```bat
set GRIDOS_MODE=hd
GridOS.bat
```

| Variable | Effect |
|----------|--------|
| `GRIDOS_MODE=hd` | 1920×1080 HDMI window (scaled VGA text) |
| `GRIDOS_MODE=4k` | 3840×2160 HDMI window |
| `GRIDOS_HEADLESS=1` | Serial shell only (no GUI) |
| `QEMU=C:\path\to\qemu-system-x86_64.exe` | Override QEMU location |
| `GRIDOS_CACHE=%LOCALAPPDATA%\GridOS\...` | Override cache directory |

The launcher uses `-display sdl,zoom-to-fit=on` on Windows. Resize the QEMU window manually if needed.

## Build from source (WSL or MSYS2)

Developers can build the kernel on Linux/WSL with the same toolchain as CI:

```bash
sudo apt install nasm gcc qemu-system-x86
make disk seed-disk
make run
```

On Windows without WSL, use **WSL2** or **MSYS2** with `nasm`, `gcc`, and QEMU installed.

Host packaging (creates Windows zip bundles):

```bash
make release-windows
# → dist/GridOS-6.5.1-Windows-x64.zip
# → dist/grid-os-windows-x64-v6.5.1.zip
```

## Release bundles

```bash
make release-windows
```

Produces:

- **`GridOS-<version>-Windows-x64.zip`** — `GridOS.bat`, `grid-os.elf`, `grid.img`, `RUN-WINDOWS.txt`
- **`grid-os-windows-x64-<version>.zip`** — source tree + prebuilt artifacts + `WINDOWS.md`

Upload to GitHub:

```bash
gh release upload v6.5.1 dist/GridOS-*-Windows-x64.zip dist/grid-os-windows-x64-*.zip
```

## Host bridges (AI / BTC / HTTPS)

Grid OS talks to host services at **`10.0.2.2`** (QEMU user networking). Run bridges in a separate terminal with **Python 3**:

```powershell
python tools\gridai_bridge.py
python tools\gridbtc_bridge.py
python tools\gridhttps_bridge.py
```

Then inside Grid OS:

```text
grid> ai ask hello
grid> btc status
grid> http get gateway /
```

## Networking

Same as Linux/macOS — see [NETWORKING.md](NETWORKING.md):

```text
grid> net ping gateway
grid> http get gateway /
grid> irc connect gateway 6667 mynick
```

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `QEMU not found` | Install QEMU and ensure `qemu-system-x86_64.exe` is on PATH |
| Black window / no SDL | Install QEMU with SDL support; try `set GRIDOS_HEADLESS=1` |
| SmartScreen blocks `.bat` | Click "More info" → "Run anyway", or unblock in file Properties |
| Antivirus quarantine | Add an exception for the unzipped folder |

## See also

- [GETTING_STARTED.md](GETTING_STARTED.md) — first boot walkthrough
- [MAC_SILICON.md](MAC_SILICON.md) — Apple Silicon bundles
- [COMMANDS.md](COMMANDS.md) — shell reference
