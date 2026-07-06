#!/usr/bin/env python3
"""Patch a Flynn disk image with a legacy vault v5 signature + payload for e2e tests."""

from __future__ import annotations

import struct
import sys
from pathlib import Path

SECTOR = 512
VAULT_SIG_LBA = 31
VAULT_LBA = 32
VAULT_SECTORS_V5 = 2
GRID_VAULT_MAGIC = 0x47524431
GRID_VAULT_VERSION_V5 = 5
# Must match sizeof(grid_vault_t) in kernel/storage.c (1028 bytes).
VAULT_STRUCT_SIZE = 1028


def crc32_update(crc: int, byte: int) -> int:
    crc ^= byte
    for _ in range(8):
        crc = ((crc >> 1) ^ 0xEDB88320) if (crc & 1) else (crc >> 1)
    return crc


def vault_checksum(raw: bytes) -> int:
    crc = 0xFFFFFFFF
    limit = len(raw) - 4
    for i in range(limit):
        crc = crc32_update(crc, raw[i])
    return crc ^ 0xFFFFFFFF


def build_v5_vault() -> bytes:
    raw = bytearray(VAULT_STRUCT_SIZE)
    struct.pack_into("<II", raw, 0, GRID_VAULT_MAGIC, GRID_VAULT_VERSION_V5)
    struct.pack_into("<I", raw, 28, 4242)  # cycles (after 16-byte user_disc)
    checksum = vault_checksum(bytes(raw))
    struct.pack_into("<I", raw, 8, checksum)
    return bytes(raw[: VAULT_SECTORS_V5 * SECTOR])


def write_sector(disk: bytearray, lba: int, data: bytes) -> None:
    off = lba * SECTOR
    disk[off : off + SECTOR] = data[:SECTOR].ljust(SECTOR, b"\x00")


def patch_disk(path: Path) -> None:
    disk = bytearray(path.read_bytes())
    sig = b"FLYNGRID" + bytes([GRID_VAULT_VERSION_V5]) + b"\x00" * (SECTOR - 9)
    write_sector(disk, VAULT_SIG_LBA, sig)
    vault = build_v5_vault()
    for i in range(VAULT_SECTORS_V5):
        write_sector(disk, VAULT_LBA + i, vault[i * SECTOR : (i + 1) * SECTOR])
    path.write_bytes(disk)
    print(f"Patched {path} with vault v5 signature + payload")


def main() -> int:
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} build/grid-test.img", file=sys.stderr)
        return 1
    path = Path(sys.argv[1])
    if not path.exists():
        print(f"Missing {path}", file=sys.stderr)
        return 1
    patch_disk(path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
