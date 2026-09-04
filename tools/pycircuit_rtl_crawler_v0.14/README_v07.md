# pyCircuit Hardware Design Mining Framework v0.7

## Milestone: Design-Class Benchmark Adapter

v0.6 answered:

```text
Can I automatically run several supported candidates?
```

v0.7 starts answering:

```text
For the SAME design target, which open-source implementation is better?
```

The first design class is:

```text
DF-09
Round-Robin Arbiter
```

with three source candidates:

```text
PULP common_cells  → cc_rr_arb_tree
BaseJump STL       → bsg_arb_round_robin
OpenTitan          → prim_arbiter_tree
```

## Why adapters are needed

Their native interfaces are not identical:

```text
PULP       req/gnt + idx + sink grant
BaseJump   reqs/grants + yumi
OpenTitan  req/gnt + idx + ready
```

v0.7 introduces a canonical benchmark interface:

```systemverilog
clk_i
rst_ni
req_i[N-1:0]
accept_i

valid_o
sel_o[N-1:0]
```

`sel_o` means the selected requester and is deliberately separated from
the source-native handshake grant signal.

## Correctness philosophy

The first cross-repository test is property-based rather than requiring
bit-identical arbitration phase/direction.

Common properties:

```text
1. no request → no valid/selection
2. selection is one-hot
3. selected lane must be requesting
4. singleton request must win
5. stable requests + backpressure → stable selection
6. all requesters active → every requester wins once in each N accepted transfers
7. sparse pair → both requesters make progress
```

This avoids declaring a valid round-robin design wrong merely because its reset
phase or scan direction differs from another repository.

## Source scope

For the first multi-repository experiment, the crawler is deliberately scoped:

```text
PULP       src/ + include/
BaseJump   bsg_misc/
OpenTitan  hw/ip/prim/rtl/
```

This is much more controlled than crawling the entire OpenTitan/Gemmini/Vortex
trees before the design-class benchmark semantics are stable.

## Run order

First, make sure the v0.6.2 compiler fix works locally.

Then in v0.7:

```bash
python smoke_test_v07.py
```

Run discovery across the three scoped repositories:

```bash
python crawler.py --sources sources.json --targets targets.json
```

Then run the first multi-repository DF-09 benchmark:

```bash
python run_design_class.py \
  --class-id DF-09 \
  --profile smoke \
  --cxx /usr/bin/g++-10
```

For broader scaling:

```bash
python run_design_class.py \
  --class-id DF-09 \
  --profile scaling \
  --cxx /usr/bin/g++-10
```

Optional mapped area:

```bash
python run_design_class.py \
  --class-id DF-09 \
  --profile standard \
  --cxx /usr/bin/g++-10 \
  --liberty /path/to/stdcells.lib
```

## Outputs

```text
design_class_results/
└── DF-09/
    └── smoke/
        ├── pulp_common_cells/...
        ├── basejump_stl/...
        ├── opentitan/...
        ├── comparison_report.json
        ├── comparison.csv
        └── comparison.html
```

The comparison table records:

```text
Build Gate
Canonical Correctness
Generic Cells
Logic Depth
Optional Liberty Area
Pareto Frontier
```

v0.7 intentionally does NOT create an arbitrary weighted score yet.
A candidate is only marked Pareto-optimal when another passing implementation
cannot beat it in both generic cell count and logic depth.

## Next after v0.7

After DF-09 is stable:

```text
v0.7.1  Harden cross-repo adapter/compile edge cases
v0.8    Add another Design Class, preferably LZC or Popcount
v0.9    Technology-mapped timing/area with fixed Liberty + constraints
v1.0    Batch Design-Class Mining + Score Card + Runtime integration proposal
```
