# Introduction to Grid OS & the GridBASIC IDE

Grid OS is Flynn's digital frontier — a real, bootable operating system where **the computer and the workshop are the same place**. When you boot Grid OS, you land in the **GridBASIC IDE**: a fullscreen editor, a programming language, and a path into the rest of the system, all in one screen.

This is not Windows, macOS, or Linux. It is something smaller, stranger, and — we hope — more honest about what a personal computer can be when **using it and making it** are not separate worlds.

---

## Who is this for?

Grid OS and the GridBASIC IDE are for **anyone curious enough to try**:

- **Beginners** who want to learn programming without installing ten tools first — type, press Esc, run.
- **Retro computing fans** who miss when the machine felt like *yours*, not a rental from a cloud.
- **Tinkerers** who enjoy sandboxes, identity discs, TCP servers, IRC bots, and Flynn-themed fun.
- **Students and teachers** who need a small, readable system to explain how an OS actually works.
- **Developers** who want to shape an OS from the kernel up — or from a BASIC line outward.

You do **not** need to be a kernel hacker to enjoy it. You **do** need patience, curiosity, and a willingness to read `:help` once in a while.

If you want a polished everyday laptop replacement for email, streaming, and banking — Grid OS is **not there yet**, and we will not pretend otherwise. If you want a place to **grow** toward that — or toward something weirder and better — you are in the right Grid.

---

## Why is it fun?

Because the whole machine feels like a **game you can edit**.

- **Boot straight into the IDE** — no hunting for an editor app. The Grid opens on code.
- **Esc is a magic door** — `:run`, `:save`, `:mods`, `:server new`, `:ircserver new`, or drop into the full `grid>` shell without leaving the editor.
- **GridBASIC is immediate** — PRINT, loops, SUB/FUNCTION, `GRID.PING`, `GRID.HTTP`, vault, packages, plot, beep. Results in seconds.
- **Ring-3 sandboxes** — spawn `gridsh`, `lightcycle`, or your own programs in isolated sandboxes with identity discs. Tron energy, real W^X enforcement.
- **Packages and modules** — 30 seeded IDE tools (`pkg mods`), plus two package bundles you can extend. Run `disc-status`, `grid-ping`, or write your own module and install it.
- **Network on the Grid** — ping the gateway, HTTP probes, IRC client *and* server, custom TCP command servers with your own keywords.
- **Host bridges** — optional AI and Bitcoin bridges on the real machine; the Grid talks to the outside world when you wire it up.

It is fun the way a **workshop** is fun: sawdust, sparks, something you built that actually runs.

---

## Why is it useful?

Even today — before it becomes anyone's daily driver — Grid OS is useful as:

| Use | What you get |
|-----|----------------|
| **Learn OS concepts** | Boot loader, paging, syscalls, sandboxes, TCP, DNS, disk — all small enough to read. |
| **Learn programming** | GridBASIC from hello world to subroutines, bytecode, and `GRID.*` system calls. |
| **Prototype ideas fast** | Scripts, servers, and modules without a heavyweight toolchain. |
| **Teach** | One ISO/QEMU session; every student sees the same Flynn disk and IDE. |
| **Experiment safely** | Sandboxed programs, capability tokens, audit log — fail closed, not fail mysteriously. |

The IDE is not a bolt-on text editor sitting on top of a foreign OS. It is **wired into** Grid OS: the same keyboard, the same disk, the same network, the same packages. That unity is the point.

---

## The potential: one system, not two

We believe the future of personal computing is not "an OS **plus** an IDE **plus** a browser **plus** a app store you do not control."

The potential of Grid OS is a **true user operating system and IDE mixed into one**:

- **You live in the editor** — but the editor can reach the shell, the disk, the network, and ring-3 programs.
- **Your programs are first-class** — saved on Flynn disk, packaged, shared, run from `:run` or `spawn`.
- **The system stays small enough to understand** — so users can *own* it, not just lease it.
- **Emergence is welcome** — packages, modules, bridges, ISO research, community extensions. CLU wanted perfection; Flynn wanted a frontier.

We are not there yet. Grid OS 7.x is a **hobby OS**, a **proof of spirit**, and a **call for builders**. But every commit — kernel fix, new module, better `:help`, clearer docs — moves the Grid closer to something a real person could wake up and use, not only admire in QEMU.

---

## An open invitation

**Anyone and everyone is invited to expand Grid OS.**

You do not need permission. Fork the repo, open a pull request, or send a patch. Useful places to help:

- **GridBASIC IDE** — editor UX, colon commands, tutorials, accessibility, themes.
- **Modules & packages** — new `flynn-ide-tools` scripts, `gen_packages.py` entries, cookbook examples.
- **User-facing OS** — shell polish, desktop/workbench, better defaults, "it just works" boot paths.
- **Kernel & drivers** — storage, network, input, real hardware beyond QEMU.
- **Docs & teaching** — guides, videos, translations, classroom labs.
- **Bridges & connectivity** — AI, web, sync, whatever connects the Grid to human life without selling the user out.

Our north star: **a personal operating system where ordinary people can use the machine and shape the machine in the same breath** — IDE and OS as one, open source, Flynn's Grid not CLU's cage.

If that excites you, welcome. End of line is optional.

---

## Where to go next

| Doc | Purpose |
|-----|---------|
| [GETTING_STARTED.md](GETTING_STARTED.md) | Install, boot, first program |
| [wiki/getting-started.md](wiki/getting-started.md) | IDE modes, samples, modules |
| [wiki/README.md](wiki/README.md) | Full GridBASIC IDE encyclopedia |
| [PACKAGES.md](PACKAGES.md) | Packages, modules, MANIFEST format |
| [VISION.md](VISION.md) | Security philosophy and Flynn's intent |
| [Cookbook](wiki/cookbook.md) | Recipes and examples |

```bash
git clone https://github.com/17060/grid-os.git
cd grid-os
make disk seed-disk
make run
# You are in the GridBASIC IDE. Press Esc. Type :help.
```

**Help us build the Grid.**
