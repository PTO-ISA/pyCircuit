# Issue 28 Atomic Rule Commands

```bash
cmake --build .pycircuit_out/acir/dev-llvm22 \
  --target ACIROpsTests CodeGenTests

ctest --test-dir .pycircuit_out/acir/dev-llvm22 \
  --output-on-failure \
  -R '^(ACIROpsTests|CodeGenTests)$'

.pycircuit_out/acir/dev-llvm22/bin/GfsimTests \
  --gtest_filter='QueueBlocksTest.MultiInputSelectedBranchStallsAtomicallyAndResetClearsCandidate'

pytest -q \
  tests/unit/test_agentic_circuit_component.py \
  tests/python/agentic-circuit/python_frontend/test_process.py

python3 flows/tools/check_api_hygiene.py \
  python/pycircuit/src/pycircuit examples/pycircuit docs README.md

git diff --check
```
