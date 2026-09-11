from __future__ import annotations

import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
CODEGEN = ROOT / "tests/mlir/agentic-circuit/CodeGen"
DEFAULT_TOOLCHAIN = ROOT / ".pycircuit_out/toolchain/install"
NATIVE_ROOT = ROOT / ".pycircuit_out/acir/dev-llvm22/bin"


class QueueLanesBackendTest(unittest.TestCase):
    def _run_fixture(
        self,
        *,
        fixture: str,
        system: str,
        harness: str,
        inventory: list[str],
    ) -> str:
        toolchain = Path(os.environ.get("PYC_TOOLCHAIN_ROOT", DEFAULT_TOOLCHAIN))
        pycc = toolchain / "bin/pycc"
        metadata = toolchain / "share/pycircuit/toolchain-metadata.json"
        runtime = toolchain / "lib/libpyc6_runtime.a"
        acir_opt = NATIVE_ROOT / "acir-opt-internal"
        pycgen = NATIVE_ROOT / "acir-queue-pycgen"
        cxx = shutil.which("c++")
        verilator = shutil.which("verilator")
        if not all(
            (
                pycc.is_file(),
                metadata.is_file(),
                runtime.is_file(),
                acir_opt.is_file(),
                pycgen.is_file(),
                cxx,
                verilator,
            )
        ):
            self.skipTest("pinned pyCircuit toolchain or backend tools unavailable")

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            frozen = root / f"{system}.frozen.mlir"
            output = root / "output"
            optimized = subprocess.run(
                (
                    str(acir_opt),
                    "--pass-pipeline=builtin.module(ac-freeze-topology)",
                    str(CODEGEN / fixture),
                    "-o",
                    str(frozen),
                ),
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, optimized.returncode, optimized.stderr)

            completed = subprocess.run(
                (
                    str(ROOT / "compiler/acir/tools/ac-queue-pyc-build.py"),
                    str(frozen),
                    "--pycgen-tool",
                    str(pycgen),
                    "--pycc",
                    str(pycc),
                    "--toolchain-lock",
                    str(ROOT / "toolchains/agentic-circuit/pyc.lock.json"),
                    "--toolchain-metadata",
                    str(metadata),
                    "--cxx",
                    str(cxx),
                    "--verilator",
                    str(verilator),
                    "--pyc-output",
                    str(output / "model.pyc"),
                    "--cpp-output-dir",
                    str(output / "cpp"),
                    "--verilog-output-dir",
                    str(output / "verilog"),
                    "--manifest",
                    str(output / "manifest.json"),
                ),
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, completed.returncode, completed.stderr)
            manifest = json.loads((output / "manifest.json").read_text())
            self.assertEqual(inventory, manifest["opcode_lowering_inventory"])

            executable = root / f"{system}_cpp"
            built = subprocess.run(
                (
                    str(cxx),
                    "-std=c++17",
                    "-I",
                    str(output / "cpp"),
                    "-I",
                    str(toolchain / "include"),
                    str(output / f"cpp/{system}.cpp"),
                    str(CODEGEN / "Inputs" / harness),
                    str(runtime),
                    "-o",
                    str(executable),
                ),
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, built.returncode, built.stderr)
            ran = subprocess.run(
                (str(executable),), text=True, capture_output=True, check=False
            )
            self.assertEqual(0, ran.returncode, ran.stdout + ran.stderr)
            return ran.stdout

    def test_pyc_cpp_executes_the_ordered_prefix_fixture(self) -> None:
        output = self._run_fixture(
            fixture="queue-lanes-pyc.mlir",
            system="lane_bundle",
            harness="queue-lanes-pyc-main.cpp",
            inventory=["ac.sink", "ac.source"],
        )
        self.assertEqual("PASS pyc-cpp lane_bundle behavior\n", output)

    def test_pyc_cpp_executes_the_lane_transform_fixture(self) -> None:
        output = self._run_fixture(
            fixture="queue-lanes-transform-pyc.mlir",
            system="lane_transform",
            harness="queue-lanes-transform-pyc-main.cpp",
            inventory=["ac.sink", "ac.source", "ac.transform"],
        )
        self.assertEqual("PASS pyc-cpp lane_transform behavior\n", output)


if __name__ == "__main__":
    unittest.main()
