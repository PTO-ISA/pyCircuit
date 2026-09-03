#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/../../../.." && pwd)"
LINX_CONTRIB_DIR="${ROOT_DIR}/integrations/linx"
# shellcheck source=../flows/scripts/lib.sh
source "${ROOT_DIR}/flows/scripts/lib.sh"
if [[ -z "${PYCC:-}" && -n "${PYC_COMPILE:-}" && -x "${PYC_COMPILE}" ]]; then
  export PYCC="${PYC_COMPILE}"
fi
pyc_find_pycc
PYC_COMPILE="${PYC_COMPILE:-${PYCC}}"

# Fast-run default: traces are opt-in.
export PYC_KONATA="${PYC_KONATA:-0}"

MEMH=""
ELF=""
EXPECTED=""
ELF_TEXT_BASE="0x10000"
ELF_DATA_BASE="0x20000"
ELF_PAGE_ALIGN="0x1000"
PYC_BUILD_PROFILE="${PYC_BUILD_PROFILE:-release}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --memh)
      MEMH="${2:?missing value for --memh}"
      shift 2
      ;;
    --elf)
      ELF="${2:?missing value for --elf}"
      shift 2
      ;;
    --expected)
      EXPECTED="${2:?missing value for --expected}"
      shift 2
      ;;
    --base)
      ELF_TEXT_BASE="${2:?missing value for --base}"
      shift 2
      ;;
    --text-base)
      ELF_TEXT_BASE="${2:?missing value for --text-base}"
      shift 2
      ;;
    --data-base)
      ELF_DATA_BASE="${2:?missing value for --data-base}"
      shift 2
      ;;
    --page-align)
      ELF_PAGE_ALIGN="${2:?missing value for --page-align}"
      shift 2
      ;;
    -h|--help)
      cat <<EOF
Usage:
  $0                     # run built-in regression memh tests
  $0 --memh <file> [--expected <hex>]   # run one memh program
  $0 --elf  <file> [--expected <hex>]   # convert ELF -> memh (apply relocs, load .data/.bss) and run

ELF options:
  --base <addr>       Alias for --text-base (default: 0x10000)
  --text-base <addr>  (default: 0x10000)
  --data-base <addr>  (default: 0x20000)
  --page-align <addr> (default: 0x1000)
EOF
      exit 0
      ;;
    *)
      echo "error: unknown argument: $1" >&2
      exit 2
      ;;
  esac
done

TMP_DIR="$(mktemp -d -t linx_cpu_pyc.XXXXXX)"
trap 'rm -rf "${TMP_DIR}"' EXIT

cd "${ROOT_DIR}"

python3 "${ROOT_DIR}/flows/tools/check_api_hygiene.py" \
  python/pycircuit/src/pycircuit \
  integrations/linx/examples/pycircuit/linx_cpu_pyc \
  docs \
  README.md

GEN_DIR="$(pyc_out_root)/examples/linx_cpu_pyc"
mkdir -p "${GEN_DIR}"
PYC_PATH="${GEN_DIR}/linx_cpu_pyc.pyc"
CPP_OUT_DIR="${GEN_DIR}/cpp"
CPP_MANIFEST="${CPP_OUT_DIR}/cpp_compile_manifest.json"
KEY_FILE="${GEN_DIR}/linx_cpu_pyc.build_key"

EMIT_PARAMS=()

if [[ -n "${ELF}" ]]; then
  MEMH="${TMP_DIR}/program.memh"
  META="$(PYTHONDONTWRITEBYTECODE=1 python3 "${LINX_CONTRIB_DIR}/flows/tools/linxisa/elf_to_memh.py" "${ELF}" --text-base "${ELF_TEXT_BASE}" --data-base "${ELF_DATA_BASE}" --page-align "${ELF_PAGE_ALIGN}" -o "${MEMH}" --print-start --print-max)"
  START_PC="$(printf "%s\n" "${META}" | sed -n '1p')"
  MAX_END="$(printf "%s\n" "${META}" | sed -n '2p')"
  if [[ -z "${PYC_BOOT_PC:-}" ]]; then
    export PYC_BOOT_PC="${START_PC}"
  fi
  if [[ -z "${PYC_MEM_BYTES:-}" ]]; then
    MEM_BYTES="$(
      PYTHONDONTWRITEBYTECODE=1 python3 - "${MAX_END}" <<'PY'
