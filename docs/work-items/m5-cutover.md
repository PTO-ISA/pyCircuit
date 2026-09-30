# M5 accepted: single-route cutover

Status: done for the revision-8 declared profile. Accepted: 2026-10-01.
Implementation commit: `d351079b` on `codex/gfsim-source-units`.
The planning checkout does not contain the implementation branch's source changes.

M4 has zero required remainder for its accepted current-platform workflow.
M5 now provides public compile/link/emit, source-owned C++ and RTL artifacts,
per-owner provenance inventories, one runtime/model ABI, isolated Runtime and
CompilerDev packages, and a self-contained current-platform wheel. The old
Python/JIT/PYC/QueueGraph routes, aliases, installers, active examples and dead
gate runners are retired. Runtime-only consumers do not require LLVM.

- [Full work item and supported profile](https://github.com/PTO-ISA/pyCircuit/blob/d351079b/docs/work-items/m5-cutover.md)
- [Independent Sol and Astra review / PM acceptance](https://github.com/PTO-ISA/pyCircuit/blob/d351079b/docs/reviews/20261001-m5-cutover-review.md)
- [Candidate-bound gates and manifest](https://github.com/PTO-ISA/pyCircuit/blob/d351079b/docs/gates/logs/20260930-m5-cutover/README.md)
- [Retirement/oracle ledger](https://github.com/PTO-ISA/pyCircuit/blob/d351079b/docs/work-items/m5-retirement-ledger.md)

Final product binding:
`8ae474b2a0c98d2909658d502f6c6763a20335b5754d7bba4d132bfaae8c1690`.
Verification: 406 system tests passed (2 agreed M6 V44 exclusions), 309 unit
tests passed (3 Windows-only skips), 20/20 native targets and 2 documented
source/example tests passed. Pre-commit, strict MkDocs, SDK schema, installed
retirement, Runtime/CompilerDev and wheel-prefix checks pass.

C3-SM B is now precisely approved; the frozen proposal and approval record are
in this planning branch. SYSTEM/EXPECT revision B remains outside scope.
M3 capability backlog, M6 hardening/platform/parallel work and M7 release work
remain. No release tag/PyPI publication, protected-branch merge or remote
branch-protection mutation is claimed.
