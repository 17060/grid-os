# Lab 2 — Memory safety: bounded arrays

**Concept:** integer overflow turning a bounds check into an out-of-bounds write.
**Code:** `kernel/basic.c` (`exec_dim`).
**Verify:** `make lab-check LAB=2` · **Claim:** `labs done 2`

---

## The idea

A kernel can't trust sizes it's handed. When GridBASIC runs `DIM A(rows, cols)`,
it must allocate `(rows+1) * (cols+1)` cells. If that multiplication is done in a
32-bit `int`, a large program can make it **overflow and wrap to a small value**
— so the kernel allocates a tiny buffer but records huge bounds, and any later
`A(i,j)` writes far outside the array. This is one of the most common real-world
memory-corruption bugs, and it hides *inside* a check that looks correct.

## Read it

Open `kernel/basic.c` and find `exec_dim`. The cell count is computed in 64-bit
and capped against the pool:

```c
long long count = (dim2 >= 0)
    ? ((long long)dim + 1) * ((long long)dim2 + 1)
    : ((long long)dim + 1);
long long pool = (long long)(sizeof(g_array_pool) / sizeof(g_array_pool[0]));
if (count < 1 || count > pool) {
    set_error("DIM: array too large");
    return;
}
```

Note the deliberate `(long long)` casts *before* the `+1` and the `*` — that's
what keeps the arithmetic from overflowing.

## Break it

Replace the 64-bit computation with the naive 32-bit version:

```c
int count = (dim2 >= 0) ? (dim + 1) * (dim2 + 1) : (dim + 1);   // BROKEN
if (count < 1) { set_error("DIM: array too large"); return; }
```

Rebuild and reason: `DIM A(65535,65535)` computes `65536 * 65536` = 2³² in a
32-bit `int`, which **wraps to 0**. The check `count < 1` fires — or, with a
value that wraps to something small-but-positive, the allocation succeeds tiny
while `A(i,j)` still indexes as if the array were huge. Either way the invariant
"allocation size matches the declared bounds" is broken.

Run the check:

```
make lab-check LAB=2
```

The interpreter host test (`tools/basic_host_test.c`) runs
`DIM A(65535,65535)` and asserts it's rejected with "array too large". Your
broken version wraps and mis-handles it, so the assertion fails.

## Fix it

Restore the 64-bit computation and cap, and `make lab-check LAB=2` goes green.

## Going deeper (optional)

The same "add before the compare wraps" trap appears in several places this
codebase had to fix — the ELF loader's `p_offset + p_filesz > size` (Lab 3) and
the DMA allocator's `aligned + size > pool` (`kernel/memory.c`). The safe idiom
is always to **subtract instead of add**: `filesz > size - offset`. Find one and
confirm it uses that form. Why is subtraction safe where addition isn't?

## Claim it

```
labs done 2
```

+40 disc XP. You've seen how a checked bound can still overflow.
