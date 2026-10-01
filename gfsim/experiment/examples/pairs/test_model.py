"""Complete pairs circuits: inputs, independent comparison and output assertions."""

from examples.testing import CircuitTestCase
from .model import pairs


class PairsTests(CircuitTestCase):
    def test_dual_input_atomic_processing_and_acceptance_anchored_events(self):
        left, right = list(range(12)), list(range(100, 112))
        c, history = self.run_circuit(pairs, left, right, period=9, ticks=150)
        self.assertEqual([entry[1] for entry in c.output.current],
                         [((i, a), (i, a + b)) for i, (a, b) in enumerate(zip(left, right))])
        alu = c.named["alu"]
        # Before RHS arrives, partial pop/push and future event must not escape.
        self.assertTrue(all(alu.rid not in history[t][0] for t in range(6)))
        self.assertTrue(all((due, mid) not in history[t][3]
                            for t in range(5) for due in range(t + 2, t + 5)
                            for mid in (alu.mid,)))
        retained_accepts = []
        for tick in range(1, len(history)):
            if alu.rid in history[tick][0]:
                self.assertIn((tick + 3, alu.mid), history[tick][3])
                if history[tick][2][alu.rid] == history[tick-1][2][alu.rid]:
                    retained_accepts.append(tick)
        self.assertTrue(retained_accepts)
