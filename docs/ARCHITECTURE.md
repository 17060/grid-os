# How Grid OS Works — an architecture guide

This is a guided tour of Grid OS's internals: how a from-scratch x86-64
operating system boots, isolates programs, schedules them, talks to a disk and
a network, and runs a BASIC interpreter — all in about 20,000 lines of C and a
little assembly. It's written to be read **alongside the code**; every section
names the files and functions it describes so you can jump in and follow along.

If you're new, read [GETTING_STARTED.md](GETTING_STARTED.md) first (how to build
and boot), then come back here to understand *why* it works.

---

## 1. The mental model: turtles all the way down

Grid OS runs inside **QEMU**, an emulator, on your real machine. That gives you
three nested layers, and it helps to keep them straight:

```
  Your real computer (macOS / Linux / Windows)
    └── qemu-system-x86_64        ← an ordinary application, emulating a PC
          └── Grid OS kernel      ← ring 0 (supervisor) INSIDE the emulated PC
                └── gridsh, gridloop, GridBASIC programs  ← ring 3 (user)
```

The x86 privilege rings, the MMU, the interrupts, the "disk", the "network
card" — everything Grid OS talks to — are things **QEMU pretends to be**. When
this guide says "the CPU raises a page fault" or "the disk completes a read,"
that's QEMU faithfully emulating what real hardware would do. Grid OS is a real
OS in every sense *except* that its hardware is virtual.

Two rings matter inside Grid OS:

- **Ring 0 (kernel):** full control — sets up page tables, handles interrupts,
  drives devices. All the `kernel/*.c` code runs here.
- **Ring 3 (user):** sandboxed programs (`user/*.c`, compiled to ELF binaries).
  They can't touch hardware or kernel memory; they ask the kernel for
  everything through *syscalls*.

---

## 2. Boot: from power-on to the shell

**Files:** `boot/boot.s`, `linker.ld`, `kernel/kernel.c`

QEMU loads the kernel and jumps to `boot/boot.s` (the multiboot entry). In
assembly, before any C can run, the boot code:

1. **Sets up paging and enters 64-bit long mode.** It builds an initial set of
   page tables (`boot_pml4`/`boot_pdpt`/`boot_pd`) that identity-map the low
   **1 GiB** of RAM using 2 MiB huge pages (`pd[i] = i * 2MB | present | writable
   | page-size`). "Identity map" means virtual address == physical address, so
   the kernel can use pointers directly.
2. **Sets up the stack.** It reserves a **2 MiB** kernel stack (`stack_bottom`
   … `stack_top`) and points `rsp` at the top. (This is bigger than you'd
   expect for a hobby OS — see §4 for why.)
3. **Jumps to `kernel_main`** in `kernel/kernel.c`.

`kernel_main` is the readable heart of boot — it's just a list of init calls, in
order:

```c
serial_init();      // COM1, so we can print before the screen is ready
console_init();     // 80x25 VGA text console
gdt_init();         // segment descriptors + the TSS (for ring transitions)
memory_init();      // full page tables, the DMA pool, user memory pools
idt_init();         // interrupt descriptor table (traps, IRQs, syscalls)
timer_init();       // the PIT — drives preemption
...
disk_init();        // probe virtio-blk / IDE, pick a backend
gfs_init();         // mount the on-disk filesystem
__asm__("sti");     // interrupts ON — the system is now live
...
shell_run();        // hand control to the Grid shell (never returns)
```

Reading `kernel_main` top to bottom is the single best way to see how the whole
system fits together.

---

## 3. The console and serial: how the OS talks to you

**Files:** `kernel/console.c`, `kernel/serial.c`

Grid OS has two output paths, and it uses both:

- **VGA text console** — writes characters directly into video memory at
  physical `0xB8000` (the classic 80×25 text buffer; each cell is a character
  byte + an attribute byte for color). `console.c` implements scrolling, the
  cursor, and colored output.
