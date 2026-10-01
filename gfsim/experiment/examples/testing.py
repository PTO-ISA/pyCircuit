"""Shared end-to-end comparison; scenario inputs and assertions live locally."""

import unittest


class CircuitTestCase(unittest.TestCase):
    def run_circuit(self, builder, *args, ticks=200, **kwargs):
        actual = builder(*args, **kwargs)
        oracle = builder(*args, **kwargs, reference=True)
        history = []
        array_ids = [id(q.readers) for q in actual.sim.queues]
        slot_ids = [[id(slot) for slot in q.slots] for q in actual.sim.queues]
        task_ids = (id(actual.sim.module_tasks.ids), id(actual.sim.rule_tasks.ids))
        for tick in range(ticks):
            with self.subTest(circuit=builder.__name__, tick=tick, options=kwargs):
                accepted, expected = actual.sim.step(), oracle.sim.step()
                self.assertEqual(set(accepted), set(expected))
                self.assertEqual(len(accepted), len(set(accepted)))
                self.assertEqual(actual.sim.snapshot(), oracle.sim.snapshot())
                self.assertEqual([q.state_version for q in actual.sim.queues],
                                 [q.state_version for q in oracle.sim.queues])
                self.assertEqual(sorted(actual.sim.events), sorted(oracle.sim.events))
                self.assertEqual(actual.sim.module_calls, oracle.sim.module_calls)
                for mid, module in enumerate(actual.sim.modules):
                    live = {q.qid for q in actual.sim.queues
                            if q.readers[mid] and q.readers[mid] == module.read_gen}
                    self.assertEqual(live, oracle.sim.reads[mid])
                for q in actual.sim.queues:
                    self.assertLessEqual(q.size(), q.capacity)
                    self.assertEqual(len(q.readers), len(actual.sim.modules))
                    self.assertTrue(all(slot.status in (0, 1) for slot in q.slots))
                history.append((accepted, tuple(actual.sim.module_calls),
                                tuple(actual.sim.rule_calls), tuple(sorted(actual.sim.events))))
        self.assertEqual(array_ids, [id(q.readers) for q in actual.sim.queues])
        self.assertEqual(slot_ids, [[id(slot) for slot in q.slots] for q in actual.sim.queues])
        self.assertEqual(task_ids, (id(actual.sim.module_tasks.ids), id(actual.sim.rule_tasks.ids)))
        return actual, history
