"""Complete lookup circuits: inputs, independent comparison and output assertions."""

from examples.testing import CircuitTestCase
from .model import lookup


class LookupTests(CircuitTestCase):
    def test_live_table_selection_during_output_backpressure(self):
        indices = ((3, 1), (8, 3), (14, 0), (18, 1), (21, 1))
        updates = ((0, ((10, 9), (24, 9))), (1, ((12, 5), (26, 7))))
        c, _ = self.run_circuit(lookup, [2, 3, 5, 7, 11, 13], indices, updates,
                                 ticks=100, period=11, depth=64)
        received = [entry[1][0] for entry in c.output.current]
        self.assertEqual([seq for seq, _ in received], list(range(6)))
        # Once configuration settles, the final coefficient applies to remaining requests.
        self.assertEqual(received[-2:], [(4, 77), (5, 91)])
        self.assertEqual(len(c.named["table"][0].readers), len(c.sim.modules))
        alternate, _ = self.run_circuit(lookup, [2, 3, 5, 7, 11, 13], indices, updates,
                                         ticks=100, period=11, depth=64, reverse=True)
        self.assertEqual(c.output.current, alternate.output.current)
