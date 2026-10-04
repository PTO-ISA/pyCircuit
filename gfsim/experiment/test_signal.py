"""Static Signal input/output contracts; Queue dynamic readers stay independent."""
import unittest

from construction import assemble
from engine import Queue, Signal, RuleEntry
from review import ReviewTrace
from test_engine import TestModule


class StaticSignalTests(unittest.TestCase):
    def mux(self, cache=True):
        selector, a, b = [Queue(initial=(v,)) for v in (0, 7, 7)]
        signal = Signal(lambda: b.peek() if selector.peek() else a.peek())
        updater = TestModule(0, lambda m: m.calls[0]())
        consumer = TestModule(1, lambda m: m.calls[0]())
        def update():
            sim = updater.engine
            if sim.begin_rule(1, (sim.tick,)):
                if sim.tick == 0:
                    selector.propose_revise(1, 1)
                elif sim.tick == 1:
                    a.propose_revise(1, 9)
                elif sim.tick == 2:
                    a.propose_revise(1, 10)
                    b.propose_revise(1, 11)
                if sim.tick < 2:
                    sim.request_wakeup(1, 0, 1)
                sim.complete_rule(1)
        def consume():
            sim = consumer.engine
            if sim.begin_rule(2):
                self.assertEqual(signal.value, 7 if sim.tick == 0 else 11)
                sim.complete_rule(2)
        updater.calls, consumer.calls = [update], [consume]
        rules = [None, RuleEntry(0, update, lambda: updater.engine.arbitrate_rule(1), revises=(0, 1, 2)),
                 RuleEntry(1, consume, lambda: consumer.engine.arbitrate_rule(2))]
        sim = assemble([selector, a, b], [updater, consumer], rules, cache,
                       module_queues=[(0, 1, 2), ()], signals=[signal],
                       module_signals=[(), ()], signal_queues=[(0, 1, 2, 2)],
                       rule_signals=[(), (), (0, 0)])
        return sim, signal

    def test_inactive_inputs_output_filter_and_full_xfer_barrier(self):
        for cache in (False, True):
            sim, signal = self.mux(cache)
            trace = ReviewTrace(sim)
            with self.assertRaisesRegex(RuntimeError, 'before initialization'):
                _ = signal.value
            for tick in range(3):
                sim.step()
                self.assertEqual(signal.evaluations, tick + 2)
                self.assertEqual(sim.module_calls[1], 1)
                self.assertEqual(sim.is_dirty(2), tick == 2)
                self.assertEqual(sim.rules[2].read_slots, [])
            sim.step()
            self.assertEqual(sim.module_calls[1], 2)
            self.assertEqual(signal.value, 11)
            self.assertTrue(all(e['inputs'] == [0, 1, 2] for f in trace.data['frames']
                                for e in f['signal_evaluations']))
            self.assertTrue(all(f['signal_readers'] == [[1]] for f in trace.data['frames']))

    def test_static_masks_survive_unread_branches_and_deselection(self):
        for cache in (False, True):
            q = Queue(initial=(0,))
            signal = Signal(q.peek)
            writer = TestModule(0, lambda m: m.calls[0]())
            reader = TestModule(1, lambda m: [call() for i, call in enumerate(m.calls)
                                if m.engine.tick != 1 or i == 1])
            control = TestModule(2, lambda m: m.calls[0]())
            def write():
                sim = writer.engine
                if sim.begin_rule(1):
                    q.propose_revise(1, q.peek() + 1)
                    sim.complete_rule(1)
            writer.calls = [write]
            entries = [None, RuleEntry(0, write, lambda: writer.engine.arbitrate_rule(1), revises=(0,))]
            bound = {0, 63, 64, 129}
            for i in range(130):
                rid = len(entries)
                def work(rid=rid):
                    sim = reader.engine
                    if sim.begin_rule(rid):
                        sim.complete_rule(rid)  # No actual Signal reads.
                reader.calls.append(work)
                entries.append(RuleEntry(1, work, lambda rid=rid: reader.engine.arbitrate_rule(rid)))
            plain = len(entries)
            def control_work():
                if control.engine.begin_rule(plain):
                    control.engine.complete_rule(plain)
            control.calls = [control_work]
            entries.append(RuleEntry(2, control_work, lambda: control.engine.arbitrate_rule(plain)))
            sim = assemble([q], [writer, reader, control], entries, cache,
                           module_queues=[(0,), (), ()], signals=[signal],
                           module_signals=[(), (), (0, 0)], signal_queues=[(0, 0)],
                           rule_signals=[(), ()] + [(0, 0) if i in bound else () for i in range(130)] + [()])
            for tick in range(4):
                sim.step()
                self.assertEqual(sim.module_calls, [tick + 1] * 3)
                self.assertEqual(len(sim.events), 3)
                for i in range(130):
                    self.assertEqual(sim.is_dirty(i + 2), i in bound)
                    self.assertFalse(sim.rules[i + 2].read_slots)
                self.assertFalse(sim.is_dirty(plain))
            self.assertEqual(sim.rule_calls[2], 3)
            self.assertEqual(sim.rule_calls[3], 1 if cache else 4)
            self.assertEqual(sim.rule_calls[plain], 1 if cache else 4)

    def test_undeclared_input_and_rule_signal_rejected(self):
        sim, signal = self.mux()
        signal.input_qids = (0, 2)  # Deliberately invalid construction.
        with self.assertRaisesRegex(ValueError, 'undeclared Signal Queue'):
            sim.step()
        sim, signal = self.mux()
        signal.dependents[1] = ()  # Module-only binding must not authorize Rule reads.
        with self.assertRaisesRegex(ValueError, 'undeclared Rule Signal'):
            sim.step()


if __name__ == '__main__':
    unittest.main()
