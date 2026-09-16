# Module config metadata pruning evidence

## Scope

- Decision 0249: nested typed config projections.
- Decision 0264: per-module Agentic Circuit/gfsim units.
- Fix whole-program composition so payload deduplication also removes a
  module-local config binding when no surviving dependent-type check references
  that root.
- Preserve both ACIR and QueueGraph rejection of an explicitly forged orphan
  config root.

## Focused evidence

All commands ran from the current pyCircuit checkout on macOS arm64 with
Python 3.14.6.

```text
PYTHONPATH="python/semantic-core/src:python/agentic-circuit/src" \
  pytest -q tests/python/agentic-circuit/python_frontend/test_queue_frontend.py
219 passed

pytest -q tests/unit -m unit
147 passed

PYTHONPATH="python/semantic-core/src:python/agentic-circuit/src" \
  ACIR_BIN="$PWD/.pycircuit_out/toolchain/install/bin" \
  pytest -q tests/integration/agentic-circuit/e2e/test_frontend_composition_features.py \
    -k parent_specialized_config_interface_generates_cpp
1 passed, 6 deselected

.pycircuit_out/acir/dev-llvm22/bin/CodeGenTests \
  --gtest_filter=QueueGraphPlanTest.RecomputesNestedConfigProjectionMetadata
1 passed

.pycircuit_out/merge-venv/bin/python \
  .pycircuit_out/merge-venv/bin/lit -v \
  .pycircuit_out/acir/dev-llvm22/tests/mlir/ACIR/static-config-metadata.mlir
1 passed

.pycircuit_out/merge-venv/bin/python \
  tools/agentic-circuit/check-contracts.py
repository contracts: OK

python3 flows/tools/check_api_hygiene.py \
  python/pycircuit/src/pycircuit examples/pycircuit docs README.md
ok: API hygiene check passed

mkdocs build --strict
Documentation built successfully
```

The focused native case freezes the ACIR, runs `acir-queue-cxxgen`, and syntax
compiles the generated C++20 source. The existing distinct child-local config
specialization test remains green and retains both namespaced roots.

## Consumer reproducer

The SuperScalarModel AGU IQ, LDA, STA, STD, LIQ, and STQ probes all generated
raw ACIR and concatenated gfsim C++ from the current worktree after this fix.
Consumer sources and product-specific assertions remain outside this framework
repository.

## Known unrelated diagnostic

Running the whole `test_frontend_composition_features.py` file also reaches an
existing failure in
`test_module_state_uses_bounded_storage_initializer_type`: its negative source
does not raise the expected bounded-state diagnostic. The config-metadata test
and the other five cases pass; this change does not touch bounded-state parsing.
