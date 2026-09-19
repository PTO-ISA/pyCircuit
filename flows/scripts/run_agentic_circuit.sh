#!/usr/bin/env bash
set -euo pipefail

source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/lib.sh"

gate_run_id="${PYC_GATE_RUN_ID:-$(date +%Y%m%d-%H%M%S)}"
docs_gate_dir="${PYC_ROOT_DIR}/docs/gates/logs/${gate_run_id}"
gate_out_dir="$(pyc_out_root)/gates/${gate_run_id}/agentic-circuit"
ac_source="${PYC_ROOT_DIR}/compiler/acir"
ac_python="${PYC_ROOT_DIR}/python/agentic-circuit"
ac_test_python="${PYC_ROOT_DIR}/tests/python/agentic-circuit"
ac_build="${AC_GATE_BUILD_ROOT:-$(pyc_out_root)/acir/dev-llvm22}"
if [[ -n "${AC_GATE_BUILD_ROOT:-}" ]]; then
  ac_python_build="${ac_build}/compiler/acir/python"
  ac_native_gfsim="${ac_build}/compiler/acir/gfsim/libgfsim.a"
else
  ac_python_build="${ac_build}/python"
  ac_native_gfsim="${ac_build}/gfsim/libgfsim.a"
fi
ac_native_opt="${ac_build}/bin/acir-opt-internal"
ac_native_cxxgen="${ac_build}/bin/acir-queue-cxxgen"
ac_tests="${PYC_ROOT_DIR}/tests"
venv="$(pyc_out_root)/agentic-circuit/venv"
mkdir -p "${docs_gate_dir}" "${gate_out_dir}" "$(dirname "${venv}")"

exec > >(tee -a "${docs_gate_dir}/agentic_circuit.stdout") \
  2> >(tee -a "${docs_gate_dir}/agentic_circuit.stderr" >&2)

pyc_log "Agentic Circuit closure run-id=${gate_run_id}"

resume_from="${AC_GATE_RESUME_FROM:-g0}"
if [[ "${resume_from}" != "g0" && "${resume_from}" != "g2" ]]; then
  pyc_die "AC_GATE_RESUME_FROM must be g0 or g2"
fi
if [[ "${resume_from}" == "g0" ]]; then
  completed_lanes='["AC G0", "AC G1", "AC G2"]'
  completion_message="Agentic Circuit G0/G1/G2 closure passed"
else
  completed_lanes='["AC G2"]'
  completion_message="Agentic Circuit G2 closure passed (G0/G1 explicitly skipped)"
fi

recorded_toolchain="${AC_GATE_TOOLCHAIN_ROOT:-${gate_out_dir}/toolchain/install}"
{
  echo "PYC_GATE_RUN_ID=${gate_run_id} AC_GATE_RESUME_FROM=${resume_from} bash flows/scripts/run_agentic_circuit.sh"
  if [[ "${resume_from}" == "g0" ]]; then
    if [[ -n "${AC_GATE_BUILD_ROOT:-}" ]]; then
      echo "# reuse integrated native build: ${ac_build}"
    else
      echo "cmake --preset dev-llvm22 -S ${ac_source} -DACIR_BUILD_TESTING=ON"
      echo "cmake --build ${ac_build}"
    fi
    echo "python3 tools/agentic-circuit/check-contracts.py"
    echo "python3 -m unittest discover -s tests/python/agentic-circuit/contracts -p 'test_*.py'"
    echo "python3 -m unittest discover -s tests/python/agentic-circuit/python_frontend -p 'test_*.py'"
    echo "python3 -m unittest discover -s tests/python/agentic-circuit/cli -p 'test_*.py'"
    echo "cmake --build ${ac_build} --target check-acir"
    echo "ctest --test-dir ${ac_build} --output-on-failure"
  fi
  if [[ -n "${AC_GATE_TOOLCHAIN_ROOT:-}" ]]; then
    echo "# reuse installed toolchain: ${recorded_toolchain}"
  else
    echo "PYC_BUILD_AGENTIC_CIRCUIT=ON bash flows/scripts/pyc build"
  fi
  echo "${recorded_toolchain}/bin/acir-opt --pass-pipeline='builtin.module(ac-freeze-topology)' <raw-queue-graph> # topology closure"
  echo "${recorded_toolchain}/bin/acc -c <verified.ac> -emit-cpp -o <model.cpp>"
  echo "${recorded_toolchain}/bin/acc -c <verified.ac> -emit-cpp-bundle -o <bundle>"
  echo "${recorded_toolchain}/bin/acc -c <verified.ac> -emit-verilog -o <model.v>"
  echo "c++ -std=c++20 -Isimulator/gfsim/include -fsyntax-only <model.cpp>"
  echo "verilator --lint-only <model.v>"
} > "${docs_gate_dir}/agentic_circuit_commands.txt"

