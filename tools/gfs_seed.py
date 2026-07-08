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
    b"20 PRINT \"GridBASIC 7.1.1 online\"\n"
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
    b"10 REM Vault from GridBASIC\n"
    b"20 GRID.VAULT.PUT \"motd\", \"hello from BASIC\"\n"
    b"30 GRID.VAULT.SYNC\n"
    b"40 PRINT GRID.VAULT.GET$(\"motd\")\n"
    b"50 END\n"
)

AIDEMO_BAS = (
    b"10 REM AI demo -- full-length PRINT (host: make ai-bridge)\n"
    b"20 PRINT \"Grid AI demo (offline fallback if no bridge)\"\n"
    b"30 GRID.AI.PRINT \"What is PRINT?\", \"EXPLAIN\"\n"
    b"40 END\n"
)

HTTPDEMO_BAS = (
    b"10 REM HTTP from GridBASIC (host bridge optional)\n"
    b"20 R$ = GRID.HTTP.GET$(\"gateway\", 80, \"/\")\n"
    b"30 IF LEN(R$) > 0 THEN PRINT \"HTTP ok\" ELSE PRINT \"HTTP skip\"\n"
    b"40 END\n"
)

BTCDEMO_BAS = (
    b"10 REM Bitcoin demo -- host: make btc-bridge (testnet/regtest)\n"
    b"20 PRINT \"=== Grid BTC ===\"\n"
    b"30 PRINT GRID.BTC.STATUS$\n"
    b"40 PRINT \"Balance: \"; GRID.BTC.BALANCE$\n"
    b"50 GRID.BTC.PRINT \"getblockchaininfo\"\n"
    b"60 END\n"
)

ADVANCEDEMO_BAS = (
    b"10 REM Advanced GridBASIC features\n"
    b"20 CONST MAX=10\n"
    b"30 DATA 1,2,3\n"
    b"40 READ A,B,C\n"
    b"50 SELECT CASE A\n"
    b"60 CASE 1\n"
    b"70   PRINT \"one\"\n"
    b"80 CASE ELSE\n"
    b"90   PRINT \"other\"\n"
    b"100 END SELECT\n"
    b"110 PRINT INSTR$(\"Grid OS\",\"OS\")\n"
    b"120 END\n"
)

AUTOEXEC_BAS = (
    b"10 REM Flynn Boot -- runs once at Grid OS startup\n"
    b"20 PRINT \"\"\n"
    b"30 PRINT \"=== Welcome to Flynn's Grid ===\"\n"
    b"40 PRINT GRID.STATUS$\n"
    b"50 PRINT \"Type 'tutorial' or Esc :load tutorial in IDE\"\n"
    b"60 PRINT \"Samples: samples   Modules: pkg mods\"\n"
    b"70 PRINT \"Disable boot script: vault put autoexec off\"\n"
    b"80 PRINT \"\"\n"
    b"90 END\n"
)

TUTORIAL_BAS = (
    b"10 REM GridBASIC Tutorial -- Flynn Boot Experience\n"
    b"20 GRID.CLS\n"
    b"30 PRINT \"=== GridBASIC Tutorial ===\"\n"
    b"40 PRINT \"1. PRINT shows text on the grid\"\n"
    b"50 PRINT \"2. LET stores numbers in variables\"\n"
    b"60 LET N = 42\n"
    b"70 PRINT \"   N = \"; N\n"
    b"80 PRINT \"3. FOR loops count for you\"\n"
    b"90 FOR I = 1 TO 3\n"
    b"100   PRINT \"   line \"; I\n"
    b"110 NEXT I\n"
    b"120 PRINT \"4. Strings join with +\"\n"
    b"130 S$ = \"hello \" + \"grid\"\n"
    b"140 PRINT \"   \"; S$\n"
    b"150 PRINT \"5. GRID.WHOAMI$ = \"; GRID.WHOAMI$\n"
    b"160 PRINT \"Try: samples   basic run /programs/subdemo.bas\"\n"
    b"170 PRINT \"End of line.\"\n"
    b"180 END\n"
)

SUBDEMO_BAS = (
    b"10 REM SUB / FUNCTION demo\n"
    b"20 SUB GREET(N$)\n"
    b"30   PRINT \"Hello, \"; N$\n"
    b"40 END SUB\n"
    b"50 FUNCTION DOUBLE(X)\n"
    b"60   DOUBLE = X * 2\n"
    b"70 END FUNCTION\n"
    b"80 CALL GREET(\"Flynn\")\n"
    b"90 PRINT \"DOUBLE(21) = \"; DOUBLE(21)\n"
    b"100 END\n"
)

