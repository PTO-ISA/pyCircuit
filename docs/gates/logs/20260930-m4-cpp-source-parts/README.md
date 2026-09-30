# M4 executable-source C++ parts — 2026-09-30

Base: `3a54380226519f4e4d22600387356fb9699eb4a3`. Candidate identity is the
17 compiler/test files in `candidate-files.json`. Current checkout:
`/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.

## Delivered scope

Private native `emitFinalCppSourceParts` consumes the same verified FinalProgram
as the existing C++/RTL paths. One structured emission core produces support,
per-SpecGroup declaration/constructor/method sections and system glue. The
existing private monolithic renderer concatenates those sections; the grouped
renderer routes them directly by verified module SourceOwner. It never splits
a whole generated C++ string. Instance OwnerRef is not used as file ownership.

Each executable source owns one hpp/cpp group with C3 source-derived namespace,
class-template family/empty specialization, state and child names. Actual
by-value children are included; parent friend templates are forward-declared.
Shared support and inline system glue are separate. Names and paths collide
closed, without hashes or numbered disambiguation. Scalar UnitAttr and indexed
u64 PortSlot ordinals use the existing shared verifier interpretation.

The new non-installed harness only transports this in-memory result as JSON
for compiler tests, after complete verification. It adds no public CLI, ODS,
IR primitive, runtime, ABI or generated.json schema. Current final IR retains
implementation units but drops declaration-only units; this slice does not
invent declaration headers. Nonempty static specialization, public emit,
complete generated-directory publication and full M4 remain outstanding.

## Organization

- Architect: `other_agent_arch_review`, Astra xhigh, read-only scope/design advice.
- Repository exploration: `cpp_bundle_explore`, Astra low, read-only ownership map.
- Implementation: `unit_pair_fix`, Luna high, names/layout/shared emitter core.
- Independent tests: `cpp_parts_tests`, separate Luna high, source-level oracle.
- PM: structural source-parts renderer, system naming hookup, private harness,
  CMake/build integration, scalar-ordinal/SmallVector fixes, formatting/evidence.
- Independent review: `other_agent_code_review`, Sol high, no authorship;
  [APPROVE](review.md), 17 hashes verified, independent focused rerun 5/5.

## Evidence

- Current-checkout targeted native build passed (`build-frozen.log`). No shared
  binaries were copied from other worktrees. Native build emits existing
  duplicate-library linker warnings; they are not suppressed.
- `python-final.xml/log/command.txt`: **68 passed**, no failures/errors/skips:
  new source-parts 5, generic roundtrip 7, masked-next 15, design bridge 41.
- `ACIRFinalProgramTests-final.xml/log`: **51 passed**; and
  `ACIRExecutableBackendClosureTests-final.xml/log`: **18 passed**. No failures,
  errors, skips or disabled cases. Exact commands are `native-final.command.txt`.
- After the final formatting and identical 99-keyword table representation
  cleanup, the source-owned path was rebuilt and rerun:
  `focused-frozen.xml/log/command.txt`: **5 passed**, zero failures/errors/skips.
  The legacy path does not consult that source-name keyword table. No existing
  assertion was relaxed. Candidate hashes bind the final code and test bytes.
- `legacy-byte-comparison.txt`: saved baseline final IR emitted exactly the same
  monolithic C++ bytes before/after the structured-core change. This is a bounded
  fixture check, not a claim of universal textual equivalence.
- Applicable changed-file pre-commit checks passed (merge conflicts, whitespace,
  Ruff, Black, Markdown, API hygiene); YAML had no applicable files.

The independent test removes Python and body/header inputs before fresh-process
final-only emission, compares repeated/reversed-unit output, compiles each header
standalone, compiles every source cpp to a separate object, and links two consumer
TUs. Omitting the child object fails with unresolved methods. Two instances share
one child type/TU, with distinct child state and parent-borrowed outputs; reset
of the same object repeats `[0, 0, 3, 10]`, matching monolithic C++ and RTL.
Six register declarations describe seven instantiated registers (five parent,
one in each of two child objects). No unity build or cpp inclusion is used.

Static-argument negative coverage is deliberately limited: a structurally valid
nonempty root SpecKey on a minimal design is rejected by common final
rehydration because that specialization has no materialized module definition.
It does not execute the grouped layout's separate empty-argument check.

Earlier failed build/test logs are retained to distinguish implementation fixes
from test-fixture corrections. The first build failed on LLVM's default inline
SmallVector size limit; explicit zero inline capacity fixed it. The first grouped
positive exposed integer-only ordinal decoding, fixed to accept valid scalar
UnitAttr. An initially malformed static mutation was corrected; its parser
failure is not semantic negative evidence. Raw tool logs retain whitespace.

## Separate pre-existing limitation

The independent author reproduced an existing link/reconstruction failure for
one rule containing `count = incoming; outgoing = count`, before grouped
emission was used. Source files, stderr and exit code are under
`multi-assignment-repro/`. The positive fixture instead uses two independent
one-output rules; it does not claim they are semantically interchangeable with
the failing source. This is a recorded M3 follow-up, not fixed by this package
and not a reopening of bounded M2 acceptance.
