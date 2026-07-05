# Flynn's Vision for Grid OS

Grid OS is built on Kevin Flynn's original idea of the Grid — not CLU's corrupted interpretation.

## What Flynn wanted

The Grid was a **digital frontier**: a place for research, creation, and discovery running faster than the real world. Users (humans) and programs (native inhabitants) would coexist. Emergence — like the ISOs — was miraculous, not a bug.

## What CLU got wrong

CLU was told to build "the perfect system." He interpreted that as:

- Eliminate anomalies
- Centralize control
- Enforce uniformity
- Escape to "perfect" the real world

Grid OS explicitly rejects this model. There is **no CLU protocol** — no single admin program with godlike authority.

## Grid OS security philosophy

### 1. Capabilities, not roles

Instead of a root user who can do anything, entities carry **capability tokens**:

- `READ_GRID` — read system state
- `WRITE_GRID` — emit output
- `SPAWN` — create new programs
- `COMMUNICATE` — inter-entity messaging
- `ADMIN` — intentionally unused in v0.1

Users receive broad creative capabilities. Programs receive the minimum needed.

### 2. Identity discs

Every entity has a **128-bit identity disc** — a unique identifier derived at boot. Actions are tied to identity, not shared passwords.

### 3. Fail closed

If a capability check fails, the action is denied and logged. The system never silently escalates privilege.

### 4. Minimal kernel

Security through simplicity: fewer subsystems means fewer vulnerabilities. v0.1 has no network stack, no filesystem, and no dynamic loading.

### 5. Emergence welcome

The **ISO Research Zone** hosts up to four Isomorphic Algorithms in strict sandboxes. Each ISO carries its own identity disc and a self-modifying genome buffer. Mutations are checksum-validated; invalid changes are rolled back. CLU would derez anomalies — Grid OS **quarantines** them for observation instead.

### 6. Ring-3 sandboxes

User programs (`gridprog`) execute in **ring 3** with dedicated page tables:

- **Code** mapped RX (no write)
- **Stack** mapped RW+NX (no execute)
- **Syscalls** via `int 0x80` with capability checks per program
- **Faults** fail closed — W^X violations derez the sandbox, not the Grid

Each spawned program receives its own **identity disc**, derived from the User's disc.

### 7. Grid Vault and serial bridge

The **Grid Vault** stores named nodes, user disc state, cycles, and ISO snapshots in a CRC32-sealed structure. **`vault sync`** writes to Flynn's **arcade disk** (`FLYNGRID` sectors on IDE) — the basement server that never powers down. Serial export/import remains for cross-machine portal jumps.

### 8. Ring-3 userland

Programs run in sandboxes with syscalls only:

| Program | Purpose |
|---------|---------|
| `gridsh` | Interactive Flynn shell |
| `discinfo` | Identity disc record |
| `gridprog` | Minimal sandbox demo |
| `lightcycle` | Tron arena game |

### 9. GridFS

Virtual paths over vault and ISO state: `ls /`, `cat /motd`, `/isos/1/genome`.

### 10. Audit log

Transparent events — capability denials, spawns, vault syncs. CLU hid; Flynn reveals.

### 11. ISO autopilot

Hardware timer evolves ISOs in the background when engaged.

### 12. GFS2FLYN (Grid OS 3.0)

On-disk Flynn archive: 64 files, 16 KB each. Host tools install ring-3 programs without rebuilding the kernel. GridLink frames vault state and live program pushes over COM1.

## Grid OS 3.0

Flynn's complete digital frontier — GFS2, GridLink portal, host install, shell history.

## Grid OS 1.0

Flynn's complete digital frontier.

## The name

**Grid OS** — the operating system of the frontier, not the operating system of control.
