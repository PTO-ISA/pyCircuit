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

This is a private-tool guard, so no compiler semantics, IR, CLI, runtime or
schema changed. The compiler-side question it exposes is deliberately left open
below.

## What remains open

1. **Should copy and constant next assignments be supported?** They are ordinary
   C1 assignments and are not in the declared bounded-M2 profile. Supporting
   them means extending the next-use route beyond the U1 masked-add profile, and
   that is a capability decision, not a bug fix.
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
3. The nine rejected compile-time shapes all fail closed with specific
   diagnostics, which is correct behaviour for an unimplemented profile; they
   are listed here so the next capability slice can pick them deliberately.

## Verification

| Lane | Result |
| --- | --- |
| `tests/system/test_masked_next_register.py` | **13 passed** (2 new link-rejection cases, 1 new supported-shapes positive) |
| Python system selectors (5 files) | **76 passed, 0 failed, 0 skipped**; 2 V44 cases deselected |
| Native lane | unchanged by this packet: no compiler library source changed and no native target links `acir-design-harness` |
