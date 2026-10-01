"""Complete memory circuits: inputs, independent comparison and output assertions."""

import random

from examples.testing import CircuitTestCase
from .model import memory


class MemoryTests(CircuitTestCase):
    def test_banked_memory_requests_responses_and_final_cells(self):
        rng = random.Random(41)
        requests = tuple((seq, rng.randrange(16), seq % 3 != 0, rng.randrange(65536))
                         for seq in range(60))
        c, _ = self.run_circuit(memory, requests, depth=8, latency=4, period=7, ticks=700)
        cells, counts, expected = [0] * 16, [0] * 16, {}
        for seq, address, write, value in requests:
            if write:
                cells[address], counts[address] = value, counts[address] + 1
            expected[seq] = cells[address]
        received = [entry[1][0] for entry in c.output.current]
        self.assertEqual(dict(received), expected)
        self.assertEqual(len(received), len(expected))
        for bank, table in enumerate(c.named["cells"]):
            for index, queue in enumerate(table):
                address = index * 2 + bank
                self.assertEqual(queue.current, (((counts[address] > 0, counts[address]), cells[address]),))
        self.assertLess(len(c.sim.modules) * 3, len(c.sim.queues) * 2)

    def test_fixed_seed_random_memory(self):
        for seed in range(6):
            rng = random.Random(seed)
            # Preserve the original workload's RNG sequence from the combined test.
            for _ in range(15):
                rng.randrange(65536)
            rng.randrange(1, 7)
            period = rng.randrange(1, 10)
            with self.subTest(seed=seed):
                requests = tuple((i, rng.randrange(12), bool(rng.randrange(2)), rng.randrange(256))
                                 for i in range(20))
                c, _ = self.run_circuit(memory, requests, banks=3, depth=4,
                                         latency=1 + seed, period=period, ticks=300,
                                         reverse=bool(seed % 2))
                state, expected = [0] * 12, {}
                for seq, address, write, data in requests:
                    if write:
                        state[address] = data
                    expected[seq] = state[address]
                self.assertEqual(dict(x[1][0] for x in c.output.current), expected)
