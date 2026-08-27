#!/usr/bin/env bash
# Freeze ACIR → lower to ACSim → emit C++ → compile → run the adder example.
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
build_dir="${ACIR_BUILD_DIR:-${repo_root}/build/local}"
acir_build="${ACIR_BUILD:-${build_dir}/bin/acir-build}"
work_dir="${script_dir}/out"

if [[ ! -x "${acir_build}" ]]; then
  echo "error: acir-build not found at ${acir_build}" >&2
  echo "set ACIR_BUILD_DIR or ACIR_BUILD, then rebuild acir-build." >&2
  exit 1
fi

rm -rf "${work_dir}"
echo "==> acir-build"
"${acir_build}" "${script_dir}/model.mlir" --output-dir="${work_dir}" \
  --profile=fast --target=x86_64-linux-gnu

echo "==> run"
set +e
"${work_dir}/sim"
status=$?
set -e

echo
echo "exit code: ${status} (completed adder reports classification=completed and diagnostic=sum=5)"
echo "artifacts: ${work_dir}"
ls -1 "${work_dir}/include/generated/model.h" \
      "${work_dir}/src/generated/model.cpp" \
      "${work_dir}/src/generated/main.cpp" \
      "${work_dir}/build-manifest.json" \
      "${work_dir}/sim"
exit 0