if [[ ! -x "${venv}/bin/python" ]]; then
  python3 -m venv "${venv}"
fi
"${venv}/bin/python" -m pip install -e "${PYC_ROOT_DIR}/python/semantic-core"
"${venv}/bin/python" -m pip install -e "${ac_python}[test]"

if [[ "${resume_from}" == "g0" ]]; then
  if [[ -n "${AC_GATE_BUILD_ROOT:-}" ]]; then
    [[ -d "${ac_build}" ]] || pyc_die "AC_GATE_BUILD_ROOT does not exist: ${ac_build}"
    [[ -d "${ac_python_build}" ]] || \
      pyc_die "integrated Agentic Circuit Python build is missing: ${ac_python_build}"
    pyc_log "AC G0/G1: reusing integrated native build ${ac_build}"
  else
    llvm_config="${LLVM_CONFIG:-}"
    if [[ -z "${llvm_config}" ]]; then
      for candidate in llvm-config-22 llvm-config; do
        if command -v "${candidate}" >/dev/null 2>&1; then
          llvm_config="$(command -v "${candidate}")"
          break
        fi
      done
    fi
    if [[ -z "${llvm_config}" ]]; then
      for candidate in \
        /opt/homebrew/opt/llvm/bin/llvm-config \
        /usr/local/opt/llvm/bin/llvm-config; do
        if [[ -x "${candidate}" ]]; then
          llvm_config="${candidate}"
          break
        fi
      done
    fi
    [[ -n "${llvm_config}" ]] || pyc_die "LLVM 22 llvm-config is required"
    [[ "$("${llvm_config}" --version | cut -d. -f1)" == "22" ]] || \
      pyc_die "Agentic Circuit requires LLVM 22"
    export LLVM_DIR="${LLVM_DIR:-$("${llvm_config}" --cmakedir)}"
    export MLIR_DIR="${MLIR_DIR:-$(dirname "${LLVM_DIR}")/mlir}"

    pyc_log "AC G0/G1: configure Agentic Circuit compiler"
    PATH="${venv}/bin:${PATH}" cmake --preset dev-llvm22 \
      -S "${ac_source}" -DACIR_BUILD_TESTING=ON
    cmake --build "${ac_build}" -j "${PYC_BUILD_JOBS:-6}"
  fi

  site_packages="$("${venv}/bin/python" -c \
    'import sysconfig; print(sysconfig.get_paths()["purelib"])')"
  for required in "${ac_native_opt}" "${ac_native_cxxgen}" "${ac_native_gfsim}"; do
    [[ -f "${required}" ]] || pyc_die "missing native Agentic test artifact: ${required}"
  done
  (
    cd "${PYC_ROOT_DIR}"
    env -u AC_GATE_TOOLCHAIN_ROOT \
      ACIR_OPT="${ac_native_opt}" \
      ACIR_QUEUE_CXXGEN="${ac_native_cxxgen}" \
      GFSIM_LIBRARY="${ac_native_gfsim}" \
      PYTHONPATH="${PYC_ROOT_DIR}/python/semantic-core/src:${ac_python}/src:${ac_test_python}:${ac_python_build}" \
      "${venv}/bin/python" tools/agentic-circuit/check-contracts.py
    env -u AC_GATE_TOOLCHAIN_ROOT \
      ACIR_OPT="${ac_native_opt}" \
      ACIR_QUEUE_CXXGEN="${ac_native_cxxgen}" \
      GFSIM_LIBRARY="${ac_native_gfsim}" \
      PYTHONPATH="${PYC_ROOT_DIR}/python/semantic-core/src:${ac_python}/src:${ac_test_python}:${ac_python_build}" \
      "${venv}/bin/python" -m unittest discover \
        -s tests/python/agentic-circuit/contracts -p 'test_*.py'
    env -u AC_GATE_TOOLCHAIN_ROOT \
      ACIR_OPT="${ac_native_opt}" \
      ACIR_QUEUE_CXXGEN="${ac_native_cxxgen}" \
      GFSIM_LIBRARY="${ac_native_gfsim}" \
      PYTHONPATH="${PYC_ROOT_DIR}/python/semantic-core/src:${ac_python}/src:${ac_test_python}:${ac_python_build}" \
      "${venv}/bin/python" -m unittest discover \
        -s tests/python/agentic-circuit/python_frontend -p 'test_*.py'
    env -u AC_GATE_TOOLCHAIN_ROOT \
      ACIR_OPT="${ac_native_opt}" \
      ACIR_QUEUE_CXXGEN="${ac_native_cxxgen}" \
      GFSIM_LIBRARY="${ac_native_gfsim}" \
      PYTHONPATH="${PYC_ROOT_DIR}/python/semantic-core/src:${ac_python}/src:${ac_test_python}:${ac_python_build}" \
      "${venv}/bin/python" -m unittest discover \
        -s tests/python/agentic-circuit/cli -p 'test_*.py'
    PYTHONPATH="${site_packages}" \
      cmake --build "${ac_build}" --target check-acir -j "${PYC_BUILD_JOBS:-6}"
    ctest --test-dir "${ac_build}" --output-on-failure \
      -j "${PYC_TEST_JOBS:-6}"
  )
