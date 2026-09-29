# Generic assignment serialized-final reconstruction fix (2026-09-29)

Scope of this packet is M3: restore plain register **copy** (`state = other`) and
**constant** (`state = 7`) assignments on the existing assignment contract. It
does not implement first-class `ac.system`, new role schema, or any other new
interface.

## Baseline and pre-fix failure

| Item | Value |
| --- | --- |
| Implementation baseline | `17f4b1e5` (R1–R5 batch accepted) |
| Planning baseline | `a7a1571e` |
| Root cause file | `compiler/acir/lib/Compiler/CheckGraph.cpp` (`hasNumericInventory`) |

Reproduced through the real chain (per-source capture → body/header → link →
final materialization → save `.ac` → reparse → emit). All three shapes link-fail
before the fix with the identical chain:

```
error: source check analysis supports only verified U1 numeric next-state rules
error: final hardware CheckGraph reconstruction failed
error: final hardware program view reconstruction failed
```

| Shape | root | link rc before fix |
| --- | --- | --- |
| `state = other` | `@module` | 1, no artifact |
| `state = 7` | `@module` | 1, no artifact |
| system generic rules (`direct_out = source`, `literal_out = 7`) | `@system` | 1, no artifact — now reproducible from the repository via `test_system_root_generic_rules_link_and_emit`, added after the review noted the row had only been covered by a localisation probe |

The dialect layer is already correct: `ACIRFinalUses.cpp` defines
`hasGenericFinalUses` as "has `ac.required_uses` and `ac.yield_bindings`, and has
**no** `ac.required_numeric`", and `verifyGenericFinalUses` strictly validates the
generic form. `tests/cpp/agentic-circuit/Dialect/ACIR/FinalGenericUsesTest.cpp`
already covers that path by reparsing a materialized package in a fresh context.
The defect is only in the **compiler-side classification** used by three analyses.

## Classification invariants (fixed before modifying code)

1. **Generic provenance is not a numeric obligation.** `ac.value.binding` and
   `ac.value.use` are synthesized by final materialization for *every*
   assignment, numeric or not. Their presence alone must not require a
   U1/composition closure. A numeric obligation is declared only by explicit
   carriers: non-empty `ac.required_numeric`, an `ac.numeric.proof` op,
   `ac.math.*` ops, or an op carrying `ac.check_template`.
2. **A numeric obligation must not be downgraded by deleting fields.**
   Classification is a disjunction over *obligation* carriers, not over
   provenance carriers. Losing `ac.required_numeric` while keeping proofs stays
   numeric and fails the closure; losing proofs while keeping
   `ac.required_numeric` stays numeric and fails; losing both while keeping
   `ac.math.*` stays numeric and fails. If every obligation carrier is gone, the
   generic path must reject the residue, and it does: `verifyGenericFinalUses`
   refuses `ac.numeric.proof` / `source.read` / `source.use` residue and requires
   each bound value to be a direct input or an in-domain constant, so a stripped
   numeric computation (`arith.addi`/`arith.andi`) cannot pass as generic.
3. **Generic stays strictly validated.** This packet adds no permissive branch.
   The generic route is the existing `hasGenericFinalUses` +
   `verifyGenericFinalUses` plus the final-rule split in
   `ACIRFinalContracts.cpp`; only direct-current and literal assignments are
   admitted, with verified source, target, domain, valid/path controls and yield
   correspondence.
4. **source and final are distinguished.** Source-stage generic rules never carry
   `ac.value.binding`/`ac.value.use` (`retainGenericSourceUses` requires them
   empty), so removing those ops from the classifier cannot change source
   classification. Final keeps finite-SSA provenance through its own carriers and
   must not re-introduce source-only ones.
5. **No dialect-name heuristics.** `arith.*` must not be read as numeric:
   boolean control and path computation use it too. Only the explicit ACIR
   obligation carriers count.

Each call site keeps its own verification responsibility; the shared helper only
classifies and never replaces a closure check.


## Root cause

`hasNumericInventory` was duplicated **four** times, each treating the generic
provenance carriers `ac.value.binding` / `ac.value.use` as a numeric claim:

