# Release Train Decision-Freeze Commands

```bash
pytest -q \
  tests/unit/test_agentic_circuit_component.py \
  tests/python/agentic-circuit/python_frontend/test_process.py

pytest -q \
  tests/python/agentic-circuit/python_frontend/test_queue_frontend.py \
  -k 'table or choose or rule'

ctest --test-dir .pycircuit_out/acir/dev-llvm22 \
  --output-on-failure \
  -R '^(ACIROpsTests|ACIRModelAnalysisTests|ACIRProcessStatePlanTests)$'

python3 flows/tools/check_decision_status.py \
  --rfc docs/rfcs/pyc6-decisions.md \
  --status docs/gates/decision_status_v6.md \
  --out .pycircuit_out/gates/20260910-release-train-decision-freeze/decision_status_report.json

mkdocs build --strict
git diff --check
```
