# W10 FinalSystem / backend closure review

Date: 2026-09-29. Candidate checkout:
`/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`, HEAD
`82f161ea95336fdd15c238e103f13b9f823e8f28` plus the preserved migration
overlay.

## Scope and independence

This review covers the bounded scalar FinalProgram materializer, frozen
FinalSystem instance/state plan, C++/Verilog emit entry points, backend closure
harness, and executable backend oracle. It does not accept the complete W10
milestone.

- implementation: Luna/low bounded lanes for FinalProgram and independent
  executable tests;
- review: independent Sol/high code-review lane;
- PM integration: test CMake wiring, automatic harness environment, focused
  rebuild, and evidence capture.

Final candidate bindings:

- `FinalProgram.h`: `b92c99705f0575949222ef05d6401a44c67be2b7a60ccb45025d140d60c7da2b`
- `FinalProgram.cpp`: `d850f06b196186692a16aaa338b85a47c7f8be059910a2872648d049d49ccfd3`
- Verilog/dispatcher `FinalEmit.cpp`: `8403d4d5514616f57b11cbcf9256c1f12db60ca4f914e466c3ba0426c4011ae1`
- C++ emitter/support: `3609ee8284e5b47413e91cb16fa6949a6063b6e090b411e889948e046a3bb7b6`
  / `796b39e14ec6c9ebb9c0e4d89659f1b30ed94c4473858d6cd2ac0069abcd87b8`
- C++ hierarchy helper:
  `b400e7bd0637bab73f66b3ab663ab105e90f5d6a8949a21dc71921bd6b8ba452`
- backend harness: `de1b6659a254787ae5609f4d9ea4d106a5be837eb60afdbd8f6c23acdcf47daa`
- runtime `SimDFF/SimModule/SimSystem/ObservationSlot`:
  `ae18e05b…` / `174f3bde…` / `bac9e22d…` / `b9873c0d…`
- `FinalProgramTest.cpp`: `2949f5fb27573d75ea47b1a4136ef4a2adf9199eb7027d7a05fcb8f4087e3abe`
- `FinalSystemPlanTest.cpp`: `c900d251af92a384625c177c2a19f1a688c71e4d7295dc38fdd511b783d66184`
- realization / multi-instance tests: `9dce3ce7…` / `607d207a…`
- three-level C++ hierarchy/path test:
  `9b0c76b04fed398407628426222276aeaf4e543cdf1dcd25754c08b907e1f4c7`
- reusable-family C++ / RTL tests: `323953507b7ff79b6b9ba1fa2f45fc30ea6e7c62c22770213aa01d55f243e425`
  / `46711dde6f72e9d7d5edcf360a1b4606e2f710c8bb55200952d620a8597feafd`
- executable backend oracle: `c0089a51bb0773ccd1e16d6627412b1945e8a348632a66eadc0712896df8d4d4`
- Numeric N0-A source-math foundation:
  `ACIROps.td=34641b1e5972c06246138984b846606746eebc9597fc33e090febde703d75bd7`,
  `ACIRMathOps.cpp=6a4ea8123ccbe71060c130fa4c671922cee9e694a602d35b9d3fee077a4c9a2d`
- Numeric N0-B0 constant exact witness:
  `ACIRNumericProofOps.cpp=a509d88416219f970acd29445f3471d911be42797a70153b3542fed2d8537daf`,
  `NumericExactProofContractsTest.cpp=94aacc2ca9920186cd4520c59387df17abc28413c94c42cdb93fb1695947b293`
- Numeric N0-B1a constant-add exact witness:
  `ACIRNumericExactAdd.cpp=82e1b3847b2a61d29c41efcfa72e6501a296368f77da03989e079451cc8fb42c`,
  `NumericExactAddProofContractsTest.cpp=49c1c3067d7c842017fa16d1cda0ff532d37f4a41d336f9b51286cd9f29d7814`
- Numeric N0-B1b variable/from_bits add witness:
  `ACIRNumericExactInputAdd.cpp=b508b61d7cfb9cc7477dd9a3fe55d77631012696f04183cb53853edf4b59d685`,
  `NumericVariableAddProofContractsTest.cpp=a5acea3ccb3356ab92d5d211f179a449306c36ffebc0ad9f5abf515069925093`
- Numeric N0-C1 transactional compiler lowering:
  `ScalarNumericLowering.cpp=7dda13148f2515d5ae30c1f1fcc53baef1f06da7c4b3c9872ef8c770b76e5e1f`,
  `ScalarNumericLoweringAnalysis.cpp=c300466377fc093b1996616332869310703a43583826ac71323d07483322d742`,
  `ScalarNumericLoweringTest.cpp=907b2a27c15a5a4d6e9564f487782b4575e9507552bea1f62bea955ce6411843`