| Site | Function | Status |
| --- | --- | --- |
| `CheckGraph.cpp:40` | `hasNumericInventory` | replaced by the shared classifier |
| `ProposalGraph.cpp:93` | `hasNumericInventory` | replaced by the shared classifier |
| `Passes/InferRuleEffects.cpp:177` | `hasNumericInventory` | replaced by the shared classifier |
| `ObservationGraph.cpp:72` | `rejectUnsupportedNumericProofs` (inline copy) | replaced by the shared classifier — **ownership expanded to this file with the integrator's explicit approval**, because it classifies every rule at `:210` and nothing links without it |

Final materialization synthesizes those two ops for *every* assignment, so on
reparse a generic rule looked numeric, was pushed into the U1/composition branch,
and failed. The packet originally listed three sites; the fourth was found by
re-running the chain after fixing the first three, which moved the failure from
`source check analysis supports only verified U1 ...` to `source observation
analysis supports only verified U1 rules without observations`.

`FinalProgram.cpp:1728` also mentions those two ops but is **correct** there: it
exempts generic finals through `hasGenericFinalUses(rule)`, so it was not
touched. `ACIRFinalUses.cpp` already defines the authoritative split
(`hasGenericFinalUses` = has `ac.required_uses` + `ac.yield_bindings` and **no**
`ac.required_numeric`) and `verifyGenericFinalUses` strictly validates the
generic form; the fix does not duplicate or weaken that.

Fix: one internal header-only classifier,
`compiler/acir/lib/Compiler/RuleInventory.h::ruleHasNumericObligation`, used by
all four sites. Each site keeps its own closure verification; the helper only
classifies.

## Files changed

| File | Change |
| --- | --- |
| `compiler/acir/lib/Compiler/RuleInventory.h` | new internal header, single classification definition |
| `compiler/acir/lib/Compiler/CheckGraph.cpp` | local predicate removed, calls the shared classifier |
| `compiler/acir/lib/Compiler/ProposalGraph.cpp` | same |
| `compiler/acir/lib/Compiler/Passes/InferRuleEffects.cpp` | same |
| `compiler/acir/lib/Compiler/ObservationGraph.cpp` | same (ownership expanded with explicit approval) |
| `tests/system/test_generic_assignment_roundtrip.py` | new: 2 roundtrip + 4 generic-carrier negatives |
| `tests/system/test_masked_next_register.py` | obsolete "copy/constant must link-reject" test replaced by a positive link test; 2 anti-downgrade negatives added |

## Positive oracle (both backends really run)

| Case | DUT assignment | observed per phase | reset/rerun | physical `ac.reg` |
| --- | --- | --- | --- | --- |
| constant | child `state = 7` | `[254, 7, 7]` | same trace | 3 |
| register copy | child `state = other` | `[254, 3, 3]` | same trace | 3 |

Both cases compile the child and the testbench per source, link, save
`test_holder.ac`, and then **emit in a new process** for C++ and Verilog; the
C++ model is compiled with clang and executed, and the Verilog is compiled with
Icarus and executed. The C++ trace comes from `Observations().Events()`; the RTL
trace from the `AC_OBS` records emitted by the observation wrapper, which per run
holds the three log records followed by the report gauge (`report("completed",
1)`), i.e. `[254, 7, 7, 1]`. The testbench owns the registers, so the child
writing `state` is the parent/child alias case, and the `ac.reg` count of 3
proves the alias adds no relay or double register. The fixture's own `assert`
fails closed on a wrong value.

## Negative oracle

| Mutation | Diagnostic |
| --- | --- |
| drop `ac.required_uses` | `proof-scoped rule must retain ac.required_numeric` |
| redirect `data_operand` | `generic final YieldBinding is stale or redirected` |
| redirect `enable_operand` | `generic final YieldBinding is stale or redirected` |
| drop the `ac.value.use` op | `generic final binding/use inventory has orphan entries` |
| U1: drop `ac.required_numeric` (proofs remain) | `generic final use rejects numeric/source evidence` |
| U1: drop `ac.required_numeric` **and** both proofs | `generic final value has no direct source authority` |

The last two are the anti-downgrade oracle: a numeric rule cannot escape its
obligation by deleting inventory. All existing U1/composition negatives are
retained and still pass.

## Negative control (the new tests are not vacuous)

