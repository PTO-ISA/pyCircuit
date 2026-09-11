# Typed MLIR rule activation gate summary

Decision 0191 moves rule activation sources, transaction closure resources,
and initial zero-input activation into verified MLIR evidence. QueueGraph now
extracts these records instead of rediscovering rule semantics in codegen.

## Evidence

- `ActivationResourceKind` is a closed ACIR enum with input Queue, output
  Queue, and state cases.
- `ac-infer-rule-activation` runs after effect/footprint analysis and before
  handshake/schedule closure.
- Lowered multi-state and zero-input firings carry
  `ac.activation_sources`, `ac.transaction_resources`, and
  `ac.initially_active` attributes.
- ACIR rejects a zero-input firing after its initial flag is forged from true
  to false.
- QueueGraph JSON preserves each rule block's typed resources and reports
  `has_activation_evidence=true`.
- Generated `offer_<input>` and `schedule_initial_work` adapters eliminate
  manual external Queue scheduling from the reusable ROB activation harness.
- Focused rule-lowering lit: 3/3 passed.
- Focused QueueGraph plan/codegen and reusable ROB integration: passed.
- Full ACIR lit: 159/159 passed.
- Full CodeGen: 105/105 passed.
- Full gfsim remains 256/256 after the activation runtime slice.
- Python frontend: 75/75 passed.
- Full Queue codegen integration: 20 passed, 1 skipped because the optional
  DavinciOO reference trace fixture was unavailable.
- C++/Python format, changed-file lint, repository contracts, strict 192-row
  decision status, MkDocs strict, and `git diff --check`: passed.

## Remaining scope

Structural non-rule blocks still derive activation from topology at QueueGraph
extraction. Decision 0192 subsequently adds nested/internal module binding.
Separate Work/Xfer closure CSR, sink-free output dequeue adapters, full ROB
tick/timeline equivalence, and semantic-change filtering remain open.
