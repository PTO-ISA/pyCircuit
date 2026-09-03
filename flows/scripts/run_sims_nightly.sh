#!/usr/bin/env bash
set -euo pipefail

source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/lib.sh"
pyc_find_pycc

PYTHONPATH_VAL="$(pyc_pythonpath)"
OUT_BASE="$(pyc_out_root)/sim_nightly"
DISCOVER="${PYC_ROOT_DIR}/flows/tools/discover_examples.py"
mkdir -p "${OUT_BASE}"

pyc_log "using pycc: ${PYCC}"

gate_run_id="${PYC_GATE_RUN_ID:-$(date +%Y%m%d-%H%M%S)}"
docs_gate_dir="${PYC_ROOT_DIR}/docs/gates/logs/${gate_run_id}"
case_log_root="${docs_gate_dir}/cases/run_sims_nightly"
mkdir -p "${case_log_root}"

case_timeout_sec="${PYC_SIM_CASE_TIMEOUT_SEC:-3600}"
retry_on_timeout="${PYC_SIM_RETRY_ON_TIMEOUT:-1}"
resume_from_case="${PYC_SIM_RESUME_FROM_CASE:-}"
resume_seen=0

if ! [[ "${case_timeout_sec}" =~ ^[0-9]+$ ]]; then
  pyc_die "PYC_SIM_CASE_TIMEOUT_SEC must be a non-negative integer, got: ${case_timeout_sec}"
fi
if ! [[ "${retry_on_timeout}" =~ ^[0-9]+$ ]]; then
  pyc_die "PYC_SIM_RETRY_ON_TIMEOUT must be a non-negative integer, got: ${retry_on_timeout}"
fi

cat > "${docs_gate_dir}/commands_run_sims_nightly.txt" <<EOF
bash flows/scripts/run_sims_nightly.sh
env:
  PYC_GATE_RUN_ID=${gate_run_id}
  PYC_SIM_CASE_TIMEOUT_SEC=${case_timeout_sec}
  PYC_SIM_RETRY_ON_TIMEOUT=${retry_on_timeout}
  PYC_SIM_RESUME_FROM_CASE=${resume_from_case}
EOF

is_timeout_rc() {
  local rc="${1:-0}"
  [[ "${rc}" -eq 124 || "${rc}" -eq 137 || "${rc}" -eq 142 ]]
}

should_run_case() {
  local case_name="$1"
  if [[ -z "${resume_from_case}" ]]; then
    return 0
  fi
  if [[ "${resume_seen}" -eq 1 ]]; then
    return 0
  fi
  if [[ "${case_name}" == "${resume_from_case}" ]]; then
    resume_seen=1
    return 0
  fi
  pyc_log "skip ${case_name} (resume from ${resume_from_case})"
  return 1
}

run_with_case_logs() {
  local case_name="$1"
  local stage="$2"
  local workdir="$3"
  shift 3
  local case_dir="${case_log_root}/${case_name}"
  mkdir -p "${case_dir}"

  local attempt=0
  local max_attempts=$((retry_on_timeout + 1))
  while true; do
    attempt=$((attempt + 1))
    local out_attempt="${case_dir}/${stage}.stdout.attempt${attempt}"
    local err_attempt="${case_dir}/${stage}.stderr.attempt${attempt}"
    local rc=0

    if [[ "${case_timeout_sec}" -gt 0 ]]; then
      if perl -e 'alarm shift; chdir shift or die "chdir failed\n"; exec @ARGV' \
        "${case_timeout_sec}" "${workdir}" "$@" >"${out_attempt}" 2>"${err_attempt}"; then
        rc=0
      else
        rc=$?
      fi
    else
      if (cd "${workdir}" && "$@" >"${out_attempt}" 2>"${err_attempt}"); then
        rc=0
      else
        rc=$?
      fi
    fi

    if [[ "${rc}" -eq 0 ]]; then
      cp -f "${out_attempt}" "${case_dir}/${stage}.stdout"
      cp -f "${err_attempt}" "${case_dir}/${stage}.stderr"
      printf '%s\n' "${rc}" > "${case_dir}/${stage}.rc"
      return 0
    fi

    if is_timeout_rc "${rc}" && [[ "${attempt}" -lt "${max_attempts}" ]]; then
      pyc_warn "timeout during ${case_name}/${stage} (attempt=${attempt}/${max_attempts}); retrying"
      continue
    fi

    cp -f "${out_attempt}" "${case_dir}/${stage}.stdout"
    cp -f "${err_attempt}" "${case_dir}/${stage}.stderr"
    printf '%s\n' "${rc}" > "${case_dir}/${stage}.rc"
    return "${rc}"
  done
}

