#!/usr/bin/env python3
"""Generate package MANIFESTs, module .bas files, and kernel/gfs.c seed block."""

from __future__ import annotations

import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PACKAGES = ROOT / "packages"

IDE_PREFIX = "/packages/flynn-ide-tools"
NET_PREFIX = "/packages/flynn-net-tools"


@dataclass(frozen=True)
class Module:
    name: str
    desc: str
    src: str
    category: str


IDE_MODULES: list[Module] = [
    Module("disc-status", "Identity disc status panel", """\
10 REM IDE module: disc-status
20 GRID.CLS
30 PRINT "=== Identity Disc ==="
40 PRINT GRID.DISC.STATUS$
50 PRINT "Entity: "; GRID.DISC.ENTITY$
60 PRINT "Level: "; GRID.DISC.LEVEL
70 PRINT "XP: "; GRID.DISC.XP
80 END
""", "disc"),
    Module("grid-ping", "Ping gateway and grid hosts", """\
10 REM IDE module: grid-ping
20 PRINT "=== Grid Ping ==="
30 PRINT "gateway: "; GRID.PING("gateway")
40 PRINT "grid: "; GRID.PING("grid")
50 PRINT "bridge: "; GRID.PING("bridge")
60 END
""", "network"),
    Module("patrol-arm", "Start recognizer patrol", """\
10 REM IDE module: patrol-arm
20 GRID.RECOGNIZER.START
30 PRINT GRID.RECOGNIZER.STATUS$
40 END
""", "patrol"),
    Module("patrol-stand-down", "Stop recognizer patrol", """\
10 REM IDE module: patrol-stand-down
20 GRID.RECOGNIZER.STOP
30 PRINT GRID.RECOGNIZER.STATUS$
40 END
""", "patrol"),
    Module("whoami-panel", "Entity type and identity", """\
10 REM IDE module: whoami-panel
20 PRINT "=== Who Am I ==="
30 PRINT "Entity: "; GRID.WHOAMI$
40 PRINT "Disc: "; GRID.DISC.ENTITY$
50 PRINT GRID.STATUS$
60 END
""", "disc"),
    Module("caps-panel", "Granted capability mask", """\
10 REM IDE module: caps-panel
20 PRINT "=== Capabilities ==="
30 PRINT "CAP mask: "; GRID.CAPS$
40 PRINT "Use 'caps' in shell for decoded list"
50 END
""", "system"),
    Module("net-status", "Virtio-net link status", """\
10 REM IDE module: net-status
20 PRINT "=== Grid Network ==="
30 PRINT GRID.NET.STATUS$
40 END
""", "network"),
    Module("dns-lookup", "Resolve Flynn host names", """\
10 REM IDE module: dns-lookup
20 PRINT "=== DNS Resolve ==="
30 PRINT "gateway -> "; GRID.DNS.RESOLVE$("gateway")
40 PRINT "grid -> "; GRID.DNS.RESOLVE$("grid")
50 PRINT "bridge -> "; GRID.DNS.RESOLVE$("bridge")
60 END
""", "network"),
    Module("vault-nodes", "List vault key nodes", """\
10 REM IDE module: vault-nodes
20 PRINT "=== Vault Nodes ==="
30 PRINT GRID.VAULT.LIST$
40 PRINT "Put: GRID.VAULT.PUT key$, val$  Sync: GRID.VAULT.SYNC"
50 END
""", "storage"),
    Module("gfs-programs", "List Flynn /programs archive", """\
10 REM IDE module: gfs-programs
20 PRINT "=== Flynn /programs ==="
30 PRINT GRID.GFS.LIST$("/programs")
40 PRINT "Run: basic run /programs/hello.bas"
50 END
""", "storage"),
    Module("jobs-monitor", "Background sandbox jobs", """\
10 REM IDE module: jobs-monitor
20 PRINT "=== Background Jobs ==="
30 PRINT GRID.JOBS.LIST$
40 PRINT "Shell: jobs   kill <#>   wait"
50 END
""", "system"),
    Module("iso-roster", "ISO research zone entities", """\
10 REM IDE module: iso-roster
20 PRINT "=== ISO Zone ==="
30 PRINT GRID.ISO.LIST$
40 PRINT "Shell: iso list   iso spawn"
50 END
""", "system"),
    Module("audit-tail", "Recent audit log entries", """\
10 REM IDE module: audit-tail
20 PRINT "=== Audit Tail ==="
30 PRINT GRID.LOG.TAIL$(8)
40 END
""", "system"),
    Module("grid-clock", "Grid cycle timer ticks", """\
10 REM IDE module: grid-clock
20 PRINT "=== Grid Clock ==="
30 PRINT "Ticks: "; GRID.TIME
40 FOR I = 1 TO 3
50   PRINT "  beat "; I; " @ "; GRID.TIME
60   GRID.WAIT 5
70 NEXT I
80 END
""", "grid"),
    Module("grid-clear", "Clear screen with Flynn banner", """\
10 REM IDE module: grid-clear
20 GRID.CLS
30 PRINT "=== Flynn GridBASIC IDE ==="
40 PRINT GRID.STATUS$
50 PRINT "Esc :help   :mods   :run"
60 END
""", "grid"),
    Module("pkg-index", "Installed packages and modules", """\
10 REM IDE module: pkg-index
20 PRINT "=== Grid Packages ==="
30 PRINT "Packages: "; GRID.PKG.LIST$
40 PRINT "Modules: "; GRID.PKG.MODS$
50 PRINT "Shell: pkg mods   basic mod run <name>"
60 END
""", "storage"),
    Module("sample-menu", "GridBASIC sample program guide", """\
10 REM IDE module: sample-menu
20 PRINT "=== GridBASIC Samples ==="
30 PRINT GRID.GFS.LIST$("/programs")
40 PRINT "Try: tutorial, hello, subdemo, grid2d, demo"
50 PRINT "IDE: Esc :load tutorial   :run demo.grid"
60 END
""", "dev"),
    Module("ide-cheatsheet", "IDE colon-command reference", """\
10 REM IDE module: ide-cheatsheet
20 PRINT "=== IDE Cheatsheet ==="
30 PRINT ":run :save :load :new :list :find :goto"
40 PRINT ":mods [cat] :mod run <n> :pkg list|mods"
50 PRINT ":tutorial :compile :samples :help"
60 PRINT "grid> pkg mods network   basic mod run <n>"
70 END
""", "dev"),
    Module("beep-scale", "PC speaker note demo", """\
10 REM IDE module: beep-scale
20 PRINT "=== Grid Beep ==="
30 GRID.NOTE 60, 120
40 GRID.NOTE 64, 120
50 GRID.NOTE 67, 120
60 GRID.BEEP 880, 200
70 PRINT "End of line."
80 END
""", "grid"),
    Module("plot-grid", "VGA plot pattern demo", """\
10 REM IDE module: plot-grid
20 GRID.CLS
30 FOR X = 0 TO 40
40   GRID.PLOT X, X, 2
50   GRID.PLOT 80 - X, X, 3
60 NEXT X
70 PRINT "Plot demo complete."
80 END
""", "grid"),
    Module("ai-ask", "Quick AI bridge question", """\
10 REM IDE module: ai-ask (host: make ai-bridge)
20 PRINT "=== Grid AI ==="
30 PRINT GRID.AI.MODELS$
40 PRINT GRID.AI.ASK$("What is PRINT in GridBASIC?", "EXPLAIN")
50 END
""", "bridge"),
    Module("btc-snapshot", "Bitcoin bridge status", """\
10 REM IDE module: btc-snapshot (host: make btc-bridge)
20 PRINT "=== Grid BTC ==="
30 PRINT GRID.BTC.STATUS$
40 PRINT GRID.BTC.HELP$
50 END
""", "bridge"),
    Module("irc-check", "IRC session status", """\
10 REM IDE module: irc-check
20 PRINT "=== Grid IRC ==="
30 PRINT GRID.IRC.STATUS$
40 PRINT "Connect: irc connect gateway 6667 griduser"
50 END
""", "network"),
    Module("hosts-table", "Show /etc/hosts from Flynn disk", """\
10 REM IDE module: hosts-table
20 PRINT "=== /etc/hosts ==="
30 H$ = GRID.GFS.READ$("/etc/hosts")
40 IF LEN(H$) > 0 THEN PRINT H$ ELSE PRINT "(missing — gfs seed)"
50 END
""", "network"),
    Module("spawn-catalog", "Ring-3 program spawn hints", """\
10 REM IDE module: spawn-catalog
20 PRINT "=== Spawn Catalog ==="
30 PRINT GRID.GFS.LIST$("/programs")
40 PRINT "Shell: spawn gridsh   spawn lightcycle"
50 PRINT "GRID.SPAWN.BG runs jobs in background"
60 END
""", "system"),
]

