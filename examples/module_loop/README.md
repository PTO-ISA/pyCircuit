# Two independent modules with feedback

`child.py` defines one Boolean DFFE module. `module_loop.py` instantiates that
same definition twice, with independent enable, data and reset-value pins.
Each enabled rising edge stores `old_q ^ data`. This uses the current typed
module/rule pins and `dffe(T=bool)`; it introduces no source collection API.

Compile the child independently, compile the parent against its published
interface, then link the explicit two-unit closure with top
`example_loop.module_loop.Top`. Both backends must consume that same final
artifact. The build entry and orchestration are maintained with the toolchain.

The independent C++ driver first uses `pyc_dut`, `SimExecutor` and pin-level
Steps for the fault/recovery self-test. It compares one worker and two workers
against this literal Work-snapshot trace:

```text
(0,1), (0,1), (1,1), (1,1), (1,0),
(0,1), (0,1), (1,1), (1,1), (0,1)
```

The first five rows cover initialization, XOR feedback and an independent hold.
The next five cover recovery/reset replay and clocked reset while both enables
are low. Work reads old Q; a transfer becomes visible in the following Work.
No `Eval` or implicit refresh is used.

At a rising sample, an unknown right enable injects a Work failure after the
left module is eligible to prepare. The driver requires no epoch advancement,
no successful output sample and a latched failed executor until Reset. Separate
runtime executor tests must inspect leaf state and pending clock history to
prove whole-tree zero commit; Reset replay alone cannot establish that claim.
`sample()` must reject before the first successful epoch and after failure.

The driver then constructs a fresh DUT using `SystemRunner --workers`, drives
pins through the host callbacks, and records eleven successful Work snapshots:

```text
(0,1), (0,1), (1,1), (1,1), (1,0), (1,0),
(0,1), (0,1), (1,1), (1,1), (0,1)
```

The extra `(1,0)` is the old-Q snapshot at the first clocked reset. The
initialization callback drives pins before Reset; the final drive returns false
before another Step. The RTL testbench uses exactly those eleven input frames,
establishes reset state outside the trace, captures old Q before changing clock,
and prints the same `WORK left right` records. It does not claim host-worker parallelism or rollback for externally
clocked RTL. Its stimulus and expected values are independent of emitter text.

The example is accepted only after real source compilation, full link, both
module builds, C++ serial/two-worker execution and RTL execution. Source files,
a test name or this README do not establish passing results. `config.json`
provides a finite 32-sample bound; source assert/log/report are outside this
first module-runner packet and must not be silently discarded.
