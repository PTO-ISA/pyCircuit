# Agentic Circuit commit-group runtime evidence

Date: 2026-09-05

Decision: 0162

Scope: gfsim Queue/Table prepare-publish-no-fail commit substrate and
Table-plus-Queue transition regressions. This evidence does not claim the
stateful `@ac.rule` MLIR or PYC/RTL follow-on work is complete.

## Results

| Lane | Command | Result |
| --- | --- | --- |
| Native build | `ninja -C .pycircuit_out/toolchain/build GfsimTests` | PASS |
| gfsim unit and integration tests | `.pycircuit_out/toolchain/build/bin/GfsimTests --gtest_brief=1` | PASS: 250 tests |
| Repository Python unit lane | `pytest -q tests/unit -m unit` | PASS: 18 tests |
| Existing direct Table backend | set `ACIR_OPT`, `ACIR_QUEUE_PLAN`, `ACIR_QUEUE_CXXGEN`, and `ACIR_QUEUE_PYCGEN` to nonexistent paths; run `PYTHONPATH=python/agentic-circuit/src pytest -q tests/integration/agentic-circuit/e2e/test_table_backend.py` | PASS: 6 tests; native-tool aggregate skipped and covered below |
| Frozen native Table plan/C++ | freeze `table_scoreboard.py` ACIR with `acir-opt --ac-freeze-topology`, then run `acir-queue-plan`, `acir-queue-cxxgen`, and compile the generated C++ | PASS: 1 Table, 1 writer, 7 blocks |
| Patch hygiene | `git diff --check` | PASS |
| Decision status | `python3 flows/tools/check_decision_status.py --status docs/gates/decision_status_v6.md --out .pycircuit_out/gates/20260905-ac-commit-groups-r1/decision_status_report.json --require-no-deferred --require-all-verified --require-concrete-evidence --require-existing-evidence` | PASS: 162 rows, 0 deferred |
| Agentic repository contract | `uv run --with jsonschema==4.25.1 --with pyyaml==6.0.2 python3 tools/agentic-circuit/check-contracts.py` | PASS: epoch 0.5, LLVM 22.1.8 |
| Documentation | `mkdocs build --strict` | PASS |

## Covered contracts

- Queue reservations are owner-tagged, exclusive, cancellable, and fail closed
  if they reach Xfer unpublished; grouped blocks reject null endpoints.
- `QueueAtomicTransform` and `QueueBarrier` reserve all endpoints before
  publishing any proposal.
- SimTable reservations use the actual index and top-level field footprint,
  preserve replace-after-field-merge ordering, and fail closed at Xfer.
- The lower-level Table-plus-Queue transition block proves allocate/replace,
  masked patch, read-and-remove under backpressure, route branch stability, and
  full cancellation after a Table conflict. Published proposals remain sealed
  under local transition reset, and Table activation is independent of resource
  ObjectId order.
