# M4 C2-DECL implementation

Status: done (C2-DECL D1–D5 bounded packet). Product base: `e7e5c513`. Planning base: `54b5cdfa`.
User approved revision A, SHA-256
`38dd31d13cff150cf7b778e9c3df469f9ab1e0b55c8b2311f7c8d05d49b736b8`.
[Approval](../rfcs/migration/approvals/c2-decl-scalar-final.md).
The proposal stays frozen; no new approval is required for D1–D5 implementation.

## Ownership

- D1/D2: unit_pair_fix, Luna high, common final declaration validation,
  materialization, reparse and frozen snapshots; assigned dialect/FinalProgram/
  FinalHardware files only. No CPP/backend presentation changes.
- D3: decl_cpp_impl, separate Luna high, FinalCpp*/FinalEmitCpp* and new private
  declaration/name emit helpers. No shared semantic/IR/Python/runtime changes.
- Independent tests: other_agent_arch_review, Astra xhigh. The native tool
  rejected a further Luna test thread due to its thread limit; this independent
  read-only instance supplies an exact test patch, which PM applies and runs.
  Do not claim a nonexistent Luna lane or architect filesystem/test execution.
- PM: private source-parts JSON transport, CMake registration, storage-inventory
  parser qualification adjustment, shared .pycircuit_out/w10-pm/build, docs,
  integration and evidence. Existing test oracles change only where the approved
  extra declaration unit/header changes inventory or global qualification.
- Final review: independent instance, no implementation/test authorship.

Writers preserve each other's files and do not recursively delegate. CPP source
parts encode header-only groups internally with empty sourcePath/source; private
JSON exposes null source_path/implementation. This is not generated.json or a
new public API. All common semantics and admission live in MLIR verification.

## Completion

Use approved proposal D1–D5 and §8 as acceptance. In particular: original owner
and all scalar definitions, empty/facade/implementation-owned cases, contextual
origin and import-module checks, strict placement/fields/types, immutable
snapshots, final-only fresh parsing, both backends, exact native constants,
header-only groups and independent compilation/ODR, rejection before output.

Wide MathInt remains valid common IR and RTL; only C++ declarations reject
unrepresentable native values. Alias bool vs integer-i1 and signedness are not
inferred from physical width. Non-scalar projection admission follows the
explicit approved rejection boundary. SYSTEM/EXPECT B, public emit, manifest
publication, runtime/ABI/SDK and M5 cutover are outside this packet.

## Verified candidate

Candidate: base `e7e5c513`, plus 34 compiler/test hashes in
[the evidence manifest](../gates/logs/20260930-c2-decl-scalar-final/candidate-sha256.json).
Product and test implementation of D1–D4 is complete. D5 integration uses
[the evidence packet](../gates/logs/20260930-c2-decl-scalar-final/README.md):
39 new system, 73 existing system, 54 + 18 native tests passed. Broader source
checks have 48 passes and one pre-existing class/self fixture failure, explicitly
recorded. Independent Astra conformance and Sol code review both APPROVE. No full release/platform/SDK claim.

Review repaired global Std/Gfsim module collisions and raw casefolded owner pairs
whose exact __init__ reduction differed. Both raw-owner regressions failed before
repair and pass afterward. Declaration-only headers compile without GFSIM includes.
The existing native storage-count assertion retains count 1 with the newly required
qualified C++ spelling. No assertion was deleted or relaxed.

The generated Unicode table is machine-generated data, reproducible from pinned
Unicode 16.0.0; its generated size is not a handwritten-file exception. Signed
alias/negative constant oracles use valid focused IR mutations, not new Python
surface syntax. The exact approved proposal remains byte-for-byte frozen.
