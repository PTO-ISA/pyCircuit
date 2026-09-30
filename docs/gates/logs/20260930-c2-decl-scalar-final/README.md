# C2-DECL scalar final declarations — 2026-09-30

Status: accepted. Independent code and architecture reviews APPROVE.
Product base: `e7e5c51329ce874193333a1bf62c86b736af6cdd`.
Checkout: `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
The exact 34 compiler/test files are bound by `candidate-sha256.json`, SHA-256
`2c24efc6e9593eb25bfa946eb1a8d97b19c83bad0480628db410a8c1aab7c0ea`.
Approved contract: [C2-DECL A](../../../rfcs/migration/c2-decl-scalar-final.md),
[approval](../../../rfcs/migration/approvals/c2-decl-scalar-final.md).

## Delivered scope

Canonical scalar definitions survive from verified owning headers to source-owned
final units. Private/unused, implementation-owned, empty and facade units retain
their original ownership. Shared MLIR verification checks the exact final shape,
origin, scalar types/values, ordering and owner identities. Frozen FinalProgram
snapshots reject subsequent declaration/unit mutation. Fresh-final checks validate
the artifact itself and do not authenticate previously present unused declarations.

Both C++ renderers use the same native scalar declaration and naming/range checks.
Declaration owners generate standalone hpp only; implementation owners retain
hpp/cpp groups. Standard-library and GFSIM references are globally qualified.
Unrepresentable native constants remain exact valid MathInt in common IR and RTL,
but reject C++ emission before output. No new primitive, public CLI, manifest,
runtime/ABI, or SYSTEM/EXPECT B change is included.

## Organization and review

Implementation: unit_pair_fix (Luna high) owns shared semantics/projection;
decl_cpp_impl (separate Luna high) owns C++ emission. Independent test design:
other_agent_arch_review (Astra xhigh), supplying exact patches applied by PM;
a further Luna instance was unavailable at that time. PM owns integration,
CMake, qualification-only adaptations, formatting, builds and evidence.
Independent code review: other_agent_code_review (Sol high),
[APPROVE](code-review.md), independently replaying seven focused cases.
Independent architecture conformance: decl_arch_conformance (Astra xhigh),
[APPROVE](architecture-review.md). No test-execution claim is attributed to
the original read-only test designer.

## Results and reproduction

The private `run_lanes.py` records exact argv, cwd and native helper environment
in each lane's command JSON. Toolchain: LLVM/MLIR 22.1.8, Apple arm64 C++20,
Python 3.14.6 / pytest 9.0.2; all native helpers built in this checkout.
`build-command.txt`, `full-05.log`, `native-06.log` record successful builds.
Existing duplicate-library linker warnings remain.

| Lane | Result | Raw evidence |
| --- | --- | --- |
| New scalar declarations | 39 passed, zero failures/errors/skips | candidate-02/scalar.xml and log |
| Source C++ groups, generic assignments, design bridge, masked-next | 73 passed, zero failures/errors/skips | candidate-02/regression.xml and log |
| Source pair, namespaces, public driver, CMake DAG | 48 passed, one pre-existing failure | candidate-02/source-driver.xml and log |
| FinalProgram + executable dual backend native | 54 + 18 passed, zero failed binaries | candidate-03/native.xml and log |
| Applicable Python pre-commit | Passed; unrelated hooks not applicable | python-lint.log |
| Unicode 16.0 full casefold generation | Byte-identical regeneration | unicode-repro.log |

Baseline `baseline/` records two independent positive failures before C2-DECL:
missing owner inventory and missing declaration headers. Review also caught the
raw owner pair `foo/__init__.py` / `FOO/__INIT__.py`: both source link and fresh-final
incorrectly accepted it before the fix (`raw-owner-before.log` and
`raw-owner-projection-before.log`). Both now reject; raw structured owner folding
and reduced import-module folding are checked independently.

The initial native run (`candidate-02/native.*`) failed only its exact C++ text
assertion after required global qualification. Its single expected spelling was
updated to `::gfsim::SimDFFE<::std::uint64_t>`, retaining count 1 and every hardware
assertion; candidate-03 supersedes that native result. Product bytes are identical
between candidate-02 and candidate-03; only this native test literal differs.

## Known limits

The broader namespace suite retains one obsolete `@module class Pass/Root` fixture,
rejected before link by the existing function-only importer. The test and rejecting
implementation are byte-identical to base (hashes in
`pre-existing-namespace-files.json`); independent reviewer confirmed the cause.
This failing lane is disclosed rather than marked green or silently deselected.

Signed aliases/negative constants use focused valid IR declaration mutations;
this packet does not add Python syntax for those forms. Header-only compile checks
use only the generated include root, without runtime include paths. The existing
regression lane proves C++/RTL traces, reset/rerun, reg counts and alias behavior.

No full release, SDK/install, Windows or legacy full-toolchain build is claimed.
M2 remains accepted; this closes scalar declaration headers within M4 only.
Generated publication/manifest and full build/run delivery remain M4 work;
public emit and hard-break route removal remain M5.
