#!/usr/bin/env python3
"""Generate flynn-ide-tools MANIFEST, module .bas files, and gfs.c seed block."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PKG = ROOT / "packages" / "flynn-ide-tools"
MOD_DIR = PKG / "modules"
VFS_PREFIX = "/packages/flynn-ide-tools"

MODULES: list[tuple[str, str, str]] = [
    (
        "disc-status",
        "Identity disc status panel",
        """\
10 REM IDE module: disc-status
20 GRID.CLS
30 PRINT "=== Identity Disc ==="
40 PRINT GRID.DISC.STATUS$
50 PRINT "Entity: "; GRID.DISC.ENTITY$
60 PRINT "Level: "; GRID.DISC.LEVEL
70 PRINT "XP: "; GRID.DISC.XP
80 END
""",
    ),
    (
        "grid-ping",
        "Ping gateway and grid hosts",
        """\
10 REM IDE module: grid-ping
20 PRINT "=== Grid Ping ==="
30 PRINT "gateway: "; GRID.PING("gateway")
40 PRINT "grid: "; GRID.PING("grid")
50 PRINT "bridge: "; GRID.PING("bridge")
60 END
""",
    ),
    (
        "patrol-arm",
        "Start recognizer patrol",
        """\
10 REM IDE module: patrol-arm
20 GRID.RECOGNIZER.START
30 PRINT GRID.RECOGNIZER.STATUS$
40 END
""",
    ),
    (
        "patrol-stand-down",
        "Stop recognizer patrol",
        """\
10 REM IDE module: patrol-stand-down
20 GRID.RECOGNIZER.STOP
30 PRINT GRID.RECOGNIZER.STATUS$
40 END
""",
    ),
    (
        "whoami-panel",
        "Entity type and identity",
        """\
10 REM IDE module: whoami-panel
20 PRINT "=== Who Am I ==="
30 PRINT "Entity: "; GRID.WHOAMI$
40 PRINT "Disc: "; GRID.DISC.ENTITY$
50 PRINT GRID.STATUS$
60 END
""",
    ),
    (
        "caps-panel",
        "Granted capability mask",
        """\
10 REM IDE module: caps-panel
20 PRINT "=== Capabilities ==="
30 PRINT "CAP mask: "; GRID.CAPS$
40 PRINT "Use 'caps' in shell for decoded list"
50 END
""",
    ),
    (
        "net-status",
        "Virtio-net link status",
        """\
10 REM IDE module: net-status
20 PRINT "=== Grid Network ==="
30 PRINT GRID.NET.STATUS$
40 END
""",
    ),
    (
        "dns-lookup",
        "Resolve Flynn host names",
        """\
10 REM IDE module: dns-lookup
20 PRINT "=== DNS Resolve ==="
30 PRINT "gateway -> "; GRID.DNS.RESOLVE$("gateway")
40 PRINT "grid -> "; GRID.DNS.RESOLVE$("grid")
50 PRINT "bridge -> "; GRID.DNS.RESOLVE$("bridge")
60 END
""",
    ),
    (
        "vault-nodes",
        "List vault key nodes",
        """\
10 REM IDE module: vault-nodes
20 PRINT "=== Vault Nodes ==="
30 PRINT GRID.VAULT.LIST$
40 PRINT "Put: GRID.VAULT.PUT key$, val$  Sync: GRID.VAULT.SYNC"
50 END
""",
    ),
    (
        "gfs-programs",
        "List Flynn /programs archive",
        """\
10 REM IDE module: gfs-programs
20 PRINT "=== Flynn /programs ==="
30 PRINT GRID.GFS.LIST$("/programs")
40 PRINT "Run: basic run /programs/hello.bas"
50 END
""",
    ),
    (
        "jobs-monitor",
        "Background sandbox jobs",
        """\
10 REM IDE module: jobs-monitor
20 PRINT "=== Background Jobs ==="
30 PRINT GRID.JOBS.LIST$
40 PRINT "Shell: jobs   kill <#>   wait"
50 END
""",
    ),
    (
        "iso-roster",
        "ISO research zone entities",
        """\
