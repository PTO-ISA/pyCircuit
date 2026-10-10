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
  pyc_begin_test_run examples "$tier"
fi
export PYTHONDONTWRITEBYTECODE=1
export PYTHONPATH="$(pyc_pythonpath)"
pyc_set_public_helpers
cd "${PYC_ROOT_DIR}"
build="${PYCIRCUIT_TEST_OUTPUT}/examples-${tier}"
cmake -S "${PYC_ROOT_DIR}/examples" -B "${build}" -G Ninja \
  -DCMAKE_PREFIX_PATH="${PYC_TOOLCHAIN_ROOT}" \
  -Dpycircuit_DIR="${PYC_TOOLCHAIN_ROOT}/share/pycircuit/cmake" -DPYC_EXAMPLE_TIER="${tier}"
if ((list_only)); then
  pyc_list_tests examples "$tier" "${build}"
  exit 0
fi
pyc_list_tests examples "$tier" "${build}" \
  > "${docs_gate_dir}/examples-${tier}-selection.json"
cmake --build "${build}" --parallel "${PYC_TEST_JOBS:-4}"
ctest --test-dir "${build}" -L "^examples$" -L "^${tier}$" --no-tests=error --output-on-failure \
  --output-junit "${docs_gate_dir}/examples-${tier}.xml"
pyc_finish_test_run examples "$tier" "registered-examples-serial-parallel-rtl"
