#!/usr/bin/env bash
# Grid OS __GRIDOS_VERSION__ — Linux launcher (QEMU required).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"
if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
  echo "Install QEMU: sudo apt install qemu-system-x86" >&2
  exit 1
fi
make -C "$ROOT" run-headless
