from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPOSITORY = Path(__file__).resolve().parents[4]
EXAMPLES = REPOSITORY / "examples/agentic-circuit" / "workspaces"
BUILD_DIRECTORY = Path(
    os.environ.get(
        "AGENTIC_CIRCUIT_TEST_BUILD_DIR",
        REPOSITORY / ".pycircuit_out" / "acir" / "dev-llvm22",
    )
).resolve()
SCENARIOS = (
    "producer_queue_consumer",
    "backpressured_pipeline",
    "request_response_memory",
    "nested_arrays",
    "multi_time_domain_bridge",
    "suspended_process",
)
COMMON_FILES = (
    "architecture.py",
    "agentic-circuit.toml",
    "trace.pto.json",
    "expected-result.json",
    "expected-hierarchy.json",
    "expected-stats.json",
    "expected-events.jsonl",
    "components/showcase.component.json",
    "components/showcase.binding.json",
)
DAVINCIOO_SCENARIOS = frozenset(
    {"producer_queue_consumer", "backpressured_pipeline", "request_response_memory"}
)
CANONICAL_ARTIFACTS = (
    "input/model.acpy.json",
    "input/model.ac.mlir",
    "frozen.ac.mlir",
    "model.acsim.mlir",
)


def _environment() -> dict[str, str]:
    environment = os.environ.copy()
    entries = (REPOSITORY / "python/agentic-circuit/src", BUILD_DIRECTORY / "python")
    environment["PYTHONPATH"] = os.pathsep.join(
        [*(str(path) for path in entries), environment.get("PYTHONPATH", "")]
    ).rstrip(os.pathsep)
    return environment


