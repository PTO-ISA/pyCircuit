# F5 verified obligation codegen and structural RTL audit

## Scope and verdict

This run closes the F5 backend slice for Decision 0272 and the RTL naming and
audit clauses of Decision 0274. One verified `ac.arch_obligation` record now
materializes the canonical `cpp`, `gfsim`, and `sva` target set, lowers its
exact condition to PYC, and reaches C++ and SVA with the same stable ID,
sampling contract, source, message, and NDF linkage.

The work remains framework-generic. It adds no consumer module, processor
schema, generated-code patch, compatibility path, or content-derived identity.
Decision 0273 and the F6 SRAM/four-state profile remain separate.

## Implemented contracts

- Runtime-checked phase-one range obligations require exactly three canonical
  materializations: `cpp`, `gfsim`, and `sva`. Each record repeats the same
  module, firing, input, maximum, sampling, condition, severity, active
  predicate, and reset/recovery disable metadata.
- QueueGraph rejects incomplete, reordered, missing, duplicated, or tampered
  materializations. Architecture obligations are legal direct children of a
  structured module case and retain their normative source provenance.
- QueueGraph PYC lowering reconstructs the admitted typed expression DAG and
  emits a gated `pyc.assert`. PYC requires the closed phase-one safety kind,
  stable ID, severity, pre-publish sampling, anchor, printable ASCII source and
  message, and unique printable NDF IDs. Liveness remains rejected.
- C++ emits an unconditional check that survives `NDEBUG`, structured failure
  text, per-obligation checks/failures counters, and an ID-addressable trace and
  probe condition.
- Verilog emits one named concurrent SVA assertion and matching same-ID cover
  property. Applicability and synchronous disable are part of the shared
  condition; no asynchronous `disable iff` is invented.
- Runtime assertions remain verification evidence. The structural checker
  reports `synthesis_admissible: false` and
  `--require-synthesis-admissible` rejects any remaining runtime obligation.
- Verilog family definitions and instance callees use deterministic readable
  lower-snake names. Normalization collisions reject rather than acquiring a
  counter, hash, truncation, or opaque suffix.
- The structural RTL audit rejects non-ASCII output, double underscores,
  non-lower-snake modules, unstable module/net/instance/assertion order,
  expressions in named port connections, unsized assignment values, dead
  internal nets, and mismatched assertion/coverage IDs. It reports expensive
  operations and supports byte comparison plus colored HTML evidence.
- Priority encoding remains a priority-structured catalog primitive. Runtime
  onehot checks cannot authorize an AND-OR implementation.

## Directed parity evidence

`architecture-obligation-backends.mlir` executes the same range condition in
generated C++ and Verilator. Value 127 passes both; value 128 fails both and
names `range:bounded`, the same source location, sampling contract, and NDF ID.
The fixture also emits ready/valid, no-partial-commit, no-stale-update, credit,
and pipeline-alignment safety assertions through the shared PYC/SVA carrier.

`architecture-obligation-lowering.mlir` proves the complete ACIR rule pipeline:
pending obligation -> exact three-target materialization -> frozen QueueGraph ->
PYC -> C++ and SVA.

## Structural evidence

- `rtl-audit.json`: ASCII and structural audit, six matched assertions and
  coverage properties, zero dead nets, zero expensive operations, and a
  byte-identical second clean emission.
- `rtl-determinism.html`: colored side-by-side deterministic RTL comparison.

## Remaining work outside F5

- P4 side-effect-free why-not-fire blocker evaluation and the broader
  firing/blocker/conflict/recovery/stale-rejection counter taxonomy remain
  later diagnostic work.
- Decision 0273 remains `gap-in-scope` for F6 exact four-state/SRAM semantics.
- Recovery, stale-update inference, wide transactions, and memory ordering are
  owned by F7-F10; the generic PYC/C++/SVA safety carrier is ready for them.
