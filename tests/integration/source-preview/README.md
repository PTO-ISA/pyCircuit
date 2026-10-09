# source preview from-source preview fixture

`src/design_top.py` is the reusable hardware design. `counter.py` is compiled
as its own source unit and instantiated twice; `types.py` is a separate
declaration unit. The normal design has no `@system`, self receiver, stimulus,
checker, clock loop, or hand-written model runner.

The `hold_top.py`, `zero_rule_top.py`, and `failure_top.py` roots are focused
runner contract cases. They exercise registered hold, zero-rule quiescence,
and a source invariant failure after one committed step. `configs/` contains
finite canonical configurations. `oracle.py` is independent of the runner and owns
the expected values, epoch/status checks, and strict JSONL completion checks.

This is a source-tree preview fixture. It does not claim a released C ABI,
installed SDK, or production-compatible generated C++ class ABI.

## Reproduce the workflow

See the [complete build, run and validation recipe](../../../docs/development/source-unit-workflow.md).
It builds this checked-in project without pytest generating any model or runner.
