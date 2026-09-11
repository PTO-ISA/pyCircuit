# ACIR multi-input rule gate

## Scope

Decision 0167 extends the pure `@ac.rule` vertical slice from one Queue input
to one or more atomic Queue inputs with one output. Python remains functional:
it exposes no readiness, pop, push, reservation, or commit operations. MLIR
materializes `ready_valid_Nx1`; QueueGraph generates one typed gfsim
`QueueAtomicTransform`.

## Evidence

- `PYTHONPATH=python/agentic-circuit/src python3 -m unittest tests/python/agentic-circuit/python_frontend/test_queue_frontend.py`
  - passed: 57 tests
- `ninja -C .pycircuit_out/acir/dev-llvm22 check-acir`
  - passed: 142 tests
- `.pycircuit_out/acir/dev-llvm22/bin/GfsimTests`
  - passed: 252 tests
  - the Table Xfer hot path updates only touched entries instead of copying the
    complete committed Table before every write commit
- `PYTHONPATH=python/agentic-circuit/src python3 -m unittest tests.integration.agentic-circuit.e2e.test_queue_codegen.QueueCodegenTest.test_multi_input_rule_infers_atomic_gfsim_backpressure`
  - passed: generated C++ compiled and executed
  - verified: a full output retains the second token in both input Queues
- `.pycircuit_out/ac-venv/bin/python tools/agentic-circuit/check-contracts.py`
  - passed: 12 public schemas, 36 stdlib components, epoch 0.5, LLVM 22.1.8
- `python3 flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit examples/pycircuit docs README.md`
  - passed
- `.pycircuit_out/ac-venv/bin/mkdocs build --strict`
  - passed

Generated diagnostic artifacts are under
`.pycircuit_out/gates/20260905-ac-multi-input-rule-r1/` and include the frozen
ACIR, QueueGraph plan, generated typed C++, and compiled object.

## Remaining boundary

Decision 0168 extends the same gate to one Table plus heterogeneous Queue
inputs. The frontend suite now passes 58 tests, the ACIR lit suite passes 143
tests, and two generated-model integration tests execute pure and stateful
multi-input backpressure cases.

This gate does not claim CFG branches, optional or multiple outputs, multiple
state proposals, dynamic checks, Reg effects, or conflict arbitration. Those
remain under `D-RULE-LOWERING-001`.
