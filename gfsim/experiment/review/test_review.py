"""Review instrumentation must preserve complete model executions and failures."""

from dataclasses import asdict
from pathlib import Path
from tempfile import TemporaryDirectory
import json
import unittest

from engine import CapacityCycle
from test_engine import cyclic_circuit
from examples.ripes5.model import CPU
from examples.ripes5.tests.programs import suite
from review import ReviewTrace


class ReviewTests(unittest.TestCase):
    def test_trace_preserves_schedule_and_exports_complete_ripes_program(self):
        case = next(c for c in suite() if c['name'] == 'array_sum')
        plain, traced = CPU(case), CPU(case)
        trace = ReviewTrace(traced.sim, title='</script><unsafe>')
        reconstructed = list(trace.data['initial'])
        for _ in range(case['max_cycles']):
            row = traced.step()
            self.assertEqual(plain.step(), row)
            self.assertEqual(plain.sim.snapshot(), traced.sim.snapshot())
            self.assertEqual(asdict(plain.sim.stats), asdict(traced.sim.stats))
            self.assertEqual(plain.sim.events, traced.sim.events)
            frame = trace.data['frames'][-1]
            for qid, state in frame['changes']:
                reconstructed[qid] = state
            self.assertEqual(reconstructed, [trace.queue_state(q) for q in traced.sim.queues])
            if row['retire'] and row['retire']['pc'] == case['end_pc']:
                break
        else:
            self.fail('end marker missing')
        self.assertEqual(row['data'][:9], [1, 2, 3, 4, 5, 6, 7, 8, 36])
        frames = trace.data['frames']
        self.assertTrue(any(f['invalidations'] for f in frames))
        self.assertTrue(any(c['dirty'] for f in frames for c in f['calls']))
        self.assertTrue(any(f['signal_evaluations'] for f in frames))
        with TemporaryDirectory() as directory:
            path = Path(directory) / 'review.html'
            trace.write_html(path)
            html = path.read_text()
            self.assertNotIn('</script><unsafe>', html)
            payload = html.split('<script id="trace" type="application/json">')[1].split('</script>')[0]
            self.assertEqual(json.loads(payload)['frames'], json.loads(json.dumps(frames)))

    def test_capacity_cycle_records_actual_edges_and_failed_tick(self):
        circuit = cyclic_circuit()
        trace = ReviewTrace(circuit.sim)
        with self.assertRaises(CapacityCycle):
            circuit.sim.step()
        frame = trace.data['frames'][0]
        self.assertEqual(frame['tick'], 0)
        self.assertIn('CapacityCycle', frame['error'])
        self.assertEqual({tuple(edge[:2]) for edge in frame['edges']}, {(1, 2), (2, 1)})
        self.assertFalse(frame['changes'])
        with self.assertRaises(RuntimeError):
            circuit.sim.step()
        self.assertEqual(len(trace.data['frames']), 1)


if __name__ == '__main__':
    unittest.main()
