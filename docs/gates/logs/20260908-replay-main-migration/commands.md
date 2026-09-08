# Commands

All environment-dependent commands use
`/home/lc/.codex/skills/pyc/scripts/run.sh` (RUN below), from this checkout.
The JSON command inventories in this directory record the initial grouped runs;
final corrections and additional checks are below.

```sh
git fetch origin main
git merge --no-commit --no-ff origin/main
RUN python tools/agentic-circuit/check-ir-coverage.py --write-ledger
RUN cmake --build .pycircuit_out/local-clang22/build --target GfsimTests acir-queue-cxxgen agentic_circuit_native acir-opt acir-opt-internal acir-queue-plan CodeGenTests -j16
RUN .pycircuit_out/local-clang22/build/bin/GfsimTests
RUN .pycircuit_out/local-clang22/build/bin/CodeGenTests
RUN env PYC_RECORD_REPLAY=1 PYC_ROB_ARTIFACT_DIR=/home/lc/pyCircuit/.pycircuit_out/replay/main-migration/verified python -m unittest discover -s tests/integration/agentic-circuit/e2e -p test_queue_codegen.py -v
RUN env PYC_RECORD_REPLAY=1 python -m unittest discover -s tests/integration/agentic-circuit/e2e -p test_queue_codegen.py -v
RUN python -c 'from lit.main import main; main()' -v .pycircuit_out/local-clang22/build/compiler/acir/tests/mlir --filter='(queue-flow-metadata|rule-owner-write-batch|atomic-transform|queue-multi-rule-multi-owner-module|rule-multi-output|value-contracts)'
RUN env ACIR_QUEUE_PLAN=/home/lc/pyCircuit/.pycircuit_out/local-clang22/build/bin/acir-queue-plan python -m unittest discover -s tests/integration/agentic-circuit/e2e -p test_multi_output_atomic.py -k test_public_python_state_and_outputs_commit_as_one_gfsim_transaction -v
RUN env ACIR_QUEUE_PLAN=/home/lc/pyCircuit/.pycircuit_out/local-clang22/build/bin/acir-queue-plan python -m unittest discover -s tests/integration/agentic-circuit/e2e -p test_aggregate_equality_invariant.py -k test_recursive_equality_and_invariant_execute_through_gfsim -v
RUN python docs/gates/logs/20260908-replay-main-migration/compare.py
RUN env PYTHONPATH=third_party/circuit-flow-viewer/src python -m unittest discover -s third_party/circuit-flow-viewer/tests -v
RUN env PYTHONPATH=third_party/circuit-flow-viewer/src python -m circuit_flow_viewer.cli render .pycircuit_out/replay/main-migration/verified/single-*/execution.pyctrace --output .pycircuit_out/replay/main-migration/viewer/replay.html
RUN env PYTHONPATH=third_party/circuit-flow-viewer/src python -m circuit_flow_viewer.cli render third_party/circuit-flow-viewer/tests/fixtures/latency.pyctrace --output .pycircuit_out/replay/main-migration/viewer/queue-latency.html
RUN env LD_LIBRARY_PATH=/home/lc/pyCircuit/.pycircuit_out/replay/browser-deps/usr/lib64 PLAYWRIGHT_BROWSERS_PATH=/home/lc/pyCircuit/.pycircuit_out/replay/browser-cache node third_party/circuit-flow-viewer/tests/browser.mjs .pycircuit_out/replay/browser/node_modules/playwright .pycircuit_out/replay/main-migration/viewer/replay.html third_party/circuit-flow-viewer/evidence/main-migration
RUN python -m ruff check third_party/circuit-flow-viewer
RUN python -m black --workers 1 --check third_party/circuit-flow-viewer/src third_party/circuit-flow-viewer/tests/test_viewer.py
RUN python flows/tools/check_decision_status.py --rfc docs/rfcs/pyc6-decisions.md --status docs/gates/decision_status_v6.md --out docs/gates/logs/20260908-replay-main-migration/decision_status_report.json --require-no-deferred --require-all-verified --require-concrete-evidence --require-existing-evidence
git diff --check
git diff --cached --check
```

The isolated CLI installation retest uses the existing test unchanged:

```python
import os, sys, unittest
from pathlib import Path
sys.path.insert(0, 'tests/python/agentic-circuit/cli')
import test_installation
os.environ.pop('AC_GATE_TOOLCHAIN_ROOT', None)
test_installation.BUILD = Path(os.environ['PYC_LOCAL_BUILD_DIR'])
unittest.TextTestRunner(verbosity=2).run(
    unittest.defaultTestLoader.loadTestsFromModule(test_installation))
```

Changed-file pre-commit uses files from `git diff --name-only origin/main`,
excluding historical `docs/gates/logs/` evidence and deleted files. Cache:
`.pycircuit_out/replay/main-migration/pre-commit-cache`.
Black/Ruff/pre-commit were installed in the fixed Python environment because
`.pre-commit-config.yaml` requires them. Browser and hooks ran outside the sandbox
with approval; no generated dependencies or browser evidence were staged.
