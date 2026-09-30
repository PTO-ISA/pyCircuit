#!/usr/bin/env bash
set -euo pipefail

source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/lib.sh"

PYTHONPATH_VALUE="${PYC_ROOT_DIR}/python/pycircuit/src${PYTHONPATH:+:${PYTHONPATH}}"
export PYTHONPATH="${PYTHONPATH_VALUE}" PYTHONDONTWRITEBYTECODE=1
if [[ -z "${PYC_TOOLCHAIN_ROOT:-}" ]]; then
  PYC_TOOLCHAIN_ROOT="$(pyc_out_root)/toolchain/install"
fi
export PYC_TOOLCHAIN_ROOT
pyc_set_public_helpers

required=(
  tests/system/test_source_unit_cmake_build.py
  tests/system/test_source_unit_pair_verification.py
  tests/system/test_m5_public_emit.py
  tests/system/test_m5_source_map.py
)
for test_file in "${required[@]}"; do
  [[ -f "${PYC_ROOT_DIR}/${test_file}" ]] || pyc_die "required source-unit example gate is missing: ${test_file}"
done

pyc_log "run source-owned compile/link/emit examples with ${PYC_TOOLCHAIN_ROOT}"
cd "${PYC_ROOT_DIR}"
python3 -m pytest -q "${required[@]}"

gate_run_id="${PYC_GATE_RUN_ID:-$(date +%Y%m%d-%H%M%S)}"
docs_gate_dir="${PYC_ROOT_DIR}/docs/gates/logs/${gate_run_id}"
mkdir -p "${docs_gate_dir}"
cat > "${docs_gate_dir}/run_examples_summary.json" <<EOF
{"run_id":"${gate_run_id}","script":"run_examples.sh","status":"pass","route":"pycircuit-compile-link-emit"}
EOF
pyc_log "source-owned examples passed"
