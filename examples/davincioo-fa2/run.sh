#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
build_dir="${ACIR_BUILD_DIR:-${repo_root}/build/local}"
acir_build="${ACIR_BUILD:-${build_dir}/bin/acir-build}"
work_dir="${script_dir}/out"
model="${script_dir}/../davincioo-matmul/model.mlir"
trace="${PTO_TRACE:-${script_dir}/fa2-b1-h1-s128-d64.pto.trace}"
timeline="${TIMELINE:-${work_dir}/fa2.perfetto.json}"

if [[ ! -f "${trace}" ]]; then
  echo "error: PTO_TRACE does not exist: ${trace}" >&2
  exit 1
fi
if [[ ! -x "${acir_build}" ]]; then
  echo "error: acir-build not found at ${acir_build}" >&2
  echo "set ACIR_BUILD_DIR or ACIR_BUILD, then rebuild acir-build." >&2
  exit 1
fi

rm -rf "${work_dir}"
"${acir_build}" "${model}" --output-dir="${work_dir}" \
  --profile=fast --target=x86_64-linux-gnu
mkdir -p "$(dirname "${timeline}")"
"${work_dir}/sim" --trace "${trace}" --timeline "${timeline}"
echo "timeline: ${timeline}"
