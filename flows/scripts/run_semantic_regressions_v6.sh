#!/usr/bin/env bash
set -euo pipefail

source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/lib.sh"
pyc_find_pycc

PYTHONPATH_VAL="$(pyc_pythonpath)"
OUT_BASE="$(pyc_out_root)/semantic_v6"
mkdir -p "${OUT_BASE}"

gate_run_id="${PYC_GATE_RUN_ID:-$(date +%Y%m%d-%H%M%S)}"
docs_gate_dir="${PYC_ROOT_DIR}/docs/gates/logs/${gate_run_id}"
mkdir -p "${docs_gate_dir}"

pyc_log "semantic regressions v6 run-id=${gate_run_id}"

run_case() {
  local case_name="$1"
  local trace_cfg="${2:-}"
  local out_dir="${OUT_BASE}/${case_name}"
  rm -rf "${out_dir}" >/dev/null 2>&1 || true
  mkdir -p "${out_dir}"
  local tb="${PYC_ROOT_DIR}/examples/pycircuit/${case_name}/tb_${case_name}.py"
  [[ -f "${tb}" ]] || pyc_die "missing tb source: ${tb}"

  local cmd=(python3 -m pycircuit.cli build
    "${tb}"
    --out-dir "${out_dir}"
    --target both
    --jobs "${PYC_SIM_JOBS:-4}"
    --logic-depth "${PYC_SIM_LOGIC_DEPTH:-256}"
    --run-verilator)
  if [[ -n "${trace_cfg}" ]]; then
    cmd+=(--trace-config "${trace_cfg}")
  fi
  PYTHONPATH="${PYTHONPATH_VAL}" PYTHONDONTWRITEBYTECODE=1 PYCC="${PYCC}" "${cmd[@]}"

  local cpp_bin
  cpp_bin="$(
    python3 - "${out_dir}/project_manifest.json" <<'PY'
import json
import sys
with open(sys.argv[1], "r", encoding="utf-8") as f:
    m = json.load(f)
print(m.get("cpp_executable", ""))
PY
  )"
  if [[ -z "${cpp_bin}" || ! -x "${cpp_bin}" ]]; then
    pyc_die "missing cpp_executable for ${case_name}: ${cpp_bin}"
  fi
  (cd "${out_dir}" && "${cpp_bin}")
}

run_case "xz_value_model_smoke" "${PYC_ROOT_DIR}/examples/pycircuit/xz_value_model_smoke/xz_value_model_smoke_trace.json"
run_case "reset_invalidate_order_smoke" "${PYC_ROOT_DIR}/examples/pycircuit/reset_invalidate_order_smoke/reset_invalidate_order_smoke_trace.json"
run_case "net_resolution_depth_smoke" ""

xz_out="${OUT_BASE}/xz_value_model_smoke"
xz_top="$(python3 - "${xz_out}/project_manifest.json" <<'PY'
import json
import sys
with open(sys.argv[1], "r", encoding="utf-8") as f:
    m = json.load(f)
print(m.get("top", ""))
PY
)"
xz_trace="${xz_out}/tb_${xz_top}/tb_${xz_top}.pyctrace"
python3 "${PYC_ROOT_DIR}/flows/tools/dump_pyctrace.py" "${xz_trace}" --manifest "${xz_out}/probe_manifest.json" --max-cycles 8 --max-events 100 --no-header \
  > "${docs_gate_dir}/semantic_xz_dump.stdout" \
  2> "${docs_gate_dir}/semantic_xz_dump.stderr"
if ! grep -Eq "known=0x[0-9a-f]+ z=0x[0-9a-f]+" "${docs_gate_dir}/semantic_xz_dump.stdout"; then
  pyc_die "semantic xz gate failed: missing known/z fields in value-change dump"
fi

rst_out="${OUT_BASE}/reset_invalidate_order_smoke"
rst_top="$(python3 - "${rst_out}/project_manifest.json" <<'PY'
import json
import sys
with open(sys.argv[1], "r", encoding="utf-8") as f:
    m = json.load(f)
print(m.get("top", ""))
PY
)"
rst_trace="${rst_out}/tb_${rst_top}/tb_${rst_top}.pyctrace"
python3 "${PYC_ROOT_DIR}/flows/tools/dump_pyctrace.py" "${rst_trace}" --manifest "${rst_out}/probe_manifest.json" --max-cycles 8 --max-events 200 --no-header \
  > "${docs_gate_dir}/semantic_reset_dump.stdout" \
  2> "${docs_gate_dir}/semantic_reset_dump.stderr"
python3 - "${rst_trace}" <<'PY'
import struct
import sys
from pathlib import Path

p = Path(sys.argv[1]).resolve()
data = p.read_bytes()
if len(data) < 16:
    raise SystemExit("trace too small")
if data[:8] != b"PYC6TRC3":
    raise SystemExit(f"unexpected trace magic: {data[:8]!r}")
off = 16
events = []
while off + 8 <= len(data):
    chunk_len, chunk_ty = struct.unpack_from("<II", data, off)
    off += 8
    payload = memoryview(data)[off : off + chunk_len]
    off += chunk_len
    if chunk_ty == 9:
        cyc = int(struct.unpack_from("<Q", payload, 0)[0])
        phase_present = int(payload[8])
        phase = int(payload[9]) if phase_present else None
        events.append(("invalidate", cyc, phase))
    elif chunk_ty == 8:
        cyc = int(struct.unpack_from("<Q", payload, 0)[0])
        phase_present = int(payload[8])
        phase = int(payload[9]) if phase_present else None
        edge = int(payload[10])
        kind = "reset_assert" if edge == 1 else "reset_deassert"
        events.append((kind, cyc, phase))

def first(kind: str):
    for idx, ev in enumerate(events):
        if ev[0] == kind:
            return idx, ev
    return None

inv = first("invalidate")
ast = first("reset_assert")
dea = first("reset_deassert")
if inv is None or ast is None or dea is None:
    raise SystemExit("missing invalidate/reset events")
if not (inv[0] < ast[0] < dea[0]):
    raise SystemExit(f"ordering violation: {events}")
if not (inv[1][1] <= ast[1][1] <= dea[1][1]):
    raise SystemExit(f"cycle monotonicity violation: {events}")
print("ok: reset/invalidate ordering")
PY

cat > "${docs_gate_dir}/semantic_regressions_summary.json" <<EOF
{
  "run_id": "${gate_run_id}",
  "script": "run_semantic_regressions_v6.sh",
  "status": "pass",
  "cases": [
    "xz_value_model_smoke",
    "reset_invalidate_order_smoke",
    "net_resolution_depth_smoke"
  ]
}
EOF

pyc_log "semantic regressions passed"
