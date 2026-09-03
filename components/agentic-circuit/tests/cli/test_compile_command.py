from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPOSITORY = Path(__file__).parents[2]
FIXTURE = Path(__file__).parent / "fixtures" / "compile"
EXPECTED_COMPILE_TREE = (
    "frozen.ac.mlir",
    "include/generated/dispatch.h",
    "include/generated/model.h",
    "include/generated/modules/top_s289ddf7a6fa5af5e.h",
    "include/generated/processes/workload_se2ae7fb1cce72c6c.h",
    "input/model.ac.mlir",
    "input/model.acpy.json",
    "model.acsim.mlir",
    "src/generated/main.cpp",
    "src/generated/model.cpp",
    "src/generated/modules/top_s289ddf7a6fa5af5e.cpp",
    "src/generated/processes/workload_se2ae7fb1cce72c6c.cpp",
)


def run_cli(*arguments: str, cwd: Path) -> subprocess.CompletedProcess[str]:
    environment = os.environ.copy()
    environment["PYTHONPATH"] = os.pathsep.join(
        (str(REPOSITORY / "src"), str(REPOSITORY / "build/dev-llvm22/python"))
    )
    return subprocess.run(
        [sys.executable, "-m", "agentic_circuit._cli", *arguments],
        cwd=cwd,
        env=environment,
        text=True,
        capture_output=True,
        check=False,
    )


def workspace(temporary: str) -> Path:
    root = Path(temporary) / "project"
    shutil.copytree(FIXTURE, root)
    return root


def relative_tree(root: Path) -> tuple[str, ...]:
    if not root.exists():
        return ()
    return tuple(
        item.relative_to(root).as_posix()
        for item in sorted(root.rglob("*"))
        if item.is_file()
    )


class CompileCommandTest(unittest.TestCase):
    def test_all_exact_emits_are_published_in_fixed_order(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            result = run_cli(
                "compile",
                "architecture.py",
                "--emit=acpy,acir,frozen-acir,acsim,cpp",
                "--output-dir",
                "build/main",
                "--json",
                cwd=root,
            )
            tree = relative_tree(root / "build/main")

        self.assertEqual(0, result.returncode, result.stderr)
        self.assertEqual(EXPECTED_COMPILE_TREE, tree)
        payload = json.loads(result.stdout)
        self.assertEqual("agentic-circuit-compile-result", payload["schema"])
        self.assertEqual(list(EXPECTED_COMPILE_TREE), payload["artifacts"])

    def test_invalid_options_fail_before_publishing(self) -> None:
        cases = (
            (("--emit=acpy,acpy",), "ACPY-CLI-EMIT"),
            (("--emit=unknown",), "ACPY-CLI-EMIT"),
            (("--stop-after=unknown",), "ACPY-CLI-STAGE"),
            (
                ("--stop-after=acpy-verify", "--emit=frozen-acir"),
                "ACPY-CLI-STAGE",
            ),
            (("--profile=custom",), "ACPY-CLI-PIPELINE"),
            (
                ("--profile=fast", "--pass-pipeline=builtin.module()"),
                "ACPY-CLI-PIPELINE",
            ),
            (("--dump-after=unknown",), "ACPY-CLI-DUMP"),
            (
                ("--dump-before=topology-freeze",) * 2,
                "ACPY-CLI-DUMP",
            ),
        )
        for index, (arguments, code) in enumerate(cases):
            with self.subTest(arguments=arguments):
                with tempfile.TemporaryDirectory() as temporary:
                    root = workspace(temporary)
                    output = root / f"build/error-{index}"
                    result = run_cli(
                        "compile",
                        "architecture.py",
                        *arguments,
                        "--output-dir",
                        str(output),
                        cwd=root,
                    )

                    self.assertEqual(2, result.returncode, result.stderr)
                    self.assertIn(code, result.stderr)
                    self.assertFalse(output.exists())

    def test_logical_dump_names_are_retained_at_a_stopped_stage(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            result = run_cli(
                "compile",
                "architecture.py",
                "--emit=frozen-acir",
                "--stop-after=topology-freeze",
                "--dump-before=topology-freeze",
                "--dump-after=topology-freeze",
                "--output-dir=build/dumps",
                "--json",
                cwd=root,
            )
            tree = relative_tree(root / "build/dumps")

        self.assertEqual(0, result.returncode, result.stderr)
        self.assertEqual(
            (
                "dumps/topology-freeze-after.mlir",
                "dumps/topology-freeze-before.mlir",
                "frozen.ac.mlir",
            ),
            tree,
        )
        self.assertEqual("topology-freeze", json.loads(result.stdout)["stage"])

    def test_frontend_stop_does_not_enter_native_lowering(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            result = run_cli(
                "compile",
                "architecture.py",
                "--emit=acpy",
                "--stop-after=acpy-verify",
                "--output-dir=build/frontend",
                "--json",
                cwd=root,
            )
            tree = relative_tree(root / "build/frontend")

        self.assertEqual(0, result.returncode, result.stderr)
        self.assertEqual(("input/model.acpy.json",), tree)
        self.assertEqual("acpy-verify", json.loads(result.stdout)["stage"])

    def test_dump_after_each_uses_every_selected_logical_stage(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            result = run_cli(
                "compile",
                "architecture.py",
                "--emit=acpy",
                "--stop-after=acpy-verify",
                "--dump-after-each",
                "--output-dir=build/after-each",
                cwd=root,
            )
            tree = relative_tree(root / "build/after-each")

        self.assertEqual(0, result.returncode, result.stderr)
        self.assertEqual(
            (
                "dumps/acpy-construction-after.mlir",
                "dumps/acpy-verify-after.mlir",
                "dumps/frontend-capture-after.mlir",
                "input/model.acpy.json",
            ),
            tree,
        )

    def test_output_symlink_cannot_redirect_generated_artifacts(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            output = root / "build/redirected"
            outside = root / "outside"
            output.mkdir(parents=True)
            outside.mkdir()
            output.joinpath("include").symlink_to(outside, target_is_directory=True)

            result = run_cli(
                "compile",
                "architecture.py",
                "--emit=cpp",
                "--output-dir",
                str(output),
                cwd=root,
            )
            outside_tree = relative_tree(outside)

        self.assertEqual(2, result.returncode, result.stderr)
        self.assertIn("ACPY-CLI-OUTPUT", result.stderr)
        self.assertEqual((), outside_tree)


if __name__ == "__main__":
    unittest.main()
