from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from jsonschema.validators import Draft202012Validator

from agentic_circuit._canonical_json import sha256_bytes
from agentic_circuit._commands.build import BuildPublication
from agentic_circuit._run import RunOptions, execute_run


REPOSITORY = Path(__file__).parents[2]
FIXTURE = Path(__file__).parent / "fixtures" / "run"


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


def load_json(path: Path) -> dict[str, object]:
    return json.loads(path.read_text())


def validate_schema(name: str, document: dict[str, object]) -> None:
    schema = load_json(REPOSITORY / "schemas" / name)
    Draft202012Validator(schema).validate(document)


class RunCommandTest(unittest.TestCase):
    def test_completed_run_publishes_exact_documents(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            build = root / "build"
            executable = build / "bin/model"
            executable.parent.mkdir(parents=True)
            executable.write_text(
                "#!/usr/bin/env python3\n"
                "import hashlib,json,pathlib,sys\n"
                "manifest=pathlib.Path(sys.argv[2]).read_bytes()\n"
                "stage=pathlib.Path(sys.argv[4]);stage.mkdir()\n"
                "stats=b'[]\\n';validation=b'{\"status\":\"passed\"}\\n'\n"
                "(stage/'stats.json').write_bytes(stats)\n"
                "(stage/'validation-report.json').write_bytes(validation)\n"
                "digest=lambda data:'sha256:'+hashlib.sha256(data).hexdigest()\n"
                "result={'schema':'agentic-circuit-run-result','version':'0.1',"
                "'contract_epoch':'0.4','run_manifest':{'path':'run-manifest.json',"
                "'sha256':digest(manifest)},'status':'completed',"
                "'termination_reason':'trace_drained','simulated_ticks':0,"
                "'domain_cycles':{},'event_count':0,'trace_position':{"
                "'next_record_index':0,'last_committed_sequence_id':None},"
                "'outputs':[{'path':'stats.json','sha256':digest(stats)},"
                "{'path':'validation-report.json','sha256':digest(validation)}],"
                "'validation':{'status':'passed','report_sha256':digest(validation)}}\n"
                "(stage/'run-result.json').write_text(json.dumps(result,"
                "sort_keys=True,separators=(',',':'))+'\\n')\n"
            )
            executable.chmod(0o755)
            executable_bytes = executable.read_bytes()
            manifest = build / "build-manifest.json"
            manifest.write_text(
                json.dumps(
                    {
                        "artifacts": [
                            {
                                "path": "bin/model",
                                "kind": "executable",
                                "sha256": sha256_bytes(executable_bytes),
                            }
                        ]
                    },
                    sort_keys=True,
                    separators=(",", ":"),
                )
                + "\n"
            )
            trace = root / "trace.json"
            trace.write_bytes((FIXTURE / "trace.json").read_bytes())
            output = root / "runs/one"
            publication = execute_run(
                BuildPublication(build, executable, manifest, "sha256:" + "0" * 64, False),
                RunOptions(trace, output, 1, None, None, (), "json", "disabled", "complete"),
            )
            run_manifest = load_json(output / "run-manifest.json")
            run_result = load_json(output / "run-result.json")
            files = {
                path.relative_to(output).as_posix()
                for path in output.rglob("*")
                if path.is_file()
            }

        self.assertEqual(0, publication.exit_code)
        validate_schema("run-manifest.schema.json", run_manifest)
        validate_schema("run-result.schema.json", run_result)
        self.assertEqual("completed", run_result["status"])
        self.assertEqual(
            {
                "bin/model",
                "build-manifest.json",
                "run-manifest.json",
                "run-result.json",
                "stats.json",
                "trace.json",
                "validation-report.json",
            },
            files,
        )

    def test_tick_cap_is_incomplete_exit_seven(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            output = root / "runs/capped"
            result = run_cli(
                "run",
                "capped_architecture.py",
                "--trace",
                "trace.json",
                "--max-ticks",
                "1",
                "--event-log",
                "jsonl",
                "--output-dir",
                "runs/capped",
                cwd=root,
            )
            run_result = load_json(output / "run-result.json")
            event_log_exists = output.joinpath("events.jsonl").is_file()

        self.assertEqual(7, result.returncode, result.stderr)
        self.assertEqual("incomplete", run_result["status"])
        self.assertEqual("max_ticks", run_result["termination_reason"])
        self.assertTrue(event_log_exists)

    def test_replay_uses_only_the_immutable_bundle(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            first = run_cli(
                "run",
                "architecture.py",
                "--trace",
                "trace.json",
                "--output-dir",
                "runs/one",
                cwd=root,
            )
            root.joinpath("architecture.py").unlink()
            root.joinpath("trace.json").unlink()
            replay = run_cli(
                "run",
                "--replay-manifest",
                "runs/one/run-manifest.json",
                "--output-dir",
                "runs/two",
                cwd=root,
            )
            first_manifest = (root / "runs/one/run-manifest.json").read_bytes()
            replay_manifest = (root / "runs/two/run-manifest.json").read_bytes()

        self.assertEqual(6, first.returncode, first.stderr)
        self.assertEqual(6, replay.returncode, replay.stderr)
        self.assertEqual(first_manifest, replay_manifest)

    def test_replay_rejects_ambient_override(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            result = run_cli(
                "run",
                "--replay-manifest",
                "runs/one/run-manifest.json",
                "--seed",
                "2",
                cwd=root,
            )

        self.assertEqual(2, result.returncode)

    def test_invalid_trace_is_preflight_five_and_preserves_output(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            output = root / "runs/one"
            output.mkdir(parents=True)
            output.joinpath("sentinel.txt").write_text("preserve\n")
            root.joinpath("trace.json").write_text("{}\n")
            result = run_cli(
                "run",
                "architecture.py",
                "--trace",
                "trace.json",
                "--output-dir",
                "runs/one",
                cwd=root,
            )

            self.assertEqual("preserve\n", output.joinpath("sentinel.txt").read_text())
            self.assertEqual({"sentinel.txt"}, {path.name for path in output.iterdir()})

        self.assertEqual(5, result.returncode, result.stderr)

    def test_raw_davincioo_jsonl_is_rejected_before_publication(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = workspace(temporary)
            raw = root / "davincioo.jsonl"
            raw.write_text(
                '{"block_idx":0,"sequence_id":0,"opcode":"TASSIGN",'
                '"input_tiles":[],"scalar_inputs":[],"output_tiles":[]}\n'
            )
            result = run_cli(
                "run",
                "architecture.py",
                "--trace",
                raw.name,
                "--output-dir",
                "runs/raw",
                cwd=root,
            )

            self.assertFalse((root / "runs/raw").exists())

        self.assertEqual(5, result.returncode, result.stderr)
        self.assertIn("ACTRACE-SCHEMA-001", result.stderr)


if __name__ == "__main__":
    unittest.main()