10 REM IDE module: iso-roster
20 PRINT "=== ISO Zone ==="
30 PRINT GRID.ISO.LIST$
40 PRINT "Shell: iso list   iso spawn"
50 END
""",
    ),
    (
        "audit-tail",
        "Recent audit log entries",
        """\
10 REM IDE module: audit-tail
20 PRINT "=== Audit Tail ==="
30 PRINT GRID.LOG.TAIL$(8)
40 END
""",
    ),
    (
        "grid-clock",
        "Grid cycle timer ticks",
        """\
10 REM IDE module: grid-clock
20 PRINT "=== Grid Clock ==="
30 PRINT "Ticks: "; GRID.TIME
40 FOR I = 1 TO 3
50   PRINT "  beat "; I; " @ "; GRID.TIME
60   GRID.WAIT 5
70 NEXT I
80 END
""",
    ),
    (
        "grid-clear",
        "Clear screen with Flynn banner",
        """\
10 REM IDE module: grid-clear
20 GRID.CLS
30 PRINT "=== Flynn GridBASIC IDE ==="
40 PRINT GRID.STATUS$
50 PRINT "Esc :help   :mods   :run"
60 END
""",
    ),
    (
        "pkg-index",
        "Installed packages and modules",
        """\
10 REM IDE module: pkg-index
20 PRINT "=== Grid Packages ==="
30 PRINT "Packages: "; GRID.PKG.LIST$
40 PRINT "Modules: "; GRID.PKG.MODS$
50 PRINT "Shell: pkg mods   basic mod run <name>"
60 END
""",
    ),
    (
        "sample-menu",
        "GridBASIC sample program guide",
        """\
10 REM IDE module: sample-menu
20 PRINT "=== GridBASIC Samples ==="
30 PRINT GRID.GFS.LIST$("/programs")
40 PRINT "Try: tutorial, hello, subdemo, grid2d"
50 PRINT "IDE: Esc :load tutorial"
60 END
""",
    ),
    (
        "ide-cheatsheet",
        "IDE colon-command reference",
        """\
10 REM IDE module: ide-cheatsheet
20 PRINT "=== IDE Cheatsheet ==="
30 PRINT ":run :save :load :new :list"
40 PRINT ":mods :mod run <n> :mod load <n>"
50 PRINT ":tutorial :compile :samples :help"
60 PRINT "grid> pkg mods   basic mod run <n>"
70 END
""",
    ),
    (
        "beep-scale",
        "PC speaker note demo",
        """\
10 REM IDE module: beep-scale
20 PRINT "=== Grid Beep ==="
30 GRID.NOTE 60, 120
40 GRID.NOTE 64, 120
50 GRID.NOTE 67, 120
60 GRID.BEEP 880, 200
70 PRINT "End of line."
80 END
""",
    ),
    (
        "plot-grid",
        "VGA plot pattern demo",
        """\
10 REM IDE module: plot-grid
20 GRID.CLS
30 FOR X = 0 TO 40
40   GRID.PLOT X, X, 2
50   GRID.PLOT 80 - X, X, 3
60 NEXT X
70 PRINT "Plot demo complete."
80 END
""",
    ),
    (
        "ai-ask",
        "Quick AI bridge question",
        """\
10 REM IDE module: ai-ask (host: make ai-bridge)
20 PRINT "=== Grid AI ==="
30 PRINT GRID.AI.MODELS$
40 PRINT GRID.AI.ASK$("What is PRINT in GridBASIC?", "EXPLAIN")
50 END
""",
    ),
    (
        "btc-snapshot",
        "Bitcoin bridge status",
        """\
10 REM IDE module: btc-snapshot (host: make btc-bridge)
20 PRINT "=== Grid BTC ==="
30 PRINT GRID.BTC.STATUS$
40 PRINT GRID.BTC.HELP$
50 END
""",
    ),
    (
        "irc-check",
        "IRC session status",
        """\
