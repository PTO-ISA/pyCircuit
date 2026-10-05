"""Check that the streaming gate rejects corrupt, truncated and hung producers."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from ..tools.benchmark_support import stream_verify
from .verify import v


class StreamTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.case = v.suite()[0]

    def producer(self, name, mode='good'):
        script = self.root / name
        script.write_text('#!' + sys.executable + '\n' + f'''
import json, sys, time
mode = {mode!r}
if mode == 'hang':
    time.sleep(30)
for cycle in range(6 if mode == 'short' else 10):
    row = dict(cycle=cycle, retired=max(0, cycle-4),
               retire={{'pc': {self.case['end_pc']}}} if cycle == 9 else None)
    if mode == 'corrupt' and cycle == 4:
        row['retired'] = 999
    if len(sys.argv) > 1:
        row['raw'] = dict(value=42)
        if mode == 'observe' and '--observe' in sys.argv and cycle == 4:
            row['raw']['value'] = 43
    print(json.dumps(row))
''')
        script.chmod(0o755)
        return script

    def run_gate(self, mode='good', native_mode='good', timeout=5):
        return stream_verify(self.case,
                             dict(generated=self.producer('generated', mode),
                                  handwritten=self.producer('handwritten'),
                                  native=self.producer('native', native_mode)),
                             self.root / 'evidence', 2, timeout=timeout)

    def test_success_digest_and_boundaries(self):
        result = self.run_gate()
        self.assertEqual((result['rows'], result['boundary']['cycle'], result['cycles']), (10, 2, 9))
        self.assertEqual(len(set(result['trace_sha256'].values())), 1)
        self.assertFalse(list((self.root / 'evidence').glob('*.jsonl')))

    def test_first_difference_neighborhood(self):
        with self.assertRaisesRegex(AssertionError, 'trace mismatch'):
            self.run_gate('corrupt')
        mismatch = json.loads((self.root / 'evidence/first-mismatch.json').read_text())
        self.assertEqual(mismatch['first_row'], 4)
        self.assertEqual([row['native']['cycle'] for row in mismatch['context']], list(range(1, 8)))

    def test_truncated_producer(self):
        with self.assertRaisesRegex(AssertionError, 'trace mismatch'):
            self.run_gate('short')
        mismatch = json.loads((self.root / 'evidence/first-mismatch.json').read_text())
        self.assertEqual(mismatch['first_row'], 6)

    def test_raw_observation_difference(self):
        with self.assertRaisesRegex(AssertionError, 'trace mismatch'):
            self.run_gate(native_mode='observe')
        mismatch = json.loads((self.root / 'evidence/first-mismatch.json').read_text())
        self.assertIn('native_raw', mismatch['differences'])

    def test_watchdog_reaps_hung_producer(self):
        with self.assertRaises((AssertionError, RuntimeError)):
            self.run_gate(mode='hang', timeout=0.5)


if __name__ == '__main__':
    unittest.main()
