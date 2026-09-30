# M6-02 accepted: incremental source-unit build and scale baseline

Status: done for this bounded macOS-arm64 packet. Accepted: 2026-10-01.
Implementation/test/evidence commit: `25152f9c` on `codex/gfsim-source-units`.
Baseline: `477beae8`; M5's supported scalar, portless, default-clock,
empty-static-argument profile is unchanged.

Two real CMake/Ninja axes are measured through the public driver: shared
leaf definition at 1/16/64 instances with three source producers, and distinct
leaves at 1/8/32 sources with U+2 producers. Every source retains its owned
body/interface and generated CPP group. All six sizes pass deterministic
rebuild and literal C++ observation oracles; distinct-32 also builds/runs RTL
with 96 reports, TERMINATED at epoch 3 and null error.

- [Full packet and scope](https://github.com/PTO-ISA/pyCircuit/blob/25152f9c/docs/work-items/m6-incremental-scale.md)
- [Independent Sol high review](https://github.com/PTO-ISA/pyCircuit/blob/25152f9c/docs/reviews/20261001-m6-incremental-scale-review.md)
- [Measurement, raw logs and manifest](https://github.com/PTO-ISA/pyCircuit/blob/25152f9c/docs/gates/logs/20261001-m6-incremental-scale/README.md)

Ten-file reviewed binding:
`619fe3ae39b5624e7429080f9015b5143acc3f8c9f904b99da3d67c024ae5e4e`.
Focused tests: 10 passed on each Python 3.12.12/3.14.6; existing public
emit/RTL/model-ABI regression: 23 passed. Independent review APPROVE; hooks
and strict docs passed. No skips in these lanes.

The measurement exposed three bounded C3 build-integration defects, now fixed:
missing counter clean byproducts/dependencies; generated Verilog CMake JSON-list
serialization splitting space-containing paths; direct Python 3.12 preparation
helper imports shadowed by sibling `types.py`. Independent tests retain artifacts
and control locks, build RTL in-place with spaces, and reject symlink-plus-`..`
paths without mutation. No source/IR/runtime/ABI or schema contract was added.

All graph/emit/model no-ops execute zero compiler commands and preserve bytes
and mtimes. Leaf edits recompile the leaf and root, retaining independent
siblings. Full backend emit still republishes everything and rebuilds all CPP
TUs. Distinct-32 cold compile+link was 2.164 seconds; CPP/RTL emits 1.107/1.114
seconds and CPP model build 6.151 seconds on this run. These are unisolated wall
clock measurements including startup, with four build jobs and no thresholds.
RSS, throughput and parallel simulation are not measured. Local Verilator 5.044
has a separate Unicode generated-source/build directory JSON limitation; this
packet does not claim arbitrary Unicode host-directory support.

M4/M5 remain accepted for their declared profile. M6-01 and M6-02 are complete;
overall M6 remains open for platform/fault/SDK coverage, backend incremental
optimization and scheduling. M7 can next form a scoped preview acceptance
candidate without claiming future capabilities or publishing a stable release.
SYSTEM/EXPECT revision B remains unapproved. Other agents' planning edits remain
outside this packet and are preserved.

Planning validation: targeted hooks pass. This planning checkout's strict MkDocs
build still aborts on 12 existing missing links: eleven in the unchanged
`single-route-migration.md` ledger and one AGENTS link in the unchanged
`20260929-design-testbench-ir-authority.md` review. The implementation checkout's
strict build passes. This packet leaves unrelated planning links and the
unapproved SYSTEM/EXPECT proposal files untouched.
