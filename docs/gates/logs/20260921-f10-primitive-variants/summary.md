# F10 primitive variants, qualification, and PPA report

## Scope

P11 requires the semantic primitive registry and the implementation catalog to
stay separate, at least two legal implementations of one semantic primitive with
legality/latency/II/port/bank/depth metadata, deterministic fail-closed
selection, and structural estimates recorded as evidence without inventing
regression thresholds.

## Variants

`pyc.priority_encode.v1` now has two qualified combinational implementations:

| implementation | module | structure | priority |
| --- | --- | --- | --- |
| `pyc.bsd.priority_encode.v1` | `pyc_priority_encode` | serial priority loop over the selected order | 100 |
| `pyc.tree.priority_encode.v1` | `pyc_priority_encode_tree` | two's-complement isolation of the selected bit plus an OR-encode of the one-hot result | 90 |

Both declare the same semantic contract: one `in_value` input, `index` and
`valid` outputs, the `WIDTH`/`ORDER_LOW` parameter bindings, `min_width = 1`,
`max_width = 64`, and a metadata block with `latency_cycles`,
`initiation_interval`, `pipeline_depth`, `banks`, `depth_entries`, `storage`,
and a documented structural estimate. The tree variant is lower priority, so the
default catalog selection is unchanged and the existing selection tests keep
passing.

## Qualification evidence

- **Observation equivalence.** `tests/system/priority_variant_parity_tb.sv`
  instantiates both implementations in both orders and compares `index`/`valid`
  for every 8-bit input: 512 vectors, `priority variant parity PASS 512`. The
  testbench is driven from the system suite.
- **Deterministic selection.** The selector picks the single highest-priority
  legal implementation for the requested width and reports
  `RTL selection is ambiguous at priority N` when two legal implementations
  share the top priority, so a catalog cannot silently depend on entry order.
- **Fail-closed legality.** A catalog whose implementation is outside the
  semantic registry width range still rejects with
  `outside the semantic registry`, and an unqualified or malformed entry still
  rejects before emit.

## PPA report

Structural estimates are recorded in the catalog and reported, not gated.
`flows/tools/report_primitive_ppa.py` renders them as the
`pyc-primitive-ppa-report-v1` JSON report with `gating: false`: the loop
implementation costs `WIDTH` serial priority comparisons, and the tree
implementation costs one isolation term plus `WIDTH` OR-encode terms. Every
entry is combinational with `latency_cycles = 0`, `initiation_interval = 1`,
`pipeline_depth = 0`, and no storage (`banks = 1`, `depth_entries = 1`,
`storage = "none"`).

```
python3 flows/tools/report_primitive_ppa.py --out docs/gates/logs/20260921-f10-primitive-variants/ppa_report.json
```

No regression threshold is added in this stage: the brief requires thresholds
only after stable baselines exist, so the report stays advisory and the only
hard gate is the selection contract itself.
