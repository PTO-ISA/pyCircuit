# Reproduction commands

Run from /home/lc/pyCircuit. RUN abbreviates
/home/lc/.codex/skills/pyc/scripts/run.sh. Build uses current checkout sources,
Clang 22 and Python 3.11. Unset AC_GATE_TOOLCHAIN_ROOT *inside* RUN for CLI tests
to avoid selecting the older installed native runtime.

```sh
RUN cmake --build .pycircuit_out/local-clang22/build --target GfsimTests acir-queue-cxxgen agentic_circuit_native -j16
RUN .pycircuit_out/local-clang22/build/bin/GfsimTests
RUN python -m unittest discover -s tests/python/agentic-circuit/tools -p test_replay.py -v
RUN env PYC_RECORD_REPLAY=1 PYC_ROB_ARTIFACT_DIR=/home/lc/pyCircuit/.pycircuit_out/replay/refactor/verified python -m unittest discover -s tests/integration/agentic-circuit/e2e -p test_queue_codegen.py -v
RUN python -m unittest discover -s tests/integration/agentic-circuit/e2e -p test_typed_system_transactions.py -v
RUN python -c 'from lit.main import main; main()' -v .pycircuit_out/local-clang22/build/compiler/acir/tests/mlir --filter='(queue-flow-metadata|rule-owner-write-batch|atomic-transform|queue-multi-rule-multi-owner-module)'
RUN python -m unittest discover -s tests/python/agentic-circuit/python_frontend -p 'test_*.py'
RUN python -m unittest discover -s tests/python/agentic-circuit/python_frontend -p test_queue_codegen.py
RUN python -m unittest discover -s tests/python/agentic-circuit/contracts -p 'test_*.py'
RUN env -u AC_GATE_TOOLCHAIN_ROOT python -m unittest discover -s tests/python/agentic-circuit/cli -p test_run_command.py
RUN env -u AC_GATE_TOOLCHAIN_ROOT python -m unittest discover -s tests/python/agentic-circuit/cli -p test_all_commands.py
RUN python flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit examples/pycircuit docs README.md
RUN python flows/tools/check_decision_status.py --rfc docs/rfcs/pyc6-decisions.md --status docs/gates/decision_status_v6.md --out docs/gates/logs/20260908-flow-observer-refactor/decision_status_report.json --require-no-deferred --require-all-verified --require-concrete-evidence --require-existing-evidence
RUN python docs/gates/logs/20260908-flow-observer-refactor/compare.py
RUN bash -c 'git diff -U0 -- "*.cpp" "*.h" | clang-format-diff.py -p1'
RUN clang-format --dry-run --Werror simulator/gfsim/include/gfsim/replay_value.h simulator/gfsim/include/gfsim/state_observation.h tests/cpp/agentic-circuit/gfsim/StateObservationTest.cpp
git diff --check
RUN python -m pre_commit --version
```

For the sizeof/timing diagnostic, the script used git ls-tree and git show to
extract simulator/gfsim/include/gfsim header sources for c0b42d91 and 24a891fe into
`.pycircuit_out/replay/refactor/before-replay` and `before-refactor`. For each
header root and current simulator/gfsim/include:

```sh
RUN c++ -std=c++20 -O2 -I<headers> docs/gates/logs/20260908-flow-observer-refactor/footprint.cpp -o .pycircuit_out/replay/refactor/<label>-bench
```

Each binary ran seven times. footprint.json records raw milliseconds, checksum
and sizes; benchmark source does not include replay code.

Independent viewer:

```sh
RUN env PYTHONPATH=third_party/circuit-flow-viewer/src python -m unittest discover -s third_party/circuit-flow-viewer/tests -p 'test_*.py'
RUN env PYTHONPATH=third_party/circuit-flow-viewer/src python -m circuit_flow_viewer.cli render <verified-single-trace> --output .pycircuit_out/replay/refactor/viewer/replay.html
RUN env PYTHONPATH=third_party/circuit-flow-viewer/src python -m circuit_flow_viewer.cli render third_party/circuit-flow-viewer/tests/fixtures/latency.pyctrace --output .pycircuit_out/replay/refactor/viewer/queue-latency.html
RUN env LD_LIBRARY_PATH=/home/lc/pyCircuit/.pycircuit_out/replay/browser-deps/usr/lib64 PLAYWRIGHT_BROWSERS_PATH=/home/lc/pyCircuit/.pycircuit_out/replay/browser-cache node third_party/circuit-flow-viewer/tests/browser.mjs .pycircuit_out/replay/browser/node_modules/playwright .pycircuit_out/replay/refactor/viewer/replay.html third_party/circuit-flow-viewer/evidence/observer-refactor
```

Browser output reports no page errors/offline=true. Its final barrier counter is
5 because the existing browser suite ends on the latency fixture; the preceding
ROB page is the 70-boundary record checked in comparison-final.json.