## Accepted sub-slice

The independent re-review accepts the frozen FinalSystem closure:

- every ProposalGraph StateID has exactly one owned `ac.reg` carrier;
- root data formals without a system-owned register are rejected;
- child formals remain aliases and never allocate storage or reset authority;
- the frozen instance table records definition, static arguments, module,
  placement, parent, ordered children, owned-state ordinals, and local graph
  slices;
- proposal contributions, commit pairs, checks, observations, and their rule
  anchors are frozen after cloned-IR rebinding and verified field by field;
- source-unit permutation preserves the semantic instance/state order;
- mutations of instance, carrier, alias, contribution, check, observation,
  proof, permit, target, and residual source facts fail before either emitter;
- generated C++ and Verilog are obtained from the same verified EmitReady
  object, and the harness stages all outputs before replacement with rollback on
  failure.

The generated C++/Icarus oracle executes reset, enabled and disabled D/E,
global permit failure, observation gauge commit/discard, failed-cycle hold, and
reset rerun. The CTest harness is self-contained and no longer requires a
manually supplied unpublished executable.

The accepted bounded runtime now removes public module `Check` and `Drive`.
`SimSystem` performs all-module `Work(epoch)`, one generated system precommit,
then owner `Xfer`; the generated owner calls `Write(D,E)` and primitive Xfer in
that method. Failed precommit performs zero Xfer and preserves Q/cycle. Reset
uses only reset transfer, zero-rule Step returns Quiescent while the system
remains Ready, repeated events form one committed batch per successful epoch,
and a partial Build failure cannot be revived by Reset.

The follow-on realization and C++ multi-instance sub-slices are also accepted:

- ordered rule inputs/outputs, placement facts, supported expression DAG
  operations, exact rule/block ownership, instance partitions, and postorder
  ordinals are frozen and mutation-tested;
- one reusable `FinalModuleDef<N>` class is emitted per exact SpecKey, while
  repeated instances are distinct nested objects with independent state and
  scratch;
- formal inputs are injected read-only views; physical registers exist only in
  their owner class; child proposals are copied to owner commits in frozen
  postorder;
- lifecycle helpers are private, modules register before Build, and root
  object-tree validation runs once from `FinalizeBuild`;
- a descendant check failure performs no failed-cycle publication or commit,
  and Reset/rerun plus source-unit permutation are deterministic.

The C++ hierarchy follow-up is independently accepted:

- reusable SpecKey classes resolve rule reads by frozen formal handle and
  parameter/element identity, and child constructors consume exact frozen
  placement actuals;
- every generated object has an immutable non-owning parent pointer and stable
  actual path (`root/...`), while the parent remains the sole owner of child
  objects;
- repeated nonleaf families use definition-local child positions for member
  access, while placement names determine user-visible instance paths;
- generated modules and systems cannot be copied or moved, and root-only
  Freeze validates parent, path, ReadView bindings, and exact-once entry;
- UTF-8 placement components use valid fixed-width C++ byte escapes and retain
  their exact runtime bytes;
- the executable three-level test inspects all runtime parent/path/name values,
  rejects a second Freeze, and proves a corrupted child parent causes Build to
  fail and remain failed across Reset, without adding a product test API.

The hierarchical Verilog H1 sub-slice is independently accepted as well:

- one reusable RTL family is emitted per source definition while repeated leaf
  and nonleaf instances retain distinct physical state and hierarchy objects;
- rule reads and child connections use frozen formal identity and placement
  handles, so an alias pattern in the representative instance cannot collapse
  two distinct formals in another instance;
- only owned registers create `always_ff` storage, formal D/E remains a
  proposal boundary, and `root_commit_ok` gates each physical owner E exactly
  once;
- child error propagation uses local child positions, and structured
  observation paths use the same positions through repeated nonleaf families;
- the simulation wrapper captures old-Q values before the edge, publishes with
  `$strobe` after NBA settlement, and passes strict Verilator lint for both the
  synthesis and simulation tops;
- the three-level `Root -> Bridge -> Probe` executable fixture verifies tied
  versus distinct read-only aliases, exact owner/site/value/epoch records, and
  Reset/rerun behavior.

Numeric N0 checkpoint A is independently accepted as a source-math foundation:

- mathematical binary operations now admit `add`, `sub`, and `and_bits` without
  introducing a finite storage domain on the mathematical result;
- `ac.math.compare` admits the closed `eq/ne/lt/le/gt/ge` predicate set and
  produces source bool i1, while each operand's signed interpretation remains
  fixed at its own `from_bits` boundary;
