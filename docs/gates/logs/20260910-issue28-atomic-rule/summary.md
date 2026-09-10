# Issue 28 atomic `@ac.rule` closure

## Scope

This slice removes the final single-input restriction from compiler-owned
conditional rule effects. A variadic-input rule may now select one functional
branch, reserve every input/output/state resource for that branch, and either
publish the complete transition or stall without falling through or partially
consuming state.

The public surface remains `@ac.rule`; no transition, safety assertion,
reservation, prepare, publish, or rollback API is exposed.

## Evidence

- `ACIROpsTests`: PASS, 1841 assertions.
- `CodeGenTests`: PASS, 6 tests.
- Focused gfsim selected-branch stall/reset test: PASS, 1 test.
- Public API/component and process tests: PASS, 11 tests.
- API hygiene: PASS.
- `git diff --check`: PASS.

The new MLIR regression lowers a two-input rule with heterogeneous optional
outputs through rule lowering, topology freeze, QueueGraph planning, native
gfsim C++ generation, and C++ compilation. The runtime regression proves a
blocked selected branch does not reroute or consume either input and that reset
clears every prepared resource before deterministic retry.

Existing stateful rule, multi-input rule, branch-local effect, and work/Xfer
evidence remains applicable:

- `docs/gates/logs/20260905-ac-stateful-rule-r1/summary.md`
- `docs/gates/logs/20260905-ac-multi-input-rule-r1/summary.md`
- `docs/gates/logs/20260906-branch-local-effects/summary.md`
- `docs/gates/logs/20260906-work-xfer-closure/summary.md`

## Result

Decision 0236 is implemented-verified for the generic ACIR/QueueGraph/gfsim
contract. Table writer arbitration and PYC/RTL Table realization remain owned
by Decisions 0237 and 0241 rather than this issue.