NET_MODULES: list[Module] = [
    Module("http-probe", "HTTP GET probe via GRID.HTTP", """\
10 REM Flynn net-tools: http-probe
20 PRINT "=== HTTP Probe ==="
30 R$ = GRID.HTTP.GET$("gateway", 80, "/")
40 IF LEN(R$) > 0 THEN PRINT "HTTP ok ("; LEN(R$); " B)" ELSE PRINT "HTTP skip (no bridge)"
50 END
""", "network"),
    Module("irc-connect", "IRC quick-connect helper", """\
10 REM Flynn net-tools: irc-connect
20 PRINT "=== IRC Connect ==="
30 PRINT GRID.IRC.STATUS$
40 PRINT "Try: irc connect gateway 6667 griduser"
50 PRINT "Then: irc join #grid   irc say #grid hello"
60 END
""", "network"),
    Module("https-bridge", "HTTPS bridge status (host bridge)", """\
10 REM Flynn net-tools: https-bridge (host: make https-bridge)
20 PRINT "=== HTTPS Bridge ==="
30 PRINT "Host: make https-bridge"
40 PRINT "Guest HTTP via GRID.HTTP.* on port 80"
50 PRINT GRID.NET.STATUS$
60 END
""", "bridge"),
    Module("grid-server", "TCP line server with custom keywords", """\
10 REM Flynn net-tools: grid-server
20 PRINT "=== Grid TCP Server ==="
30 PRINT "IDE: Esc :server new — edit template"
40 PRINT "Shell: server listen 7700"
50 PRINT GRID.SERVER.STATUS$
60 PRINT "Built-ins: PING HELP STATUS ECHO QUIT"
70 PRINT "Custom keywords: TIME VER HELLO <name> in template"
80 END
""", "network"),
    Module("irc-server", "Flynn IRC server with !bot commands", """\
10 REM Flynn net-tools: irc-server
20 PRINT "=== Flynn IRC Server ==="
30 PRINT "IDE: Esc :ircserver new — edit !commands"
40 PRINT "Shell: ircserver listen 6667"
50 PRINT GRID.IRCSERVER.STATUS$
60 PRINT "Connect: irc connect localhost 6667 nick"
70 PRINT "Join #grid and try !time !help !motd !ver"
80 END
""", "network"),
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


def write_package(
    pkg_dir: Path,
    vfs_prefix: str,
    pkg_name: str,
    version: str,
    desc: str,
    modules: list[Module],
) -> None:
    mod_dir = pkg_dir / "modules"
    mod_dir.mkdir(parents=True, exist_ok=True)
    for mod in modules:
        mod_dir.joinpath(f"{mod.name}.bas").write_text(mod.src, encoding="utf-8")

    lines = [
        f"name={pkg_name}",
        f"version={version}",
        f"desc={desc}",
        f"file={vfs_prefix}/MANIFEST",
    ]
    for mod in modules:
        path = f"{vfs_prefix}/modules/{mod.name}.bas"
        lines.append(f"file={path}")
    for mod in modules:
        path = f"{vfs_prefix}/modules/{mod.name}.bas"
        lines.append(f"mod={mod.name}:{path}:{mod.desc}:{mod.category}")
    pkg_dir.joinpath("MANIFEST").write_text("\n".join(lines) + "\n", encoding="utf-8")


def seed_block_for_manifest(manifest_path: Path, vfs_prefix: str, modules: list[Module]) -> str:
    chunks: list[str] = []
    manifest = manifest_path.read_text(encoding="utf-8")
    chunks.append(
        f'    seed_one("{vfs_prefix}/MANIFEST",\n'
        + c_escape(manifest)
        + f",\n             {len(manifest.encode('utf-8'))});\n"
    )
    for mod in modules:
        path = f"{vfs_prefix}/modules/{mod.name}.bas"
        data = mod.src.encode("utf-8")
        chunks.append(
            f'    seed_one("{path}",\n'
            + c_escape(mod.src)
            + f",\n             {len(data)});\n"
        )
    return "\n".join(chunks)


def patch_pkg_c(manifests: list[tuple[str, str]]) -> None:
    pkg = ROOT / "kernel" / "pkg.c"
    text = pkg.read_text(encoding="utf-8")
    chunks: list[str] = []
    for vfs_path, manifest in manifests:
        chunks.append(
            f'    (void)pkg_parse_manifest_text(\n'
            + c_escape(manifest)
            + f',\n             "{vfs_path}");'
        )
    block = "\n".join(chunks)
    start = text.index("    /* AUTO:PKG_SEED:BEGIN */")
    end = text.index("    /* AUTO:PKG_SEED:END */", start)
    new_text = text[: start + len("    /* AUTO:PKG_SEED:BEGIN */")] + "\n" + block + "\n" + text[end:]
    pkg.write_text(new_text, encoding="utf-8")


def patch_gfs_c(block: str) -> None:
    gfs = ROOT / "kernel" / "gfs.c"
    text = gfs.read_text(encoding="utf-8")
    start = text.index('    seed_one("/packages/flynn-ide-tools/MANIFEST"')
    end = text.index("    return 0;", start)
    gfs.write_text(text[:start] + block + "\n" + text[end:], encoding="utf-8")


def main() -> int:
    ide_dir = PACKAGES / "flynn-ide-tools"
    net_dir = PACKAGES / "flynn-net-tools"

    write_package(
        ide_dir,
        IDE_PREFIX,
        "flynn-ide-tools",
        "2.1",
        "25 GridBASIC IDE tools for Flynn's Grid (7.1.1 categories)",
        IDE_MODULES,
    )
    write_package(
        net_dir,
        NET_PREFIX,
        "flynn-net-tools",
        "1.0",
        "Flynn network bridge helpers for GridBASIC IDE",
        NET_MODULES,
    )

    block = seed_block_for_manifest(ide_dir / "MANIFEST", IDE_PREFIX, IDE_MODULES)
    block += "\n" + seed_block_for_manifest(net_dir / "MANIFEST", NET_PREFIX, NET_MODULES)
    patch_gfs_c(block)
    patch_pkg_c(
        [
            (f"{IDE_PREFIX}/MANIFEST", (ide_dir / "MANIFEST").read_text(encoding="utf-8")),
            (f"{NET_PREFIX}/MANIFEST", (net_dir / "MANIFEST").read_text(encoding="utf-8")),
        ]
    )

    print(
        f"Generated {len(IDE_MODULES)} IDE modules + {len(NET_MODULES)} net modules "
        "+ MANIFESTs + gfs.c seeds"
    )
    subprocess.run(
        [sys.executable, str(ROOT / "tools" / "sync_basic_wiki.py")],
        check=False,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
