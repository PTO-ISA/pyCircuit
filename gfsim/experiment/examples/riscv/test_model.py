"""All CPU scenarios enter through assembly and run to retirement or failure."""

from pathlib import Path
from itertools import product
import random
import unittest

from .model import build_cpu
from .records import ModelError, Fetched, Decoded, Executed, Completed, Busy, Retired
from .reference import Interpreter


class PipelineCPUTests(unittest.TestCase):
    def run_program(self, source, *, latency=1, cache=True, reverse=False, max_cycles=5000):
        cpu = build_cpu(source, memory_latency=latency, data_words=64, cache=cache, reverse=reverse)
        oracle = Interpreter(cpu.words, data_words=64)
        storage = [id(q.readers) for q in cpu.sim.queues]
        expected_types = (Fetched, Decoded, Executed, Completed)
        for _ in range(max_cycles):
            actual = cpu.step(trace=True)
            if actual is not None:
                self.assertEqual(actual, oracle.step(), f'tick {cpu.sim.tick - 1}')
                self.assertEqual(cpu.register_values(), tuple(oracle.registers))
            for q, expected_type in zip(cpu.links, expected_types):
                if not q.empty():
                    self.assertIs(type(q.peek()), expected_type)
            self.assertIs(type(cpu.busy.peek()), Busy)
            self.assertIs(type(cpu.retirement.peek()), Retired)
            self.assertIs(type(cpu.retirement.peek().instruction), Completed)
            if cpu.halted:
                break
        else:
            self.fail(f'CPU did not HALT after {max_cycles} cycles')
        self.assertTrue(oracle.halted)
        self.assertEqual(cpu.memory_values(), tuple(oracle.memory))
        self.assertEqual(storage, [id(q.readers) for q in cpu.sim.queues])
        self.assertTrue(all(q.empty() for q in cpu.links))
        self.assertFalse(cpu.busy.peek().valid)
        return cpu

    def test_real_encoding_and_five_stage_throughput(self):
        source = 'addi x1,x0,1\naddi x2,x0,2\nadd x3,x1,x2\nhalt'
        cpu = self.run_program(source)
        self.assertEqual(cpu.words, (0x00100093, 0x00200113, 0x002081b3, 0x00100073))
        self.assertEqual(cpu.retire_ticks, [4, 5, 6, 7])
        self.assertEqual(cpu.register_values()[3], 3)
        self.assertEqual(len(cpu.stages), 5)
        # Once drained, subscriptions cause at most final no-effect Work, then quiet.
        for _ in range(4):
            cpu.step()
        calls = tuple(cpu.sim.module_calls)
        for _ in range(4):
            cpu.step()
        self.assertEqual(tuple(cpu.sim.module_calls), calls)
        self.assertFalse(cpu.sim.events)

    def test_alu_signed_values_wrap_and_x0(self):
        cpu = self.run_program('''
            addi x1,x0,-1
            addi x2,x1,1
            lui x3,0x80000
            slt x4,x3,x1
            slt x5,x1,x0
            sub x6,x0,x1
            and x7,x1,x3
            or x8,x3,x6
            xor x9,x1,x3
            addi x0,x0,99
            add x10,x0,x6
            halt
        ''')
        self.assertEqual(cpu.register_values()[:11],
                         (0, 0xffffffff, 0, 0x80000000, 1, 1, 1,
                          0x80000000, 0x80000001, 0x7fffffff, 1))

    def test_forwarding_priority_and_wb_to_id_snapshot(self):
        cpu = self.run_program('''
            addi x1,x0,1
            addi x1,x1,2
            add x2,x1,x1
            nop
            nop
            add x3,x2,x1
            halt
        ''')
        self.assertEqual(cpu.register_values()[1:4], (3, 6, 9))
        self.assertEqual(cpu.retire_ticks, list(range(4, 11)))
        # Separate distance-three dependency places producer in WB while consumer
        # reads in ID. Observe the actual named pipeline record after that tick.
        source = 'addi x1,x0,17\nnop\nnop\nadd x2,x1,x1\nhalt'
        checked = self.run_program(source)
        cpu = build_cpu(source)
        for _ in range(5):
            cpu.step()
        decoded = cpu.links[1].peek()
        self.assertEqual((decoded.pc, decoded.left, decoded.right), (12, 17, 17))
        cpu.run()
        self.assertEqual(cpu.retired, checked.retired)

    def test_load_use_one_bubble_and_store_forwarding(self):
        cpu = self.run_program('''
            addi x1,x0,7
            sw x1,0(x0)
            lw x2,0(x0)
            add x3,x2,x1
            sw x3,4(x0)
            halt
        ''')
        self.assertEqual(cpu.retire_ticks, [4, 5, 6, 8, 9, 10])
        self.assertEqual(cpu.memory_values()[:2], (7, 14))
        self.assertEqual(cpu.trace[4][1][:2], (12, 8))
        self.assertEqual(cpu.trace[5][1][:2], (12, None))

    def test_taken_branch_kills_wrong_path_and_preserves_older_store(self):
        for wrong in ('addi x9,x0,99', 'sw x1,4(x0)', 'halt', '.word 0'):
            with self.subTest(wrong=wrong):
                cpu = self.run_program(f'''
                    addi x1,x0,7
                    sw x1,0(x0)
                    beq x1,x1,target
                    {wrong}
                    {wrong}
                target:
                    addi x2,x0,9
                    halt
                ''')
                self.assertEqual([p.pc for p in cpu.retired], [0, 4, 8, 20, 24])
                self.assertEqual(cpu.retire_ticks, [4, 5, 6, 9, 10])
                self.assertEqual(cpu.memory_values()[:2], (7, 0))
                self.assertEqual(cpu.register_values()[9], 0)

    def test_jal_jalr_calls_and_backward_loop(self):
        cpu = self.run_program('''
            addi x2,x0,0
            addi x3,x0,4
        loop:
            jal x1,increment
            addi x3,x3,-1
            bne x3,x0,loop
            halt
        increment:
            addi x2,x2,3
            jalr x0,0(x1)
        ''')
        self.assertEqual(cpu.register_values()[2], 12)
        self.assertEqual(sum(p.pc == 24 for p in cpu.retired), 4)

    def test_program_sum_with_latency_and_instruction_generated_data(self):
        source = (Path(__file__).parent / 'programs' / 'sum.s').read_text()
        for latency in (1, 3, 5):
            with self.subTest(latency=latency):
                cpu = self.run_program(source, latency=latency)
                self.assertEqual(cpu.memory_values()[:11], (*range(1, 11), 55))
                self.assertEqual(cpu.register_values()[4], 55)
                self.assertEqual(len(cpu.retired), 98)

    def test_memory_event_latency_and_busy_capacity(self):
        source = 'addi x1,x0,7\nsw x1,0(x0)\nlw x2,0(x0)\nadd x3,x2,x1\nhalt'
        for latency in (3, 5):
            with self.subTest(latency=latency):
                cpu = self.run_program(source, latency=latency)
                self.assertEqual(cpu.retire_ticks, [4, 4 + latency, 4 + 2 * latency,
                                                    6 + 2 * latency, 7 + 2 * latency])
                # MEM start at tick 4; finish at start+latency-1, WB one tick later.
                busy_ticks = [t for t, _, busy, _ in cpu.trace if busy == 4]
                self.assertEqual(busy_ticks, list(range(5, 4 + latency)))
                # EX/MEM is an additional waiting slot while the request is busy.
                self.assertTrue(any(busy == 4 and pcs[2] == 8
                                    for _, pcs, busy, _ in cpu.trace))

    def test_backpressure_forwarding_and_branch_during_memory_wait(self):
        source = '''
            addi x1,x0,41
            sw x0,0(x0)
            addi x2,x0,1
            add x3,x1,x2
            beq x3,x1,wrong
            addi x4,x3,1
            beq x4,x4,done
        wrong:
            sw x1,4(x0)
            halt
        done:
            sw x4,8(x0)
            halt
        '''
        baseline = self.run_program(source, latency=5)
        self.assertEqual(baseline.register_values()[3:5], (42, 43))
        self.assertEqual(baseline.memory_values()[1:3], (0, 43))
        for cache, reverse in ((False, False), (True, True), (False, True)):
            other = self.run_program(source, latency=5, cache=cache, reverse=reverse)
            self.assertEqual(other.retired, baseline.retired)
            self.assertEqual(other.retire_ticks, baseline.retire_ticks)
        # Retained candidates can commit without re-entering their stage Work.
        direct = []
        for tick, _, _, accepted in baseline.trace[1:]:
            for rid in accepted:
                mid = baseline.stages[rid - 1].mid
                if baseline.work_history[tick][0][mid] == baseline.work_history[tick - 1][0][mid]:
                    direct.append(rid)
                    self.assertEqual(baseline.work_history[tick][1][rid],
                                     baseline.work_history[tick - 1][1][rid])
        self.assertTrue(direct)

    def test_newest_pending_load_overrides_older_matching_value(self):
        cpu = self.run_program('''
            addi x1,x0,9
            sw x1,0(x0)
            addi x1,x0,77
            lw x1,0(x0)
            add x2,x1,x1
            halt
        ''', latency=5)
        self.assertEqual(cpu.register_values()[1:3], (9, 18))

    def test_dependency_windows_across_long_memory_stalls(self):
        # Exhaust the placement of writes/loads/stores/nops in a short window.
        # The final consumer uses both a recently overwritten register and an
        # older one whose forwarding source may have left the pipeline.
        choices = ('addi x1,x1,1', 'lw x1,0(x0)', 'sw x2,4(x0)', 'nop')
        for window in product(choices, repeat=3):
            with self.subTest(window=window):
                source = '\n'.join(('addi x2,x0,19', 'sw x2,0(x0)',
                                     'addi x1,x0,3', *window,
                                     'add x3,x1,x2', 'sw x3,8(x0)', 'halt'))
                self.run_program(source, latency=5)

    def test_halt_and_invalid_instruction_on_unreachable_path(self):
        cpu = self.run_program('halt\n.word 0\nsw x1,0(x0)')
        self.assertEqual(cpu.retire_ticks, [4])
        self.assertEqual(cpu.memory_values(), (0,) * 64)

    def test_reachable_errors_terminate_simulation(self):
        for source in ('.word 0\nhalt', 'lw x1,2(x0)\nhalt',
                       'sw x0,256(x0)\nhalt', 'jal x0,2\nhalt',
                       'addi x1,x0,1'):
            with self.subTest(source=source):
                cpu = build_cpu(source, data_words=64)
                with self.assertRaises(ModelError):
                    cpu.run(100)
                with self.assertRaisesRegex(RuntimeError, 'continuation is forbidden'):
                    cpu.step()

    def test_fixed_seed_random_programs(self):
        for seed in range(8):
            rng = random.Random(seed)
            lines = [f'addi x{r},x0,{rng.randrange(-50, 51)}' for r in range(1, 8)]
            for i in range(45):
                rd, a, b = (rng.randrange(1, 8) for _ in range(3))
                kind = rng.randrange(5)
                if kind == 0:
                    lines.append(f'{rng.choice(("add", "sub", "and", "or", "xor", "slt"))} x{rd},x{a},x{b}')
                elif kind == 1:
                    lines.append(f'addi x{rd},x{a},{rng.randrange(-128, 128)}')
                elif kind == 2:
                    lines.append(f'sw x{a},{4 * rng.randrange(8)}(x0)')
                elif kind == 3:
                    lines.append(f'lw x{rd},{4 * rng.randrange(8)}(x0)')
                else:
                    lines.extend((f'beq x{a},x{b},skip_{i}', f'addi x{rd},x{rd},1', f'skip_{i}: nop'))
            lines.append('halt')
            source = '\n'.join(lines)
            with self.subTest(seed=seed):
                cpu = self.run_program(source, latency=1 + 2 * (seed % 3))
                other = self.run_program(source, latency=1 + 2 * (seed % 3), cache=False, reverse=True)
                self.assertEqual(cpu.retired, other.retired)
                self.assertEqual(cpu.retire_ticks, other.retire_ticks)
