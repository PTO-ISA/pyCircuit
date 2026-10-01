# M3-E01 S1 independent code review

Date: 2026-10-01. Reviewer: `/root/m6_resource_independent_review`, independent Sol/high code-review instance. The reviewer did not author the implementation or tests.

## Code Review Summary

**Candidate aggregate:** `3405b937fb6ddce11a21214c6ec3722a73a15455087a33101705ea72a55e1159`  
**Baseline:** `1efec35eaa19eab2315797340904def88b24a640`  
**Approved C2-DECL-R B:** `ea242da0d85de4f51c439051c80c2e7ce12c17dca5ef9a63b8743f0f280a0043`  
**Files reviewed:** 10 candidate files, plus the approved contract, approval record, work packet, and relevant baseline context  
**Total issues:** 0

### By Severity

- CRITICAL: 0
- HIGH: 0
- MEDIUM: 0
- LOW: 0

### Issues

None.

### Spec and root-cause assessment

The candidate implements only the bounded S1 declaration slice. It projects complete source-owned two-field flat finite record declarations into the common final package, keeps the constructor as canonical provenance without a final executable function, preserves owner/origin/field order and complete unused/private declaration inventory, and retains deterministic source-unit and declaration ordering.

The additional constructor preflight repairs the actual erasure-safety gap instead of adding a fallback. It requires a direct two-operation create/return body, direct entry data arguments with matching logical field constraints, the exact created nominal result, and the final evaluation-path argument as returned validity. Ordinary helpers and unsupported supplied records remain explicit failures. Wider/nested records retain their existing source compile boundary and fail at S1 link/final projection.

Common final verification admits records only as direct declarations and rejects record payloads, record value operations, state, ports, and later-slice behavior. The shared emission guard runs after FinalProgram verification and the EmitReady requirement, then rejects record declarations for C++, Verilog, runner, and source-parts entry points. It cannot access missing hardware on an AnalysisClosed program and cannot publish incomplete record backend artifacts.

Final record snapshots cover properties, ordinary attributes, MLIR location, parent/block, and ordered inventory. Frozen-object edits fail, while a self-consistent standalone final remains subject only to intrinsic final verification. No FileLineColLoc-only restriction remains; field SourceSpan paths still bind to the SourceOwner. No ODS, public CLI, runtime, receipt, source-map, model ABI, or compatibility route was added.

### Candidate-bound evidence

- Recomputed every manifest file SHA-256 and the documented sorted `path + NUL + sha256hex + LF` aggregate; all matched `.pycircuit_out/m3-s1-candidate.json`.
- Independent system rerun: `10 passed` in 4.11 seconds; JUnit `.pycircuit_out/m3-s1-review/final-system.xml`.
- Independent native record rerun: `4 passed`; JUnit `.pycircuit_out/m3-s1-review/final-native.xml`.
- Independent complete `ACIRFinalProgramTests` rerun: `58 passed`; JUnit `.pycircuit_out/m3-s1-review/final-native-full.xml`.
- Independent pre-commit run over all candidate files passed merge-conflict, EOF, whitespace, Ruff, and Black hooks.
- The same frozen 10-test system file run against freshly rebuilt pristine-baseline harnesses produced the expected failing-first result: 6 S1-positive/exact-diagnostic failures and 4 pre-existing rejection passes, with no errors or skips. Evidence: `.pycircuit_out/m3-s1-failing-first-final.xml` and `.pycircuit_out/m3-s1-failing-first-final.log`.
- Independent coherent-constructor probes changed every owning/import-snapshot carrier together. Wrong returned validity failed with `record constructor return/create closure is invalid`; a computed field failed with `record constructor is not a direct two-operation body`; neither published a final. Evidence: `.pycircuit_out/m3-s1-review/ctor-fullclosure-6tvcz7lo`.
- Independent standalone-location probe changed a final record op to `loc(unknown)` while retaining owner-bound field SourceSpans; fresh verification passed at `.pycircuit_out/m3-s1-review/unknown-loc.final.ac`. The native frozen-location mutation rejected the same class of in-memory change.
- Integration evidence reports 19 applicable native CTest binaries passing with 268 individual tests and no fail/error/skip/disabled, 119 installed scalar/source/public-driver regressions passing, 10 final-record system cases passing, four native record cases passing, and a 12-command installed-prefix smoke passing.
- Independent Astra architecture review is CONFORMANT and bound to the seven compiler-source hashes at `.pycircuit_out/m3-s1-conformance/review.md`.

### Bounded residuals

`ACIRSourceMathContractsTests` is not reported as passing. A fresh pristine baseline build from `1efec35e` and the candidate build, with matched LLVM/MLIR 22.1.8 and CMake flags, both produce the exact same three APInt `bitPosition < getBitWidth()` failures in the same NumericComposition tests. Baseline and candidate XML are `.pycircuit_out/m3-s1-baseline-three.xml` and `.pycircuit_out/m3-s1-candidate-three.xml`; matched configuration is recorded in `.pycircuit_out/m3-s1-baseline-flags.json`. This is a pre-existing gap, not an S1 regression, and no full 20-binary CTest pass is claimed.

S2 helper/value/use closure, S3 record state/reset/commit, S4 C++/RTL record emission, full E01, product-profile expansion, and release/platform closure remain open. S1 must not be described as executable record support.

### Recommendation

**APPROVE** the candidate for the bounded M3-E01 S1 declaration-projection slice.