Stashing the five compiler files, rebuilding and re-running the roundtrip file
gives **6 failed** with the full pre-fix chain, which the reviewer reproduced
independently in its own scratch build of `17f4b1e5`:

```
error: source check analysis supports only verified U1 numeric next-state rules
error: final hardware CheckGraph reconstruction failed
error: final hardware program view reconstruction failed
error: linked design is not reconstructible by the emit path; this source shape is not supported yet
```

Restoring the fix and rebuilding returns the same cases green. Raw log:
`negative-control.log`.

## Verification

| Lane | Tests | failures | errors | skipped | disabled |
| --- | --- | --- | --- | --- | --- |
| `focused.xml` (3 system files) | 63 | 0 | 0 | 0 | — |
| `system.xml` (6 system files, `-k 'not v44'`) | 90 | 0 | 0 | 0 | — |
| native, all 20 `ACIR*Tests` binaries | 355 | 0 | 0 | 0 | 0 |

All exit codes are 0. Selector inventory of `system.xml`:
`test_generic_assignment_roundtrip` 7, `test_masked_next_register` 15,
`test_source_design_bridge` 41, `test_source_numeric_next` 6,
`test_unified_register_backends` 10, `test_v41_v42_source_fixtures` 11. The two
V44 cases remain deselected and DEFERRED to M6.

M2 closure invariants are unchanged and asserted in this lane:
`test_v41_v42_cpp_and_verilog_share_trace_and_register_inventory[single-module-expected_trace0-4]`
and `[two-level-system-expected_trace1-5]` (trace plus 4 and 5 physical
registers), plus `test_v42_approved_sources_publish_hierarchy_without_relay_registers`.

## Limits

- Only direct-current and literal generic assignments are in scope; arbitrary
  arithmetic, dynamic collections, complex conditions and helpers are unchanged
  and still rejected.
- Multi-value observation emission remains a deferred M3/M4 capability gap; it
  was not touched.
- `link` still does not call any emitter.
- No `.td`, new op/type/attribute, `ac.system`/`ac.root_kind`/artifact-role,
  Python surface, public CLI, manifest schema, runtime or frozen contract was
  changed.
- This packet is M3 capability repair and closes one M4 usability gap; it is not
  M4 completion and does not authorize first-class system, new role, external
  typed DUT, SDK, parallel scheduling or M5 cutover.

## Independent review

Requested from a separate instance; not self-signed. The verdict is to be
appended here before the packet is accepted.


## Independent review

Reviewer: a separate instance that rebuilt the pre-fix compiler in its own
scratch tree, re-ran every lane, and attacked the classifier with eight further
downgrade mutations. Verdict on `d877ff93`: **PASS** — no downgrade path found,
no missed site, no vacuous test, no weakened oracle, scope exact, and every
evidence number reproduced.

Its two informational findings and three unverified items are addressed here
rather than left implicit:

1. *(info)* the two anti-downgrade tests are enforced at **parse time by the
   dialect-level final verifier**, not by `ruleHasNumericObligation`, so they are
   defence in depth rather than unit coverage of the new classifier. The test
   docstring now says so explicitly. The reviewer's structural argument is also
   recorded: both `hasNumericNextUseContract` and `hasNumericCompositionContract`
   require `ac.required_numeric`, so a rule carrying only value provenance never
   had a numeric contract to skip — the old code turned "no contract" into a hard
   error rather than performing a closure check.
2. *(info)* the negative-control chain is now quoted in full above.
3. *(unverified)* the third baseline row had no committed fixture. A committed
   test, `test_system_root_generic_rules_link_and_emit`, now covers the
   system-root generic shape through link plus both emitters.
4. *(unverified)* the `ObservationGraph.cpp` ownership expansion rests on the
   integrator's explicit approval given in this session when the fourth copy was
   found; it has no repository artifact, and it is recorded here as a process
   fact rather than a verifiable one. The reviewer confirmed the diff itself is
   the include plus a comment plus the single classification call.
5. *(unverified)* hand-forging a fully consistent generic final rule is out of
   scope; every deletion or rewrite the reviewer could construct fails closed.

**Acceptance:** PASS applies to `d877ff93`. The follow-up commit after it is
clarity only — one added positive test plus documentation — and the same
reviewer was asked to confirm it. This packet is not self-signed and does not
mark M4, M5 or M7 done.
