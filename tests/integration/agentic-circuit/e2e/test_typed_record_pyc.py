from __future__ import annotations

import importlib.util
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE

ROOT = Path(__file__).resolve().parents[4]
FIXTURE_ROOT = (
    ROOT / "tests/integration/agentic-circuit/e2e/fixtures/typed_transactions"
)
FIXTURE = FIXTURE_ROOT / "record_transform.py"
BUILD_ROOT = ROOT / ".pycircuit_out/layout-root"


class TypedRecordPycTest(unittest.TestCase):
    def test_stateless_record_is_scalar_packed_and_backend_equivalent(self) -> None:
        cxx = shutil.which("c++")
        verilator = shutil.which("verilator")
        toolchain = Path(os.environ.get("PYC_TOOLCHAIN_ROOT", BUILD_ROOT))
        tools = {
            "opt": Path(
                os.environ.get("ACIR_OPT", toolchain / "bin/acir-opt-internal")
            ),
            "pycgen": Path(
                os.environ.get("ACIR_QUEUE_PYCGEN", toolchain / "bin/acir-queue-pycgen")
            ),
            "pycc": Path(os.environ.get("PYCC", toolchain / "bin/pycc")),
            "runtime": Path(
                os.environ.get("PYC_RUNTIME_LIB", toolchain / "lib/libpyc6_runtime.a")
            ),
        }
        configured_include = os.environ.get("PYC_RUNTIME_INCLUDE")
        installed_include = toolchain / "include"
        runtime_include = (
            Path(configured_include)
            if configured_include
            else installed_include
            if installed_include.is_dir()
            else ROOT / "library"
        )
        if cxx is None or verilator is None:
            self.skipTest("C++ compiler or Verilator is unavailable")
        if any(not path.is_file() for path in tools.values()):
            self.skipTest("current-checkout PYC toolchain is unavailable")

        import agentic_circuit as ac

        spec = importlib.util.spec_from_file_location("ac_typed_record", FIXTURE)
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load typed record fixture")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        self.addCleanup(sys.modules.pop, spec.name, None)
        spec.loader.exec_module(module)

        specialization = ac.jit(module.record_transform, workspace=FIXTURE_ROOT)
        raw_acir = specialization.lower_acir()
        self.assertEqual(1, raw_acir.count("ac.var.record"))

        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            raw = output / "record.raw.mlir"
            frozen = output / "record.frozen.mlir"
            pyc = output / "record.pyc"
            cpp = output / "cpp"
            verilog = output / "verilog"
            raw.write_text(raw_acir, encoding="utf-8")
            optimized = subprocess.run(
                (
                    str(tools["opt"]),
                    f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                    str(raw),
                    "-o",
                    str(frozen),
                ),
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, optimized.returncode, optimized.stderr)
            lowered = subprocess.run(
                (str(tools["pycgen"]), str(frozen)),
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, lowered.returncode, lowered.stderr)
            pyc.write_text(lowered.stdout, encoding="utf-8")
            self.assertIn("%in_data: i12", lowered.stdout)
            self.assertEqual(3, lowered.stdout.count("pyc.extract"))
            self.assertEqual(1, lowered.stdout.count("pyc.concat"))

            for backend, destination in (("cpp", cpp), ("verilog", verilog)):
                command = [
                    str(tools["pycc"]),
                    str(pyc),
                    f"--emit={backend}",
                    f"--out-dir={destination}",
                    "--hierarchy-policy=strict",
                    "--inline-policy=off",
                    "--build-profile=dev-fast",
                ]
                if backend == "cpp":
                    command.append("--cpp-split=module")
                else:
                    command.append("--include-primitives")
                compiled = subprocess.run(
                    command,
                    cwd=ROOT,
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(0, compiled.returncode, compiled.stderr)

            cpp_harness = output / "cpp_harness.cpp"
            cpp_executable = output / "cpp_model"
            cpp_harness.write_text(
                """#include "record_transform.hpp"
#include <array>
#include <cstdint>
#include <iostream>

int main() {
  constexpr std::array<std::uint16_t, 4> inputs{0, 2665, 4094, 1281};
  pyc::gen::record_transform dut;
  for (std::uint64_t cycle = 0; cycle < 12; ++cycle) {
    dut.rst = pyc::cpp::Wire<1>(cycle == 0 ? 1 : 0);
    const bool offered = cycle >= 1 && cycle <= inputs.size();
    dut.in_valid = pyc::cpp::Wire<1>(offered ? 1 : 0);
    dut.in_data = pyc::cpp::Wire<12>(offered ? inputs[cycle - 1] : 0);
    dut.out_ready = pyc::cpp::Wire<1>(1);
    dut.clk = pyc::cpp::Wire<1>(0);
    dut.step();
    dut.clk = pyc::cpp::Wire<1>(1);
    dut.step();
    if (dut.out_valid.value())
      std::cout << cycle << " " << dut.out_data.value() << "\\n";
    dut.clk = pyc::cpp::Wire<1>(0);
    dut.step();
  }
}
""",
                encoding="utf-8",
            )
            cpp_sources = sorted(cpp.glob("record_transform*.cpp"))
            self.assertGreater(len(cpp_sources), 0)
            cpp_build = subprocess.run(
                (
                    cxx,
                    "-std=c++17",
                    "-I",
                    str(cpp),
                    "-I",
                    str(runtime_include),
                    *(str(path) for path in cpp_sources),
                    str(cpp_harness),
                    str(tools["runtime"]),
                    "-o",
                    str(cpp_executable),
                ),
                cwd=output,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, cpp_build.returncode, cpp_build.stderr)
            cpp_run = subprocess.run(
                (str(cpp_executable),),
                cwd=output,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, cpp_run.returncode, cpp_run.stderr)

            verilator_harness = output / "verilator_harness.cpp"
            verilator_harness.write_text(
                """#include "Vrecord_transform.h"
#include <array>
#include <cstdint>
#include <iostream>

int main() {
  constexpr std::array<std::uint16_t, 4> inputs{0, 2665, 4094, 1281};
  Vrecord_transform dut;
  for (std::uint64_t cycle = 0; cycle < 12; ++cycle) {
    dut.rst = cycle == 0 ? 1 : 0;
    const bool offered = cycle >= 1 && cycle <= inputs.size();
    dut.in_valid = offered ? 1 : 0;
    dut.in_data = offered ? inputs[cycle - 1] : 0;
    dut.out_ready = 1;
    dut.clk = 0;
    dut.eval();
    dut.clk = 1;
    dut.eval();
    if (dut.out_valid)
      std::cout << cycle << " " << dut.out_data << "\\n";
    dut.clk = 0;
    dut.eval();
  }
}
""",
                encoding="utf-8",
            )
            object_dir = output / "verilator_obj"
            verilator_build = subprocess.run(
                (
                    verilator,
                    "--cc",
                    "--exe",
                    "--build",
                    "-Wno-fatal",
                    "--top-module",
                    "record_transform",
                    "--Mdir",
                    str(object_dir),
                    str(verilog / "pyc_primitives.v"),
                    str(verilog / "record_transform.v"),
                    str(verilator_harness),
                ),
                cwd=output,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, verilator_build.returncode, verilator_build.stderr)
            verilator_run = subprocess.run(
                (str(object_dir / "Vrecord_transform"),),
                cwd=output,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, verilator_run.returncode, verilator_run.stderr)
            self.assertEqual(cpp_run.stdout, verilator_run.stdout)
            transactions = [
                int(line.split()[1]) for line in cpp_run.stdout.splitlines()
            ]
            self.assertEqual([3, 2666, 3585, 1282], transactions)


if __name__ == "__main__":
    unittest.main()