fi

pyc_log "AC G2: build and install the repo-local pyc6 + AC toolchain"
if [[ -n "${AC_GATE_TOOLCHAIN_ROOT:-}" ]]; then
  toolchain="${AC_GATE_TOOLCHAIN_ROOT}"
  pyc_log "AC G2: reusing integrated toolchain ${toolchain}"
else
  gate_toolchain="${gate_out_dir}/toolchain"
  PYC_BUILD_DIR="${gate_toolchain}/build" \
  PYC_INSTALL_PREFIX="${gate_toolchain}/install" \
  PYC_BUILD_AGENTIC_CIRCUIT=ON \
    bash "${PYC_ROOT_DIR}/flows/scripts/pyc" build
  toolchain="${gate_toolchain}/install"
fi
acir_opt="${toolchain}/bin/acir-opt"
acc="${toolchain}/bin/acc"
pycc="${toolchain}/bin/pycc"
cxx="$(command -v c++ || true)"
verilator="$(command -v verilator || true)"
for required in "${acir_opt}" "${acc}" "${pycc}"; do
  [[ -f "${required}" ]] || pyc_die "missing integrated toolchain artifact: ${required}"
done
[[ -n "${cxx}" ]] || pyc_die "C++ compiler is required for AC G2"
[[ -n "${verilator}" ]] || pyc_die "Verilator is required for AC G2"

for case_name in arbiter atomic-transform bit-widths masked-match popcount; do
  case_dir="${gate_out_dir}/${case_name}"
  if [[ -e "${case_dir}" ]]; then
    pyc_die "AC G2 output already exists: ${case_dir}"
  fi
  mkdir -p "${case_dir}"
  "${acir_opt}" \
    --pass-pipeline='builtin.module(ac-freeze-topology)' \
    "${ac_tests}/mlir/agentic-circuit/CodeGen/${case_name}.mlir" \
    -o "${case_dir}/${case_name}.ac"
  "${acc}" -c "${case_dir}/${case_name}.ac" \
    -emit-cpp -o "${case_dir}/${case_name}.cpp"
  "${acc}" -c "${case_dir}/${case_name}.ac" \
    -emit-cpp-bundle -o "${case_dir}/${case_name}"
  "${acc}" -c "${case_dir}/${case_name}.ac" \
    -emit-verilog -o "${case_dir}/${case_name}.v"
  top_name="$(sed -n 's/^module \([A-Za-z_][A-Za-z0-9_$]*\).*/\1/p' \
    "${case_dir}/${case_name}.v" | head -n 1)"
  [[ -n "${top_name}" ]] || \
    pyc_die "ACC Verilog output has no top module: ${case_name}"
  "${cxx}" -std=c++20 -I"${PYC_ROOT_DIR}/simulator/gfsim/include" \
    -fsyntax-only "${case_dir}/${case_name}.cpp"
  "${verilator}" --lint-only -Wno-fatal --top-module "${top_name}" \
    -I"${toolchain}/include/verilog" \
    "${case_dir}/${case_name}.v"
  [[ -s "${case_dir}/${case_name}/include/generated/model.h" ]] || \
    pyc_die "ACC bundle is missing its public model header: ${case_name}"
done

cat > "${docs_gate_dir}/agentic_circuit_summary.json" <<EOF
{
  "run_id": "${gate_run_id}",
  "script": "run_agentic_circuit.sh",
  "status": "pass",
  "lanes": ${completed_lanes},
  "resume_from": "${resume_from}",
  "pyc_interface": "pyc6",
  "cases": ["arbiter", "atomic-transform", "bit-widths", "masked-match", "popcount", "acc-python-dut"]
}
EOF

pyc_log "${completion_message}"
