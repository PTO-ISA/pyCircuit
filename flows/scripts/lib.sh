#!/usr/bin/env bash
set -euo pipefail

PYC_ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"

pyc_log() { echo "[pyc] $*"; }
pyc_warn() { echo "[pyc][warn] $*" >&2; }
pyc_die() { echo "[pyc][error] $*" >&2; exit 1; }

pyc_toolchain_root() {
  if [[ -n "${PYC_TOOLCHAIN_ROOT:-}" && -d "${PYC_TOOLCHAIN_ROOT}" ]]; then
    echo "${PYC_TOOLCHAIN_ROOT}"
    return 0
  fi
  local candidate="${PYC_ROOT_DIR}/.pycircuit_out/toolchain/install"
  if [[ -x "${candidate}/bin/pycircuit" ]]; then
    echo "${candidate}"
    return 0
  fi
  return 1
}

pyc_pythonpath() {
  if [[ "${PYC_USE_INSTALLED_PYTHON_PACKAGE:-0}" == "1" ]]; then
    echo "${PYC_PYTHONPATH:-}"
  elif [[ -n "${PYC_PYTHONPATH:-}" ]]; then
    echo "${PYC_PYTHONPATH}"
  else
    echo "${PYC_ROOT_DIR}/python/pycircuit/src${PYTHONPATH:+:${PYTHONPATH}}"
  fi
}

pyc_set_public_helpers() {
  local root
  root="$(pyc_toolchain_root)" || pyc_die "set PYC_TOOLCHAIN_ROOT to the installed pyCircuit toolchain"
  local build="${PYC_BUILD_DIR:-$(pyc_out_root)/toolchain/build}"
  export PYC_BUILD_DIR="${build}"
  export PYC_TOOLCHAIN_ROOT="${root}"
  export PYCIRCUIT_SOURCE_COMPILER="${PYCIRCUIT_SOURCE_COMPILER:-${root}/bin/pycircuit-source-unit}"
  export PYCIRCUIT_LINKER="${PYCIRCUIT_LINKER:-${root}/bin/pycircuit-link}"
  export PYCIRCUIT_EMITTER="${PYCIRCUIT_EMITTER:-${root}/bin/pycircuit-emit}"
  export PYCIRCUIT_NATIVE_BUILD="${PYCIRCUIT_NATIVE_BUILD:-${build}}"
  export PYCIRCUIT_COMPILER_INSTALL="${PYCIRCUIT_COMPILER_INSTALL:-${root}}"
  export PYCIRCUIT_TEST_PREFIX="${PYCIRCUIT_TEST_PREFIX:-${root}}"
  export PYCIRCUIT_TEST_OUTPUT="${PYCIRCUIT_TEST_OUTPUT:-$(pyc_out_root)/source-gate-consumers}"
  export PATH="${root}/bin:${build}/bin:${PATH}"
  for helper in "${PYCIRCUIT_SOURCE_COMPILER}" "${PYCIRCUIT_LINKER}" "${PYCIRCUIT_EMITTER}"; do
    [[ -x "${helper}" ]] || pyc_die "required native helper is missing or not executable: ${helper}"
  done
}

pyc_out_root() { echo "${PYC_ROOT_DIR}/.pycircuit_out"; }

# Both test entry points retain the actual CTest selection and discard stale
# success before building/running. --list never publishes a passing receipt.
pyc_begin_test_run() {
  local suite="$1" tier="$2"
  gate_run_id="${PYC_GATE_RUN_ID:-$(date +%Y%m%d-%H%M%S)}"
  docs_gate_dir="${PYC_ROOT_DIR}/docs/gates/logs/${gate_run_id}"
  mkdir -p "${docs_gate_dir}"
  rm -f "${docs_gate_dir}/${suite}-${tier}-summary.json"

}

pyc_finish_test_run() {
  local suite="$1" tier="$2" scope="$3"
  "${PYC_PYTHON_EXECUTABLE:-python3}" - "$docs_gate_dir" "$suite" "$tier" "$scope" <<'PYTHON'
import json
import sys
from pathlib import Path
folder, suite, tier, scope = sys.argv[1:]
folder = Path(folder)
selection = json.loads((folder / f"{suite}-{tier}-selection.json").read_text())
names = [test["name"] for test in selection["tests"]]
if not names:
    raise SystemExit("empty CTest selection cannot pass")
(folder / f"{suite}-{tier}-summary.json").write_text(json.dumps({
    "run_id": folder.name, "suite": suite, "tier": tier, "status": "pass",
    "scope": scope, "ctest_count": len(names), "tests": names,
    "package_tests": suite == "api" and tier == "nightly",
}, indent=2) + "\n")
PYTHON
  pyc_log "${suite} ${tier} passed; selection and results: ${docs_gate_dir}"
}

pyc_list_tests() {
  # CTest's --no-tests=error does not reject an empty --show-only result.
  ctest --test-dir "$3" -L "^$1$" -L "^$2$" --show-only=json-v1 --no-tests=error |
    "${PYC_PYTHON_EXECUTABLE:-python3}" -c '
import json, sys
selection = json.load(sys.stdin)
if not selection["tests"]:
    raise SystemExit("empty CTest selection")
print(json.dumps(selection, indent=2))'
}
