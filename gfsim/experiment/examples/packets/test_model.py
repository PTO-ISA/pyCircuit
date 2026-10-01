"""Complete packets circuits: inputs, independent comparison and output assertions."""

from examples.testing import CircuitTestCase
from .model import packets


class PacketsTests(CircuitTestCase):
    def test_live_packet_configuration_and_branch_cancellation(self):
        lanes = (list(range(1, 21)), list(range(21, 41)))
        changes = ((3, (0, 10, 1)), (9, (1, 20, 2)), (15, (0, 30, 3)),
                   (19, (0, 30, 4)), (25, (2, 3, 5)), (27, (2, 3, 5)))
        c, _ = self.run_circuit(packets, lanes, changes, period=7, ticks=350)
        received = [entry[1][0] for entry in c.output.current]
        expected_ids = {lane * 10000 + i for lane in range(2)
                        for i, value in enumerate(lanes[lane]) if value % 7}
        self.assertEqual({seq for seq, _ in received}, expected_ids)
        self.assertEqual(len(received), len(expected_ids))
        for seq, value in received:
            original = lanes[seq // 10000][seq % 10000]
            self.assertIn(value - original, (0, 10, 20, 30, 3))
        for lane in range(2):
            sequence = [seq % 10000 for seq, _ in received if seq // 10000 == lane]
            self.assertEqual(sequence, sorted(sequence))
        reversed_order, _ = self.run_circuit(packets, lanes, changes, period=7,
                                             ticks=350, reverse=True)
        self.assertEqual(c.output.current, reversed_order.output.current)
