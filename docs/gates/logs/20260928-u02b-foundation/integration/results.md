# U02-B foundation current-checkout integration check

PM check of clean isolated HEAD `6d56f4a1c90b1a2766ee7e9f0e563b5bca8c4125`
in `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
Configured a new LLVM/MLIR 22 build at
`.pycircuit_out/u02b-shared-test/build`; no binary or library was copied from
another checkout.

```text
cmake --build .pycircuit_out/u02b-shared-test/build --target \
  ACIRSourceContractsTests ACIRSourceMathContractsTests \
  ACIRSourceUnitTests ACIRNamespaceContractsTests \
  ACIRSourceModuleContractsTests acir-source-unit-harness -j 6
ctest --test-dir .pycircuit_out/u02b-shared-test/build --output-on-failure
# 5/5 passed
ACIR_SOURCE_UNIT_HARNESS="$PWD/.pycircuit_out/u02b-shared-test/build/bin/acir-source-unit-harness" \
PYTHONPATH="$PWD/python/semantic-core/src:$PWD/python/pycircuit/src:$PWD/python/agentic-circuit/src:$PWD" \
pytest tests/system/test_source_unit_packet.py \
  tests/system/test_source_namespace_bindings.py \
  tests/system/test_source_module_units.py -q
# 78/78 passed, zero skips
```

Clang-format dry-run on modified C++ files, Ruff format/check on modified
Python system tests, and `git diff --check` passed. One Ruff-only assertion
layout correction was committed as `6d56f4a1`; it changed no test expression.

This is source/header and IR-contract regression evidence, not acceptance of
math review-A repairs until independent review. It does not exercise an actual
linked/final program, C++ or Verilog emitter, installed SDK, or SSM ELF.
