#!/usr/bin/env python3
"""Seed Flynn GridFS (GFS2FLYN) on build/grid.img with ring-3 ELF programs."""

import sys
from pathlib import Path

from gfs_common import (
    GFS_DATA_BASE_LBA,
    GFS_FILE_CAP,
    GFS_INODE_MAX,
    GFS_INODE_TABLE_LBA,
    GFS_INODE_TABLE_SECTORS,
    GFS_SECTORS_PER_FILE,
    GFS_SUPER_LBA,
    SECTOR,
    build_superblock,
    seed_file,
    write_sector,
)

WELCOME_GRID = (
    b"10 REM Flynn's Grid Workshop\n"
    b"20 PRINT \"========================\"\n"
    b"30 PRINT \" GRID WORKSHOP READY.\"\n"
    b"40 PRINT \" Flynn's frontier awaits.\"\n"
    b"50 PRINT \"========================\"\n"
    b"60 CYCLES\n"
    b"70 END\n"
)

HELLO_BAS = (
    b"10 REM GridBASIC demo -- the Grid counts\n"
    b"20 PRINT \"GridBASIC 6.5 online\"\n"
    b"30 FOR I = 1 TO 5\n"
    b"40   PRINT \"grid line \"; I\n"
    b"50 NEXT I\n"
    b"60 S$ = \"hello \" + \"grid\"\n"
    b"70 PRINT S$\n"
    b"80 PRINT \"ping gw: \"; GRID.PING(\"gateway\")\n"
    b"90 PRINT \"ticks: \"; GRID.TIME\n"
    b"100 END\n"
)

NETDEMO_BAS = (
    b"10 REM Network demo -- DNS names + ping\n"
    b"20 PRINT \"Grid OS network demo\"\n"
    b"30 PRINT \"ping gateway: \"; GRID.PING(\"gateway\")\n"
    b"40 PRINT \"ping grid: \"; GRID.PING(\"grid\")\n"
    b"50 END\n"
)

VAULTDEMO_BAS = (
    b"10 REM Vault demo (run after: vault put motd hello)\n"
    b"20 PRINT \"Vault keys live in Grid memory + disk sync\"\n"
    b"30 PRINT \"Use: vault put key value / vault sync\"\n"
    b"40 END\n"
)

AIDEMO_BAS = (
    b"10 REM AI demo -- full-length PRINT (host: make ai-bridge)\n"
    b"20 PRINT \"Grid AI demo (offline fallback if no bridge)\"\n"
    b"30 GRID.AI.PRINT \"What is PRINT?\", \"EXPLAIN\"\n"
    b"40 END\n"
)

HTTPDEMO_BAS = (
    b"10 REM HTTP demo -- use shell: http get gateway 18080 /\n"
    b"20 PRINT \"HTTP client lives in shell (GET/POST + keep-alive)\"\n"
    b"30 PRINT \"Host HTTPS: make https-bridge on port 8768\"\n"
    b"40 END\n"
)

ETC_HOSTS = (
    b"# Grid OS static hosts (also: built-in gateway/grid/ai/btc + UDP DNS)\n"
    b"10.0.2.2   bridge gateway gw ai btc\n"
    b"10.0.2.15  grid host localhost\n"
    b"192.168.1.50  lab\n"
)


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    disk_path = root / "build" / "grid.img"
    build_dir = root / "build"

    if not disk_path.exists():
        print("Missing build/grid.img — run: make disk", file=sys.stderr)
        return 1

    disk = bytearray(disk_path.read_bytes())
    if len(disk) < (GFS_DATA_BASE_LBA + GFS_INODE_MAX * GFS_SECTORS_PER_FILE) * SECTOR:
        print("Disk too small for GridFS layout", file=sys.stderr)
        return 1

    write_sector(disk, GFS_SUPER_LBA, build_superblock())

    for sector in range(GFS_INODE_TABLE_LBA, GFS_INODE_TABLE_LBA + GFS_INODE_TABLE_SECTORS):
        write_sector(disk, sector, b"\x00" * SECTOR)

    files = [
        (1, "/flynn/motd", b"The Grid is open. Flynn's archive linked.\n"),
        (2, "/programs/gridsh", (build_dir / "gridsh.elf").read_bytes()),
        (3, "/programs/discinfo", (build_dir / "discinfo.elf").read_bytes()),
        (4, "/programs/gridprog", (build_dir / "gridprog.elf").read_bytes()),
        (5, "/programs/lightcycle", (build_dir / "lightcycle.elf").read_bytes()),
        (8, "/programs/gridloop", (build_dir / "gridloop.elf").read_bytes()),
        (6, "/grid/recognizer.log", b"Recognizer patrol: sector clear. End of line.\n"),
        (7, "/source/welcome.grid", WELCOME_GRID),
        (9, "/programs/hello.bas", HELLO_BAS),
        (10, "/programs/netdemo.bas", NETDEMO_BAS),
        (11, "/programs/vaultdemo.bas", VAULTDEMO_BAS),
        (12, "/etc/hosts", ETC_HOSTS),
        (13, "/programs/aidemo.bas", AIDEMO_BAS),
        (14, "/programs/httpdemo.bas", HTTPDEMO_BAS),
    ]

    for slot, path, payload in files:
        if isinstance(payload, Path):
            payload = payload.read_bytes()
        if len(payload) > GFS_FILE_CAP:
            print(f"{path}: too large for GFS2 slot", file=sys.stderr)
            return 1
        seed_file(disk, slot, path, payload)
        print(f"  seeded {path} ({len(payload)} B) -> slot {slot}")

    disk_path.write_bytes(disk)
    print(f"GFS2FLYN written to {disk_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