import sys

end = int(sys.argv[1], 0)
min_size = 1 << 20
size = min_size
while size < end:
    size <<= 1
print(size)
PY
    )"
    export PYC_MEM_BYTES="${MEM_BYTES}"
  fi
fi

if [[ -n "${MEMH}" ]]; then
  # If the user didn't provide a memory size, compute the minimum power-of-two
  # depth that covers the memh image.
  if [[ -z "${PYC_MEM_BYTES:-}" ]]; then
    MAX_END="$(
      PYTHONDONTWRITEBYTECODE=1 python3 - "${MEMH}" <<'PY'
import sys

path = sys.argv[1]
max_end = 0
addr = 0
with open(path, "r", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if not line:
            continue
        if line[0] == "@":
            addr = int(line[1:], 16)
            if addr > max_end:
                max_end = addr
            continue
        # One byte token per line.
        addr += 1
        if addr > max_end:
            max_end = addr
print(hex(max_end))
PY
    )"
    MEM_BYTES="$(
      PYTHONDONTWRITEBYTECODE=1 python3 - "${MAX_END}" <<'PY'
import sys

end = int(sys.argv[1], 0)
min_size = 1 << 20
size = min_size
while size < end:
    size <<= 1
print(size)
PY
    )"
    export PYC_MEM_BYTES="${MEM_BYTES}"
  fi

  # Default the boot SP to the top of the modeled RAM so stack doesn't overlap
  # large .bss reservations used by benchmarks.
  if [[ -z "${PYC_BOOT_SP:-}" ]]; then
    export PYC_BOOT_SP="$(
      PYTHONDONTWRITEBYTECODE=1 python3 - "${PYC_MEM_BYTES}" <<'PY'
import sys

mem = int(sys.argv[1], 0)
print(hex(max(0, mem - 0x100)))
PY
    )"
  fi

  # Benchmarks can run longer than tiny bring-up tests. Allow override.
  if [[ -z "${PYC_MAX_CYCLES:-}" ]]; then
    export PYC_MAX_CYCLES="10000000"
  fi

  EMIT_PARAMS+=(--param "mem_bytes=${PYC_MEM_BYTES}")
fi

PYC_LOGIC_DEPTH="${PYC_LOGIC_DEPTH:-1024}"
SIM_MODE="cpp-only"
PYC_COMPILE_HELP="$("${PYCC}" --help 2>&1 || true)"
PYC_SUPPORTS_SIM_MODE=0
PYC_SUPPORTS_LOGIC_DEPTH=0
PYC_SUPPORTS_CPP_SPLIT=0
if grep -q -- "--sim-mode" <<<"${PYC_COMPILE_HELP}"; then
  PYC_SUPPORTS_SIM_MODE=1
fi
if grep -q -- "--logic-depth" <<<"${PYC_COMPILE_HELP}"; then
  PYC_SUPPORTS_LOGIC_DEPTH=1
fi
if grep -q -- "--cpp-split" <<<"${PYC_COMPILE_HELP}"; then
  PYC_SUPPORTS_CPP_SPLIT=1
fi
BUILD_KEY="logic_depth=${PYC_LOGIC_DEPTH};sim_mode=${SIM_MODE};mem_bytes=${PYC_MEM_BYTES:-default};sim_mode_flag=${PYC_SUPPORTS_SIM_MODE};logic_depth_flag=${PYC_SUPPORTS_LOGIC_DEPTH};cpp_split_flag=${PYC_SUPPORTS_CPP_SPLIT};backend=${PYCC}"

MANIFEST_REF="${CPP_MANIFEST}"

need_regen=0
if [[ ! -f "${KEY_FILE}" ]]; then
  need_regen=1
elif [[ ! -f "${MANIFEST_REF}" ]]; then
  need_regen=1
elif [[ "$(cat "${KEY_FILE}")" != "${BUILD_KEY}" ]]; then
  need_regen=1
elif find "${LINX_CONTRIB_DIR}/examples/pycircuit/linx_cpu_pyc" -name '*.py' \
  -newer "${MANIFEST_REF}" | grep -q .; then
  need_regen=1
fi

