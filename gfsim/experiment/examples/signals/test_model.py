"""End-to-end clocked circuits; no direct Queue writes or injected wakeups."""
from dataclasses import asdict
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from construction import assemble
from engine import Queue, Signal, RuleEntry, NeedInput
from review import ReviewTrace
from .model import Config, Sample, build, workload
from .reference import Reference


class StreamTests(unittest.TestCase):
    def test_full_stream_matches_clocked_reference_and_order(self):
        for frequent in (False, True):
            words, script = workload(frequent=frequent)
            reference_rows = None
            for cache in (False, True):
                for reverse in (False, True):
                    with self.subTest(frequent=frequent, cache=cache, reverse=reverse):
                        sim = build(words=words, script=script, cache=cache, reverse=reverse)
                        ref = Reference(words, script)
                        trace = ReviewTrace(sim)
                        rows = []
                        for tick in range(120):
                            sim.step()
                            self.assertEqual(sim.snapshot(), ref.step(), f'tick {tick}')
                            self.assertEqual(tuple(s.value for s in sim.signals), ref.signals())
                            rows.append((sim.snapshot(), sorted(sim.last_accepted)))
                            self.assertLessEqual(trace.data['frames'][-1]['stats']['module_work'], 5)
                        if reference_rows is None:
                            reference_rows = rows
                        self.assertEqual(rows, reference_rows)
                        self.assertEqual(sim.queues[6].peek()[0], sum(map(len, words)))
                        self.assertTrue(any(f['stats']['signal_work'] == 0 for f in trace.data['frames']))

    def test_pending_proposal_is_recomputed_then_deselected(self):
        sim = build()
        trace = ReviewTrace(sim)
        for _ in range(4):
            sim.step()
        frames = trace.data['frames']
        # A full output holds the second bank-0 item. Bias changes at tick 2 Xfer.
        self.assertEqual(frames[2]['rules'][4]['proposals'][-1]['push']['value'], 11)
        self.assertTrue(any(i['resource'] == sim.signals[1].resource_id and 4 in i['rules']
                            for i in frames[2]['invalidations']))
        self.assertEqual(frames[3]['rules'][4]['proposals'][-1]['push']['value'], 111)
        self.assertNotIn(4, sim.last_accepted)
        sim.step()  # enabled=False is now observed by the Module.
        self.assertFalse(sim.rules[4].complete)
        self.assertEqual(sim.rules[4].read_slots, [])
        sim.step()  # New bank selection and downstream pop: never submit stale 111.
        self.assertEqual(sim.queues[5].peek(), Sample(True, 1, 0, 30))

    def test_xfer_barrier_dedup_and_actual_input_replacement(self):
        sim = build()
        trace = ReviewTrace(sim)
        sim.step()  # Both source Queues change, helper sees only the selected bank.
        self.assertEqual(sim.signals[1].value, Sample(True, 0, 0, 10))
        initial = trace.data['frames'][0]['signal_evaluations']
        self.assertEqual(sum(e['sid'] == 1 and not e['initial'] for e in initial), 1)
        for _ in range(5):
            sim.step()
        f = trace.data['frames'][5]
        # At tick 5 both banks may update as consumption frees space. The result
        # must describe final state, never an intermediate pop-before-push state.
        evaluations = [e for e in f['signal_evaluations'] if e['sid'] == 1]
        self.assertEqual(len(evaluations), 1)
        self.assertEqual(evaluations[0]['inputs'], [1, 4])
        self.assertEqual(sim.signals[1].value, Sample(True, 1, 1, 31))
        s = sim.signals[1]
        self.assertNotEqual(s.input_reads[s.input_qids.index(0)], s.read_gen)

    def test_diagnostics_preserve_execution_and_export_signals(self):
        plain, traced = build(), build()
        trace = ReviewTrace(traced, title='Signal <review>')
        for _ in range(120):
            self.assertEqual(plain.step(), traced.step())
            self.assertEqual(plain.snapshot(), traced.snapshot())
            self.assertEqual(plain.events, traced.events)
            self.assertEqual(asdict(plain.stats), asdict(traced.stats))
        self.assertEqual(len(trace.data['signals']), 2)
        with TemporaryDirectory() as directory:
            path = Path(directory) / 'review.html'
            trace.write_html(path)
            self.assertIn('Signal 求值与通知', path.read_text())


class RegisterWriter:
    """One clocked Rule drives a fixed register-write program."""
    def __init__(self, writes, cycles):
        self.writes, self.cycles, self.rid = writes, cycles, 1

    def Work(self):
        e, tick = self.engine, self.engine.tick
        if not e.begin_rule(1, (tick,)):
            return
        for when, queue, value in self.writes:
            if tick == when:
                queue.propose_revise(1, value)
        if tick + 1 < self.cycles:
            e.request_wakeup(1, self.mid, 1)
        e.complete_rule(1)

    def arbitrate(self):
        return self.engine.arbitrate_rule(1)


class ReaderBank:
    """Generated bank of status Rules; optional timer tests reuse without effects."""
    def __init__(self, signal, count=1, control=None, cycles=0):
        self.signal, self.count, self.control, self.cycles = signal, count, control, cycles
        self.values = [None] * count  # Observation only; never drives model behavior.

    def Work(self):
        e = self.engine
        if self.control is not None:
            self.control.value
        for local in range(self.count):
            rid = local + 2
            if not e.begin_rule(rid):
                continue
            if self.count == 1 or local in (0, 63, 64, 69):
                self.values[local] = self.signal.value
            e.complete_rule(rid)
        if self.cycles:
            rid, tick = self.count + 2, e.tick
            if e.begin_rule(rid, (tick,)):
                if tick + 1 < self.cycles:
                    e.request_wakeup(rid, self.mid, 1)
                e.complete_rule(rid)


