# pyCircuit v0.11 — Second Design Class: Synchronous FIFO

## Why FIFO is the next benchmark

DF-09 proved the framework for a stateful arbitration/control design.

The second class deliberately exercises a different kind of hardware:

```text
FIFO-SYNC
=
state
+ payload datapath
+ storage
+ ready/valid
+ backpressure
+ reset/clear
+ Width × Depth parameters
```

Three independent open-source implementations are compared:

```text
PULP common_cells  → cc_fifo
BaseJump STL       → bsg_fifo_1r1w_small
OpenTitan          → prim_fifo_sync
```

## Canonical contract

All candidates are adapted to:

```text
clk_i
rst_ni
clr_i

in_valid_i
in_ready_o
in_data_i

out_valid_o
out_ready_i
out_data_o
```

For fair comparison:

```text
PULP       FallThrough = 0
BaseJump   harden_p = 0, ready_THEN_valid_p = 0
OpenTitan  Pass = 0, Secure = 0
```

The benchmark intentionally does not expose occupancy counters because not all
three native modules expose the same feature. We compare the common FIFO
contract, not optional metadata logic.

## Stateful correctness

A single canonical scoreboard checks:

```text
FIFO ordering
no overflow / underflow through canonical handshake
full / empty behavior via ready-valid
stable head data under backpressure
simultaneous push + pop
logical clear
mixed deterministic pseudo-random traffic
```

## First run without copying Git repositories

Do not `cp -a` old Git work directories on `/mnt/e`.

Reuse the existing repository checkout directly:

```bash
python crawler.py \
  --sources sources.json \
  --targets targets.json \
  --workdir ../pycircuit_rtl_crawler_v0.9.3/work \
  --no-update
```

Then run smoke benchmark:

```bash
python run_fifo_benchmark.py \
  --profile smoke \
  --workdir ../pycircuit_rtl_crawler_v0.9.3/work \
  --cxx /usr/bin/g++-10
```

Expected shape:

```text
pulp_common_cells/cc_fifo
  w32_d4 | closure PASS | build PASS | corr PASS | synth PASS

basejump_stl/bsg_fifo_1r1w_small
  w32_d4 | closure PASS | build PASS | corr PASS | synth PASS

opentitan/prim_fifo_sync
  w32_d4 | closure PASS | build PASS | corr PASS | synth PASS
```

## Scaling

After smoke is stable:

```bash
python run_fifo_benchmark.py \
  --profile scaling \
  --workdir ../pycircuit_rtl_crawler_v0.9.3/work \
  --cxx /usr/bin/g++-10
```

This compares:

```text
DATA_W = 32
DEPTH  = 2 / 4 / 8 / 16
```

## Technology-mapped area

Use the same Nangate45 reference Liberty:

```bash
python run_fifo_benchmark.py \
  --profile scaling \
  --workdir ../pycircuit_rtl_crawler_v0.9.3/work \
  --cxx /usr/bin/g++-10 \
  --liberty ../pycircuit_rtl_crawler_v0.9.3/reference_libs/nangate45/NangateOpenCellLibrary_typical.lib
```

The first v0.11 milestone focuses on:

```text
Discovery
→ Dependency Closure
→ Configured Build
→ Stateful Correctness
→ Generic QoR
→ optional Mapped Area
```

Timing and Runtime Catalog ingestion are added after the FIFO correctness
semantics are confirmed on all three repositories.
