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
EXAMPLE = REPOSITORY / "examples/agentic-circuit" / "workspaces" / "npu"
BUILD_DIRECTORY = Path(
    os.environ.get(
        "AGENTIC_CIRCUIT_TEST_BUILD_DIR",
        REPOSITORY / ".pycircuit_out" / "acir" / "dev-llvm22",
    )
).resolve()


def _environment() -> dict[str, str]:
    environment = os.environ.copy()
    entries = (REPOSITORY / "python/agentic-circuit/src", BUILD_DIRECTORY / "python")
    environment["PYTHONPATH"] = os.pathsep.join(
        [*(str(path) for path in entries), environment.get("PYTHONPATH", "")]
    ).rstrip(os.pathsep)
    return environment


def _run_cli(*arguments: str, cwd: Path = EXAMPLE) -> dict[str, object]:
    completed = subprocess.run(
        [sys.executable, "-m", "agentic_circuit._cli", *arguments],
        cwd=cwd,
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


def _copy_workspace(destination: Path) -> None:
    shutil.copytree(
        EXAMPLE,
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


def _selected_result(run_directory: Path) -> dict[str, object]:
    result = json.loads((run_directory / "run-result.json").read_text())
    statistics = json.loads((run_directory / "stats.json").read_text())
    architectural_values = {
        entry["name"].removeprefix("architectural_"): entry["value"]
        for entry in statistics
        if entry["name"].startswith("architectural_")
    }
    return {
        "schema": "workspace-npu-result",
        "version": "0.1",
        "status": result["status"],
        "termination_reason": result["termination_reason"],
        "simulated_ticks": result["simulated_ticks"],
        "trace_position": result["trace_position"],
        "architectural_values": architectural_values,
    }


class WorkspaceNpuTest(unittest.TestCase):
    maxDiff = None

    def test_adapter_reproduces_checked_in_canonical_trace(self) -> None:
        with tempfile.TemporaryDirectory(prefix="workspace-npu-adapter-") as temporary:
            output = Path(temporary) / "pto-trace.json"
            completed = subprocess.run(
                [
                    sys.executable,
                    os.fspath(
                        REPOSITORY / "compiler/acir/tools/import-davincioo-pto-trace.py"
                    ),
                    os.fspath(EXAMPLE / "traces/davincioo.jsonl"),
                    os.fspath(output),
                    "--source-program",
                    "examples/agentic-circuit/workspaces/npu",
                ],
                cwd=REPOSITORY,
                env=_environment(),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, completed.returncode, completed.stderr)
            self.assertEqual(
                (EXAMPLE / "traces/pto-trace.json").read_bytes(),
                output.read_bytes(),
            )
            trace = json.loads(output.read_text())
            self.assertEqual(
                "davincioo@e73633301cabed0d871ea5ff66e76a91df870aeb",
                trace["metadata"]["producer"],
            )
            self.assertEqual(6, trace["metadata"]["record_count"])

    def test_hierarchical_npu_build_run_and_goldens(self) -> None:
        checked = _run_cli(
            "check", "architecture.py", "--project", "agentic-circuit.toml", "--json"
        )
        self.assertEqual("passed", checked["status"])
        hierarchy = _run_cli(
            "inspect",
            "hierarchy",
            "--project",
            "agentic-circuit.toml",
            "--format",
            "json",
            "--json",
        )
        self.assertEqual(
            json.loads((EXAMPLE / "expected-hierarchy.json").read_text()), hierarchy
        )
        paths = {record["path"] for record in hierarchy["records"]}
        for suffix in (
            "trace_source",
            "frontend_decode",
            "frontend_dispatch",
            "backend_dependencies",
            "backend_issue_scalar",
            "backend_issue_vector",
            "backend_issue_cube",
            "backend_issue_tma",
            "execution_scalar_unit_0",
            "execution_vector_unit_0",
            "execution_vector_unit_1",
            "execution_cube_unit_0",
            "execution_tma_unit_0",
            "memory_load_store",
            "memory_scratchpad",
            "memory_controller",
            "completion",
            "retirement",
        ):
            self.assertIn(f"top.{suffix}", paths)

        with tempfile.TemporaryDirectory(prefix="workspace-npu-run-") as temporary:
            output = Path(temporary)
            built = _run_cli(
                "build",
                "architecture.py",
                "--project",
                "agentic-circuit.toml",
                "--output-dir",
                os.fspath(output / "build"),
                "--json",
            )
            self.assertEqual("passed", built["status"])
            ran = _run_cli(
                "run",
                "architecture.py",
                "--project",
                "agentic-circuit.toml",
                "--trace",
                "traces/pto-trace.json",
                "--stats-format",
                "json",
                "--event-log",
                "jsonl",
                "--expect-termination",
                "--output-dir",
                os.fspath(output / "run"),
                "--json",
            )
            self.assertEqual("completed", ran["status"])
            stats = json.loads((output / "run/stats.json").read_text())
            events = (output / "run/events.jsonl").read_text().splitlines()
            self.assertEqual(
                json.loads((EXAMPLE / "expected-result.json").read_text()),
                _selected_result(output / "run"),
            )
            self.assertEqual(
                (EXAMPLE / "expected-stats.json").read_bytes(),
                (output / "run/stats.json").read_bytes(),
            )
            self.assertEqual(
                (EXAMPLE / "expected-events.jsonl").read_bytes(),
                (output / "run/events.jsonl").read_bytes(),
            )
            self.assertEqual(
                6,
                next(
                    item["value"]
                    for item in stats
                    if item["name"] == "architectural_retired_instructions"
                ),
            )
            event_records = [json.loads(line) for line in events]
            complete_order = [
                item["args"]["gfsim_root_sequence_id"]
                for item in event_records
                if item["name"] == "complete"
            ]
            retire_order = [
                item["args"]["gfsim_root_sequence_id"]
                for item in event_records
                if item["name"] == "retire"
            ]
            self.assertNotEqual(sorted(complete_order), complete_order)
            self.assertEqual(list(range(6)), retire_order)

            _pack(output / "run/events.jsonl", output / "perfetto.json")
            self.assertEqual(
                (EXAMPLE / "expected-perfetto.json").read_bytes(),
                (output / "perfetto.json").read_bytes(),
            )

    def test_replay_and_equivalent_roots_are_byte_identical(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="workspace-npu-determinism-"
        ) as temporary:
            root = Path(temporary)
            first = root / "alpha" / "npu"
            second = root / "unrelated" / "omega" / "npu"
            _copy_workspace(first)
            _copy_workspace(second)

            for workspace in (first, second):
                _run_cli(
                    "run",
                    "architecture.py",
                    "--project",
                    "agentic-circuit.toml",
                    "--trace",
                    "traces/pto-trace.json",
                    "--stats-format",
                    "json",
                    "--event-log",
                    "jsonl",
                    "--expect-termination",
                    "--output-dir",
                    "artifacts/run",
                    "--json",
                    cwd=workspace,
                )
                _pack(
                    workspace / "artifacts/run/events.jsonl",
                    workspace / "artifacts/perfetto.json",
                )

            for replay_name in ("replay-one", "replay-two"):
                _run_cli(
                    "run",
                    "--replay-manifest",
                    "artifacts/run/run-manifest.json",
                    "--output-dir",
                    f"artifacts/{replay_name}",
                    "--json",
                    cwd=first,
                )

            for relative in ("run-result.json", "stats.json", "events.jsonl"):
                expected = (first / "artifacts/run" / relative).read_bytes()
                self.assertEqual(
                    expected, (second / "artifacts/run" / relative).read_bytes()
                )
                self.assertEqual(
                    expected,
                    (first / "artifacts/replay-one" / relative).read_bytes(),
                )
                self.assertEqual(
                    expected,
                    (first / "artifacts/replay-two" / relative).read_bytes(),
                )
            self.assertEqual(
                (first / "artifacts/perfetto.json").read_bytes(),
                (second / "artifacts/perfetto.json").read_bytes(),
            )


if __name__ == "__main__":
    unittest.main()
