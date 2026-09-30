# M4 completion acceptance

Status: **done**, scoped to revision 8's documented macOS source-tree preview.
Implementation: `codex/gfsim-source-units`, commit `26e096c0`.

The user requested continuing until M4 was complete. The checked-in workflow now
independently compiles every Python source through public compile/link, saves
`design_top.ac`, builds actual source-owned C++ translation units and Verilator,
and runs both with one SystemRunner/SimExecutor. The external testbench owns
expected values. Explicit observation sinks are protected; the user clarified
that omitted `--events` is silent. Duplicate report names per actual instance
are rejected by common MLIR verification.

- [Reproducible build/run guide](https://github.com/PTO-ISA/pyCircuit/blob/26e096c0/docs/development/migration-preview.md)
- [Final implementation work item](https://github.com/PTO-ISA/pyCircuit/blob/26e096c0/docs/work-items/m4-completion-workflow.md)
- [Candidate-bound evidence and reviews](https://github.com/PTO-ISA/pyCircuit/blob/26e096c0/docs/gates/logs/20260930-m4-completion/README.md)

Independent focused lane: 17 system plus 10 protocol tests, zero skips.
Existing regressions: 331 passes, three Windows-only skips. Native: 116 passes
in seven binaries. A fresh 106-step native compiler build and fresh documented
build/run succeeded; both runners produced identical 13-line captures accepted
by the external oracle. Strict MkDocs and applicable pre-commit passed.
Independent Sol code review and Astra architecture/M4-exit review both APPROVE.

The revision-8 M4 exit does not require all full C3 packaging. Full DUT ABI/source
maps, source-owned RTL distribution, installed SDK, public new emit and old-route
retirement remain explicit later work. SYSTEM/EXPECT B remain unapproved.
