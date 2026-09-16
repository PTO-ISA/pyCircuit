# Issue 129 mixed-field firing writes

Decisions: 0154, 0236, 0243, 0261, 0263.

One rule firing may now retain an ordered owner-local Table batch whose
proposals carry different field schemas. QueueGraph admits a pair only when its
indices are statically disjoint, its presences are structurally exclusive, or
both proposals are field mode with disjoint canonical fields. Possible
same-field overlap and replace overlap remain fail closed.

GFSim reserves and publishes each firing as one batch of per-record
`index/value/mode/field_mask` footprints. Commit applies field records before
replace records, keeps source order within each class, and never exposes a
partially prepared batch. Generated merge policies use static field ordinals
and a mask-aware function call without runtime reflection.

Field provenance now follows local aliases, safe selects, and closed pure
helper returns. Isolated module graphs expand pure helper calls before storage
selection, so system/module-local and ordinary/`@ac.inline` spellings produce
the same proven footprint when their SSA roots are equivalent.

## Results

- Full Python frontend: 404 passed, 2 skipped.
- GFSim native: 257 passed.
- CodeGen native: 172 passed, including flat and structured mixed-schema C++
  compilation.
- Table backend: 8 passed. The frontend-derived fixture freezes one firing with
  two same-index field schemas, checks QueueGraph JSON masks, generates PYC,
  compiles/runs GFSim C++, and preserves the untouched field.
- Table PYC C++/Verilator parity: 3 passed.
- Changed MLIR storage-selection and branch-lowering tests: 3 passed.
- `check-acir`: the changed tests pass. The full lane reports the same nine
  unrelated baseline failures already recorded in
  `20260915-issue129-static-field-writes/summary.md`; the one generated-string
  expectation introduced by this change was updated and passes.
- Official `run_agentic_circuit.sh` AC G2 closure passed with G0/G1 explicitly
  skipped; its summary records all Table GFSim and PYC parity cases as passing.

Dynamic conflict assertions remain outside this change. A malformed or
overlapping runtime batch, output backpressure, failed prepare, reset, or
out-of-range footprint cannot commit partial Table state.
