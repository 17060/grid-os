# Grid OS — OS internals labs

A hands-on curriculum for learning how an operating system actually works, using
Grid OS's own kernel as the textbook. You don't read *about* a scheduler — you
open `kernel/sched.c`, break it, watch it starve a job, fix it, and prove the
fix with a test.

Each completed lab grants **disc XP**, persisted in the vault, so learning the
internals advances your Grid identity (see the rank ladder below). It's OS
education as a Tron progression.

## How a lab works — break it, observe, fix it, verify

The shipped code is already correct, so every lab uses the same loop that real
kernel engineers use:

1. **Read** the concept and the code (each lab points at exact files/functions).
2. **Break it** — make a specific change the lab describes, rebuild (`make`),
   and **observe the symptom** yourself (a test failing, or wrong behavior in
   the running OS). This is the part that teaches.
3. **Fix it** — restore the correct code.
4. **Verify** — run the lab's check on the host:
   ```
   make lab-check LAB=<n>
   ```
5. **Claim** — inside Grid OS (boot with `make run`, press Esc for the `grid>`
   prompt):
   ```
   labs done <n>
   ```
   You earn disc XP; `labs` shows your progress and rank.

You can see the whole curriculum from inside the OS at any time by typing
`labs`.

## The labs

| # | Lab | Concept | File |
|---|-----|---------|------|
| 1 | [Scheduler: round-robin & fairness](01-scheduler.md) | preemption, quanta, starvation | `kernel/sched.c` |
| 2 | [Memory safety: bounded arrays](02-memory-safety.md) | integer overflow → OOB write | `kernel/basic.c` |
| 3 | [The ELF program loader](03-elf-loader.md) | program headers, W^X, entry point | `kernel/elf.c` |
| 4 | [The virtio-blk disk driver](04-virtio-blk.md) | vrings, DMA, async completion | `kernel/virtio_blk.c` |

Do them in order — each builds intuition for the next. Every one of these is
based on a **real bug** that lived in this codebase, so you're not doing
toy exercises: you're learning the exact traps that catch working kernels.

## Rank ladder

Disc XP accumulates and levels you up (level *n* needs *n×100* XP). Each lab is
worth **40 XP**. Your entity promotes as you climb:

- **User** — levels 1–9 (where everyone starts)
- **Program** — level 10+ (you understand the machine)
- higher tiers await…

Check your rank anytime with `labs` or `disc`.

## Prerequisites

Read [../ARCHITECTURE.md](../ARCHITECTURE.md) first — it's the map of the whole
system, and each lab assumes you know roughly where its subsystem sits. Then
`make run`, type `help`, and poke around before you start breaking things.
