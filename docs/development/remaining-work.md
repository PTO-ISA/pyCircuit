# Deferred migration and validation

User direction on 2026-10-09 ends example migration in PR #271. Unfinished
example migration and full-catalog acceptance are tracked in
[issue #272](https://github.com/PTO-ISA/pyCircuit/issues/272), separately from
the refactor merge. Existing supported tests and independent oracles remain.
This inventory records gaps; removal or deferral is not a passing result.

- Complete system migration and full verification for the retained examples.
  Regular-clock benches retain their original module drivers and independent
  oracles; their reset, held-clock and four-state scenarios require separate
  verification where generated default-clock systems cannot express them.
- Complete the remaining original-root mappings and acceptance checks against
  the independent 93-root inventory. Removed rows and source catalog entries
  do not count as verified execution.
- Finish bare `@system`, `@rule`, `@module` spelling across examples and docs.
- Complete broader API/system registration and platform/package acceptance.
  The refactor still requires its bounded API, system, example and CI checks;
  results must identify the exact tested candidate.
- Verify source-import IR, transformed IR and both backend artifacts per root.
- Run deferred nightly coverage, reference/mutation matrices and platform/release
  checks through the existing entrypoints.
- Retain the restored full asserted-module queue vector gate and independent
  checkers. Preserve errors, sticky failure and the historical positive/fault
  scenarios through subsequent compiler/runtime changes.
- Fix the remaining concrete limitations in
  [known limitations](known-limitations.md), including generated namespace
  collisions, explicit physical domains, wide/four-state observations and RTL
  diagnostics. Direct fixed-type imports and explicit imported Struct expression
  constructors now have independent positive and rejection coverage.

The baseline `8887e6de` inventory contains 93 historical roots. The current
catalog maps 85 and leaves 8 unmapped; 72 historical roots have a registered
`@system`. These are source mapping counts, not a claim that all mapped roots
have completed acceptance. The catalog has 64 examples and 25 API-owned cases;
61 examples and 13 API-owned cases have registered systems.

The exact eight unmapped baseline roots are:

| Baseline source under `examples/agentic-circuit/` | Root |
| --- | --- |
| `blocks/array_combinators.py` | `array_combinators`, `array_reductions`, `array_scans` |
| `blocks/bounded_integer_operations.py` | `bounded_integer_operations`, `recursive_array_updates` |
| `blocks/multirate_compute.py` | `multirate_compute` |
| `types/nested_config_types.py` | `nested_config_types` |
| `types/parameterized_types.py` | `parameterized_types` |

The two restored aggregate payload pipelines retain their original token
transformations and independent module oracles. Their owning tests additionally
exercise complete native/RTL histories, reset, held clocks and four-state
payloads, alongside 48-epoch generated systems. Source mapping and focused
checks do not replace final candidate acceptance for the complete catalog.

The slot/mailbox and issue owning gate checks nine complete generated literal
histories on native workers 1/2 and RTL, including reset-reachable extensions
of the historical bodies (7 and 42 epochs). The exact old host-initialized
6/30-epoch histories remain reference-only; the tracked raw default stimulus
functions are not the full generated scenarios. The scalar parameterized case
covers default-width17 identity execution, with the original nested identity
helper inlined; it does not restore the original `param[int]` or general external
parameter bindings. Preserve these boundaries when counting mapped roots.

The routed graph owns all fourteen queues and four branches. Its gate checks
eleven complete module histories (1,792 attempts per native worker/RTL profile)
and two closed systems of 81 and 727 cycles. Original full-root tests checked
topology, artifacts and builds; these runtime histories are new independent
coverage. The selected native/spec scheduler and reorder policy does not claim
equivalence to the retired PYC first-slot/u8-key policy. Four seeded overflow and
clock-control cases remain reference-only, with no complete X/Z or physical
clock-control acceptance claim.

Closed-system generation has focused independent execution and failure-atomicity
coverage. Full example adaptation and full release validation are incomplete.