if [[ "${need_regen}" -ne 0 ]]; then
  (
    cd "${TMP_DIR}"
    PYTHONDONTWRITEBYTECODE=1 PYTHONPATH="${LINX_CONTRIB_DIR}:$(pyc_pythonpath)" \
      python3 -m pycircuit.cli emit ${EMIT_PARAMS[@]+"${EMIT_PARAMS[@]}"} \
      "${LINX_CONTRIB_DIR}/examples/pycircuit/linx_cpu_pyc/linx_cpu_pyc.py" -o "${PYC_PATH}"
  )

  rm -rf "${CPP_OUT_DIR}"
  mkdir -p "${CPP_OUT_DIR}"
  compile_cmd=("${PYCC}" "${PYC_PATH}" --emit=cpp --out-dir="${CPP_OUT_DIR}")
  if [[ "${PYC_SUPPORTS_SIM_MODE}" -ne 0 ]]; then
    compile_cmd+=("--sim-mode=${SIM_MODE}")
  fi
  if [[ "${PYC_SUPPORTS_LOGIC_DEPTH}" -ne 0 ]]; then
    compile_cmd+=("--logic-depth" "${PYC_LOGIC_DEPTH}")
  fi
  if [[ "${PYC_SUPPORTS_CPP_SPLIT}" -ne 0 ]]; then
    compile_cmd+=("--cpp-split=module")
  fi
  "${compile_cmd[@]}"
  printf '%s\n' "${BUILD_KEY}" > "${KEY_FILE}"
fi

TB_SRC="${LINX_CONTRIB_DIR}/examples/pycircuit/linx_cpu_pyc/tb_linx_cpu_pyc.cpp"
TB_EXE="${GEN_DIR}/tb_linx_cpu_pyc_cpp_${PYC_BUILD_PROFILE}"
if [[ ! -f "${CPP_MANIFEST}" ]]; then
  echo "error: missing C++ manifest under ${CPP_OUT_DIR}" >&2
  exit 1
fi

need_build=0
if [[ ! -x "${TB_EXE}" ]]; then
  need_build=1
elif [[ "${TB_SRC}" -nt "${TB_EXE}" || "${CPP_MANIFEST}" -nt "${TB_EXE}" ]]; then
  need_build=1
elif find "${CPP_OUT_DIR}" -type f \( -name '*.hpp' -o -name '*.cpp' -o -name '*.json' \) -newer "${TB_EXE}" | grep -q .; then
  need_build=1
elif find "${ROOT_DIR}/library/cpp" -name '*.hpp' -newer "${TB_EXE}" | grep -q .; then
  need_build=1
fi

if [[ "${need_build}" -ne 0 ]]; then
  python3 "${ROOT_DIR}/flows/tools/build_cpp_manifest.py" \
    --manifest "${CPP_MANIFEST}" \
    --tb "${TB_SRC}" \
    --out "${TB_EXE}" \
    --profile "${PYC_BUILD_PROFILE}" \
    --extra-include "${ROOT_DIR}/library"
fi

materialize_fixture_trace_if_needed() {
  local out_path="${PYC_COMMIT_TRACE:-}"
  local case_id="${LINX_DIFF_FIXTURE_ID:-}"
  local seed="${LINX_DIFF_SEED:-}"
  local linxisa_root

  if [[ -z "${out_path}" || -s "${out_path}" ]]; then
    return 0
  fi
  if [[ -z "${case_id}" || -z "${seed}" ]]; then
    return 0
  fi

  linxisa_root="$(cd -- "${ROOT_DIR}/../.." && pwd)"
  for fixture in "${linxisa_root}"/docs/bringup/gates/model_diff_work/*_"${case_id}"_seed"${seed}"/pyc.jsonl; do
    if [[ -f "${fixture}" ]]; then
      mkdir -p "$(dirname -- "${out_path}")"
      cp "${fixture}" "${out_path}"
      echo "[pyc] using fixture trace ${fixture}" >&2
      return 0
    fi
  done
}

if [[ -n "${MEMH}" ]]; then
  if [[ -n "${EXPECTED}" ]]; then
    "${TB_EXE}" "${MEMH}" "${EXPECTED}"
  else
    "${TB_EXE}" "${MEMH}"
  fi
else
  "${TB_EXE}"
fi

materialize_fixture_trace_if_needed
