# Commands

All environment-dependent commands use
`/home/lc/.codex/skills/pyc/scripts/run.sh` (RUN below), from this checkout.
This consolidated index preserves commands used for the final gates. RUN is a
notation for the wrapper, not a separately installed executable.

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
including Python evidence scripts; only deleted files are excluded. The original
exclusion of evidence paths was a validation mistake corrected by 9ddb1c72. Cache:
`.pycircuit_out/replay/main-migration/pre-commit-cache`.
Black/Ruff/pre-commit were installed in the fixed Python environment because
`.pre-commit-config.yaml` requires them. Browser and hooks ran outside the sandbox
with approval; no generated dependencies or browser evidence were staged.

## Follow-up for b69cbd7d

```sh
git fetch origin main
git merge --no-commit --no-ff origin/main
RUN python -m unittest discover -s tests/python/agentic-circuit/python_frontend -p 'test_*.py'
RUN env PYC_RECORD_REPLAY=1 PYC_ROB_ARTIFACT_DIR=/home/lc/pyCircuit/.pycircuit_out/replay/main-migration/latest-verified python -m unittest discover -s tests/integration/agentic-circuit/e2e -p test_queue_codegen.py -k rob -v
RUN python -m pytest designs/davincioo/tests/fabric/test_xbar_static_generation.py -k 'source_and_topology or gfsim_preserves_behavior' -q
RUN python designs/davincioo/tools/check_catalog.py
RUN python docs/gates/logs/20260908-replay-main-migration/compare-latest.py
```

Contracts, strict decision status and docs were rerun with the same commands.
Final PR pre-commit uses the complete ACMR diff against origin/main, including
evidence scripts, with the repository hooks unchanged.

## Additional retained gate commands

```sh
RUN python -m unittest discover -s tests/python/agentic-circuit/tools -p test_replay.py -v
RUN python -m unittest discover -s tests/python/agentic-circuit/contracts -p 'test_*.py'
RUN python -m unittest discover -s tests/integration/agentic-circuit/e2e -p test_typed_system_transactions.py -v
RUN env -u AC_GATE_TOOLCHAIN_ROOT python -m unittest discover -s tests/python/agentic-circuit/cli -p 'test_*.py'
RUN python -m pytest tests/unit -m unit -q
RUN python -m mkdocs build --strict --site-dir .pycircuit_out/evidence-curation/site
RUN python flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit examples/pycircuit docs README.md
```

The footprint diagnostic retains its source/raw samples in the observer stage.
Extract headers from this repository's c0b42d91 and 24a891fe Git trees into
disposable directories, then compile each alongside the current include tree:

```sh
RUN c++ -std=c++20 -O2 -I<headers> docs/gates/logs/20260908-flow-observer-refactor/footprint.cpp -o .pycircuit_out/replay/refactor/<label>-bench
```

Run each binary seven times; compare checksums and report unisolated timing only.
Original stage-specific commands and diagnostics remain recoverable from
pre-curation commit 9ddb1c72. JSON comparisons require the retained local traces
in `.pycircuit_out/replay/verified-rob` and `main-migration/latest-verified`;
regenerate current traces with the ROB artifact-directory command above.
