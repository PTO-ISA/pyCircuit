# Examples

This directory contains folderized pyCircuit examples.

## Layout contract

Each example case `X` is a folder. Runnable compiler/simulation cases use:

- `X/X.py`: design (`@module def build(...)` or
  `def build(m: CycleAwareCircuit, domain: CycleAwareDomain, ...)`)
- `X/tb_X.py`: testbench (`@testbench def tb(...)`)
- `X/X_config.py`: default params + TB presets + `SIM_TIER`

Some focused API or visualization examples are not simulation cases and may
carry only their design and supporting assets. `discover_examples.py` is the
authority for the folderized gate set; a CycleAware design uses
`compile_cycle_aware()` for canonical JIT compilation or
`build_cycle_aware()` for explicit Python elaboration.

## Smoke checks

Compiler smoke (`emit + pycc`):

```bash
bash flows/scripts/run_examples.sh
```

Simulation smoke (strict normal-tier examples, C++ + Verilator):

```bash
bash flows/scripts/run_sims.sh
```

Nightly simulation smoke (normal + heavy tiers):

```bash
bash flows/scripts/run_sims_nightly.sh
```

Semantic closure lane (pyc6 decision regressions):

```bash
bash flows/scripts/run_semantic_regressions_v6.sh
```

## Refresh Procedure (pyc6)

Use a single run-id to refresh compile/sim evidence and decision coverage artifacts:

```bash
RUN_ID=pyc6-refresh
PYC_GATE_RUN_ID="${RUN_ID}" \
PYC_DECISION_STATUS_STRICT=1 \
bash flows/scripts/run_examples.sh
```

Strict decision coverage can also be invoked directly:

```bash
python3 flows/tools/check_decision_status.py \
  --status docs/gates/decision_status_v6.md \
  --out .pycircuit_out/gates/${RUN_ID}/decision_status_report.json \
  --require-no-deferred \
  --require-all-verified \
  --require-concrete-evidence \
  --require-existing-evidence
```

## Semantic smoke examples (pyc6)

- `xz_value_model_smoke`: validates v3 trace value payload (`value`, `known`, `z`) emission.
- `reset_invalidate_order_smoke`: validates reset/invalidate ordering in trace events.
- `net_resolution_depth_smoke`: validates hierarchical combinational depth propagation in a simple chain.

## Artifact policy

Generated artifacts are local-only and written under:

- `.pycircuit_out/`

They are intentionally not checked into git.

## Product boundary

Examples in this repository demonstrate supported framework features. Complete
processor, accelerator, SoC, and board designs belong to their consumer
repositories and use pyCircuit as a pinned package/toolchain dependency.
Repository layout checks reject product-system orchestration classes under the
framework example tree. The FM16 full-mesh system is owned by
[`hengliao1972/DavinciOO`](https://github.com/hengliao1972/DavinciOO/tree/main/srcs/core/system/fm16).
