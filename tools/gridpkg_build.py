#!/usr/bin/env python3
"""Emit a GridLink PKG frame from a package MANIFEST and local files.

Usage:
  python3 tools/gridpkg_build.py packages/flynn-ide-tools/MANIFEST
  python3 tools/gridpkg_build.py --output dist/flynn-ide-tools.gridpkg MANIFEST
  ./tools/gridctl portal-pkg-push packages/flynn-ide-tools/MANIFEST | ...

Guest: portal pkg  or  pkg recv  or  GRID.PORTAL.PKG / GRID.PKG.RECV
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

GRIDLINK_PKG = b"#GRIDLINK/1.0/PKG\n"
GRIDLINK_END = b"#GRIDLINK/END\n"


def parse_manifest(text: str) -> tuple[str, list[str]]:
    name = ""
    paths: list[str] = []
    seen: set[str] = set()

    def add(path: str) -> None:
        path = path.strip()
        if not path or path in seen:
            return
        seen.add(path)
        paths.append(path)

    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("name="):
            name = line[5:].strip()
        elif line.startswith("file="):
            add(line[5:])
        elif line.startswith("mod="):
            parts = line[4:].split(":", 2)
            if len(parts) >= 2:
                add(parts[1])

    return name, paths


def host_path(manifest: Path, pkg_name: str, vfs_path: str) -> Path:
    prefix = f"/packages/{pkg_name}/"
    if vfs_path.startswith(prefix):
        rel = vfs_path[len(prefix) :]
        return manifest.parent / rel
    if vfs_path.endswith("/MANIFEST") or vfs_path.endswith("MANIFEST"):
        return manifest
    raise ValueError(f"cannot map vfs path outside package: {vfs_path}")


def build_frame(manifest: Path) -> bytes:
    text = manifest.read_text(encoding="utf-8")
    pkg_name, vfs_paths = parse_manifest(text)
    if not pkg_name:
        raise SystemExit(f"{manifest}: missing name= in MANIFEST")

    out = bytearray(GRIDLINK_PKG)
    for vfs_path in vfs_paths:
        host = host_path(manifest, pkg_name, vfs_path)
        if not host.is_file():
            raise SystemExit(f"missing host file for {vfs_path}: {host}")
        data = host.read_bytes()
        out.extend(f"{vfs_path} {len(data)}\n".encode("ascii"))
        out.extend(data)
    out.extend(GRIDLINK_END)
    return bytes(out)


def main() -> int:
    parser = argparse.ArgumentParser(description="Build GridLink PKG frame from MANIFEST")
    parser.add_argument("manifest", type=Path, help="Path to package MANIFEST")
    parser.add_argument(
        "--output",
        "-o",
        type=Path,
        help="Write .gridpkg frame to file instead of stdout",
    )
    args = parser.parse_args()

    manifest = args.manifest.resolve()
    if not manifest.is_file():
        print(f"Missing manifest: {manifest}", file=sys.stderr)
        return 1

    frame = build_frame(manifest)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(frame)
        print(f"Wrote {args.output} ({len(frame)} B)", file=sys.stderr)
        return 0

    sys.stdout.buffer.write(frame)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
