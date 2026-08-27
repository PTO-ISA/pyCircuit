#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
build_dir="${ACIR_BUILD_DIR:-${repo_root}/build/local}"
acir_build="${ACIR_BUILD:-${build_dir}/bin/acir-build}"
work_dir="${script_dir}/out"

if [[ -z "${PTO_TRACE:-}" ]]; then
  echo "error: PTO_TRACE must name a PTO JSONL trace" >&2
  echo "example: PTO_TRACE=${script_dir}/synthetic.pto.trace $0" >&2
  exit 1
fi
if [[ ! -f "${PTO_TRACE}" ]]; then
  echo "error: PTO_TRACE does not exist: ${PTO_TRACE}" >&2
  exit 1
fi
if [[ ! -x "${acir_build}" ]]; then
  echo "error: acir-build not found at ${acir_build}" >&2
  echo "set ACIR_BUILD_DIR or ACIR_BUILD, then rebuild acir-build." >&2
  exit 1
fi

rm -rf "${work_dir}"
"${acir_build}" "${script_dir}/model.mlir" --output-dir="${work_dir}" \
  --profile=fast --target=x86_64-linux-gnu
"${work_dir}/sim" --trace "${PTO_TRACE}"
