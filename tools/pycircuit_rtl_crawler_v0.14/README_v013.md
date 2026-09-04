# pyCircuit v0.13 — Generic Runtime Design Catalog

## Milestone

The Runtime Catalog is no longer tied to DF-09's integer `N`.

Record identity is now:

```text
Design Class
× Source Implementation
× Canonical Configuration
```

Examples:

```text
DF-09
× basejump_stl/bsg_arb_round_robin
× n16

FIFO-SYNC
× opentitan/prim_fifo_sync
× w32_d16
```

A configuration may contain class-specific parameters such as:

```text
n
data_width
capacity
random_steps
```

## Selection completeness

Every configuration records:

```text
expected_candidates
observed_candidates
valid_candidates
selection_complete
```

If one candidate fails closure/build/correctness/synthesis/timing, the catalog
can still preserve partial evidence, but it marks the selection as INCOMPLETE.

This prevents a partially evaluated candidate set from being mistaken for a
final area/timing winner.

## FIFO catalog

After the v0.12.1 FIFO scaling + timing run:

```bash
python build_runtime_catalog.py \
  --class-id FIFO-SYNC \
  --profile scaling \
  --results-root design_class_results \
  --liberty ../pycircuit_rtl_crawler_v0.9.3/reference_libs/nangate45/NangateOpenCellLibrary_typical.lib
```

Validate:

```bash
python validate_runtime_catalog.py \
  runtime_design_library/FIFO-SYNC/scaling/runtime_catalog.json \
  --require-complete-selection
```

Expected:

```text
CATALOG_VALIDATE_PASS
records: 12
all selections complete: True
```

## DF-09 methodology consistency

v0.12.1 introduced formal/assertion sanitization before QoR accounting.
Before treating DF-09 as a frozen production catalog, rerun DF-09 synthesis and
timing under the same methodology, then rebuild its catalog.

## Multi-class index

Once DF-09 and FIFO-SYNC catalogs are present:

```bash
python build_library_index.py
```

Outputs:

```text
runtime_design_library/
├── library_index.json
├── library_index.csv
├── library_index.html
├── DF-09/...
└── FIFO-SYNC/...
```

This is the first top-level view of the pyCircuit Runtime Hardware Design Library.