- `MathFromBits` accepts a direct rule input or its exact earlier SourceRead in
  the same block and rejects forged, cross-owner, nested-block, width, and
  logical-domain substitutions;
- the source op defines its valid result; structural validity equations remain
  the responsibility of the later exact proof/lowering verifier.

This checkpoint does not implement `ac.value.binding`, `ac.numeric.proof`,
scalar numeric lowering, Python BinOp/Compare production, graph admission, or
backend arithmetic execution.

Numeric N0-B0 is independently accepted as the first exact-witness slice:

- typed `ac.value.binding` and `ac.numeric.proof` retain the approved full
  field spelling while the admitted capability is deliberately limited to one
  constant exact witness;
- the verifier closes ProofScope, required_numeric, ValueID, evidence origin,
  owning definition, source path, actual SSA, true path/valid controls, and
  evidence ordering without a backend interpreter;
- the mathematical constant must use a canonical singleton integer domain,
  minimum signless i1..i64 storage, and an identical finite SSA bit pattern;
- `0`, `1`, `-1`, `UINT64_MAX`, and `INT64_MIN` pass; both i65 boundaries,
  `low_bits`, nonconstant recipes, inputs, checks, malformed segments, extra or
  missing operands, foreign origins, UnknownLoc, and missing/duplicate evidence
  reject;
- Rule verification enforces one-to-one required_numeric coverage. Existing
  graph, FinalProgram, and backend proof rejection remains unchanged.

B0 does not admit from_bits/add/sub/compare/and_bits witness recipes or numeric
lowering and does not complete N0.

Numeric N0-B1a is independently accepted for one constant-add exact recipe:

- required_numeric and obligations contain ordered constant A, constant B, and
  `add(node0,node1)` nodes with three distinct ValueIDs and ValueBindings;
- each constant and the result use canonical singleton domains and minimum
  signed or unsigned i1..i64 storage;
- the verifier matches the existing finite SSA graph: minimal constants,
  required direct `extui`/`extsi`, and one flag-free `arith.addi` whose actual
  operands may be exchanged without changing source node order;
- arbitrary-precision MathInt comparison occurs before finite conversion, node
  indices use closed u32 decoding, and mixed signedness is extended separately
  before APInt addition;
- signed/unsigned boundary additions, actual operand exchange, malformed
  recipe/identity/extension/add/control/inventory cases, constants above 66
  bits, BoolAttr indices, i65 results, and result-narrowing capability are
  independently covered.

B1a does not admit variable/from_bits leaves, sub/compare/and_bits recipes,
range checks, truncation, graph admission, FinalProgram lowering, or backend
arithmetic execution.

Numeric N0-B1b is independently accepted for one declared current input plus a
constant exact-add witness:

- four scoped IDs/bindings separate authoritative input I, from_bits result F,
  constant C, and sum S; I/F share the exact SourceRead SSA and domain without
  merging identity;
- proof input lists, segment sizes, actual operand count, required_numeric and
  binding order form one closed profile before any operand indexing;
- input authority is checked against the real rule handle, StateRef, owning reg
  or current formal PortSlot, physical type, and declared integer domain;
- arbitrary-precision interval arithmetic independently derives
  `[L+k,U+k)`, canonical signedness and minimum i1..i64 storage, rejecting
  narrowing and i65 capability;
- the verifier matches the existing SourceRead, constant, direct extui/extsi,
  and flag-free addi graph without modifying IR;
- a test-private evaluator executes the actual finite SSA graph for every value
  in representative small intervals and self-checks that changing an actual
  arithmetic edge changes the observed result.

B1b does not make Python `current + 1`, proposal graphs, FinalProgram, C++ or
RTL arithmetic execution available.

Numeric N0-C1 is independently accepted as the first compiler-produced scalar
lowering:

- the private pass accepts only a verified source implementation unit and
  requires its existing canonical proof scope;
- I is derived from SourceRead origin slot zero while the supplied F/C/S IDs
  and slots are preserved;
- the whole unit is cloned, every supported rule is lowered, and the complete
  clone plus independent B1b witnesses verify before root attributes/body are
  published; failure is caller-byte-preserving and root identity remains
  stable;
- exact arbitrary-precision interval arithmetic creates canonical finite
  domains, per-operand `extui`/`extsi`, and one flag-free `arith.addi`;
- source math, dead controls, damaged output, unsupported inventory, and mixed
  source/evidence states are closed by implementation and regressions;
- valid lowered/no-op units are idempotent, while positive multi-rule and
  later-rule rollback cases are independently covered.

The final architecture and code re-reviews pass after repairing repeated
control cleanup and unit-wide mixed-evidence classification. Detailed candidate
binding and evidence are in
[the N0-C1 review](20260929-n0-c1-scalar-lowering.md). The pass remains private;
Python production and graph/final/backend numeric admission are still closed.

