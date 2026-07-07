#!/usr/bin/env python3
"""Sync auto-generated sections in docs/wiki from the live codebase.

Run after changing MANIFEST, version banners, or module count:
  python3 tools/sync_basic_wiki.py

Updates marked blocks between AUTO:BEGIN / AUTO:END comments.
"""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "packages" / "flynn-ide-tools" / "MANIFEST"
WIKI_README = ROOT / "docs" / "wiki" / "README.md"
WIKI_MODULES = ROOT / "docs" / "wiki" / "package-modules.md"
BASIC_C = ROOT / "kernel" / "basic.c"


def parse_manifest(path: Path) -> tuple[str, str, list[tuple[str, str, str]]]:
    name = version = desc = ""
    mods: list[tuple[str, str, str]] = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if line.startswith("name="):
            name = line[5:]
        elif line.startswith("version="):
            version = line[5:]
        elif line.startswith("desc="):
            desc = line[5:]
        elif line.startswith("mod="):
            parts = line[4:].split(":", 2)
            if len(parts) >= 3:
                mods.append((parts[0], parts[1], parts[2]))
    return name, version, mods


def read_grid_status_version() -> str:
    text = BASIC_C.read_text(encoding="utf-8")
    m = re.search(r'GRID\.STATUS\$"\)\s*\{\s*return make_str\("([^"]+)"\)', text)
    return m.group(1) if m else "Grid OS 7.1"


def replace_block(path: Path, tag: str, content: str) -> None:
    if not path.is_file():
        return
    text = path.read_text(encoding="utf-8")
    pattern = (
        rf"(<!-- AUTO:{tag}:BEGIN -->)\n.*?\n(<!-- AUTO:{tag}:END -->)"
    )
    new = rf"\1\n{content}\n\2"
    updated, n = re.subn(pattern, new, text, count=1, flags=re.DOTALL)
    if n == 0:
        raise SystemExit(f"missing AUTO:{tag} block in {path}")
    path.write_text(updated, encoding="utf-8")


def main() -> int:
    if not MANIFEST.is_file():
        print("skip: no MANIFEST")
        return 0

    pkg_name, pkg_ver, mods = parse_manifest(MANIFEST)
    version = read_grid_status_version()

    meta = (
        f"- **Grid OS version:** {version}\n"
        f"- **Package:** `{pkg_name}` v{pkg_ver} — {len(mods)} IDE modules\n"
        f"- **Last synced by:** `python3 tools/sync_basic_wiki.py`"
    )
    replace_block(WIKI_README, "META", meta)

    rows = ["| Module | Path | Description |", "|--------|------|-------------|"]
    for name, path, desc in mods:
        rows.append(f"| `{name}` | `{path}` | {desc} |")
    replace_block(WIKI_MODULES, "MODULE_TABLE", "\n".join(rows))

    print(f"synced wiki: {len(mods)} modules, {version}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
