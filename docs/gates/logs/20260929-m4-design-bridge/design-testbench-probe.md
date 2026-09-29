# design / testbench boundary probe — module-root design artifact (2026-09-29)

Purpose: find the **smallest real change** that yields an independent design
artifact separate from a system testbench, without inventing IR or
self-approving an interface. Per the user's naming rule an `.ac` artifact is
named after the Python source file it comes from (`<stem>.ac`), so the probe
now writes `increment.ac` / `blinker.ac` / `test_increment.ac` rather than a
reserved label. Companion to
`../../reviews/20260929-design-testbench-ir-authority.md` in the planning branch.

Method: read-only probe using the committed `acir-design-harness` (`ede5aec7`)
on real Python sources compiled per source unit, linked at three different roots.
Script: `probe/m4_design_probe.py` (deterministic, no repo writes).

```
P=/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit
PYTHONPATH="$P/python/pycircuit/src:$P/python/semantic-core/src:$P/python/agentic-circuit/src" \
  python3 docs/gates/logs/20260929-m4-design-bridge/probe/m4_design_probe.py /tmp/m4-design-probe
```

## Observed results

| Scenario | Root | Link | Artifact | Emit |
| --- | --- | --- | --- | --- |
| A — DUT-only closure, `@module` **with typed ports** (`Increment(enabled, incoming, outgoing)`) | `demo.increment.Increment` | **rc=1**: `selected root has an unbound data formal or synthetic root StateID`, then `final program ProposalGraph storage closure failed` | none | — |
| B — DUT-only closure, **portless** `@module` (`Blinker` with two regs and one rule) | `demo.blinker.Blinker` | rc=0, 36 640 bytes | `ac.system` present (entry = the DUT), `ac.expect` present (**the design's own range checks**), no `ac.observe`, no stimulus identifiers | cpp 10 347 B; verilog 3 467 B |
| C — current M2 shape, `@system` carrying stimulus/phase/check/report | `demo.test_increment.TestIncrement` | rc=0, 193 173 bytes | `ac.system`, `ac.expect`, `ac.observe` **and** `TestIncrement` all present | cpp 31 021 B; verilog 11 822 B |

`FinalModelSim` (the simulation observation wrapper) appears in the **Verilog
output of every scenario, including both design-only scenarios**, in the same
text as the hardware `FinalModel`. The C++ output of B and C contains no
`FinalModelSim`, but it does carry the hardware `Work`/`Xfer`/`HasWork`/
`ChecksPass` rule-commit lifecycle, which is hardware semantics rather than
testbench stimulus.

## Conclusions

1. **A DUT-only design artifact already exists today for a portless root**
   (`blinker.py` → `blinker.ac`). That
   is exactly C3-C's approved base root: "基础 root 是普通 portless `@module`".
   Nothing new is needed to *name* or *produce* a design artifact in that shape,
   and its `ac.expect` entries are the design's own range checks, which the audit
   explicitly allows to remain in the design.
2. **A real DUT with typed external inputs/outputs is blocked on the external
   typed DUT I/O contract.** Scenario A fails because the module's formals have
   no parent to bind them once the module *is* the root. Delivering
   a ported design artifact therefore needs the D05
   external typed DUT / port contract, whose revision B is independently
   approval-ready but **not yet approved by the user**. This — not renaming and
   not more emitter work — is the actual blocker behind the user's
   "generated output should be a design and a testbench" correction.
3. **The design/testbench *artifact* separation is still not real for the
   current M2 fixture.** Scenario C shows stimulus, phase, checks and report
   inside the same artifact as the hardware, because the selected root is the
   test system.
4. **Generated file roles are not yet separated.** C3-C already approves the role
   set `header/source/cmake/rtl/runtime-glue/source-map`, so emitting the
   hardware `FinalModel` and the simulation wrapper as different role files is
   executing an approved contract, not adding one. Today no new-route bundle
   writer emits `generated.json` at all; `emitFinalVerilog` returns a single
   text blob containing both.

## What this changes about the next packets

- The boundary work cannot be closed by renaming or by the private bridge. It
  needs (a) the approved D05 typed-port contract for a portable DUT, and
  (b) a produced-artifact split whose roles C3-C already fixes.
- Until D05 is approved, the honest M4 scope is: portless module-root design
  artifacts, the generated-role split, and keeping the system path as the
  testbench — stated as such, not as an independent DUT delivery.
