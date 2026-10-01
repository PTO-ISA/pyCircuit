"""Complete pipeline circuits: inputs, independent comparison and output assertions."""

import random

from examples.testing import CircuitTestCase
from .model import pipeline


class PipelineTests(CircuitTestCase):
    def test_elastic_pipeline_output_latency_throughput_and_drain(self):
        values = [3, 9, 0xffffffff, 17, 88, 5]
        c, history = self.run_circuit(pipeline, values, length=4, period=1, ticks=50)
        self.assertEqual(c.output.current,
                         tuple((i + 5, ((i, (v + 4) & 0xffffffff),))
                               for i, v in enumerate(values)))
        self.assertEqual(history[-1][1:3], history[-10][1:3])  # Drained circuit stays asleep.
        self.assertFalse(c.sim.events)

    def test_multientry_fifos_wrap_and_drain_under_backpressure(self):
        values = list(range(80))
        c, _ = self.run_circuit(pipeline, values, length=3, period=5,
                                 capacity=4, ticks=450)
        self.assertEqual([entry[1][0] for entry in c.output.current],
                         [(i, value + 3) for i, value in enumerate(values)])
        self.assertTrue(all(q.empty() for q in c.named["links"]))

    def test_backpressure_releases_retained_computation_without_work(self):
        values = list(range(18))
        c, history = self.run_circuit(pipeline, values, length=5, period=11, ticks=230)
        self.assertEqual([entry[1][0] for entry in c.output.current],
                         [(i, v + 5) for i, v in enumerate(values)])
        direct = 0
        for tick in range(1, len(history)):
            for m in c.named["computes"]:
                if m.rid in history[tick][0] and history[tick][1][m.mid] == history[tick-1][1][m.mid]:
                    direct += 1
                    self.assertEqual(history[tick][2][m.rid], history[tick-1][2][m.rid])
        self.assertGreater(direct, 20)

    def test_control_updates_reuse_heavy_pipeline_work(self):
        args = dict(length=2, period=13, iterations=20, control=tuple(range(80)))
        c, _ = self.run_circuit(pipeline, list(range(8)), ticks=130, **args)
        uncached = pipeline(list(range(8)), cache=False, **args)
        for _ in range(130):
            uncached.sim.step()
        self.assertEqual(c.output.current, uncached.output.current)
        self.assertEqual(c.sim.module_calls, uncached.sim.module_calls)
        self.assertGreater(c.sim.stats.cache_hits, 20)
        self.assertLess(c.sim.stats.rule_work, uncached.sim.stats.rule_work)
        expected = []
        for seq, value in enumerate(range(8)):
            for _ in range(2):
                for _ in range(20):
                    value = (value * 1664525 + 1013904223) & 0xffffffff
                value = (value + 1) & 0xffffffff
            expected.append((seq, value))
        self.assertEqual([entry[1][0] for entry in c.output.current], expected)

    def test_long_prefilled_pipeline_uses_bounded_explicit_stack(self):
        length = 1100
        c = pipeline([7], length=length, period=1, prefill=True)
        for _ in range(length + 5):
            c.sim.step()
        expected = [(-i - 1, 100 + length) for i in range(length, -1, -1)] + [(0, 7 + length)]
        self.assertEqual([entry[1][0] for entry in c.output.current], expected)
        self.assertGreater(c.sim.stats.max_stack, 1000)

    def test_fixed_seed_random_pipeline(self):
        for seed in range(6):
            rng = random.Random(seed)
            with self.subTest(seed=seed):
                values = [rng.randrange(65536) for _ in range(15)]
                length, period = rng.randrange(1, 7), rng.randrange(1, 10)
                c, _ = self.run_circuit(pipeline, values, length=length, period=period,
                                         ticks=180, reverse=bool(seed % 2))
                self.assertEqual([x[1][0] for x in c.output.current],
                                 [(i, v + length) for i, v in enumerate(values)])