def register_circuit(queues, signals, signal_queues, writes=(), *, count=1,
                     control=False, timer=False, cycles=8):
    writer = RegisterWriter(writes, cycles)
    reader = ReaderBank(signals[0], count, signals[1] if control else None,
                        cycles if timer else 0)
    writer.mid, reader.mid = 0, 1
    entries = [None, RuleEntry(0, writer.Work, writer.arbitrate,
                              revises=tuple(range(len(queues))))]
    # These fixed entry stubs stand for generated per-Rule arbitration functions.
    for rid in range(2, count + 2 + bool(timer)):
        entries.append(RuleEntry(1, reader.Work,
                                 lambda rid=rid: reader.engine.arbitrate_rule(rid)))
    sim = assemble(queues, [writer, reader], entries, module_queues=[range(len(queues)), ()],
                   signals=signals, module_signals=[(), range(len(signals))],
                   signal_queues=signal_queues)
    return sim, reader


class SubscriptionCircuitTests(unittest.TestCase):
    def test_unchanged_result_replaces_inputs_and_does_not_wake(self):
        index, a, b = Queue(initial=(0,)), Queue(initial=(7,)), Queue(initial=(7,))
        def select():
            return (a, b)[index.peek()].peek()
        signal = Signal(select)
        # Switching to equal b must replace the a subscription even with no notification.
        writes = ((0, index, 1), (1, a, 9), (2, b, 10))
        sim, reader = register_circuit([index, a, b], [signal], [(0, 1, 2)], writes)
        counts = []
        for _ in range(5):
            sim.step()
            counts.append((sim.stats.signal_work, sim.module_calls[1]))
        self.assertEqual(counts, [(2, 1), (2, 1), (3, 1), (3, 2), (3, 2)])
        self.assertEqual(reader.values, [10])

    def test_multiword_rule_dirty_and_module_control_share_slots(self):
        q = Queue(initial=(0,))
        first, control = Signal(q.peek), Signal(lambda: q.peek() % 2)
        sim, reader = register_circuit([q], [first, control], [(0,), (0,)],
                                       ((0, q, 1),), count=70, control=True)
        sim.step()
        # R0/63 in word 0; R64/69 in word 1. Control read marks no extra Rules.
        self.assertEqual(sim.modules[1].dirty_words, [1 | (1 << 63), 1 | (1 << 5)])
        self.assertEqual(len([e for e in sim.events if e == (1, 1)]), 2)
        sim.step()
        self.assertEqual(sim.module_calls[1], 2)  # Two notifications, one Work.
        self.assertEqual(reader.values[69], 1)
        self.assertEqual(sim.stats.cache_hits, 66)

    def test_timer_no_effect_cache_and_empty_input_subscription(self):
        q = Queue()
        def observe():
            value = q.try_peek()
            return value is not None, 0 if value is None else value[1]
        signal = Signal(observe)
        # A real source Rule pushes into the initially empty input.
        from .model import Source
        source = Source(1, (42,), Queue(initial=(0,)), q)
        reader = ReaderBank(signal, cycles=4)
        source.mid, reader.mid = 0, 1
        entries = [None, RuleEntry(0, source.send, source.arbitrate, pushes=(0,), revises=(1,)),
                   RuleEntry(1, reader.Work, lambda: reader.engine.arbitrate_rule(2)),
                   RuleEntry(1, reader.Work, lambda: reader.engine.arbitrate_rule(3))]
        sim = assemble([q, source.position], [source, reader], entries,
                       module_queues=[(0, 1), ()], signals=[signal],
                       module_signals=[(), (0,)], signal_queues=[(0,)])
        for _ in range(4):
            sim.step()
        self.assertEqual(reader.values, [(True, 42)])
        self.assertEqual(sim.rule_calls[2], 2)
        self.assertEqual(sim.stats.cache_hits, 2)
        self.assertEqual(sim.rules[2].accepted_tick, -1)
        self.assertEqual(sim.stats.signal_work, 2)

    def test_initialization_and_invalid_helper_contracts(self):
        q = Queue(initial=(3,))
        s = Signal(q.peek)
        with self.assertRaisesRegex(RuntimeError, 'before initialization'):
            _ = s.value
        sim, reader = register_circuit([q], [s], [(0,)])
        sim.step()
        self.assertEqual(reader.values, [3])
        cases = (
            ([Queue(initial=(1,))], lambda queues: [Signal(queues[0].peek)], [()], ValueError),
            ([], lambda queues: [Signal(lambda: [])], [()], TypeError),
            ([Queue()], lambda queues: [Signal(queues[0].peek)], [(0,)], NeedInput),
        )
        for queues, factory, inputs, error in cases:
            broken, _ = register_circuit(queues, factory(queues), inputs)
            with self.assertRaises(error):
                broken.step()
            with self.assertRaisesRegex(RuntimeError, 'has failed'):
                broken.step()
        a = Signal(lambda: 1)
        b = Signal(lambda: a.value)
        broken, _ = register_circuit([], [a, b], [(), ()])
        with self.assertRaisesRegex(RuntimeError, 'another Signal'):
            broken.step()


if __name__ == '__main__':
    unittest.main()
