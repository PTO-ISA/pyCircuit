from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from agentic_circuit._queue_frontend import lower_queue_source


ROOT = Path(__file__).resolve().parents[4]
PYC_REPOSITORY = ROOT
DEFAULT_TOOLCHAIN = PYC_REPOSITORY / ".pycircuit_out/toolchain/install"
CASES = (
    "pyc_barrier_pipeline",
    "pyc_credit_pipeline",
    "pyc_firing_pipeline",
    "pyc_loop_control_pipeline",
    "pyc_memory_pipeline",
    "pyc_recursive_pipeline",
    "pyc_select_pipeline",
)


class InventoryDeterminismTest(unittest.TestCase):
    def test_expanded_inventory_is_byte_identical_across_output_roots(self) -> None:
        toolchain = Path(os.environ.get("PYC_TOOLCHAIN_ROOT", DEFAULT_TOOLCHAIN))
        pycc = toolchain / "bin" / "pycc"
        metadata = toolchain / "share" / "pycircuit" / "toolchain-metadata.json"
        cxx = shutil.which("c++")
        verilator = shutil.which("verilator")
        if (
            not pycc.is_file()
            or not metadata.is_file()
            or cxx is None
            or verilator is None
        ):
            self.skipTest(
                "pinned pyCircuit toolchain, C++, or Verilator is unavailable"
            )

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for system in CASES:
                with self.subTest(system=system):
                    source = (
                        ROOT / "examples/agentic-circuit" / "pipelines" / f"{system}.py"
                    )
                    raw = root / f"{system}.raw.ac.mlir"
                    frozen = root / f"{system}.frozen.ac.mlir"
                    raw.write_text(
                        lower_queue_source(source.read_text(encoding="utf-8"), system),
                        encoding="utf-8",
                    )
                    optimized = subprocess.run(
                        (
                            str(ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"),
                            str(raw),
                        ),
                        text=True,
                        capture_output=True,
                        check=False,
                    )
                    self.assertEqual(0, optimized.returncode, optimized.stderr)
                    frozen.write_text(optimized.stdout, encoding="utf-8")

                    artifacts: list[dict[str, bytes]] = []
                    for run in range(2):
                        output = root / f"{system}-run-{run}"
                        completed = subprocess.run(
                            (
                                str(ROOT / "compiler/acir/tools/ac-queue-pyc-build.py"),
                                str(frozen),
                                "--pycgen-tool",
                                str(
                                    ROOT
                                    / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-pycgen"
                                ),
                                "--pycc",
                                str(pycc),
                                "--toolchain-lock",
                                str(ROOT / "toolchains/agentic-circuit/pyc.lock.json"),
                                "--toolchain-metadata",
                                str(metadata),
                                "--cxx",
                                cxx,
                                "--verilator",
                                verilator,
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
                        artifacts.append(
                            {
                                path.relative_to(output).as_posix(): path.read_bytes()
                                for path in sorted(output.rglob("*"))
                                if path.is_file()
                            }
                        )
                    self.assertEqual(artifacts[0], artifacts[1])


if __name__ == "__main__":
    unittest.main()
