# Framework boundary cleanup commands

```bash
cmake --build .pycircuit_out/acir/dev-llvm22 -j 6
ctest --test-dir .pycircuit_out/acir/dev-llvm22 --output-on-failure
pytest -q tests/unit -m unit
.pycircuit_out/agentic-circuit/venv/bin/python -m pytest -q \
  tests/python/agentic-circuit/cli \
  tests/python/agentic-circuit/contracts
python3.11 -m venv .pycircuit_out/framework-boundary-py311
.pycircuit_out/framework-boundary-py311/bin/python -m pip install \
  -e python/semantic-core -e 'python/agentic-circuit[test]'
cmake -S compiler/acir -B .pycircuit_out/acir/py311-boundary -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLLVM_DIR=/opt/homebrew/opt/llvm/lib/cmake/llvm \
  -DMLIR_DIR=/opt/homebrew/opt/llvm/lib/cmake/mlir \
  -DPython3_EXECUTABLE="$PWD/.pycircuit_out/framework-boundary-py311/bin/python" \
  -DACIR_TEST_PYTHON="$PWD/.pycircuit_out/framework-boundary-py311/bin/python" \
  -DACIR_LIT_EXECUTABLE="$PWD/.pycircuit_out/agentic-circuit/venv/bin/lit"
cmake --build .pycircuit_out/acir/py311-boundary -j 6
cmake --install .pycircuit_out/acir/py311-boundary \
  --prefix .pycircuit_out/sdk-py311-boundary
AC_GATE_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/sdk-py311-boundary" \
  .pycircuit_out/framework-boundary-py311/bin/python -m pytest -q \
  tests/python/agentic-circuit/cli/test_model_plan_command.py
.pycircuit_out/agentic-circuit/venv/bin/python \
  tools/agentic-circuit/check-contracts.py
python3 flows/tools/check_api_hygiene.py \
  python/pycircuit/src/pycircuit examples/pycircuit docs README.md
python3 flows/tools/check_decision_status.py \
  --rfc docs/rfcs/pyc6-decisions.md \
  --status docs/gates/decision_status_v6.md \
  --out docs/gates/logs/20260909-framework-boundary-cleanup/decision_status_report.json \
  --require-concrete-evidence \
  --require-existing-evidence
mkdocs build --strict
git diff --check
```
