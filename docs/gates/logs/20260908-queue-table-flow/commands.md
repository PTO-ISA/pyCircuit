# Commands and outcomes

Working directory: `/home/lc/pyCircuit`. Every environment-dependent command uses
`/home/lc/.codex/skills/pyc/scripts/run.sh` (abbreviated `RUN` below). Build inputs
and native linkage came from this checkout, not another worktree or install.

```sh
RUN cmake --build .pycircuit_out/local-clang22/build --target GfsimTests acir-queue-cxxgen agentic_circuit_native -j16
RUN .pycircuit_out/local-clang22/build/bin/GfsimTests
RUN python -m unittest discover -s tests/python/agentic-circuit/tools -p test_replay.py -v
RUN env PYC_RECORD_REPLAY=1 PYC_ROB_ARTIFACT_DIR=/home/lc/pyCircuit/.pycircuit_out/replay/verified-rob python -m unittest discover -s tests/integration/agentic-circuit/e2e -p test_queue_codegen.py -k rob -v
RUN env PYC_RECORD_REPLAY=1 PYC_ROB_ARTIFACT_DIR=/home/lc/pyCircuit/.pycircuit_out/replay/flow-final-rob python -m unittest discover -s tests/integration/agentic-circuit/e2e -p test_queue_codegen.py -v
RUN python -m unittest discover -s tests/integration/agentic-circuit/e2e -p test_typed_system_transactions.py -v
RUN python -c 'from lit.main import main; main()' -v .pycircuit_out/local-clang22/build/compiler/acir/tests/mlir --filter='(queue-flow-metadata|rule-owner-write-batch|atomic-transform|queue-multi-rule-multi-owner-module)'
RUN python -m unittest discover -s tests/python/agentic-circuit/python_frontend -p 'test_*.py'
RUN python tools/agentic-circuit/check-ir-coverage.py --write-ledger
RUN python -m unittest discover -s tests/python/agentic-circuit/contracts -p 'test_*.py'
RUN env -u AC_GATE_TOOLCHAIN_ROOT python -m unittest discover -s tests/python/agentic-circuit/cli -p test_run_command.py
RUN env -u AC_GATE_TOOLCHAIN_ROOT python -m unittest discover -s tests/python/agentic-circuit/cli -p test_all_commands.py -v
RUN python flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit examples/pycircuit docs README.md
RUN python flows/tools/check_decision_status.py --rfc docs/rfcs/pyc6-decisions.md --status docs/gates/decision_status_v6.md --out docs/gates/logs/20260908-queue-table-flow/decision_status_report.json --require-no-deferred --require-all-verified --require-concrete-evidence --require-existing-evidence
RUN bash -c 'git diff -U0 -- "*.cpp" "*.h" | clang-format-diff.py -p1'
RUN clang-format --dry-run --Werror simulator/gfsim/include/gfsim/replay.h simulator/gfsim/include/gfsim/replay_session.h tests/cpp/agentic-circuit/gfsim/ReplayTest.cpp
RUN python -m pre_commit --version
git diff --check
git diff --cached --name-only
```

## Independent tool checks

```sh
RUN env PYTHONPATH=/home/lc/pyCircuit/third_party/circuit-flow-viewer/src python -m unittest discover -s third_party/circuit-flow-viewer/tests -v
RUN python -m pip wheel --no-deps --no-build-isolation third_party/circuit-flow-viewer --wheel-dir .pycircuit_out/replay/wheels
RUN python -m pip install --no-deps --no-index --target .pycircuit_out/replay/wheel-smoke .pycircuit_out/replay/wheels/circuit_flow_viewer-0.1.0-py3-none-any.whl
RUN env PYTHONPATH=/home/lc/pyCircuit/.pycircuit_out/replay/wheel-smoke python -m circuit_flow_viewer.cli render third_party/circuit-flow-viewer/tests/fixtures/latency.pyctrace --output .pycircuit_out/replay/wheel-smoke.html
RUN env LD_LIBRARY_PATH=/home/lc/pyCircuit/.pycircuit_out/replay/browser-deps/usr/lib64 PLAYWRIGHT_BROWSERS_PATH=/home/lc/pyCircuit/.pycircuit_out/replay/browser-cache node third_party/circuit-flow-viewer/tests/browser.mjs .pycircuit_out/replay/browser/node_modules/playwright .pycircuit_out/replay/flow-review/replay.html third_party/circuit-flow-viewer/evidence
```

The browser command required sandbox escalation for Chromium IPC. Playwright
1.58.2, Chromium, and unpacked libgbm/libdrm dependencies stayed under disposable
`.pycircuit_out/replay/` paths; no system installation was performed. HTTP libdrm
download returned 502, and the same repository's HTTPS URL succeeded.

Native `.pyctrace` producer tests write their own fixtures at runtime. The
independent package retains small transaction/latency fixtures and has no
pyCircuit import or installation dependency. Browser screenshots remain in its
own `evidence` directory, not in framework source or framework release artifacts.
