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

## Save a Mac Silicon bundle

Creates a tarball with the built kernel, seeded Flynn disk, and source — ready to archive or share:

```bash
make save-macos-arm64
# → dist/grid-os-macos-arm64-<version>.tar.gz
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
make btc-bridge     # port 8767 → Bitcoin Core RPC (use testnet/regtest)
```

Guest gateway: **10.0.2.2**

## Tests

```bash
make test
```

Same suite as GitHub Actions CI.

See also: [GETTING_STARTED.md](GETTING_STARTED.md) · [COMMANDS.md](COMMANDS.md)
