# AGENTS.md

## Cursor Cloud specific instructions

This monorepo holds **two independent products** that share *Tron* branding:

1. **Grid OS** (repo root) — a from-scratch x86_64 OS (C + NASM), built with the
   top-level `Makefile` and run in QEMU. See `README.md`.
2. **GridBasic** (`gridbasic/`) — a pure-Python 3 BASIC language + web IDE that
   uses only the standard library (no `pip install`). See `gridbasic/README.md`.

### System dependencies
The update script installs `nasm` and `qemu-system-x86` (the only missing
system packages; `gcc`, `ld`, `objcopy`, `make`, `python3`, `dd`, `zip` ship
with the base image). The `Makefile` auto-detects the absence of an
`x86_64-elf-gcc` cross toolchain and falls back to the host `gcc` with
`HOST_GCC=1`/`-no-pie`, so no cross toolchain is needed here.

### Grid OS — build / test / run
Standard commands live in the top-level `Makefile` and mirror
`.github/workflows/test.yml`:
- Build: `make` (default target), then `make disk seed-disk` to create the
  128MB GFS disk `build/grid.img`, and `make sync-basic-wiki`.
- Tests: `make test-host`, `make test-qemu-smoke`, `make test-e2e`
  (or `make test` for all three).
- Run: `make run-headless` (serial-only, best for cloud/headless) or
  `make run` (VGA text window).

Non-obvious caveats:
- **There is no KVM in the cloud VM** (`/dev/kvm` is absent), so QEMU runs under
  pure TCG emulation. `make test-e2e` still passes but takes ~60s; if you tighten
  its timeout, use the `QEMU_TEST_TIMEOUT` env var (default 240s) rather than
  editing the Makefile.
- `make test-qemu-smoke` intentionally kills QEMU after ~8s
  (`terminating on signal 15`) — that is success, not a failure.
- There is **no linter** configured for either product.

### GridBasic — test / run
- Run from the `gridbasic/` directory (the package is `gridbasic/gridbasic`, so
  `python3 -m gridbasic ...` only resolves when the CWD is `gridbasic/`, not the
  repo root and not the nested package dir).
- Tests: `python3 tests/test_gridbasic.py` (44 tests, 0 deps).
- Run a program: `python3 -m gridbasic examples/tour.gb`; REPL: `python3 -m gridbasic`.
- Web IDE: `python3 -m gridbasic --ide` serves at http://localhost:8765.
- Optional: `GRIDBASIC_AI_KEY` enables the `AI.api` LLM bridge; without it the
  built-in offline AI models still work.

### Optional Grid OS host bridges
`make ai-bridge` (8766), `make btc-bridge` (8767), `make https-bridge` (8768),
`make ws-bridge` (8769) forward guest `GRID.*` calls to host services. All are
optional and only needed for live AI/Bitcoin/HTTPS/WebSocket demos.
