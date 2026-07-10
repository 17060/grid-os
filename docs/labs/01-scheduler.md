# Lab 1 — Scheduler: round-robin & fairness

**Concept:** preemptive multitasking, time quanta, and starvation.
**Code:** `kernel/sched.c` (`sched_service`, `sched_on_timer`).
**Verify:** `make lab-check LAB=1` · **Claim:** `labs done 1`

---

## The idea

Grid OS runs background jobs (`spawn bg <name>`) with **preemption**: a timer
interrupt periodically stops the running program and lets the kernel pick
another. Two questions any scheduler must answer:

1. **Which job runs next?** Grid OS uses **round-robin**: a cursor (`next_run`)
   remembers where it stopped and resumes *past* it, so jobs take turns.
2. **How long does each run?** For a **quantum** — `SCHED_QUANTUM` timer ticks —
   after which `sched_on_timer` flags a preempt.

Get either wrong and you get **starvation**: one job hogs the CPU forever while
others never run.

## Read it

Open `kernel/sched.c` and find `sched_service()`. Notice the scan:

```c
for (int k = 0; k < SCHED_JOBS_MAX; ++k) {
    int i = (next_run + k) % SCHED_JOBS_MAX;   // <-- round-robin cursor
    ...
    next_run = (i + 1) % SCHED_JOBS_MAX;        // advance past the slot we ran
    program_run(jobs[i].program_id);
    return;
}
```

## Break it

Change the cursor line to a plain flat scan that always starts at slot 0:

```c
    int i = k;   // BROKEN: always re-scans from slot 0
```

Rebuild (`make`) and reason about it: `sched_service` is called each idle turn.
With a flat scan it always finds the *lowest-index active job* first and runs
that one — so the job in slot 0 runs every single time, and any higher slot
**never gets the CPU until slot 0 exits.** That's starvation.

Now run the check and watch it fail:

```
make lab-check LAB=1
```

The scheduler's host unit test (`tools/sched_host_test.c`) launches three jobs
and asserts the dispatch order is `1,2,3,1,2,3,…`. With your broken scan it gets
`1,1,1,…` and reports:

```
dispatch[1]=1 want 2 (old flat scan re-picks slot 0 -> starvation)
```

## Fix it

Restore the round-robin cursor (`int i = (next_run + k) % SCHED_JOBS_MAX;`) and
run `make lab-check LAB=1` again — green.

## Going deeper (optional)

Look at `sched_on_timer()`. It checks whether the **running** job's slice has
elapsed (using `running_slot`). An earlier version checked *every* active job,
so an idle job's stale clock would cut the running job's turn short — unfair, but
not starving. Try reverting that to `for (int i...) { ... jobs[i].slice_start ... }`
and the second half of the host test (quantum fairness) fails. Why does the
running job's slice, and only its slice, decide the quantum?

## Claim it

Once `make lab-check LAB=1` passes, boot the OS (`make run`, press Esc):

```
labs done 1
```

+40 disc XP. You now understand preemption, quanta, and starvation.
