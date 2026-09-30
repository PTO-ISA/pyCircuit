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

# Preserve the semantic source and same-final oracles through the public
# compile/link/emit route. Keep each test's independent malformed-input,
# cross-backend, and final-IR checks intact.
tests=(
  tests/system/test_source_compile_publication.py
  tests/system/test_source_design_bridge.py
  tests/system/test_source_module_units.py
  tests/system/test_source_namespace_bindings.py
  tests/system/test_source_numeric_expressions.py
  tests/system/test_source_numeric_next.py
  tests/system/test_source_observations.py
  tests/system/test_source_transport_mlir.py
  tests/system/test_source_unit_cmake_build.py
  tests/system/test_source_unit_packet.py
  tests/system/test_source_unit_pair_verification.py
  tests/system/test_generic_assignment_roundtrip.py
  tests/system/test_generic_multi_assignment.py
  tests/system/test_final_scalar_declarations.py
  tests/system/test_cpp_source_parts.py
  tests/system/test_masked_next_register.py
  tests/system/test_unified_register_backends.py
  tests/system/test_m5_public_emit.py
  tests/system/test_m5_source_map.py
  tests/system/test_m5_runtime_install.py
  tests/system/test_m5_source_rtl.py
  tests/system/test_m6_publication_process_recovery.py
  tests/system/test_m6_relocated_compiler.py
  tests/system/test_m6_incremental_build.py
  tests/system/test_m6_example_build.py
  tests/system/test_m7_cmake_presets.py
)
for test_file in "${tests[@]}"; do
  [[ -f "${PYC_ROOT_DIR}/${test_file}" ]] || pyc_die "semantic oracle is missing: ${test_file}"
done

gate_run_id="${PYC_GATE_RUN_ID:-$(date +%Y%m%d-%H%M%S)}"
docs_gate_dir="${PYC_ROOT_DIR}/docs/gates/logs/${gate_run_id}"
mkdir -p "${docs_gate_dir}"
cat > "${docs_gate_dir}/semantic_regressions_commands.txt" <<EOF
PYTHONPATH=<checkout>/python/pycircuit/src PYC_TOOLCHAIN_ROOT=${PYC_TOOLCHAIN_ROOT} ACIR_BACKEND_CLOSURE_HARNESS=${ACIR_BACKEND_CLOSURE_HARNESS} python3 -m pytest -q ${tests[*]} -k 'not test_v44_schedule_permutations_preserve_values_errors_and_events and not test_v44_source_reorder_compares_explicit_semantic_identity'
EOF
pyc_log "run source-owned semantic regressions (run-id=${gate_run_id})"
cd "${PYC_ROOT_DIR}"
python3 -m pytest -q "${tests[@]}" \
  -k 'not test_v44_schedule_permutations_preserve_values_errors_and_events and not test_v44_source_reorder_compares_explicit_semantic_identity' \
  >"${docs_gate_dir}/semantic_regressions.stdout" \
  2>"${docs_gate_dir}/semantic_regressions.stderr" || {
    cat "${docs_gate_dir}/semantic_regressions.stdout"
    cat "${docs_gate_dir}/semantic_regressions.stderr" >&2
    cat > "${docs_gate_dir}/semantic_regressions_summary.json" <<EOF
{"run_id":"${gate_run_id}","script":"run_semantic_regressions_v6.sh","status":"fail","oracle":"common-final-program"}
EOF
    exit 1
  }
cat "${docs_gate_dir}/semantic_regressions.stdout"
cat > "${docs_gate_dir}/semantic_regressions_summary.json" <<EOF
{"run_id":"${gate_run_id}","script":"run_semantic_regressions_v6.sh","status":"pass","oracle":"common-final-program","tests":${#tests[@]}}
EOF
pyc_log "source-owned semantic regressions passed"
