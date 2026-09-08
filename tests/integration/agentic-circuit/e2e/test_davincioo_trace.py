from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]


class DavinciOOTraceTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        super().setUpClass()
        if shutil.which("cmake") is None or shutil.which("c++") is None:
            raise unittest.SkipTest("CMake and a C++ compiler are required")
        cls.reference_directory = tempfile.TemporaryDirectory()
        build = Path(cls.reference_directory.name) / "reference"
        configured = subprocess.run(
            (
                "cmake",
                "-S",
                str(ROOT / "third_party/references/davincioo-gfsim"),
                "-B",
                str(build),
                "-DCMAKE_BUILD_TYPE=Release",
                "-DBUILD_TESTING=OFF",
            ),
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        if configured.returncode != 0:
            raise AssertionError(configured.stderr)
        compiled = subprocess.run(
            ("cmake", "--build", str(build), "--target", "davincioo-gfsim-reference"),
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        if compiled.returncode != 0:
            raise AssertionError(compiled.stderr)
        cls.reference = build / "davincioo-gfsim-reference"

    @classmethod
    def tearDownClass(cls) -> None:
        cls.reference_directory.cleanup()
        super().tearDownClass()

    def test_generated_gfsim_matches_reference_projection_and_artifacts(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "run"
            completed = subprocess.run(
                (
                    sys.executable,
                    str(ROOT / "compiler/acir/tools/run-davincioo.py"),
                    "--output-dir",
                    str(output),
                    "--reference-executable",
                    str(self.reference),
                ),
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, completed.returncode, completed.stderr)
            report = json.loads((output / "run.json").read_text(encoding="utf-8"))
            self.assertEqual(15, report["record_count"])
            self.assertEqual(453, report["cycles"])
            self.assertEqual(report["reference_cycles"], report["cycles"])
            self.assertEqual(list(range(15)), report["retirement_order"])
            self.assertEqual(
                [0, 2, 3, 5, 7, 9, 10, 12, 1, 4, 6, 8, 11, 13, 14],
                report["completion_order"],
            )
            oracle = json.loads(
                (output / "oracle-report.json").read_text(encoding="utf-8")
            )
            self.assertEqual("passed", oracle["status"])
            self.assertIsNone(oracle["first_divergence"])
            self.assertTrue(all(oracle["comparisons"].values()))
            self.assertIn("reference_runtime_verified=true", completed.stdout)
            source_trace = (
                ROOT
                / "third_party/references/davincioo-gfsim/upstream/tests/fixtures/traces"
                / "examples_intermediate_softmax.pto.trace"
            )
            self.assertEqual(
                source_trace.read_bytes(), (output / "reference-input.jsonl").read_bytes()
            )
            observations = json.loads(
                (output / "reference-runtime-observations.json").read_text(
                    encoding="utf-8"
                )
            )
            self.assertEqual(
                "sha256:4125323d00bd259abe992c1b207526ac7dd834a3fc42a1a03d7d1c894b350016",
                observations["raw_trace_sha256"],
            )
            self.assertEqual(
                "davincioo-pinned-reference-projection",
                oracle["reference_model"]["kind"],
            )
            artifacts = ROOT / "tests/goldens/agentic-circuit/davincioo"
            self.assertEqual(
                (artifacts / "davincioo-softmax-run.json").read_bytes(),
                (output / "run.json").read_bytes(),
            )
            self.assertEqual(
                (artifacts / "davincioo-softmax-swimlane.svg").read_bytes(),
                (output / "swimlane.svg").read_bytes(),
            )

    def test_oracle_is_independent_of_input_root_and_python_hash_seed(self) -> None:
        source_trace = (
            ROOT
            / "third_party/references/davincioo-gfsim/upstream/tests/fixtures/traces"
            / "examples_intermediate_softmax.pto.trace"
        )
        source_projection = (
            ROOT / "tests/goldens/agentic-circuit/davincioo/softmax-projection.json"
        )
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            outputs = []
            for index, seed in enumerate(("1", "987654")):
                input_root = root / f"input-{index}"
                input_root.mkdir()
                trace = input_root / "trace.jsonl"
                projection = input_root / "projection.json"
                shutil.copyfile(source_trace, trace)
                shutil.copyfile(source_projection, projection)
                output = root / f"output-{index}"
                completed = subprocess.run(
                    (
                        sys.executable,
                        str(ROOT / "compiler/acir/tools/run-davincioo.py"),
                        "--trace",
                        str(trace),
                        "--projection",
                        str(projection),
                        "--output-dir",
                        str(output),
                        "--reference-executable",
                        str(self.reference),
                    ),
                    cwd=input_root,
                    env={**os.environ, "PYTHONHASHSEED": seed},
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(0, completed.returncode, completed.stderr)
                outputs.append(output)
            for name in (
                "canonical-trace.json",
                "reference-input.jsonl",
                "reference-result.json",
                "candidate-result.json",
                "oracle-report.json",
                "reference-runtime-observations.json",
                "run.json",
            ):
                self.assertEqual(
                    (outputs[0] / name).read_bytes(),
                    (outputs[1] / name).read_bytes(),
                )


if __name__ == "__main__":
    unittest.main()
