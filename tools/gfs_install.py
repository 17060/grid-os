#!/usr/bin/env python3
"""Install a ring-3 ELF onto the Flynn arcade disk without re-seeding everything."""

import sys
from pathlib import Path

from gfs_common import GFS_FILE_CAP, install_file


def main() -> int:
    if len(sys.argv) != 3:
        print("Usage: gfs_install.py <vfs-path> <host-elf>", file=sys.stderr)
        print("Example: gfs_install.py /programs/gridsh build/gridsh.elf", file=sys.stderr)
        return 1

    vfs_path = sys.argv[1]
    host_path = Path(sys.argv[2])
    root = Path(__file__).resolve().parent.parent
    disk_path = root / "build" / "grid.img"

    if not disk_path.exists():
        print("Missing build/grid.img — run: make disk && make seed-disk", file=sys.stderr)
        return 1
    if not host_path.is_file():
        print(f"Missing host file: {host_path}", file=sys.stderr)
        return 1

    payload = host_path.read_bytes()
    if len(payload) > GFS_FILE_CAP:
        print(f"File too large ({len(payload)} B, max {GFS_FILE_CAP} B)", file=sys.stderr)
        return 1

    install_file(disk_path, vfs_path, payload)
    print(f"Installed {host_path} -> {vfs_path} ({len(payload)} B) on {disk_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
