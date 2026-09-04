# pyCircuit v0.9 — Technology-Aware Area × Timing Benchmark

## What this adds

v0.8 established:

```text
same canonical interface
same correctness properties
same Nangate45 Liberty
same Yosys/ABC mapping
        ↓
mapped area
```

v0.9 adds standalone OpenSTA:

```text
mapped_netlist.v
+ same Liberty
+ same timing constraints
        ↓
OpenSTA
        ↓
worst slack
        ↓
pre-layout critical-delay proxy
        ↓
Fmax proxy
        ↓
Area × Timing Pareto
```

## Important terminology

The reported frequency is intentionally named:

```text
Fmax Proxy
```

not:

```text
signoff Fmax
```

because the current flow does not include:

- SPEF / routed parasitics
- clock-tree synthesis
- routing delay
- physical buffering
- realistic external input delay
- realistic output loading

It is a **mapped-cell, no-parasitic, pre-layout timing comparison**.

## First preflight

```bash
python check_sta.py \
  reference_libs/nangate45/NangateOpenCellLibrary_typical.lib
```

Expected:

```text
basic_timing_flow : PASS
```

## Run timing on existing mapped results

If v0.8 already generated the mapped netlists:

```bash
python run_timing_benchmark.py \
  --class-id DF-09 \
  --profile scaling \
  --liberty reference_libs/nangate45/NangateOpenCellLibrary_typical.lib
```

## Run area + timing end-to-end

```bash
python run_area_timing_benchmark.py \
  --class-id DF-09 \
  --profile scaling \
  --liberty reference_libs/nangate45/NangateOpenCellLibrary_typical.lib \
  --cxx /usr/bin/g++-10
```

## Timing benchmark constraints

The first benchmark intentionally uses a simple canonical constraint model:

```text
clock               = 100 ns loose reference
input delay          = 0
output delay         = 0
reset path           = false path
wire parasitics      = none
```

The 100 ns clock is not the target frequency. It is deliberately loose.
The script derives:

```text
critical_delay ≈ reference_period - worst_slack
Fmax_proxy     = 1000 / critical_delay_ns
```

under the zero-I/O-delay benchmark assumptions.

## Outputs

```text
design_class_results/DF-09/scaling/
└── timing_analysis/
    ├── timing_summary.csv
    ├── timing_report.json
    └── area_timing.html
```

Each case also receives:

```text
timing.tcl
sta_stdout.log
sta_stderr.log
```

for traceability.

## Next methodology step

Once this flow is stable, improve the timing environment:

```text
fixed input transition
fixed output load
        ↓
separate timing path classes
(in→out / in→reg / reg→out / reg→reg)
        ↓
optional SPEF / OpenROAD physical flow
        ↓
post-route timing
```
