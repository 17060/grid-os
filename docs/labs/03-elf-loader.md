# Lab 3 — The ELF program loader

**Concept:** how an executable file becomes a running process; W^X.
**Code:** `kernel/elf.c` (`elf_load`), `kernel/memory.c` (`memory_map_user_segment`).
**Verify:** `make lab-check LAB=3` · **Claim:** `labs done 3`

---

## The idea

A program on disk is an **ELF file**: a header, then a table of *program
headers*, each describing a `PT_LOAD` segment — a chunk of the file to place at
some virtual address with some permissions (read / write / execute). To "run" a
program, the loader walks that table and maps each segment into a fresh address
space with exactly the right permissions, then jumps to the entry point.

Permissions matter for security. Grid OS enforces **W^X** ("write xor execute"):
a page is writable *or* executable, never both, so a program can't rewrite its
own code and run it. The loader translates each segment's flags into page-table
bits.

## Read it

Open `kernel/elf.c` and find the segment loop in `elf_load`:

```c
int writable   = (ph->p_flags & PF_W) ? 1 : 0;
int executable = (ph->p_flags & PF_X) ? 1 : 0;
const void *segment = base + ph->p_offset;
if (memory_map_user_segment(pml4, ph->p_vaddr, segment,
                            (size_t)ph->p_filesz, (size_t)ph->p_memsz,
                            writable, executable) != 0) {
    return -1;
}
```

`executable` comes straight from the segment's `PF_X` flag. The code segment has
it set; that's what lets the CPU fetch instructions from it.

## Break it

Force every segment to be non-executable:

```c
int executable = 0;   // BROKEN: even the code segment can't be executed
```

Rebuild and reason: the `.text` segment now maps as a **no-execute** page. Grid
OS enables the NX bit (see `enable_nxe` in `memory.c`), so when a spawned program
jumps to its entry point, the very first instruction fetch hits an NX page and
the CPU raises a **page fault** — the sandbox reporting "Program fault (W^X
enforced)".

Run the check:

```
make lab-check LAB=3
```

The end-to-end test boots the OS and does `spawn gridsh`, which loads `gridsh`
from disk through `elf_load`. With execute stripped, gridsh faults instead of
running, and the test's "unexpected program fault" guard trips.

## Fix it

Restore `executable = (ph->p_flags & PF_X) ? 1 : 0;` and the e2e test passes.

## Going deeper (optional)

The nastiest real bug this loader ever had was in the ELF *header* struct: it was
missing three fields (`e_shoff`, `e_flags`, `e_ehsize`), so `e_phnum` (the
segment count) was read from the wrong offset and came out **0** — the loop ran
zero times, mapped nothing, and every disk program faulted at its entry. It
survived under clang but hung the CI under gcc. Look at `elf64_ehdr_t` in
`elf.c` and compare it to the real ELF-64 header layout. Why did `e_entry` still
read correctly while `e_phnum` didn't?

## Claim it

```
labs done 3
```

+40 disc XP. You understand how a file becomes a process.