run_case_examples() {
  local name="$1"
  local tb_src="$2"

  if ! should_run_case "${name}"; then
    return 0
  fi

  local out_dir="${OUT_BASE}/${name}"
  rm -rf "${out_dir}" >/dev/null 2>&1 || true
  mkdir -p "${out_dir}"
  pyc_log "sim(example-nightly) ${name}: ${tb_src}"

  if ! run_with_case_logs "${name}" build "${PYC_ROOT_DIR}" \
      env PYTHONPATH="${PYTHONPATH_VAL}" PYTHONDONTWRITEBYTECODE=1 PYCC="${PYCC}" \
      python3 -m pycircuit.cli build \
      "${tb_src}" \
      --out-dir "${out_dir}" \
      --target both \
      --jobs "${PYC_SIM_JOBS:-4}" \
      --logic-depth "${PYC_SIM_LOGIC_DEPTH:-256}" \
      --run-verilator; then
    pyc_die "build failed: ${name} (see ${case_log_root}/${name}/build.stderr)"
  fi

  local cpp_bin
  cpp_bin="$(
    python3 - "${out_dir}/project_manifest.json" <<'PY'
import json
import sys
with open(sys.argv[1], "r", encoding="utf-8") as f:
    manifest = json.load(f)
print(manifest.get("cpp_executable", ""))
PY
  )"
  if [[ -z "${cpp_bin}" || ! -x "${cpp_bin}" ]]; then
    pyc_die "missing or non-executable cpp_executable for ${name}: ${cpp_bin}"
  fi
  pyc_log "run(cpp) ${name}: ${cpp_bin}"
  if ! run_with_case_logs "${name}" run_cpp "${out_dir}" "${cpp_bin}"; then
    pyc_die "cpp execution failed: ${name} (see ${case_log_root}/${name}/run_cpp.stderr)"
  fi
}

while IFS=$'\t' read -r name _design tb _cfg _tier; do
  [[ -n "${name}" ]] || continue
  run_case_examples "example_${name}" "${tb}"
done < <(python3 "${DISCOVER}" --root "${PYC_ROOT_DIR}/examples/pycircuit" --tier all --format tsv)

run_case_nonexample() {
  local name="$1"
  local src="$2"
  if ! should_run_case "${name}"; then
    return 0
  fi
  local out_dir="${OUT_BASE}/${name}"
  rm -rf "${out_dir}" >/dev/null 2>&1 || true
  mkdir -p "${out_dir}"
  pyc_log "sim(non-example) ${name}: ${src}"

  if ! run_with_case_logs "${name}" build "${PYC_ROOT_DIR}" \
      env PYTHONPATH="${PYTHONPATH_VAL}" PYTHONDONTWRITEBYTECODE=1 PYCC="${PYCC}" \
      python3 -m pycircuit.cli build \
      "${src}" \
      --out-dir "${out_dir}" \
      --target verilator \
      --jobs "${PYC_SIM_JOBS:-4}" \
      --logic-depth "${PYC_SIM_LOGIC_DEPTH:-256}" \
      --run-verilator; then
    pyc_die "build failed: ${name} (see ${case_log_root}/${name}/build.stderr)"
  fi
}

run_case_nonexample issq "${PYC_ROOT_DIR}/designs/blocks/IssueQueue/tb_issq.py"
run_case_nonexample regfile "${PYC_ROOT_DIR}/designs/blocks/RegisterFile/tb_regfile.py"
run_case_nonexample bypass_unit "${PYC_ROOT_DIR}/designs/blocks/BypassUnit/tb_bypass_unit.py"

if [[ -n "${resume_from_case}" && "${resume_seen}" -eq 0 ]]; then
  pyc_die "resume case not found: ${resume_from_case}"
fi

cat > "${docs_gate_dir}/run_sims_nightly_summary.json" <<EOF
{
  "run_id": "${gate_run_id}",
  "script": "run_sims_nightly.sh",
  "status": "pass",
  "case_timeout_sec": ${case_timeout_sec},
  "retry_on_timeout": ${retry_on_timeout}
}
EOF

pyc_log "all nightly sims passed"
