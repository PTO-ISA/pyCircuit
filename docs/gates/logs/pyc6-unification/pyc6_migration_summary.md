# pyCircuit 6 migration evidence

Run ID: `pyc6-unification`

## Verified contracts

- Product and package version: pyCircuit `6.0.0`.
- Public Cycle-Aware implementation: `pycircuit.v6`; no `pycircuit.v5` shim.
- Runtime target and archive: `pyc6_runtime` / `libpyc6_runtime.a`.
- Binary trace schema magic: `PYC6TRC3`.
- Semantic closure lane: `flows/scripts/run_semantic_regressions_v6.sh`.
- Decision coverage: 149 rows, zero deferred.

## Commands and results

```text
pytest tests/unit -m unit
PASS: 51 tests

pre-commit run --files <changed-files>
PASS

python3 flows/tools/check_api_hygiene.py compiler/frontend/pycircuit designs/examples docs README.md
PASS

python3 flows/tools/check_decision_status.py --status docs/gates/decision_status_v6.md ... --require-no-deferred --require-all-verified --require-concrete-evidence --require-existing-evidence
PASS: 149 decisions

mkdocs build --strict
PASS

bash flows/scripts/pyc build --llvm-config <LLVM-22 llvm-config> --build-dir .pycircuit_out/pyc6-validation/toolchain-build --install-prefix .pycircuit_out/pyc6-validation/toolchain-install
PASS: pycc, pyc-opt, libpyc6_runtime.a

PYC_GATE_RUN_ID=pyc6-unification bash flows/scripts/run_examples.sh
PASS

PYC_GATE_RUN_ID=pyc6-unification bash flows/scripts/run_sims.sh
PASS

PYC_GATE_RUN_ID=pyc6-unification bash flows/scripts/run_sims_nightly.sh
PASS

PYC_GATE_RUN_ID=pyc6-unification bash flows/scripts/run_semantic_regressions_v6.sh
PASS: C++ and Verilator

bash contrib/linx/flows/tools/run_linx_cpu_pyc_cpp.sh
PASS: 129 cycles

python3 ../bringup/check_pycircuit_interface_contract.py --root ../.. --strict
PASS: interface version 2.0

python3 ../bringup/check_trace_semver_compat.py --root ../.. --strict
PASS: LinxTrace version 1.0

bash contrib/linx/flows/tools/run_linx_qemu_vs_pyc.sh
BLOCKED BEFORE EXECUTION: qemu-system-linx64 is not installed on the validation host.
```

Detailed example, simulation, semantic, trace, and decision-status outputs are
stored beside this summary and under `cases/`.
