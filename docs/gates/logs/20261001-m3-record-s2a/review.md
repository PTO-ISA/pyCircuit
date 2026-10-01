# M3-E01 S2A independent code review

Date: 2026-10-01  
Role: independent Sol code review (`gpt-5.6-sol`, high)  
Baseline: `4c3a4be690f30453acdc551e224e9ef258291ee1`  
Approved contract: C2-DECL-R revision B,
`ea242da0d85de4f51c439051c80c2e7ce12c17dca5ef9a63b8743f0f280a0043`  
Candidate: 19-file aggregate
`eaee80593852c78d3a5c5150c42afe47184283d669b72fb5599964914040e3ec`  
Product-source aggregate: 15-file aggregate
`95e3469d0cb433ce0b510c263b179d4e860da0227837de22f34fdd68d5032238`

## Verdict

**APPROVE for the bounded S2A rule/op verifier slice.** No blocking or
non-blocking code-review findings remain on the frozen candidate.

This verdict does not approve executable record state, whole-package record
closure, module-level record selection/commit, helper expansion, dynamic live
paths, reset materialization, either emitter, or product-profile expansion.
Those responsibilities remain closed for S2B/S2C/S3/S4. The positive S2A rule
seam and fresh rule reparse pass while `verifyFinalHardware` continues to fail
as required.

## Contract review

- The only schema changes are the approved `ac.required_records` operation
  attribute and the ODS widening of the first `ac.value.binding/use` operand to
  `AnyType`; actual op verifiers immediately restrict values to existing
  signless scalars or the admitted nominal record packet.
- The verifier requires exactly one owned current record and the same next
  handle, one read, two ordinal field gets, one ordered create, one next use,
  and one yield contribution. RequiredRecord dictionaries have exact 4/5/4
  variant shapes, structurally sorted unique IDs, allowed forward references,
  and a closed read/get/create DAG.
- Nominal authority is resolved from the complete final package. Both
  declaration-only and implementation provider units use direct, canonical
  source-owned declarations; consumer snapshots, duplicates, misplaced and
  nested records, case-fold/import-owner aliases, and foreign nominals reject.
- Actual block arguments, register handles, StateRefs, ValueBindings, struct
  operands/results, field names and ordinals, origins, validity, path, use,
  yield operands and types are checked against retained obligations. The
  current S2A slice accepts proved-true entry paths and literal true/false next
  paths; dynamic Boolean paths reject until S2B provides threaded-path proof.
- The normalized single-contribution selector has exactly two synthetic gets,
  two typed zero leaves, two selects sharing `E = path && valid`, and one
  aggregate create. Exact-use accounting rejects field swaps, foreign or
  redirected controls, true/false arm swaps, nonzero placeholders, leaked
  zeros, source/synthetic confusion, orphan nodes, hidden operations and extra
  users.
- Numeric, helper/expanded-origin, check/observation, `scf`, `index`,
  helper-return and other unproved forms reject. The record route cannot
  downgrade the existing generic/numeric verifier responsibilities.
- Existing scalar behavior remains on its original signless-width and
  wide-intermediate path. The S1 emitter guard, record reset/state closure and
  whole hardware package remain closed.

## Findings closed before final freeze

Review found and the authors corrected: implementation-provider lookup at the
wrong nesting level; positive-fixture mismatch between aggregate validity and
selector enable; missing ACIR dialect loading in the placement test; acceptance
of arbitrary Boolean field paths; accidental dependence on an unapproved
module `ac.domain` attribute; and test-fixture SSA lifetime/reparse-location
faults. Each correction invalidated the prior manifest. This verdict applies
only to the final aggregate above.

## Evidence

- Independent rehash: all 19 manifest entries and the sorted aggregate match
  `.pycircuit_out/m3-s2a-candidate.json`.
- Independent incremental build: PASS,
  `.pycircuit_out/m3-s2a-review/build.log`.
- Independent focused packet/adversarial run: 11/11 PASS,
  `.pycircuit_out/m3-s2a-review/record.log` and `record.xml`.
- Independent full `ACIRFinalProgramTests`: 69/69 PASS,
  `.pycircuit_out/m3-s2a-review/final-program.log` and `final-program.xml`.
- PM native lane: 19/19 CTest targets PASS,
  `.pycircuit_out/m3-s2a-native.log` and `m3-s2a-native.xml`.
- Existing Python/source/CLI regressions: 129/129 PASS,
  `.pycircuit_out/m3-s2a-regressions.xml`.
- Pre-commit/API hooks: PASS,
  `.pycircuit_out/m3-s2a-hooks.log`.
- Independent Astra architecture check: CONFORMANT for this intermediate slice,
  `.pycircuit_out/m3-s2a-conformance/review.md`.
- Pristine-baseline failing-first evidence includes the genuine wrong-placement
  failure for `ac.required_records`; early full-packet fixture syntax/abort
  output is not treated as a semantic failing-first claim.
- The native math lane remains 88/91 with the same three previously reproduced
  SourceMath/APInt baseline failures. No full-native PASS is claimed.

Independent evidence SHA-256:

- `build.log`: `8c35b1682f73592d30612d7b0fabdb384fcfff35cb5a076cb48b85b1368aa794`
- `record.log`: `7e0f21e5c79f886b66fe0943e7c270688112d907ae4d42ce94fced100b93cc72`
- `record.xml`: `137f09bccbecf15f7641104a154b47cffdc3cf5a0935170d1313bc830260b77f`
- `final-program.log`: `0963927104ad6e163579091339990e3e988631f47dc651d20d37b782df5d6816`
- `final-program.xml`: `5485ed3a99d7a1a9242563c2bea88ad40af010e21570f0485a601a4d75631bc8`
