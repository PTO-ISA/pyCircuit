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
  export ACIR_SOURCE_UNIT_HARNESS="${ACIR_SOURCE_UNIT_HARNESS:-${root}/bin/acir-source-unit-harness}"
  export ACIR_DESIGN_HARNESS="${ACIR_DESIGN_HARNESS:-${root}/bin/acir-design-harness}"
  export ACIR_CPP_SOURCE_PARTS_HARNESS="${ACIR_CPP_SOURCE_PARTS_HARNESS:-${root}/bin/acir-cpp-source-parts-harness}"
  export PYCIRCUIT_NATIVE_BUILD="${PYCIRCUIT_NATIVE_BUILD:-${build}}"
  export PYCIRCUIT_COMPILER_INSTALL="${PYCIRCUIT_COMPILER_INSTALL:-${root}}"
  export PYCIRCUIT_M5_TEST_OUTPUT="${PYCIRCUIT_M5_TEST_OUTPUT:-$(pyc_out_root)/m5-gate-consumers}"
  export PATH="${root}/bin:${build}/bin:${PATH}"
  for helper in "${ACIR_SOURCE_UNIT_HARNESS}" "${ACIR_DESIGN_HARNESS}" "${ACIR_CPP_SOURCE_PARTS_HARNESS}"; do
    [[ -x "${helper}" ]] || pyc_die "required native helper is missing or not executable: ${helper}"
  done
}

pyc_out_root() { echo "${PYC_ROOT_DIR}/.pycircuit_out"; }
