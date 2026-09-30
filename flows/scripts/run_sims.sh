#!/usr/bin/env bash
set -euo pipefail

source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/lib.sh"
export PYTHONPATH="${PYC_ROOT_DIR}/python/pycircuit/src${PYTHONPATH:+:${PYTHONPATH}}"
export PYTHONDONTWRITEBYTECODE=1
if [[ -z "${PYC_TOOLCHAIN_ROOT:-}" ]]; then
  PYC_TOOLCHAIN_ROOT="$(pyc_out_root)/toolchain/install"
fi
if [[ -z "${PYC_BUILD_DIR:-}" ]]; then
  PYC_BUILD_DIR="$(pyc_out_root)/toolchain/build"
fi
ACIR_BACKEND_CLOSURE_HARNESS="${ACIR_BACKEND_CLOSURE_HARNESS:-${PYC_BUILD_DIR}/bin/acir-backend-closure-harness}"
[[ -x "${ACIR_BACKEND_CLOSURE_HARNESS}" ]] || pyc_die "missing ACIR backend closure harness: ${ACIR_BACKEND_CLOSURE_HARNESS}"
export ACIR_BACKEND_CLOSURE_HARNESS PYC_BUILD_DIR
export PYC_TOOLCHAIN_ROOT
pyc_set_public_helpers

tests=(
  tests/system/test_generic_assignment_roundtrip.py
  tests/system/test_generic_multi_assignment.py
  tests/system/test_final_scalar_declarations.py
  tests/system/test_cpp_source_parts.py
  tests/system/test_masked_next_register.py
  tests/system/test_unified_register_backends.py
)
for test_file in "${tests[@]}"; do
  [[ -f "${PYC_ROOT_DIR}/${test_file}" ]] || pyc_die "required same-final simulation gate is missing: ${test_file}"
done

pyc_log "run C++/Verilog same-final semantic gates"
cd "${PYC_ROOT_DIR}"
python3 -m pytest -q "${tests[@]}" \
  -k 'not test_v44_schedule_permutations_preserve_values_errors_and_events and not test_v44_source_reorder_compares_explicit_semantic_identity'

gate_run_id="${PYC_GATE_RUN_ID:-$(date +%Y%m%d-%H%M%S)}"
docs_gate_dir="${PYC_ROOT_DIR}/docs/gates/logs/${gate_run_id}"
mkdir -p "${docs_gate_dir}"
cat > "${docs_gate_dir}/run_sims_summary.json" <<EOF
{"run_id":"${gate_run_id}","script":"run_sims.sh","status":"pass","oracle":"common-final-program"}
EOF
pyc_log "same-final simulation gates passed"
