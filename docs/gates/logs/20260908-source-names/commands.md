# Reproduction

Run from /home/lc/pyCircuit through /home/lc/.codex/skills/pyc6/scripts/run.sh.
The commands below use `RUN` as an abbreviation for that executable.

```sh
RUN=/home/lc/.codex/skills/pyc6/scripts/run.sh
$RUN cmake --build .pycircuit_out/local-clang22/build --target install -j 12
$RUN python -m pytest tests/python/agentic-circuit/python_frontend/test_source_names.py tests/integration/agentic-circuit/e2e/test_source_names.py tests/python/agentic-circuit/python_frontend/test_rule_literal_context.py -q --tb=short
$RUN python -m pytest tests/python/agentic-circuit/python_frontend -q --tb=short
$RUN .pycircuit_out/local-clang22/build/bin/GfsimTests --gtest_filter='ReplayTest.*'
$RUN python -m pytest tests/integration/agentic-circuit/e2e/test_queue_codegen.py -k 'circular_rob or reusable_rob_scan or host_result_boundary or nested_python_module_calls' -q --tb=short
$RUN env PYC_DAVINCIOO_ROB_OUT=/home/lc/pyCircuit/.pycircuit_out/davincioo-rob/20260908-source-names python -m pytest designs/davincioo/tests/spe/ooo/test_rob.py -q --tb=short
$RUN python -m pytest tests/python/agentic-circuit/cli/test_run_command.py -q --tb=short
$RUN env LD_LIBRARY_PATH=/home/lc/pyCircuit/.pycircuit_out/replay/browser-deps/usr/lib64 PLAYWRIGHT_BROWSERS_PATH=/home/lc/pyCircuit/.pycircuit_out/replay/browser-cache node third_party/circuit-flow-viewer/tests/browser_source_names.mjs .pycircuit_out/replay/browser/node_modules/playwright .pycircuit_out/davincioo-rob/20260908-source-names/recovery/replay.html .pycircuit_out/davincioo-rob/20260908-source-names/browser
$RUN env LD_LIBRARY_PATH=/home/lc/pyCircuit/.pycircuit_out/replay/browser-deps/usr/lib64 PLAYWRIGHT_BROWSERS_PATH=/home/lc/pyCircuit/.pycircuit_out/replay/browser-cache node third_party/circuit-flow-viewer/tests/browser_nested.mjs .pycircuit_out/replay/browser/node_modules/playwright .pycircuit_out/davincioo-rob/20260908-source-names/recovery/replay.html .pycircuit_out/davincioo-rob/20260908-source-names/browser
```
