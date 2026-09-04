# pyCircuit v0.10 — Runtime Design Records & Catalog

## Why this milestone exists

The benchmark flow already produces valid evidence:

```text
Discovery → Closure → Correctness → Synthesis
→ Technology Mapping → OpenSTA → Area × Timing Pareto
```

But benchmark CSV/log files are not yet a Hardware Design Library.

v0.10 converts those results into stable **Design Records** that a pyCircuit
runtime/compiler selection layer can consume later.

## Record identity

One record represents:

```text
Design Class × Source Implementation × Parameter Configuration
```

For DF-09 this means examples such as:

```text
DF-09 × BaseJump bsg_arb_round_robin × N=16
DF-09 × OpenTitan prim_arbiter_tree × N=16
```

## Record contents

Each record stores:

```text
Design class / operation
Source repo / module / commit when available
Canonical adapter
Parameter configuration
Build / correctness / synthesis / timing gates
Generic cells / logic depth
Mapped area
Critical delay
Fmax proxy
Area × Timing Pareto status
Yosys / OpenSTA versions
Liberty path + SHA256
Evidence file paths
```

## Build catalog from the completed DF-09 experiment

The current timing results were written into the v0.9.3 results tree, so v0.10
can consume them directly without copying Git repositories:

```bash
python build_runtime_catalog.py \
  --class-id DF-09 \
  --profile scaling \
  --results-root ../pycircuit_rtl_crawler_v0.9.3/design_class_results \
  --liberty ../pycircuit_rtl_crawler_v0.9.3/reference_libs/nangate45/NangateOpenCellLibrary_typical.lib
```

Outputs:

```text
runtime_design_library/DF-09/scaling/
├── runtime_catalog.json
├── design_records.jsonl
├── design_records.csv
└── runtime_catalog.html
```

Validate:

```bash
python validate_runtime_catalog.py \
  runtime_design_library/DF-09/scaling/runtime_catalog.json
```

Expected:

```text
CATALOG_VALIDATE_PASS
records: 12
```

## Selection semantics

The catalog does not invent one arbitrary weighted score.

For each N it stores:

```text
area_winner
    minimum mapped area

timing_winner
    minimum critical delay

pareto_set
    implementations not dominated in both area and delay
```

This allows future pyCircuit policy to choose according to objective rather
than hiding trade-offs behind a fixed score.

## Next milestone

Once DF-09 records are validated, extend the same design-record schema to a
second Design Class. The recommended next class is INT-10 LZC / leading-zero
count because it is combinational, has a simple independent golden model, and
provides a clean test that the framework is not arbiter-specific.
