from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from agentic_circuit._canonical_json import canonical_json_bytes
from tools.perfetto_trace import PerfettoTraceError, pack_event_jsonl


ROOT = Path(__file__).resolve().parents[4]


def encoded(*events: dict[str, object]) -> bytes:
    return b"".join(
        json.dumps(event, separators=(",", ":")).encode("utf-8") + b"\n"
        for event in events
    )


def event(
    phase: object,
    *,
    time: int = 2,
    delta: int = 3,
    owner: int = 5,
    local_index: int = 0,
    **fields: object,
) -> dict[str, object]:
    value: dict[str, object] = {
        "name": "work",
        "cat": "execution",
        "ph": phase,
        "ts": time * 1024 + delta,
        "pid": 0,
        "tid": owner,
        "args": {
            "gfsim_epoch_time": time,
            "gfsim_epoch_delta": delta,
            "gfsim_object_id": owner,
            "gfsim_local_committed_index": local_index,
            "gfsim_root_sequence_id": 17,
            "engine": "cube",
        },
    }
    value.update(fields)
    return value


class PerfettoTraceLibraryTest(unittest.TestCase):
    def test_empty_input_is_one_canonical_trace_document(self) -> None:
        self.assertEqual(b'{"traceEvents":[]}\n', pack_event_jsonl(b""))

    def test_process_and_thread_metadata_are_preserved_exactly(self) -> None:
        process = {
            "name": "process_name",
            "ph": "M",
            "pid": 0,
            "args": {"name": "gfsim"},
        }
        thread = {
            "name": "thread_name",
            "ph": "M",
            "pid": 0,
            "tid": 5,
            "args": {"name": "/generated/root/cube"},
        }
        output = pack_event_jsonl(encoded(process, thread))
        self.assertEqual(
            canonical_json_bytes({"traceEvents": [process, thread]}) + b"\n",
            output,
        )
        self.assertEqual([process, thread], json.loads(output)["traceEvents"])

    def test_all_runtime_phases_preserve_input_order(self) -> None:
        instant = event("i", s="t")
        complete = event("X", local_index=1, dur=9)
        counter = event(
            "C",
            local_index=2,
            args={
                "gfsim_epoch_time": 2,
                "gfsim_epoch_delta": 3,
                "gfsim_object_id": 5,
                "gfsim_local_committed_index": 2,
                "occupancy": 4,
            },
        )
        flow_start = event("s", local_index=3, id=17)
        flow_end = event("f", local_index=4, id=17, bp="e")
        events = [instant, complete, counter, flow_start, flow_end]

        output = pack_event_jsonl(encoded(*events))

        self.assertEqual(events, json.loads(output)["traceEvents"])
        self.assertEqual(canonical_json_bytes({"traceEvents": events}) + b"\n", output)

    def test_malformed_duplicate_and_unknown_fields_are_rejected(self) -> None:
        duplicate = (
            b'{"name":"work","name":"again","cat":"execution","ph":"C",'
            b'"ts":0,"pid":0,"tid":0,"args":{"gfsim_epoch_time":0,'
            b'"gfsim_epoch_delta":0,"gfsim_object_id":0,'
            b'"gfsim_local_committed_index":0}}\n'
        )
        for source in (
            duplicate,
            b"not-json\n",
            b"\xff\n",
            encoded(event("C", unknown=True)),
            encoded(event("B")),
            encoded(event(["C"])),
            b"\n",
        ):
            with self.subTest(source=source):
                with self.assertRaises(PerfettoTraceError):
                    pack_event_jsonl(source)

    def test_phase_specific_fields_and_runtime_identity_are_exact(self) -> None:
        cases = (
            event("i"),
            event("i", s="g"),
            event("X"),
            event("X", dur=-1),
            event("C", s="t"),
            event("s"),
            event("f", id=1),
            event("f", id=1, bp="x"),
            event("C", pid=1),
            event("C", pid=False),
            event("C", tid=6),
            event("C", ts=0),
        )
        for value in cases:
            with self.subTest(value=value):
                with self.assertRaisesRegex(PerfettoTraceError, "ACPERFETTO-SCHEMA"):
                    pack_event_jsonl(encoded(value))

    def test_committed_keys_must_be_strictly_increasing(self) -> None:
        later = event("C", time=3, owner=1, local_index=0)
        earlier = event("C", time=2, owner=9, local_index=0)
        duplicate = event("C", time=3, owner=1, local_index=0)
        for source in (encoded(later, earlier), encoded(later, duplicate)):
            with self.subTest(source=source):
                with self.assertRaisesRegex(PerfettoTraceError, "ACPERFETTO-ORDER"):
                    pack_event_jsonl(source)

    def test_repeated_packing_is_byte_identical(self) -> None:
        source = encoded(event("i", s="t"), event("X", local_index=1, dur=4))
        self.assertEqual(pack_event_jsonl(source), pack_event_jsonl(source))


class PerfettoTraceCommandTest(unittest.TestCase):
    command = ROOT / "compiler/acir/tools" / "pack-perfetto-trace.py"

    def run_command(
        self, *arguments: object, cwd: Path | None = None
    ) -> subprocess.CompletedProcess[bytes]:
        return subprocess.run(
            [
                sys.executable,
                os.fspath(self.command),
                *(os.fspath(arg) for arg in arguments),
            ],
            cwd=cwd or ROOT,
            env={
                **os.environ,
                "PYTHONPATH": os.fspath(ROOT / "python/agentic-circuit/src"),
            },
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )

    def test_help_and_exact_argument_surface(self) -> None:
        result = self.run_command("--help")
        self.assertEqual(0, result.returncode)
        self.assertIn(b"INPUT OUTPUT", result.stdout)
        self.assertEqual(b"", result.stderr)
        self.assertEqual(2, self.run_command().returncode)

    def test_command_atomically_publishes_library_bytes(self) -> None:
        source = encoded(event("C"))
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            input_path = root / "events.jsonl"
            output_path = root / "perfetto.json"
            input_path.write_bytes(source)

            result = self.run_command(input_path, output_path, cwd=root)

            self.assertEqual(0, result.returncode, result.stderr.decode())
            self.assertEqual(b"", result.stdout)
            self.assertEqual(b"", result.stderr)
            self.assertEqual(pack_event_jsonl(source), output_path.read_bytes())
            self.assertEqual([], list(root.glob(".perfetto.json.*.tmp")))

    def test_failure_preserves_existing_output_and_cleans_stage(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            input_path = root / "events.jsonl"
            output_path = root / "perfetto.json"
            input_path.write_bytes(b"not-json\n")
            output_path.write_bytes(b"prior\n")

            result = self.run_command(input_path, output_path, cwd=root)

            self.assertEqual(2, result.returncode)
            self.assertEqual(b"", result.stdout)
            self.assertIn(b"ACPERFETTO-JSON", result.stderr)
            self.assertEqual(b"prior\n", output_path.read_bytes())
            self.assertEqual([], list(root.glob(".perfetto.json.*.tmp")))

    def test_alias_and_missing_parent_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            input_path = root / "events.jsonl"
            input_path.write_bytes(encoded(event("C")))
            alias = self.run_command(input_path, input_path, cwd=root)
            self.assertEqual(2, alias.returncode)
            self.assertIn(b"ACPERFETTO-IO", alias.stderr)
            self.assertEqual(encoded(event("C")), input_path.read_bytes())

            missing = self.run_command(input_path, root / "missing" / "out.json")
            self.assertEqual(2, missing.returncode)
            self.assertIn(b"ACPERFETTO-IO", missing.stderr)


if __name__ == "__main__":
    unittest.main()