GRID2D_BAS = (
    b"10 REM 2D array demo\n"
    b"20 DIM M(3,3)\n"
    b"30 FOR R = 0 TO 3\n"
    b"40   FOR C = 0 TO 3\n"
    b"50     M(R,C) = R * 10 + C\n"
    b"60   NEXT C\n"
    b"70 NEXT R\n"
    b"80 PRINT \"M(2,3) = \"; M(2,3)\n"
    b"90 END\n"
)

DEMO_BAS = (
    b"10 REM Bytecode demo -- compile with :compile demo\n"
    b"20 PRINT \"GridBASIC bytecode demo\"\n"
    b"30 PRINT \"Run: Esc :run demo.grid\"\n"
    b"40 END\n"
)

PACKAGES_ROOT = Path(__file__).resolve().parent.parent / "packages"
REDTEAM_ROOT = Path(__file__).resolve().parent.parent / "programs" / "redteam"
BLACKHAT_ROOT = Path(__file__).resolve().parent.parent / "programs" / "blackhat"
WHITETEAM_ROOT = Path(__file__).resolve().parent.parent / "programs" / "whiteteam"
BLUETEAM_ROOT = Path(__file__).resolve().parent.parent / "programs" / "blueteam"


def lab_seed_files(root: Path, vfs_dir: str) -> list[tuple[str, bytes]]:
    out: list[tuple[str, bytes]] = []
    if not root.is_dir():
        return out
    for path in sorted(root.glob("*.bas")):
        out.append((f"{vfs_dir}/{path.name}", path.read_bytes()))
    return out


def redteam_seed_files() -> list[tuple[str, bytes]]:
    return lab_seed_files(REDTEAM_ROOT, "/programs/redteam")


def blackhat_seed_files() -> list[tuple[str, bytes]]:
    return lab_seed_files(BLACKHAT_ROOT, "/programs/blackhat")


def whiteteam_seed_files() -> list[tuple[str, bytes]]:
    return lab_seed_files(WHITETEAM_ROOT, "/programs/whiteteam")


def blueteam_seed_files() -> list[tuple[str, bytes]]:
    return lab_seed_files(BLUETEAM_ROOT, "/programs/blueteam")


def package_seed_files() -> list[tuple[str, bytes]]:
    """Return (vfs_path, payload) pairs from all packages/*/MANIFEST."""
    out: list[tuple[str, bytes]] = []
    seen: set[str] = set()

    manifests = sorted(PACKAGES_ROOT.glob("*/MANIFEST"))
    if not manifests:
        return out

    for manifest in manifests:
        pkg_name = manifest.parent.name
        text = manifest.read_text(encoding="utf-8")
        paths: list[str] = []
        for raw in text.splitlines():
            line = raw.strip()
            if line.startswith("file="):
                path = line[5:].strip()
            elif line.startswith("mod="):
                parts = line[4:].split(":", 2)
                path = parts[1].strip() if len(parts) >= 2 else ""
            else:
                continue
            if not path or path in seen:
                continue
            seen.add(path)
            paths.append(path)

        for vfs in paths:
            if vfs.endswith("/MANIFEST"):
                host = manifest
            else:
                prefix = f"/packages/{pkg_name}/"
                if not vfs.startswith(prefix):
                    raise SystemExit(f"unexpected vfs path in {manifest}: {vfs}")
                rel = vfs[len(prefix) :]
                host = manifest.parent / rel
            if not host.is_file():
                raise SystemExit(f"missing package file for {vfs}: {host}")
            out.append((vfs, host.read_bytes()))

    return out


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
        (15, "/programs/advancedemo.bas", ADVANCEDEMO_BAS),
        (16, "/programs/autoexec.bas", AUTOEXEC_BAS),
        (17, "/programs/tutorial.bas", TUTORIAL_BAS),
        (18, "/programs/subdemo.bas", SUBDEMO_BAS),
        (19, "/programs/grid2d.bas", GRID2D_BAS),
        (20, "/programs/demo.bas", DEMO_BAS),
        (21, "/programs/btc-demo.bas", BTCDEMO_BAS),
    ]

    slot = 22
    for path, payload in package_seed_files():
        files.append((slot, path, payload))
        slot += 1

    for path, payload in redteam_seed_files():
        files.append((slot, path, payload))
        slot += 1

    for path, payload in blackhat_seed_files():
        files.append((slot, path, payload))
        slot += 1

    for path, payload in whiteteam_seed_files():
        files.append((slot, path, payload))
        slot += 1

    for path, payload in blueteam_seed_files():
        files.append((slot, path, payload))
        slot += 1

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