- **Serial (COM1)** — `serial.c` writes to the emulated UART at I/O port
  `0x3F8`. This is invaluable: it works *before* the console is up and it's what
  the automated tests read (QEMU's `-serial stdio` pipes it to your terminal).

Input is **polled**, not interrupt-driven: the shell's read loop repeatedly
checks the PS/2 keyboard port (`ps2_has_data`) and the serial port. Crucially,
while it waits for a keypress with nothing to do, `console_idle()` runs
`sched_service()` — that's how background jobs get CPU time (see §6).

> Note: `serial.c` and `console.c` are deliberately compiled at `-O0`. At `-O2`
> GCC inlines their output loops and mis-optimizes past a string's NUL
> terminator, spewing megabytes down the wire. A good reminder that
> freestanding kernel code and aggressive optimizers can disagree.

---

## 4. Memory: page tables, pools, and W^X

**Files:** `kernel/memory.c`, `kernel/include/memory.h`

Grid OS does **not** have a general-purpose heap (no `malloc`). Instead it uses
a few fixed pools, which makes memory usage exact and easy to reason about — you
can see it live with the **`meminfo`** shell command.

- **DMA pool** (`dma_pool`, 128 KiB): a simple *bump allocator*
  (`memory_dma_alloc`) for buffers the emulated devices read/write — the virtio
  rings and request buffers. It never frees; it just hands out the next aligned
  chunk. Its bounds check is written overflow-safe (`size > total - aligned`)
  so a huge request can't wrap past the guard.
- **User page-table pool** (`user_pt_pool`, 24 pages) and **user data pool**
  (`user_data_pool`, 16 pages): pages for ring-3 programs' page tables and their
  code/stack. Each page is *owner-tagged* with the program's id, so when a
  program exits `memory_release_user` reclaims its pages — repeated spawns don't
  leak.

**Per-program address spaces.** `memory_create_user_tables` builds a fresh PML4
for each program. `memory_map_user_segment` maps a program's segments with
explicit permissions and enforces **W^X** ("write xor execute"): a page can be
writable *or* executable, never both. Try to map a `W|X` segment and it's
rejected. This is a core security property — a program can't rewrite its own
code and jump to it.

**Why the 2 MiB stack?** GridBASIC's expression evaluator is recursive, and its
`value_t` (a ~1 KiB string buffer) is returned *by value* through the eval
chain — roughly 18 KiB of stack per nested expression level. Bare metal has no
stack guard page, so a stack overflow silently corrupts the BSS below it. The
big stack plus a recursion-depth cap (`MAX_EVAL_DEPTH`, see §11) keep that safe.

---

## 5. Programs and the ring-3 sandbox

**Files:** `kernel/program.c`, `kernel/elf.c`, `kernel/syscall.c`,
`boot/interrupts.s`, `user/*.c`, `user/linker.ld`

User programs (`gridsh`, `gridloop`, …) are ordinary C compiled to **ELF64**
executables that run in ring 3. There are two ways they reach the kernel:

**Loading.** `program_spawn_named` tries the disk first (`program_spawn_from_disk`
reads `/programs/<name>` off GFS) and falls back to a copy compiled into the
kernel. Disk programs go through `elf_load` (`kernel/elf.c`), which parses the
ELF program headers and maps each `PT_LOAD` segment at its virtual address with
its flags. User programs are linked (`user/linker.ld`) so each segment sits on
its own page — otherwise a read-only segment sharing a page with the code
segment would strip its execute bit.

**Entering ring 3.** `program_enter` calls `enter_usermode` (in
`boot/interrupts.s`), which loads the program's CR3 (its page tables), pushes an
`iret` frame with the user code/stack selectors and `RFLAGS` (interrupts
enabled), and `iretq`s down to ring 3. Control returns to the kernel only on a
**syscall**, a **timer preempt**, or a **fault**.

**Syscalls.** `kernel/syscall.c` is the boundary: a ring-3 program raises the
syscall interrupt with a number and arguments, and the kernel dispatches
(`SYS_WRITE`, `SYS_READ_LINE`, `SYS_EXIT`, filesystem, vault, …). This is where
user requests become kernel actions.

**See it for yourself.** `spawn escape` runs a deliberately malicious program
(`user/escape.c`) that tries to write to kernel memory. Kernel pages are mapped
supervisor-only, so the write faults, the kernel kills the program, and its
"sandbox FAILED" line never prints — memory protection, live. And `syscalls`
shows the calls any program made across that boundary.

---

## 6. The scheduler: preemptive, round-robin, fair

**Files:** `kernel/sched.c`, `boot/interrupts.s` (`isr_timer`), `kernel/timer.c`

Grid OS multitasks background jobs (`spawn bg <name>`) with **preemption**:

- Each dispatch, `sched_service()` picks the next runnable job using a
  **round-robin cursor** (`next_run`) — it resumes scanning just past the slot
  it ran last time, so jobs take turns instead of the lowest slot monopolizing
  the CPU.
- It records which slot is running (`running_slot`) and runs it via
  `program_run` → `program_enter`.
- On every timer tick, `sched_on_timer()` checks whether the **running** job has
  used its quantum (`SCHED_QUANTUM` ticks). If so it sets a flag, and the timer
  ISR (`isr_timer`) saves the job's full register context and returns control to
  the kernel — which lets `sched_service` pick the next job on the following
  idle turn.

A job shorter than one quantum runs to completion without ever being preempted
(correct — nothing to interrupt). Two long-running jobs genuinely time-share and
finish together. The whole scheduler is covered by a host unit test
(`tools/sched_host_test.c`, run by `make test-host`) that asserts round-robin
order and quantum fairness — so the behavior can't silently regress.

---

## 7. Interrupts

**Files:** `kernel/idt.c`, `boot/interrupts.s`, `kernel/timer.c`

`idt_init` fills the Interrupt Descriptor Table with handlers whose assembly
stubs live in `boot/interrupts.s`. The interesting ones:

- **Timer (IRQ0):** `isr_timer` advances the tick counter, runs periodic work,
  and — if a user program's quantum expired — preempts it (§6). It carefully
  switches to the kernel's CR3 on entry, because a running program's page tables
  only map the low 4 MiB of the kernel.
- **Page fault (#14):** `idt_handle_page_fault`. When a ring-3 program touches
  memory it isn't allowed to, this fires, the program is marked faulted, and
  control returns to the kernel — the sandbox catching an escape attempt.
- **Syscall:** the software-interrupt entry that lands in `syscall.c` (§5).

---

## 8. The filesystem: GFS ("Flynn disk")

**Files:** `kernel/gfs.c`, `tools/gfs_common.py`, `tools/gfs_seed.py`

GFS is a deliberately simple on-disk filesystem so its layout is easy to learn:

```
  LBA 64        superblock  (magic "GFS2FLYN", version, geometry, checksum)
  LBA 65…       inode table (each inode: valid flag, size, path)
  LBA 128…      file data    (each file gets a fixed run of sectors)
```

`gfs_init` reads the superblock, validates its magic/version/checksum, loads the
inode table, and marks the disk mounted. Reads and writes are done in whole
512-byte sectors through the `disk_*` layer. The kernel constants in `gfs.c`
must match the seed tool (`tools/gfs_common.py`) exactly — that's how a disk
image written on your host is readable by the guest.

A subtlety worth internalizing: because every transfer is a full sector, a
buffer handed to `disk_read` must be **at least 512 bytes**. Reading a
28-byte superblock struct directly would overflow the caller's stack — GFS uses
a `union { superblock; uint8_t[512]; }` to get a sector-sized, aligned buffer.

Watch it live: **`gfs`** lists the filesystem; **`ls /programs`**, **`cat`**,
and **`basic run /programs/hello.bas`** all read through this layer.

---

## 9. The disk driver: virtio-blk

**Files:** `kernel/virtio_blk.c`, `kernel/disk.c`

`disk_init` probes for a **virtio-blk** device (QEMU's paravirtual disk) and
falls back to legacy IDE PIO. Virtio is the most instructive driver in the tree
because it shows how a guest and a host device cooperate through shared memory:

- A **virtqueue** is a ring of *descriptors* (each pointing at a buffer), an
  *available* ring (guest → device: "here's a request"), and a *used* ring
  (device → guest: "here's the completion"). All of it lives in the DMA pool.
- To read a sector, the driver fills three descriptors (request header, data
  buffer, status byte), publishes them in the available ring, and *notifies* the
  device. The device (QEMU) processes the request **asynchronously** and posts a
  completion in the used ring; the driver polls `used->idx` until it appears,
  then copies the data back to the caller.

Getting this exactly right is famously fiddly, and the code carries hard-won
comments about it: `used->idx` must be read `volatile` (or the optimizer hoists
the poll), the port-I/O helpers need a `"memory"` clobber (or ring writes get
reordered past the notify), a polling driver must set the PCI *Interrupt
Disable* bit (or the unacknowledged interrupt storms), each request runs under
`cli`/`sti` (the timer ISR must not reenter the single request slot), and a
premature poll timeout must resync to the device's index (or the ring desyncs
for the rest of the session). Every one of those is a real bug that was found
and fixed — the comments explain the "why" so the next reader doesn't reintroduce
them.

---

## 10. Networking

**Files:** `kernel/net.c`, `kernel/tcp.c`, `kernel/dns.c`, `kernel/http.c`,
`kernel/irc.c`

Grid OS has a hand-written network stack over a **virtio-net** device, layered
the way a textbook draws it:

```
  Ethernet / ARP  (net.c)
    └── IPv4 / ICMP / UDP / TCP   (net.c, tcp.c)
          ├── DNS resolver         (dns.c)
          ├── HTTP/1.1 client      (http.c)
          └── IRC client           (irc.c)
```

`net_poll` pulls received frames from the virtio-net used ring and dispatches by
type. TCP maintains up to 8 concurrent connections, so IRC, HTTP, and the AI/BTC
bridges can run at once. From the shell: `net ping gateway`, `http get …`, and
`irc connect …`. The AI and Bitcoin features connect to *host bridges* you start
with `make ai-bridge` / `make btc-bridge`, with offline fallbacks when they're
not running.

---

## 11. GridBASIC: the language and IDE

**Files:** `kernel/basic.c`, `kernel/basic_pp.c`, `kernel/basic_ide.c`

GridBASIC is a complete interpreter — arguably the point of the whole OS. Its
pipeline:

1. **Tokenize** the source into `g_tokens` (a lexer).
2. **Execute** statements one at a time, driven by a token cursor (`g_cur`).
   Control flow (`FOR`/`WHILE`/`GOSUB`/`SUB`) uses frame stacks and a line →
   token index table for jumps.
3. **Evaluate expressions** with a classic *recursive-descent* chain:
   `eval_expr → eval_or → … → eval_primary`, where `eval_primary` handles
   literals, variables, parenthesised sub-expressions, and function calls, and
   recurses back to the top for nested expressions.

Values are a tagged union (`value_t`: a number *or* a 1 KiB string). Because
that struct is passed by value through the eval chain, deep nesting is
expensive — hence the `MAX_EVAL_DEPTH` recursion cap and the 2 MiB stack (§4).

`basic_pp.c` is a small preprocessor (`#IF`/`#INCLUDE`), and `basic_ide.c` is the
full-screen editor with the `grid>` colon-command bar. Programs reach the rest
of the OS through ~85 `GRID.*` bindings — `GRID.PLOT`, `GRID.HTTP.GET$`,
`GRID.VAULT.PUT`, `GRID.SPAWN.BG`, and so on — which is how a BASIC program can
draw graphics, make HTTP requests, or launch background jobs.

---

## 12. Observability and testing: how to *see* it work

You can't learn a system you can't observe. Grid OS exposes its state through
the shell and guards its behavior with tests.

**Look at live state:**

| Command   | Shows |
|-----------|-------|
| `meminfo` | DMA pool + user page pools, used vs total |
| `jobs`    | background jobs and their scheduler state |
| `gfs`     | on-disk filesystem contents |
| `cycles` / `status` | runtime status, disk backend, mount state |
| `about`   | build + feature summary |

**Tests (`make test-host`, `make test-e2e`):**

- **Host unit tests** (`tools/*_host_test.c`) compile individual kernel modules
  against small stubs and run assertions on the host — fast and deterministic.
  They cover the BASIC interpreter, the preprocessor, the vault, TCP/net, spawn
  handling, and the scheduler.
- **End-to-end test** boots the real kernel in QEMU, drives it over serial, runs
  the interpreter self-test, spawns a ring-3 program, and powers off — proving
  the whole thing works together.
- **CI** (`.github/workflows/test.yml`) runs all of this on every push, so the
  green badge on the README means "this builds and boots right now."

---

## 13. A reading order for newcomers

If you want to actually understand the code, read it in this order:

1. `kernel/kernel.c` — `kernel_main`, the whole system as a list of init calls.
2. `boot/boot.s` — long mode, page tables, the stack.
3. `kernel/console.c` / `kernel/serial.c` — the simplest real drivers.
4. `kernel/memory.c` — page tables and the pools (run `meminfo` alongside).
5. `kernel/sched.c` + `boot/interrupts.s` (`isr_timer`) — preemptive scheduling.
6. `kernel/program.c` + `kernel/elf.c` + `kernel/syscall.c` — the ring-3 sandbox.
7. `kernel/gfs.c` + `kernel/virtio_blk.c` — filesystem and a paravirtual driver.
8. `kernel/basic.c` — the interpreter (big, but the eval chain is the core).

Each of these is a clean, self-contained example of a fundamental OS concept.

---

## 14. Honest limitations (and why they're fine here)

Grid OS is a **teaching and hobby OS**, and it's honest about that:

- It runs in an **emulator**, not on real hardware — no drivers for real GPUs,
  USB, Wi-Fi, etc.
- The console is 80×25 text (there's an experimental 4K framebuffer path).
- The security model is single-user with a capability *concept*, not a hardened
  multi-user system; the network stack does not validate TCP checksums/sequence
  numbers or DNS responses (documented, and a good exercise to harden).
- There's no self-hosting toolchain — you build it from your host.

None of that detracts from the goal: Grid OS is a place to *read, run, break, and
learn how an operating system actually works*, end to end, in a codebase small
enough to hold in your head. Start it with `make run`, type `help`, and poke
around — then open the files above and see how each command reaches down through
the layers.