Numeric N0-D1–D3 are now independently accepted as the remaining bounded
scalar closure for this phase:

- D1 lowers exact input-constant subtraction and all six comparisons with
  sign-aware common representations and canonical bool results;
- D2 lowers exact `and_bits` and explicit full-mask low-bit add/sub without
  inventing a binding for an elided wide intermediate;
- D3 lowers one checked integer boundary using separate demand and safety,
  one owned range check, and an exact guarded `scf.if` conversion;
- all three retain whole-unit transactionality, source ownership, stable IDs,
  idempotence, multi-rule rollback, and closed evidence inventories.

Detailed candidate binding and review disposition are recorded in
[the D1–D3 review](20260929-n0-d1-d3-numeric-closure.md). Python production,
FinalProgram numeric admission, and backend arithmetic remain closed.

## Evidence

From the candidate checkout:

```text
cmake --build .pycircuit_out/w10-pm/build --target \
  ACIRFinalProgramTests ACIRObservationContractsTests \
  ACIRBackendClosureTests ACIRExecutableBackendClosureTests \
  ACIRModuleGraphTests ACIRProposalContractsTests \
  ACIRCheckContractsTests ACIRSourceLinkAdmissionTests -j 6

env -u ACIR_BACKEND_CLOSURE_HARNESS \
  ctest --test-dir .pycircuit_out/w10-pm/build --output-on-failure \
  -R 'ACIR(FinalProgram|ObservationContracts|BackendClosure|ExecutableBackendClosure|ModuleGraph|ProposalContracts|CheckContracts|SourceLinkAdmission)Tests'
```

The final PM run additionally included RegRuntime, SystemLifecycle, and the
observation runtime:

```text
env -u ACIR_BACKEND_CLOSURE_HARNESS \
  ctest --test-dir .pycircuit_out/w10-pm/build --output-on-failure \
  -R 'ACIR(RegRuntime|SystemLifecycle|Observation|FinalProgram|BackendClosure|ExecutableBackendClosure|ModuleGraph|ProposalContracts|CheckContracts|SourceLinkAdmission)Tests'
```

Result: the complete configured ACIR lane passed 19/19 CTest targets.
`ACIRFinalProgramTests` contains 47/47 passing cases. The executable
backend target includes repeated-child C++, hierarchical RTL, strict
Verilator/Icarus, and three-level reusable-family alias oracles. Its final
focused inventory is 9/9 passing cases, including UTF-8 runtime paths and a
corrupted-parent Build failure. `git diff --check` and clang-format verification
passed. `ACIRSourceMathContractsTests` contains 75/75 passing
N0-A/B0/B1a/B1b/C1/D1/D2/D3 cases.

The remaining approved end-to-end matrix was run separately and remains red:

```text
ACIR_BACKEND_CLOSURE_HARNESS=<current harness> \
  pytest -q tests/system/test_unified_register_backends.py \
  --junitxml=.pycircuit_out/w10-pm/v41-v44.xml
```

Result after removing synthesized result summaries: 1 passed and 7 failed
fail-closed. The harness now compiles and runs its minimal generated C++ and
Verilog artifacts, but the required backend-by-system numeric executions,
terminal Result/event streams, zero-rule FinalProgram, scheduling permutations,
and source-reorder mappings remain unavailable.

## Verdict and remaining blocker

The [Python numeric producer bridge](20260929-python-numeric-bridge.md) is
accepted for one unused local add/sub/comparison expression per rule. Real
Python now produces the C1/D1 source recipe and reaches finite lowering through
the private harness. Its 32 new tests and 68 combined frontend tests pass.
Numeric results still cannot feed next-state writes, conditions or observations;
the next step is numeric-use and yield ownership, followed by Graph/Final/backend
admission.

Verdict for the frozen FinalSystem/backend-closure sub-slice: **accepted**.

An independent Sol/high re-review found no remaining issue in the bounded
single-instance precommit lifecycle after the event-batch, partial-Build, and
executable-state regressions. That sub-slice is **accepted**.

Overall W10 remains **request changes**. C++ and RTL hierarchy, physical owner
storage, formal identity, transactional commit, structured observations,
three-level reusable-family execution, the N0-A source-math operation
foundation, the N0-B0/B1a/B1b exact-witness slices, N0-C1 input-add lowering,
and N0-D1/D2/D3 scalar numeric closure are accepted. V41-V44 still require
Python arithmetic/control-flow production, graph/backend execution, lifecycle Result
handling, runner events, schedule permutations, and semantic source-reorder
mapping. W11 must not start while those W10 exits remain open.
