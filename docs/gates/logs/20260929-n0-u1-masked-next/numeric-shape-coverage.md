# Numeric next-assignment shape coverage, and the link/emit fail-closed guard

Measured on the frozen revision with the real chain
(`--lower-numeric` → link → emit), driven through the public test helpers in
`tests/system/test_masked_next_register.py`. This is reconnaissance evidence for
the W10 numeric route, not a new capability claim.

## Coverage map

| Shape | Result |
| --- | --- |
| `state = (state + 1) & 255` | **supported** — link and emit both succeed (the U1 profile) |
| `if en: state = (state + 1) & 255` | **supported** |
| `if other == 0: state = (state + 1) & 255` | **supported** |
| `log("info", "v", state)` (observe only) | **supported** |
| `state = state + 1` (no mask) | rejected at compile: `numeric next requires explicit (state + 1) & 255` |
| `state = (state - 1) & 255` | rejected at compile: same diagnostic (add only) |
| `state = (state * 2) & 255` | rejected at compile: same diagnostic |
| `state = (1 + state) & 255` | rejected at compile: same diagnostic (operand order fixed) |
| `state = (state + 1) & 127` | rejected at compile: same diagnostic (mask must be full) |
| `state = (state + other) & 255` | rejected at compile: `numeric next requires one unconditional masked assignment` |
| `tmp = state + 1; state = tmp & 255` | rejected at compile: same diagnostic (no consumed temporaries) |
| two masked updates in one rule | rejected at compile: same diagnostic |
| `state = other` (register copy) | **link used to succeed, emit then failed** — now rejected at link |
| `state = 7` (constant assignment) | **link used to succeed, emit then failed** — now rejected at link |

The last two rows were the defect. Both shapes linked successfully and produced
an artifact, but emitting that artifact failed with
`source check analysis supports only verified U1 numeric next-state rules`,
because final materialization synthesizes `ac.value.binding`/`ac.value.use` for
every assignment and `hasNumericInventory` counts those ops as numeric
inventory (`CheckGraph.cpp:40`). A tool that accepts a design and then cannot
emit it is not fail-closed.

## The guard

`acir-design-harness` now reconstructs its own link result from a **clone** of
the materialized hardware module through the same entry the emit path uses
(`buildFinalProgramFromHardware`) before publishing anything. A design whose
final artifact cannot be reconstructed is rejected at link with
`linked design is not reconstructible by the emit path; this source shape is not
supported yet`, and no file is written.

**Scope of this guarantee (corrected 2026-09-29):** it proves that the *shared
final IR can be rebuilt by the emit path*. It is **not** a promise that every
backend can emit every shape. Backend capability limits are enforced by each
emitter when that backend actually runs — for example a multi-value observation
links today and is then rejected by both emitters
(`C++ emitter supports scalar observations only`,
`RTL supports zero or one local observation value`), with no backend output and
no change to an existing output. See
`tests/system/test_source_design_bridge.py::test_multi_value_observation_is_a_known_backend_capability_limit`.
Link must not call the emitters to widen this guarantee, because C3 requires link
to stay codegen-free; a stronger capability gate belongs in a shared analysis,
pass or verifier and needs its own approved packet.

This is a private-tool guard, so no compiler semantics, IR, CLI, runtime or
schema changed.

## What remains open

1. **Plain copy and constant next assignments are an implementation gap under an
   existing contract, not a new language decision** (corrected 2026-09-29).
   `FinalUses.cpp:16-65` already carries direct-current/literal assignment
   authority and `ACIRFinalContracts.cpp:437-450` already distinguishes numeric
   from generic final uses, so `state = other` and `state = 7` are inside the
   approved assignment semantics. What is inconsistent is the classification
   during serialized-final reconstruction. The private bridge's link rejection is
   a conservative capability limit, **not** the final fix, and the plain
   assignment is **not** retired; no further user decision about whether plain
   assignment is allowed is required. The follow-up fix packet must first freeze
   the source/final classification invariants with independent counterexamples
   proving that numeric proof, RequiredUse, `YieldBinding` and check closure are
   not relaxed, and only then change the shared analysis.
2. **`hasNumericInventory` is duplicated verbatim in three places**
   (`CheckGraph.cpp:40`, `Passes/InferRuleEffects.cpp:177`,
   `ProposalGraph.cpp:93`) and treats `ac.value.binding`/`ac.value.use` as a
   numeric claim. Those ops are synthesized for *every* assignment during final
   materialization, which is what makes the predicate true for a non-numeric
   rule. Removing them from the predicate would let the reconstruction path
   accept copy/constant shapes, but it also changes route selection in
   `InferRuleEffects` and the numeric-rule verification trigger in
   `ProposalGraph`, so it needs a design decision rather than a drive-by edit.
   The dedicated U1 closure verifier (`ACIRNumericNextUse.cpp`) is separate and
   would keep enforcing closure either way.
3. **Multi-value observation emission is a deferred capability gap for M3/M4,
   not completed.** A rule that observes two values in one `print`/`log`/`report`
   links today and is then rejected by both emitters
   (`C++ emitter supports scalar observations only`,
   `RTL supports zero or one local observation value`). The current behaviour is
   pinned by
   `tests/system/test_source_design_bridge.py::test_multi_value_observation_is_a_known_backend_capability_limit`.
   Delivering it is an M3 capability addition with its own approved contract and
   independent tests; it is not claimed here.
4. The nine rejected compile-time shapes all fail closed with specific
   diagnostics, which is correct behaviour for an unimplemented profile; they
   are listed here so the next capability slice can pick them deliberately.

## Verification

| Lane | Result |
| --- | --- |
| `tests/system/test_masked_next_register.py` | **13 passed** (2 new link-rejection cases, 1 new supported-shapes positive) |
| Python system selectors (5 files) | **76 passed, 0 failed, 0 skipped**; 2 V44 cases deselected |
| Native lane | unchanged by this packet: no compiler library source changed and no native target links `acir-design-harness` |


## Erratum log

- 2026-09-29: the guard's claim was narrowed from "emit can consume it" to "the
  shared final IR can be rebuilt", and the multi-value observation limit is now
  recorded with its own regression instead of being implied away.
- 2026-09-29: multi-value observation emission is now explicitly assigned to
  M3/M4 as a deferred capability gap, so the evidence packet's claim that it is
  "listed as a later M3/M4 capability gap" is backed by a deliverable.
- 2026-09-29: plain copy/constant next assignment was reclassified from "new
  capability decision" to "serialized-final reconstruction gap under the
  existing assignment contract". Historical measurements above are unchanged.
