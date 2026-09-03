#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
# shellcheck source=../../../flows/scripts/lib.sh
source "${ROOT_DIR}/flows/scripts/lib.sh"
pyc_find_pycc

OUT_ROOT="${ROOT_DIR}/.pycircuit_out/integrations/janus/janus_tmu_pyc"
mkdir -p "${OUT_ROOT}"
PYC_LOGIC_DEPTH="${PYC_LOGIC_DEPTH:-64}"

tmp_pyc="$(mktemp -t "pycircuit.janus.tmu.XXXXXX.pyc")"

PYTHONDONTWRITEBYTECODE=1 PYTHONPATH="$(pyc_pythonpath):${ROOT_DIR}/integrations/janus/pyc" \
  python3 -m pycircuit.cli emit "${ROOT_DIR}/integrations/janus/pyc/janus/tmu/janus_tmu_pyc.py" -o "${tmp_pyc}"

"${PYCC}" "${tmp_pyc}" --logic-depth "${PYC_LOGIC_DEPTH}" --emit=verilog -o "${OUT_ROOT}/janus_tmu_pyc.v"
"${PYCC}" "${tmp_pyc}" --logic-depth "${PYC_LOGIC_DEPTH}" --emit=cpp -o "${OUT_ROOT}/janus_tmu_pyc.hpp"

mv -f "${OUT_ROOT}/janus_tmu_pyc.hpp" "${OUT_ROOT}/janus_tmu_pyc_gen.hpp"

pyc_log "ok: wrote TMU outputs under ${OUT_ROOT}"
