# Grid OS on Linux

Run **Grid OS 7.1.1** on Linux with QEMU.

## Quick start

```bash
make disk seed-disk
make run          # GUI (GTK)
make run-headless # serial shell only
```

## Release bundle

```bash
make release-linux
```

Produces `dist/grid-os-linux-x64-*.tar.gz` with prebuilt kernel, Flynn disk, and `GridOS-Linux.sh` launcher.

## Bridges (host)

| Bridge | Port | Command |
|--------|------|---------|
| AI | 8766 | `make ai-bridge` |
| BTC | 8767 | `make btc-bridge` |
| HTTPS | 8768 | `make https-bridge` |
| WebSocket | 8769 | `make ws-bridge` |

## GridLink packages

Host push a `.gridpkg` manifest over serial:

```text
#GRIDLINK/1.0/PKG
/programs/hello.bas 247
...bytes...
#GRIDLINK/END
```

Guest: `portal pkg` or `pkg recv` or `GRID.PORTAL.PKG` in GridBASIC.

See [PACKAGES.md](PACKAGES.md) for manifest format and IDE modules.
