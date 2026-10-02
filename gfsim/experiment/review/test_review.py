"""Review instrumentation must preserve complete model executions and failures."""

from dataclasses import asdict
from pathlib import Path
from tempfile import TemporaryDirectory
import json
import unittest

from engine import CapacityCycle
from test_engine import cyclic_circuit
from examples.riscv.model import build_cpu
from examples.riscv.reference import Interpreter
from review import ReviewTrace


class ReviewTests(unittest.TestCase):
    def test_trace_preserves_schedule_and_explains_dependencies(self):
        source = 'addi x1,x0,7\nsw x1,0(x0)\nlw x2,0(x0)\nadd x3,x2,x1\nhalt'
        plain, traced = [build_cpu(source, memory_latency=5, data_words=8) for _ in range(2)]
        trace = ReviewTrace(traced.sim, title='</script><unsafe>')
        reconstructed = list(trace.data['initial'])
        for _ in range(60):
            self.assertEqual(plain.step(), traced.step())
            self.assertEqual(plain.sim.snapshot(), traced.sim.snapshot())
            self.assertEqual(asdict(plain.sim.stats), asdict(traced.sim.stats))
            self.assertEqual(plain.sim.events, traced.sim.events)
            frame = trace.data['frames'][-1]
            for qid, state in frame['changes']:
                reconstructed[qid] = state
            self.assertEqual(reconstructed, [trace.queue_state(q) for q in traced.sim.queues])
        frames = trace.data['frames']
        self.assertTrue(any(f['invalidations'] for f in frames))
        self.assertTrue(any(c['dirty'] for f in frames for c in f['calls']))
        self.assertTrue(all(not r['dirty'] for f in frames for r in f['rules'][1:]))
        with TemporaryDirectory() as directory:
            path = Path(directory) / 'review.html'
            trace.write_html(path)
            html = path.read_text()
            self.assertNotIn('</script><unsafe>', html)
            payload = html.split('<script id="trace" type="application/json">')[1].split('</script>')[0]
            self.assertEqual(json.loads(payload)['frames'], json.loads(json.dumps(frames)))

    def test_cpu_trace_reaches_halt_and_matches_independent_interpreter(self):
        cpu = build_cpu('addi x1,x0,7\nsw x1,0(x0)\nlw x2,0(x0)\nadd x3,x2,x1\nhalt',
                        memory_latency=5, data_words=8)
        trace = ReviewTrace(cpu.sim)
        oracle = Interpreter(cpu.words, data_words=8)
        for _ in range(100):
            retired = cpu.step()
            if retired is not None:
                self.assertEqual(retired, oracle.step())
                self.assertEqual(cpu.register_values(), tuple(oracle.registers))
            if cpu.halted:
                break
        self.assertTrue(cpu.halted)
        self.assertEqual(cpu.memory_values(), tuple(oracle.memory))
        self.assertEqual(cpu.register_values()[3], 14)
        self.assertTrue(any(r['events'] for f in trace.data['frames'] for r in f['rules'][1:]))
        self.assertTrue(any('pc' in json.dumps(v) for f in trace.data['frames'] for _, v in f['changes']))

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
