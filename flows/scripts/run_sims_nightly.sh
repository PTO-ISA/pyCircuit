#!/usr/bin/env bash
set -euo pipefail

source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/lib.sh"
export PYTHONPATH="${PYC_ROOT_DIR}/python/pycircuit/src${PYTHONPATH:+:${PYTHONPATH}}"
export PYTHONDONTWRITEBYTECODE=1
if [[ -z "${PYC_TOOLCHAIN_ROOT:-}" ]]; then
  PYC_TOOLCHAIN_ROOT="$(pyc_out_root)/toolchain/install"
fi
export PYC_TOOLCHAIN_ROOT
pyc_set_public_helpers

tests=(tests/system/test_m5_source_rtl.py tests/system/test_m5_runtime_install.py)
for test_file in "${tests[@]}"; do
  [[ -f "${PYC_ROOT_DIR}/${test_file}" ]] || pyc_die "required runtime/RTL release smoke is missing: ${test_file}"
done

pyc_log "run native RTL and relocated runtime consumer smoke on $(uname -s)-$(uname -m)"
cd "${PYC_ROOT_DIR}"
python3 -m pytest -q "${tests[@]}"

gate_run_id="${PYC_GATE_RUN_ID:-$(date +%Y%m%d-%H%M%S)}"
docs_gate_dir="${PYC_ROOT_DIR}/docs/gates/logs/${gate_run_id}"
mkdir -p "${docs_gate_dir}"
cat > "${docs_gate_dir}/run_sims_nightly_summary.json" <<EOF
{"run_id":"${gate_run_id}","script":"run_sims_nightly.sh","status":"pass","scope":"current-runner-only"}
EOF
pyc_log "runtime and RTL smoke passed"
