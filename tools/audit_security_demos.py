#!/usr/bin/env python3
"""Audit security lab GridBASIC demos: counts, paths, duplicates, inode budget."""

from __future__ import annotations

import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PROGRAMS = ROOT / "programs"

LABS: dict[str, tuple[str, int]] = {
    "redteam": ("rt", 100),
    "blackhat": ("bh", 100),
    "whiteteam": ("wt", 100),
    "blueteam": ("bt", 100),
    "purpleteam": ("pt", 25),
    "greenteam": ("gt", 75),
    "yellowteam": ("yt", 50),
    "orangeteam": ("ot", 50),
    "greyteam": ("gy", 100),
}

GFS_READ_RE = re.compile(r'GRID\.GFS\.READ\$\("([^"]+)"\)')
GFS_WRITE_RE = re.compile(r'GRID\.GFS\.WRITE\s+"([^"]+)"')

# Paths seeded by gfs_seed.py (static + packages + all lab .bas files).
STATIC_SEED = {
    "/flynn/motd",
    "/etc/hosts",
    "/grid/recognizer.log",
    "/source/welcome.grid",
    "/programs/hello.bas",
    "/programs/netdemo.bas",
    "/programs/vaultdemo.bas",
    "/programs/aidemo.bas",
    "/programs/httpdemo.bas",
    "/programs/advancedemo.bas",
    "/programs/autoexec.bas",
    "/programs/tutorial.bas",
    "/programs/subdemo.bas",
    "/programs/grid2d.bas",
    "/programs/demo.bas",
    "/programs/btc-demo.bas",
}


def seeded_paths() -> set[str]:
    paths = set(STATIC_SEED)
    packages = ROOT / "packages"
    if packages.is_dir():
        for manifest in packages.glob("*/MANIFEST"):
            text = manifest.read_text(encoding="utf-8")
            for raw in text.splitlines():
                line = raw.strip()
                if line.startswith("file="):
                    paths.add(line[5:].strip())
                elif line.startswith("mod="):
                    parts = line[4:].split(":", 2)
                    if len(parts) >= 2:
                        paths.add(parts[1].strip())
            paths.add(f"/packages/{manifest.parent.name}/MANIFEST")
    for lab in LABS:
        lab_dir = PROGRAMS / lab
        if lab_dir.is_dir():
            for bas in lab_dir.glob("*.bas"):
                paths.add(f"/programs/{lab}/{bas.name}")
    return paths


def vfs_tag(path: str) -> str:
    return path.strip("/").replace("/", "-").replace(".", "-")


def audit() -> int:
    errors: list[str] = []
    warnings: list[str] = []
    seeded = seeded_paths()
    all_filenames: Counter[str] = Counter()

    for lab, (prefix, expected) in LABS.items():
        lab_dir = PROGRAMS / lab
        if not lab_dir.is_dir():
            errors.append(f"missing lab directory: programs/{lab}")
            continue

        demos = sorted(p for p in lab_dir.glob("*.bas") if p.name != "menu.bas")
        menu = lab_dir / "menu.bas"
        if not menu.is_file():
            errors.append(f"{lab}: missing menu.bas")

        if len(demos) != expected:
            errors.append(f"{lab}: expected {expected} demos, found {len(demos)}")

        for demo in demos:
            all_filenames[demo.name] += 1
            if not demo.name.startswith(prefix):
                errors.append(f"{lab}: bad prefix on {demo.name}")
            nums = re.findall(rf"{prefix}(\d{{2}})", demo.name)
            if len(nums) != 1:
                warnings.append(f"{lab}: unexpected numbering in {demo.name}")

        if menu.is_file() and f"Total demos: {expected}" not in menu.read_text(encoding="utf-8"):
            warnings.append(f"{lab}: menu.bas demo count mismatch")

    for name, count in all_filenames.items():
        if count > 1:
            errors.append(f"duplicate demo filename across labs: {name} ({count}x)")

    # Cross-lab filename uniqueness within each lab
    for lab in LABS:
        names = [p.name for p in (PROGRAMS / lab).glob("*.bas")]
        dup = [n for n, c in Counter(names).items() if c > 1]
        if dup:
            errors.append(f"{lab}: duplicate filenames: {', '.join(dup)}")

    # GFS READ paths that should exist at seed time (exclude runtime writes)
    runtime_only_prefixes = (
        "/programs/blackhat/drop-",
        "/programs/redteam/canary.txt",
        "/programs/redteam/rt98.txt",
        "/programs/purpleteam/pt03-drop.txt",
    )
    read_paths: set[str] = set()
    for lab_dir in (PROGRAMS / name for name in LABS):
        for bas in lab_dir.glob("*.bas"):
            text = bas.read_text(encoding="utf-8")
            read_paths.update(GFS_READ_RE.findall(text))

    for path in sorted(read_paths):
        if any(path.startswith(p) for p in runtime_only_prefixes):
            continue
        if path not in seeded:
            errors.append(f"GFS READ references unseeded path: {path}")

    # Orange intel filename clarity (same trailing tag = confusing menus)
    ot_tags: Counter[str] = Counter()
    for bas in (PROGRAMS / "orangeteam").glob("ot*-intel-gfs-*.bas"):
        tag = bas.name.split("-intel-gfs-", 1)[-1].replace(".bas", "")
        ot_tags[tag] += 1
    for tag, count in ot_tags.items():
        if count > 1:
            warnings.append(f"orangeteam: {count} demos share gfs tag '{tag}'")

    # Inode budget (matches gfs_seed slot assignment)
    slot_count = 21 + len(list((ROOT / "packages").glob("*/MANIFEST"))) * 3  # rough
    try:
        from gfs_common import GFS_INODE_MAX

        demo_files = sum(
            len(list((PROGRAMS / lab).glob("*.bas")))
            for lab in LABS
        )
        # Recompute accurately: static 21 + packages + all lab bas
        pkg_files = 0
        for manifest in (ROOT / "packages").glob("*/MANIFEST"):
            pkg_files += sum(1 for _ in manifest.read_text().splitlines() if _.strip().startswith(("file=", "mod=")))
            pkg_files += 1
        total = 21 + pkg_files + demo_files
        if total > GFS_INODE_MAX:
            errors.append(f"inode overflow: {total} files > GFS_INODE_MAX ({GFS_INODE_MAX})")
        else:
            print(f"inode budget: {total}/{GFS_INODE_MAX} slots used")
    except ImportError:
        pass

    print("Security lab audit")
    print("-" * 40)
    for lab, (_, expected) in LABS.items():
        count = len(list((PROGRAMS / lab).glob("*.bas"))) - 1
        status = "OK" if count == expected else "FAIL"
        print(f"  {lab:12} {count:3}/{expected:<3} {status}")

    if warnings:
        print("\nWarnings:")
        for w in warnings:
            print(f"  WARN: {w}")

    if errors:
        print("\nErrors:")
        for e in errors:
            print(f"  ERR:  {e}")
        return 1

    print("\nAudit passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(audit())
