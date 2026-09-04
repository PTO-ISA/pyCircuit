# pyCircuit v0.12 — Generic Design-Class Timing Backend

## Milestone

Timing is no longer hard-coded to DF-09.

Before:

```text
run_timing_benchmark.py
assumes:
  N
  req_i
  accept_i
  valid_o
  sel_o
```

Now:

```text
Design Class
    ↓
timing_contract in design_class_specs.yaml
    ↓
Generic OpenSTA Backend
```

Each class declares:

```yaml
clock_port:
timed_inputs:
timed_outputs:
false_path_inputs:
default_period_ns:
semantics:
```

## DF-09 contract

```text
clock        clk_i
timed input  req_i*, accept_i
timed output valid_o, sel_o*
false path   rst_ni
```

## FIFO-SYNC contract

```text
clock        clk_i

timed input:
  clr_i
  in_valid_i
  in_data_i*
  out_ready_i

timed output:
  in_ready_o
  out_valid_o
  out_data_o*

false path:
  rst_ni
```

`clr_i` remains timed because it is a synchronous logical clear in the
canonical FIFO contract.

## WSL staging retained

OpenSTA still runs from Linux-native scratch:

```text
/tmp/pycircuit_sta/
```

while persistent source/results can stay on `/mnt/e`.

## FIFO timing run

The FIFO mapped results were generated in v0.11.2, so use that directory as
`--results-root`:

```bash
python run_timing_benchmark.py \
  --class-id FIFO-SYNC \
  --profile scaling \
  --results-root ../pycircuit_rtl_crawler_v0.11.2/design_class_results \
  --liberty ../pycircuit_rtl_crawler_v0.9.3/reference_libs/nangate45/NangateOpenCellLibrary_typical.lib \
  --workers 2 \
  --timeout-sec 30
```

Or use the convenience wrapper:

```bash
python run_fifo_timing.py \
  --profile scaling \
  --results-root ../pycircuit_rtl_crawler_v0.11.2/design_class_results \
  --liberty ../pycircuit_rtl_crawler_v0.9.3/reference_libs/nangate45/NangateOpenCellLibrary_typical.lib \
  --workers 2 \
  --timeout-sec 30
```

## First quick test

Only Depth=16:

```bash
python run_fifo_timing.py \
  --profile scaling \
  --results-root ../pycircuit_rtl_crawler_v0.11.2/design_class_results \
  --liberty ../pycircuit_rtl_crawler_v0.9.3/reference_libs/nangate45/NangateOpenCellLibrary_typical.lib \
  --only-config w32_d16 \
  --workers 1 \
  --timeout-sec 30
```

## Output

```text
design_class_results/FIFO-SYNC/scaling/timing_analysis/
├── timing_summary.csv
├── timing_report.json
└── area_timing.html
```

The report evaluates Pareto independently for each canonical configuration:

```text
w32_d2
w32_d4
w32_d8
w32_d16
```

and reports:

```text
area_winner
timing_winner
pareto_set
```

## Important timing semantics

The current metric remains:

```text
mapped-cell + zero-I/O-budget + no-parasitic pre-layout timing proxy
```

not signoff Fmax.

For FIFO, the worst path may belong to:

```text
reg → reg
input → reg
reg → output
input → output
```

under the common canonical interface. A later methodology upgrade can split
these into separate path classes and add fixed input slew / output load.