10 REM IDE module: irc-check
20 PRINT "=== Grid IRC ==="
30 PRINT GRID.IRC.STATUS$
40 PRINT "Connect: irc connect gateway 6667 griduser"
50 END
""",
    ),
    (
        "hosts-table",
        "Show /etc/hosts from Flynn disk",
        """\
10 REM IDE module: hosts-table
20 PRINT "=== /etc/hosts ==="
30 H$ = GRID.GFS.READ$("/etc/hosts")
40 IF LEN(H$) > 0 THEN PRINT H$ ELSE PRINT "(missing — gfs seed)"
50 END
""",
    ),
    (
        "spawn-catalog",
        "Ring-3 program spawn hints",
        """\
10 REM IDE module: spawn-catalog
20 PRINT "=== Spawn Catalog ==="
30 PRINT GRID.GFS.LIST$("/programs")
40 PRINT "Shell: spawn gridsh   spawn lightcycle"
50 PRINT "GRID.SPAWN.BG runs jobs in background"
60 END
""",
    ),
]


def c_escape(data: str) -> str:
    out: list[str] = []
    for line in data.split("\n"):
        if line:
            esc = line.replace("\\", "\\\\").replace('"', '\\"')
            out.append(f'             "{esc}\\n"')
        else:
            out.append('             "\\n"')
    return "\n".join(out)


def write_manifest() -> None:
    lines = [
        "name=flynn-ide-tools",
        "version=2.0",
        "desc=25 GridBASIC IDE tools for Flynn's Grid",
        f"file={VFS_PREFIX}/MANIFEST",
    ]
    for name, _desc, _src in MODULES:
        path = f"{VFS_PREFIX}/modules/{name}.bas"
        lines.append(f"file={path}")
    for name, desc, _src in MODULES:
        path = f"{VFS_PREFIX}/modules/{name}.bas"
        lines.append(f"mod={name}:{path}:{desc}")
    text = "\n".join(lines) + "\n"
    PKG.joinpath("MANIFEST").write_text(text, encoding="utf-8")


def write_modules() -> None:
    MOD_DIR.mkdir(parents=True, exist_ok=True)
    for name, _desc, src in MODULES:
        MOD_DIR.joinpath(f"{name}.bas").write_text(src, encoding="utf-8")


def write_gfs_c_block() -> str:
    chunks: list[str] = []
    manifest = PKG.joinpath("MANIFEST").read_text(encoding="utf-8")
    chunks.append(
        "    seed_one(\"/packages/flynn-ide-tools/MANIFEST\",\n"
        + c_escape(manifest)
        + f",\n             {len(manifest.encode('utf-8'))});\n"
    )
    for name, _desc, src in MODULES:
        path = f"/packages/flynn-ide-tools/modules/{name}.bas"
        data = src.encode("utf-8")
        chunks.append(
            f"    seed_one(\"{path}\",\n"
            + c_escape(src)
            + f",\n             {len(data)});\n"
        )
    return "\n".join(chunks)


def patch_gfs_c(block: str) -> None:
    gfs = ROOT / "kernel" / "gfs.c"
    text = gfs.read_text(encoding="utf-8")
    start = text.index('    seed_one("/packages/flynn-ide-tools/MANIFEST"')
    end = text.index("    return 0;", start)
    new_text = text[:start] + block + "\n" + text[end:]
    gfs.write_text(new_text, encoding="utf-8")


def main() -> int:
    if len(MODULES) != 25:
        raise SystemExit(f"expected 25 modules, got {len(MODULES)}")
    write_modules()
    write_manifest()
    block = write_gfs_c_block()
    patch_gfs_c(block)
    print(f"Generated {len(MODULES)} modules + MANIFEST + gfs.c seeds")
    import subprocess
    subprocess.run([sys.executable, str(ROOT / "tools" / "sync_basic_wiki.py")], check=False)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
