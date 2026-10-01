# M3-E01 S1 independent architecture conformance

Date: 2026-10-01. Reviewer: `/root/m3_m6_plan_validation`, independent Astra/high architecture-validation instance. Reviewer did not author the implementation, tests or approved proposal. Scope is the seven frozen compiler files, not full E01 implementation or release acceptance.

Verdict: **CONFORMANT for the bounded S1 declaration-projection slice; no blocking architecture findings**.

## Exact binding

Checkout: `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
Base HEAD: `1efec35eaa19eab2315797340904def88b24a640`.
Approved C2-DECL-R B SHA-256: `ea242da0d85de4f51c439051c80c2e7ce12c17dca5ef9a63b8743f0f280a0043` (rehashed unchanged).
Source manifest: `.pycircuit_out/m3-s1-source-freeze.json`, SHA-256 `8ac1d9f37b127caf1851de556a5cc67775cc8393c754e0825f1c83af542e1417`.
Every source file was independently rehashed and matched:

| File | SHA-256 |
| --- | --- |
| `compiler/acir/lib/Compiler/FinalDeclarations.cpp` | `f53725189c45fe46405ba8a7a8572f2e34ae920036ed69a4d2656cb5b8edffce` |
| `compiler/acir/lib/Compiler/FinalEmit.cpp` | `038e8d8fccd791c233facbcf75408ec66f06d4d3eed934409f6350aa7e073c6d` |
| `compiler/acir/lib/Compiler/FinalHardware.cpp` | `ed419d96a528a00bdf2a31f177a7b86282a1a09fe60839400affd33b4e675ad8` |
| `compiler/acir/lib/Compiler/FinalProgram.cpp` | `b40a2981642d75f893cb51b3a51c8773319016d011be3b3bbebb9f458020f15e` |
| `compiler/acir/lib/Dialect/ACIR/ACIRFinalDeclarations.cpp` | `0e4acb8c72442c75aed63c1bc43911bf9acf3a9ae4c66266387de61fa9a8b2dc` |
| `compiler/acir/lib/Dialect/ACIR/ACIRFinalDeclarations.h` | `0c62cfb33d511d190a0d420b6a6b55763b46bef593dabea5ec8848a9bfdc4f1e` |
| `compiler/acir/lib/Dialect/ACIR/ACIRHardwareClosure.cpp` | `4370410872db3b8f7360f9ebffd609d66d3712c74a65ba92fee8fc11cec4fe58` |

## Conformance evidence from source inspection

- **Owning authority and complete projection:** `FinalDeclarations.cpp` selects record definitions from admitted owning headers, checks canonical registry identity, clones original declarations and preserves existing owner-unit ordering. Imported snapshots/facades cannot become authority. `FinalHardware.cpp` inserts these declarations into the existing source-owned unit rather than manufacturing another unit or module.
- **Exact B §4 record envelope:** `ACIRFinalDeclarations.cpp::verifyFinalRecordDeclaration` checks the three inherent properties, three ordinary attributes, zero operands/results/regions, definition role, canonical qualified symbol/owner and exact `<record>.__init__` provenance. It requires two distinct nonempty field names, exact four-field dictionaries, finite bool/integer types, canonical record-relative origins with empty expansion, owner-matching spans and distinct AST paths. The reused logical-integer validator enforces signless minimal widths 1–64, range and signedness; aggregate field widths need not match. No callable final constructor is required.
- **Erasure safety without source-language expansion:** `FinalProgram.cpp` runs the extra constructor check after source/header admission and before materialization. The constructor must have one block containing only create/return; each field is a data entry argument with the exact field logical constraint; data return is the created nominal record and valid return is the final evaluation-path argument. Parameter/field order may differ and a parameter may initialize more than one matching field. Ordinary helpers remain rejected. Existing general source/importer validators were not changed, so wider/nested source declarations can still compile before S1 final capability rejection.
- **Common final closure and isolation:** `ACIRHardwareClosure.cpp` admits records only as direct declarations, retains package owner/symbol/unit uniqueness and sort rules, and rejects record operations/payloads in operands, results and block arguments. S1 does not silently enable record state, value proofs, reset payloads, data ports or `ac.required_records`.
- **Exact in-memory freeze:** record declarations enter the existing operation snapshots, including properties, ordinary attributes, MLIR location, parent/block and inventory order. `sameOperation` and declaration-inventory checks enforce unchanged frozen content. Fresh standalone final validates intrinsic consistency; this does not add a claim of historical source authentication.
- **No incomplete backend success:** the shared `FinalEmit.cpp` guard rejects any record declaration before CPP, RTL, runner, source-parts and combined Verilog emission. The before/after discipline remains common to both backends. The implementation does not synthesize record behavior or omit the unavailable record header in a successful product artifact.
- **No unapproved interfaces:** the diff changes shared projection/verifier/freeze/guard implementation only. It adds no ODS op/attribute/type, CLI/receipt/runtime/model ABI, public authoring API, alternative scheduler or fallback route. Profile expansion is explicitly withheld by the S1 work item.

## Validation and exclusions

Performed read-only diff inspection against the base, contract/approval checks, source dependency inspection, independent SHA-256 comparison and `git diff --check` for the seven files (exit 0). No product compiler, executable probe, test gate or rebuild was run by this reviewer. Concurrent test/CMake and work-item edits were observed but were outside this source binding and were not changed.

The reported targeted test results belong to the independent testing/integration lanes, not this review. The broader SourceMath/APInt failures and fresh-baseline comparison remain the PM's unresolved validation disposition until their evidence is complete; this conformance verdict does not waive them or certify the whole native suite.

S2 helper/value obligations, S3 state/reset/commit, S4 record emission, full E01, platform/SDK/release closure and product-profile support remain unaccepted by this review. PM final acceptance also requires independent code review and final candidate-bound tests. Any of the seven source hashes changing invalidates this conformance binding and requires impact review.
