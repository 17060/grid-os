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
PACKAGES = ROOT / "packages"
WIKI_README = ROOT / "docs" / "wiki" / "README.md"
WIKI_MODULES = ROOT / "docs" / "wiki" / "package-modules.md"
BASIC_C = ROOT / "kernel" / "basic.c"


def parse_mod_line(rest: str) -> tuple[str, str, str, str] | None:
    """Parse mod=name:path:description:category — description may contain ':'."""
    first = rest.find(":")
    if first <= 0:
        return None
    second = rest.find(":", first + 1)
    if second < 0:
        return None
    name = rest[:first]
    path = rest[first + 1 : second]
    if not path.startswith("/"):
        return None
    last = rest.rfind(":")
    if last > second:
        desc = rest[second + 1 : last]
        category = rest[last + 1 :]
    else:
        desc = rest[second + 1 :]
        category = "general"
    return name, path, desc, category


def parse_manifest(path: Path) -> tuple[str, str, str, list[tuple[str, str, str, str]]]:
    name = version = desc = ""
    mods: list[tuple[str, str, str, str]] = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if line.startswith("name="):
            name = line[5:]
        elif line.startswith("version="):
            version = line[8:]
        elif line.startswith("desc="):
            desc = line[5:]
        elif line.startswith("mod="):
            parsed = parse_mod_line(line[4:])
            if parsed:
                mods.append(parsed)
    return name, version, desc, mods


def read_grid_status_version() -> str:
    text = BASIC_C.read_text(encoding="utf-8")
    m = re.search(r'GRID\.STATUS\$"\)\s*\{\s*return make_str\("([^"]+)"\)', text)
    return m.group(1) if m else "Grid OS 7.1.1"


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
    manifests = sorted(PACKAGES.glob("*/MANIFEST"))
    if not manifests:
        print("skip: no MANIFESTs")
        return 0

    version = read_grid_status_version()
    pkg_lines: list[str] = []
    rows = [
        "| Package | Module | Category | Path | Description |",
        "|---------|--------|----------|------|-------------|",
    ]
    total_mods = 0

    for manifest in manifests:
        pkg_name, pkg_ver, pkg_desc, mods = parse_manifest(manifest)
        pkg_lines.append(f"- **`{pkg_name}`** v{pkg_ver} — {len(mods)} modules — {pkg_desc}")
        total_mods += len(mods)
        for name, path, desc, category in mods:
            rows.append(
                f"| `{pkg_name}` | `{name}` | `{category}` | `{path}` | {desc} |"
            )

    meta = (
        f"- **Grid OS version:** {version}\n"
        f"- **Packages:** {len(manifests)} seeded ({total_mods} IDE modules total)\n"
        + "\n".join(pkg_lines)
        + "\n"
        f"- **Last synced by:** `python3 tools/sync_basic_wiki.py`"
    )
    replace_block(WIKI_README, "META", meta)
    replace_block(WIKI_MODULES, "MODULE_TABLE", "\n".join(rows))

    print(f"synced wiki: {total_mods} modules in {len(manifests)} packages, {version}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
