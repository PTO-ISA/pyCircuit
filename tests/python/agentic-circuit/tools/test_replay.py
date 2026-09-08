from __future__ import annotations

import io
import os
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path

from replay_format import ReplayError, _Decoder, integer, read_replay, records


ROOT = Path(__file__).resolve().parents[4]


class ReplayTest(unittest.TestCase):
    def test_exact_integer_and_float_bits(self):
        value = _Decoder(b"\x02" + struct.pack("<IBQ", 64, 0, 2**64 - 1)).value()
        self.assertEqual(integer(value), 2**64 - 1)
        self.assertEqual(value["width"], 64)
        self.assertEqual(
            _Decoder(b"\x06" + bytes.fromhex("010000000000f87f")).value(),
            {"float64_bits": "010000000000f87f"},
        )

    def test_invalid_binary_inputs_are_rejected(self):
        for data in (b"", b"PYC6TRC3", b"PYC6TRC3" + struct.pack("<II", 4, 1)):
            with self.subTest(data=data), self.assertRaises(ReplayError):
                list(records(io.BytesIO(data)))
        for data in (b"\x01\x02", b"\x07", b"\x03\x05\x00\x00\x00x"):
            with self.subTest(data=data), self.assertRaises(ReplayError):
                _Decoder(data).value()

    def test_native_atomic_journal_and_truncated_tail(self):
        executable = ROOT / ".pycircuit_out/local-clang22/build/bin/GfsimTests"
        if not executable.is_file():
            self.skipTest("current-checkout GfsimTests is not built")
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary)
            completed = subprocess.run(
                [str(executable), "--gtest_filter=ReplayTest.FullMultiOwnerJournalPreservesBackpressureAndRetry"],
                env={**os.environ, "PYC_REPLAY_TEST_OUT": temporary},
                capture_output=True,
                text=True,
            )
            self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)
            path = output / "transaction.pyctrace"
            replay = read_replay(path)
            self.assertTrue(replay.complete, replay.diagnostic)
            self.assertEqual(len(replay.commits), 4)
            # Independent runtime projection, written without ReplayValue/recorder snapshots.
            native = (output / "transaction.native.tsv").read_text().splitlines()
            state = dict(replay.initial)
            for i, commit in enumerate(replay.commits):
                state.update({k: c["after"] for k, c in commit["changes"].items()})
                values = [i, integer(state["2"]["entries"][0]),
                          *map(integer, state["3"]["entries"]),
                          len(state["0"]["values"]), len(state["1"]["values"]),
                          *map(integer, state["0"]["values"]), *map(integer, state["1"]["values"])]
                self.assertEqual(list(map(int, native[i].split())), values)

            self.assertEqual(replay.commits[1]["changes"], {})
            self.assertEqual(set(replay.commits[3]["changes"]), {"0", "1", "2", "3"})
            self.assertEqual(integer(replay.final["2"]["entries"][0]), 1)
            self.assertEqual(
                [integer(v) for v in replay.final["3"]["entries"]],
                [2**64 - 1, 2**64 - 1],
            )
            self.assertFalse(any(integer(e["batch"]) == 1 for e in replay.events))
            committed = [e for e in replay.events if integer(e["batch"]) == 3]
            self.assertEqual({e["operation"] for e in committed}, {"3:0"})
            self.assertEqual({e["action"] for e in committed}, {"enqueue", "dequeue", "state_read", "state_write"})
            self.assertTrue(all("after" in e for e in committed if e["action"] == "state_write"))
            raw = path.read_bytes()
            # An interrupted final record preserves all closed barriers.
            path.write_bytes(raw[:-5])
            partial = read_replay(path)
            self.assertFalse(partial.complete)
            self.assertEqual(partial.final, replay.final)
            self.assertEqual(len(partial.commits), 4)
            # A trace ending inside the fourth barrier never publishes its changes.
            offset = 16
            while offset < len(raw):
                length, kind = struct.unpack_from("<II", raw, offset)
                record = _Decoder(raw[offset + 8 : offset + 8 + length]).value()
                if record["kind"] == "commit" and integer(record["batch"]) == 3:
                    break
                offset += 8 + length
            path.write_bytes(raw[: offset + 12])
            partial = read_replay(path)
            self.assertFalse(partial.complete)
            self.assertEqual(len(partial.commits), 3)
            self.assertEqual(integer(partial.final["2"]["entries"][0]), 0)

    def test_equal_tokens_keep_identity_through_delay_and_simultaneous_push_pop(self):
        executable = ROOT / ".pycircuit_out/local-clang22/build/bin/GfsimTests"
        if not executable.is_file():
            self.skipTest("current-checkout GfsimTests is not built")
        with tempfile.TemporaryDirectory() as temporary:
            result = subprocess.run([str(executable), "--gtest_filter=ReplayTest.EqualTokensAndDelayedReadinessHaveDistinctIdentity"],
                                    env={**os.environ, "PYC_REPLAY_TEST_OUT": temporary}, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            replay = read_replay(Path(temporary) / "latency.pyctrace")
            self.assertTrue(replay.complete, replay.diagnostic)
            enqueued = [integer(e["token"]) for e in replay.events if e["action"] == "enqueue"]
            ready = [integer(e["token"]) for e in replay.events if e["action"] == "ready"]
            popped = [integer(e["token"]) for e in replay.events if e["action"] == "dequeue"]
            self.assertEqual(len(set(enqueued)), 3)
            self.assertEqual(ready, enqueued)
            self.assertEqual(popped, enqueued[:1])
            self.assertEqual(list(map(integer, replay.final["0"]["tokens"])), enqueued[1:])


if __name__ == "__main__":
    unittest.main()
