"""Complete feedback circuits: inputs, independent comparison and output assertions."""

from engine import CapacityCycle
from examples.testing import CircuitTestCase
from .model import feedback


class FeedbackTests(CircuitTestCase):
    def test_feedback_with_space_and_self_recycling(self):
        for hops in (0, 1, 9):
            for self_loop in (False, True):
                c, _ = self.run_circuit(feedback, ((7, hops),), self_loop=self_loop, ticks=30)
                self.assertEqual(c.output.current, ((hops + 2, ((7, 0),)),))
                self.assertTrue(all(q.empty() for q in c.named["ring"]))
        # Both queues full, but exiting branch creates space and breaks the static loop.
        c, _ = self.run_circuit(feedback, ((0, 0), (1, 3)), ticks=20)
        self.assertEqual({entry[1][0] for entry in c.output.current}, {(0, 0), (1, 0)})

    def test_real_dynamic_capacity_cycle_terminates_feedback_network(self):
        for reference in (False, True):
            for reverse in (False, True):
                c = feedback(((0, 2), (1, 2)), reference=reference, reverse=reverse)
                with self.assertRaises(CapacityCycle):
                    c.sim.step()
                self.assertEqual(c.output.current, ())
                if not reference:
                    with self.assertRaisesRegex(RuntimeError, 'continuation is forbidden'):
                        c.sim.step()
