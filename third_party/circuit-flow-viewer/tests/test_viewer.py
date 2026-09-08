from __future__ import annotations

import io
import json
import struct
import tempfile
import unittest
from pathlib import Path

from circuit_flow_viewer.cli import render
from circuit_flow_viewer.reader import ReplayError, _Decoder, read_replay, records

FIXTURE = Path(__file__).parent / "fixtures/transaction.pyctrace"


class ViewerTest(unittest.TestCase):
    def test_real_atomic_transaction_fixture(self):
        replay = read_replay(FIXTURE)
        self.assertTrue(replay.complete, replay.diagnostic)
        self.assertEqual(len(replay.commits), 4)
        self.assertEqual(replay.commits[1]["changes"], {})
        final = replay.final["3"]["entries"]
        self.assertEqual([v["bits"] for v in final], [str(2**64-1)]*2)

    def test_standalone_output_is_deterministic_and_embedded(self):
        with tempfile.TemporaryDirectory() as temporary:
            a = render(FIXTURE, Path(temporary) / "a.html")
            b = render(FIXTURE, Path(temporary) / "b.html")
            self.assertEqual(a.read_bytes(), b.read_bytes())
            html = a.read_text()
            self.assertNotIn('src="http', html)
            self.assertIn(str(2**64-1), html)
            self.assertNotIn('/*RECORDING*/null', html)
            payload = html.split('const recording = ',1)[1].split(';\nconst $',1)[0]
            self.assertTrue(json.loads(payload)["complete"])

    def test_truncated_final_record_shows_complete_barriers_only(self):
        with tempfile.TemporaryDirectory() as temporary:
            trace = Path(temporary) / "partial.pyctrace"
            trace.write_bytes(FIXTURE.read_bytes()[:-5])
            replay = read_replay(trace)
            self.assertFalse(replay.complete)
            self.assertEqual(len(replay.commits),4)
            self.assertEqual(replay.final,read_replay(FIXTURE).final)
            page = render(trace,Path(temporary)/'partial.html')
            self.assertIn('"complete":false',page.read_text())

    def test_invalid_header_and_integer_encoding(self):
        with self.assertRaises(ReplayError):
            list(records(io.BytesIO(b"garbage")))
        with self.assertRaises(ReplayError):
            _Decoder(b'\x02'+struct.pack('<IBQ',65,0,0)).value()


if __name__ == '__main__':
    unittest.main()
