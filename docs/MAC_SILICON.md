# Grid OS on Apple Silicon (M1 / M2 / M3 / M4)

Grid OS is an **x86_64 guest** — it runs inside **QEMU** on your Mac. Apple Silicon hosts use native ARM QEMU to emulate the Grid; you do not need Rosetta for QEMU itself.

## Quick start

```bash
# 1. Install tools (Homebrew)
brew install nasm qemu x86_64-elf-gcc

# 2. Clone and boot
git clone https://github.com/17060/grid-os.git
cd grid-os
make disk seed-disk
make run              # cocoa GUI window
```

## Display modes (macOS cocoa)

| Command | Window |
|---------|--------|
| `make run` | Default, zoom-to-fit |
| `make run-hd` | 1920×1080 HDMI (scaled VGA text) |
| `make run-4k` | 3840×2160 HDMI (scaled VGA text) |
| `make run-headless` | Serial shell only (no window) |

**4K / HD resize:** launchers call AppleScript to resize the QEMU window. Grant **Accessibility** to Terminal or Cursor in **System Settings → Privacy & Security → Accessibility**, or drag-resize manually (`zoom-to-fit` scales the console).

## Release bundles (v6.5+)

Build both distributable formats in one step:

```bash
make release-mac
# → dist/grid-os-macos-arm64-v6.5.tar.gz
# → dist/GridOS-6.5-macOS-AppleSilicon.command
```

Upload to GitHub:

```bash
gh release upload v6.5 dist/*
```

### Standalone single file

One double-clickable file — no git clone, no `make`. Only **QEMU** required:

```bash
make standalone-macos
# or: GRID_OS_VERSION=6.5 make standalone-macos
# → dist/GridOS-6.5-macOS-AppleSilicon.command
```

Copy that **one file** to your Mac, then:

```bash
brew install qemu
chmod +x GridOS-6.5-macOS-AppleSilicon.command
./GridOS-6.5-macOS-AppleSilicon.command
# or double-click in Finder
```

Options:

```bash
GRIDOS_MODE=hd  ./GridOS-*.command    # 1920×1080 window
GRIDOS_MODE=4k  ./GridOS-*.command    # 3840×2160 window
```

Payload extracts once to `~/.grid-os/standalone-<version>/`.

### Full source tarball

Creates a tarball with the built kernel, seeded Flynn disk, and source — ready to archive or share:

```bash
make save-macos-arm64
# or: GRID_OS_VERSION=v6.5 make save-macos-arm64
# → dist/grid-os-macos-arm64-v6.5.tar.gz
```

Or:

```bash
./tools/gridctl save-macos
```

Restore on any Mac with QEMU:

```bash
tar xzf grid-os-macos-arm64-*.tar.gz
cd grid-os
make run
```

## Build from scratch

If `x86_64-elf-gcc` is missing from Homebrew, try:

```bash
brew tap mess937/osdev
brew install x86_64-elf-gcc
```

The Makefile prefers `x86_64-elf-gcc` on macOS and falls back to host `gcc` on Linux CI.

## Host bridges (optional)

Run in separate Terminal tabs while Grid OS is up:

```bash
make ai-bridge    # port 8766 → Ollama / OpenAI-compatible API
make btc-bridge   # port 8767 → Bitcoin Core RPC (use testnet/regtest)
```

Guest gateway: **10.0.2.2** (DNS alias: `gateway`, `ai`, `btc`)

## Tests

```bash
make test
```

Same suite as GitHub Actions CI (host + QEMU e2e).

See also: [GETTING_STARTED.md](GETTING_STARTED.md) · [NETWORKING.md](NETWORKING.md) · [COMMANDS.md](COMMANDS.md)