def _run_cli(workspace: Path, *arguments: str) -> dict[str, object]:
    completed = subprocess.run(
        [sys.executable, "-m", "agentic_circuit._cli", *arguments],
        cwd=workspace,
        env=_environment(),
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode != 0:
        raise AssertionError(
            f"CLI failed ({completed.returncode}): {' '.join(arguments)}\n"
            f"stdout:\n{completed.stdout}\nstderr:\n{completed.stderr}"
        )
    if completed.stderr:
        raise AssertionError(f"structured CLI wrote stderr: {completed.stderr}")
    return json.loads(completed.stdout)


def _selected_result(run_directory: Path) -> dict[str, object]:
    result = json.loads((run_directory / "run-result.json").read_text())
    statistics = json.loads((run_directory / "stats.json").read_text())
    architectural_values = {
        entry["name"].removeprefix("architectural_"): entry["value"]
        for entry in statistics
        if entry["name"].startswith("architectural_")
    }
    return {
        "schema": "workspace-showcase-result",
        "version": "0.1",
        "status": result["status"],
        "termination_reason": result["termination_reason"],
        "simulated_ticks": result["simulated_ticks"],
        "trace_position": result["trace_position"],
        "architectural_values": architectural_values,
    }


def _copy_workspace(source: Path, destination: Path) -> None:
    shutil.copytree(
        source,
        destination,
        ignore=shutil.ignore_patterns("build", "__pycache__", "*.pyc"),
    )


def _pack(events: Path, output: Path) -> None:
    completed = subprocess.run(
        [
            sys.executable,
            os.fspath(REPOSITORY / "compiler/acir/tools/pack-perfetto-trace.py"),
            os.fspath(events),
            os.fspath(output),
        ],
        cwd=REPOSITORY,
        env=_environment(),
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode != 0:
        raise AssertionError(completed.stderr)


class WorkspaceExampleTest(unittest.TestCase):
    maxDiff = None

    def test_all_six_workspaces_own_the_complete_golden_set(self) -> None:
        self.assertTrue(EXAMPLES.is_dir())
        self.assertEqual(
            {*SCENARIOS, "npu"},
            {path.name for path in EXAMPLES.iterdir() if path.is_dir()},
        )
        for name in SCENARIOS:
            with self.subTest(example=name):
                root = EXAMPLES / name
                for relative in COMMON_FILES:
                    self.assertTrue((root / relative).is_file(), relative)
                if name in DAVINCIOO_SCENARIOS:
                    self.assertTrue((root / "source.davincioo.jsonl").is_file())
                manifest = (root / "agentic-circuit.toml").read_text()
                self.assertIn(f'name = "workspace-{name.replace("_", "-")}"', manifest)
                trace = json.loads((root / "trace.pto.json").read_text())
                self.assertEqual("pto-trace", trace["schema"])
                self.assertEqual("0.1", trace["version"])

    def test_davincioo_sources_reproduce_the_checked_in_canonical_trace(self) -> None:
        adapter = REPOSITORY / "compiler/acir/tools" / "import-davincioo-pto-trace.py"
        with tempfile.TemporaryDirectory(prefix="workspace-adapter-") as temporary:
            output_root = Path(temporary)
            for name in sorted(DAVINCIOO_SCENARIOS):
                with self.subTest(example=name):
                    example = EXAMPLES / name
                    output = output_root / f"{name}.pto.json"
                    completed = subprocess.run(
                        [
                            sys.executable,
                            os.fspath(adapter),
                            os.fspath(example / "source.davincioo.jsonl"),
                            os.fspath(output),
                            "--source-program",
                            "examples/beginner_matmul",
                        ],
                        cwd=REPOSITORY,
                        env=_environment(),
                        text=True,
                        capture_output=True,
                        check=False,
                    )
                    self.assertEqual(0, completed.returncode, completed.stderr)
                    self.assertEqual("", completed.stdout)
                    self.assertEqual(
                        (example / "trace.pto.json").read_bytes(),
                        output.read_bytes(),
                    )

    def test_public_pipeline_goldens_replay_and_root_independence(self) -> None:
        with tempfile.TemporaryDirectory(prefix="workspace-e2e-") as temporary:
            temporary_root = Path(temporary)
            for name in SCENARIOS:
                with self.subTest(example=name):
                    source = EXAMPLES / name
                    first = temporary_root / "alpha" / name
                    second = temporary_root / "unrelated" / "omega" / name
                    _copy_workspace(source, first)
                    _copy_workspace(source, second)
                    first_artifacts = first / "artifacts"
                    second_artifacts = second / "artifacts"

                    checked = _run_cli(
                        first,
                        "check",
                        "architecture.py",
                        "--project",
                        "agentic-circuit.toml",
                        "--json",
                    )
                    self.assertEqual("passed", checked["status"])
                    _run_cli(
                        first,
                        "elaborate",
                        "architecture.py",
                        "--project",
                        "agentic-circuit.toml",
                        "--emit",
                        "acir",
                        "-o",
                        os.fspath(first_artifacts / "elaborated.ac.mlir"),
                        "--json",
                    )
                    _run_cli(
                        first,
                        "compile",
                        "architecture.py",
                        "--project",
                        "agentic-circuit.toml",
                        "--emit",
                        "acpy,acir,frozen-acir,acsim,cpp",
                        "--output-dir",
                        os.fspath(first_artifacts / "compile"),
                        "--json",
                    )
                    built = _run_cli(
                        first,
                        "build",
                        "architecture.py",
                        "--project",
                        "agentic-circuit.toml",
                        "--output-dir",
                        os.fspath(first_artifacts / "build"),
                        "--json",
                    )
                    self.assertEqual("passed", built["status"])
                    ran = _run_cli(
                        first,
                        "run",
                        "architecture.py",
                        "--project",
                        "agentic-circuit.toml",
                        "--trace",
                        "trace.pto.json",
                        "--stats-format",
                        "json",
                        "--event-log",
                        "jsonl",
                        "--expect-termination",
                        "--output-dir",
                        os.fspath(first_artifacts / "run"),
                        "--json",
                    )
                    self.assertEqual("completed", ran["status"])
                    hierarchy = _run_cli(
                        first,
                        "inspect",
                        "hierarchy",
                        "--project",
                        "agentic-circuit.toml",
                        "--format",
                        "json",
                        "--json",
                    )

                    expected_result = json.loads(
                        (source / "expected-result.json").read_text()
                    )
                    expected_hierarchy = json.loads(
                        (source / "expected-hierarchy.json").read_text()
                    )
                    self.assertEqual(
                        expected_result, _selected_result(first_artifacts / "run")
                    )
                    self.assertEqual(expected_hierarchy, hierarchy)
                    self.assertEqual(
                        (source / "expected-stats.json").read_bytes(),
                        (first_artifacts / "run/stats.json").read_bytes(),
                    )
                    self.assertEqual(
                        (source / "expected-events.jsonl").read_bytes(),
                        (first_artifacts / "run/events.jsonl").read_bytes(),
                    )

                    for replay_name in ("replay-one", "replay-two"):
                        replayed = _run_cli(
                            first,
                            "run",
                            "--replay-manifest",
                            os.fspath(first_artifacts / "run/run-manifest.json"),
                            "--output-dir",
                            os.fspath(first_artifacts / replay_name),
                            "--json",
                        )
                        self.assertEqual("completed", replayed["status"])
                        for relative in (
                            "run-result.json",
                            "stats.json",
                            "events.jsonl",
                        ):
                            self.assertEqual(
                                (first_artifacts / "run" / relative).read_bytes(),
                                (first_artifacts / replay_name / relative).read_bytes(),
                            )

                    _run_cli(
                        second,
                        "compile",
                        "architecture.py",
                        "--project",
                        "agentic-circuit.toml",
                        "--emit",
                        "acpy,acir,frozen-acir,acsim",
                        "--output-dir",
                        os.fspath(second_artifacts / "compile"),
                        "--json",
                    )
                    _run_cli(
                        second,
                        "run",
                        "architecture.py",
                        "--project",
                        "agentic-circuit.toml",
                        "--trace",
                        "trace.pto.json",
                        "--stats-format",
                        "json",
                        "--event-log",
                        "jsonl",
                        "--expect-termination",
                        "--output-dir",
                        os.fspath(second_artifacts / "run"),
                        "--json",
                    )
                    for relative in CANONICAL_ARTIFACTS:
                        self.assertEqual(
                            (first_artifacts / "compile" / relative).read_bytes(),
                            (second_artifacts / "compile" / relative).read_bytes(),
                        )
                    for relative in ("stats.json", "events.jsonl"):
                        self.assertEqual(
                            (first_artifacts / "run" / relative).read_bytes(),
                            (second_artifacts / "run" / relative).read_bytes(),
                        )
                    _pack(
                        first_artifacts / "run/events.jsonl",
                        first_artifacts / "perfetto.json",
                    )
                    _pack(
                        second_artifacts / "run/events.jsonl",
                        second_artifacts / "perfetto.json",
                    )
                    self.assertEqual(
                        (first_artifacts / "perfetto.json").read_bytes(),
                        (second_artifacts / "perfetto.json").read_bytes(),
                    )


if __name__ == "__main__":
    unittest.main()
