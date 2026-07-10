# Lab 4 — The virtio-blk disk driver

**Concept:** how a guest OS and a virtual device cooperate through shared memory.
**Code:** `kernel/virtio_blk.c` (`submit_request`, `wait_used`).
**Verify:** `make lab-check LAB=4` · **Claim:** `labs done 4`

---

## The idea

Grid OS reads its disk from a **virtio-blk** device — QEMU's paravirtual disk.
Guest and device talk through a **virtqueue** in shared memory: a ring of
*descriptors* (pointers to buffers), an *available* ring (guest says "here's a
request"), and a *used* ring (device says "here's the completion"). To read a
sector the driver:

1. fills descriptors for a request header, a **data buffer**, and a status byte,
2. publishes them and notifies the device,
3. polls the used ring until the device posts the completion, then
4. **copies the data the device wrote into the caller's buffer.**

Step 4 is easy to forget — and if you do, every read silently returns garbage.

## Read it

Open `kernel/virtio_blk.c` and find the end of `submit_request`:

```c
int rc = wait_used(0);
/* On a read the device fills blk.data_buf; copy it back into the caller's
 * buffer. Without this, virtio_blk_read() returned success while leaving the
 * caller's buffer untouched, so the on-disk GFS superblock read as garbage
 * and the disk never mounted. */
if (rc == 0 && !write) {
    for (size_t i = 0; i < VIRTIO_SECTOR_SIZE; ++i) {
        ((uint8_t *)buffer)[i] = blk.data_buf[i];
    }
}
return rc;
```

The device writes into the driver's own `blk.data_buf`; the caller passed a
*different* buffer. That copy is the bridge between them.

## Break it

Delete the copy-back:

```c
int rc = wait_used(0);
/* (copy-back removed) */
return rc;
```

Rebuild and reason: `virtio_blk_read` now returns success but leaves the caller's
buffer holding whatever was there before. The first thing the OS reads is the
GFS **superblock** (`gfs_init`); it reads garbage, its magic/checksum don't
match, and the filesystem never mounts — so `/programs/*` becomes unreadable.

Run the check:

```
make lab-check LAB=4
```

The end-to-end test boots and depends on the mounted disk (it spawns `gridsh`
and reads package data). With reads returning garbage, the boot's disk-backed
steps fail.

## Fix it

Restore the copy-back loop and the e2e test passes.

## Going deeper (optional)

This driver is a minefield of shared-memory subtleties, and the comments in
`wait_used` / `submit_request` document each one: `used->idx` must be read
`volatile` (or the optimizer hoists the poll and spins on a stale value), the
port-I/O helpers need a `"memory"` clobber (or ring writes get reordered past
the device notify), a polling driver must set the PCI *Interrupt Disable* bit
(or the unacknowledged interrupt storms the CPU), each request runs under
`cli`/`sti` (the timer must not reenter the single request slot), and a premature
poll timeout must resync to the device's index (or the ring desyncs for the rest
of the session). Pick one, remove it, and see what breaks — every one is a real
bug that took real debugging to find.

## Claim it

```
labs done 4
```

+40 disc XP. You've seen how a guest driver and a virtual device shake hands.
