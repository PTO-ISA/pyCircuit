#!/usr/bin/env bash
set -euo pipefail
source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/lib.sh"
tier=gate
list_only=0
while (($#)); do
  case "$1" in
    --tier) tier="${2:?--tier requires gate or nightly}"; shift 2 ;;
    --list) list_only=1; shift ;;
    -h|--help) echo "Usage: $0 [--tier gate|nightly] [--list]"; exit 0 ;;
    *) pyc_die "unknown option: $1" ;;
  esac
done
[[ "$tier" == gate || "$tier" == nightly ]] || pyc_die "tier must be gate or nightly"
if ((!list_only)); then
  pyc_begin_test_run api "$tier"
fi
export PYTHONDONTWRITEBYTECODE=1
export PYTHONPATH="$(pyc_pythonpath)"
pyc_set_public_helpers
test_python="$(pyc_require_pytest)"
cd "${PYC_ROOT_DIR}"
# Reconfigure the existing native build; never create a second test toolchain.
[[ -f "${PYC_BUILD_DIR}/CMakeCache.txt" ]] || pyc_die "build the CompilerDev testing profile first"
pyc_require_real_lit "${PYC_BUILD_DIR}"
nightly_python_tests=(
  tests/system/test_runtime_install.py
  tests/system/test_public_emit.py
  tests/system/test_physical_rtl_export.py
  tests/system/test_source_system_execution.py
  tests/system/test_source_map.py
  tests/system/test_incremental_build.py
  tests/system/test_relocated_compiler.py
  tests/system/test_publication_process_recovery.py
  tests/system/test_cmake_presets.py
)
gate_python_tests=(
  tests/system/test_physical_rtl_export.py::test_physical_reset_hold_and_four_state_payload
  tests/system/test_physical_rtl_export.py::test_python_four_state_structural_cpp_runs_from_generated_bundle
)
cmake -S "${PYC_ROOT_DIR}" -B "${PYC_BUILD_DIR}" -DPYC_BUILD_TESTING=ON -DPYC_TEST_TIER="${tier}"
if ((list_only)); then
  pyc_list_tests api "$tier" "${PYC_BUILD_DIR}"
  if [[ "$tier" == nightly ]]; then
    printf "Nightly Python tests: %s\n" "${nightly_python_tests[*]}" >&2
  else
    printf "Gate Python tests: %s\n" "${gate_python_tests[*]}" >&2
  fi
  exit 0
fi
pyc_list_tests api "$tier" "${PYC_BUILD_DIR}" \
  > "${docs_gate_dir}/api-${tier}-selection.json"
rm -f "${PYC_BUILD_DIR}/api-${tier}.xml" "${docs_gate_dir}/api-${tier}.xml"
# Preserve CTest's detailed result even if a test fails.
status=0
cmake --build "${PYC_BUILD_DIR}" --target check-pycircuit --parallel "${PYC_TEST_JOBS:-4}" || status=$?
if [[ -f "${PYC_BUILD_DIR}/api-${tier}.xml" ]]; then
  cp "${PYC_BUILD_DIR}/api-${tier}.xml" "${docs_gate_dir}/api-${tier}.xml"
fi
((status == 0)) || exit "$status"
if [[ "$tier" == gate ]]; then
  "${test_python}" -m pytest -q "${gate_python_tests[@]}"
fi
if [[ "$tier" == nightly ]]; then
  "${test_python}" -m pytest -q "${nightly_python_tests[@]}" \
    --junitxml="${docs_gate_dir}/api-package-nightly.xml"
fi
pyc_finish_test_run api "$tier" "common-ir-source-runtime"
