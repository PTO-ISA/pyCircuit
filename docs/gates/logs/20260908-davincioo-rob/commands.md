# Reproduction commands

Run from `/home/lc/pyCircuit`. `RUN` below is an abbreviation, not a new tool:

```sh
RUN=/home/lc/.codex/skills/pyc6/scripts/run.sh
$RUN python -m agentic_circuit._cli doctor --json
$RUN cmake --build /home/lc/pyCircuit/.pycircuit_out/local-clang22/build --target gfsim -j 2
$RUN python -m pytest -q designs/davincioo/tests/spe/ooo/test_rob.py
$RUN python -m pytest -q designs/davincioo/tests/test_contracts.py designs/davincioo/tests/spe/iex
$RUN python -m pytest -q tests/python/agentic-circuit/python_frontend/test_rule_literal_context.py
$RUN python -m pytest -q tests/integration/agentic-circuit/e2e/test_queue_codegen.py -k 'circular_rob or reusable_rob_scan or host_result_boundary'
$RUN python -m pytest -q tests/python/agentic-circuit/python_frontend/test_queue_frontend.py tests/integration/agentic-circuit/e2e/test_queue_codegen.py
$RUN python -m pytest -q tests/python/agentic-circuit/python_frontend tests/python/agentic-circuit/contracts tests/python/agentic-circuit/cli designs/davincioo/tests/test_contracts.py designs/davincioo/tests/spe/iex
$RUN python -m unittest discover -s tests/python/agentic-circuit/tools -p test_replay.py -v
$RUN env PYTHONPATH=third_party/circuit-flow-viewer/src python -m unittest discover -s third_party/circuit-flow-viewer/tests -v
$RUN python designs/davincioo/tools/check_catalog.py
$RUN mkdocs build --site-dir .pycircuit_out/davincioo-rob/20260908-rob/site
$RUN python flows/tools/check_decision_status.py --rfc docs/rfcs/pyc6-decisions.md --status docs/gates/decision_status_v6.md --out .pycircuit_out/davincioo-rob/20260908-rob/decision-status.json
$RUN python flows/tools/check_decision_status.py --rfc docs/rfcs/pyc6-decisions.md --status docs/gates/decision_status_v6.md --out docs/gates/logs/20260908-davincioo-rob/decision_status_report.json --require-no-deferred --require-all-verified --require-concrete-evidence --require-existing-evidence
```

`test_rob.py` is the repeatable model/driver/trace/HTML generator. Override
`PYC_DAVINCIOO_ROB_OUT` to retain a separate run. It records only after the normal
functional run passes, and checks full projection equality with recording.

The independent browser command uses the already-installed local development
browser dependencies. It required sandbox escalation for Chromium's system calls:

```sh
$RUN env LD_LIBRARY_PATH=/home/lc/pyCircuit/.pycircuit_out/replay/browser-deps/usr/lib64 PLAYWRIGHT_BROWSERS_PATH=/home/lc/pyCircuit/.pycircuit_out/replay/browser-cache node third_party/circuit-flow-viewer/tests/browser_nested.mjs .pycircuit_out/replay/browser/node_modules/playwright .pycircuit_out/davincioo-rob/20260908-rob/recovery/replay.html .pycircuit_out/davincioo-rob/20260908-rob/browser
```

Formatting used `pre-commit run --files <changed-files>` with `SKIP=black`, then
`black --check <one-file>` for each affected design Python file. This avoids the
sandbox-stalled multiprocess formatter while applying the same rules. The new
C++ driver was formatted with `clang-format`.
