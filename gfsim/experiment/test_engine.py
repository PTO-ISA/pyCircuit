"""Small circuits exercising dynamic subscriptions, independently of CPU logic."""
import unittest

from construction import assemble
from engine import Queue, RuleEntry, NeedInput, CapacityCycle


class Circuit:
    """Test-only netlist builder; actions use the public generated-code protocol."""
    def __init__(self, queues, groups, resources=None, cache=True):
        entries, modules = [None], []
        for mid, (select, actions) in enumerate(groups):
            module = TestModule(mid, select)
            modules.append(module)
            for action, pops, pushes, revises in actions:
                rid = len(entries)
                def work(*args, rid=rid, action=action, module=module):
                    e = module.engine
                    if not e.begin_rule(rid, args):
                        return
                    try:
                        action(e, rid, *args)
                    except NeedInput:
                        e.abort_rule(rid)
                        return
                    e.complete_rule(rid)
                module.calls.append(work)
                entries.append(RuleEntry(mid, work, lambda rid=rid, m=module: m.engine.arbitrate_rule(rid),
                                         pops, pushes, revises))
        self.sim = assemble(queues, modules, entries, cache, module_queues=resources)


class TestModule:
    def __init__(self, mid, select):
        self.mid, self.select, self.calls = mid, select, []

    def Work(self):
        self.select(self)


def read(e, rid, q):
    return q.peek()


def one(m):
    m.calls[0]()


def action(fn, *, pops=(), pushes=(), revises=()):
    return fn, pops, pushes, revises


class DynamicReadersTests(unittest.TestCase):
    def test_precise_multiword_masks_and_aliases(self):
        q, out = Queue(initial=(4,)), Queue(initial=(0,))
        def compute(e, r):
            read(e, r, q)
            read(e, r, q)  # Same resource/alias records one edge.
        acts = [action(compute if i in (0, 63, 64, 69) else lambda e, r: None)
                for i in range(70)]
        def all_rules(m):
            for call in m.calls:
                call()
        c = Circuit([q, out], [(all_rules, acts), (one, [action(
            lambda e, r: q.propose_revise(r, 5), revises=(0,))])], [[0, 0, 1], [0]])
        c.sim.step()
        self.assertEqual(c.sim.modules[0].dirty_words, [1 | (1 << 63), 1 | (1 << 5)])
        self.assertEqual([r.read_slots for r in c.sim.rules[1:71]],
                         [[0] if i in (0, 63, 64, 69) else [] for i in range(70)])
        self.assertEqual(q.readers, [0, 1])
        self.assertEqual(c.sim.stats.read_registrations, 5)
        self.assertFalse(c.sim.arbitrate_rule(1))

    def test_control_wakeup_equal_var_cache_hit_and_changed_var(self):
        control, out = Queue(initial=(0,)), Queue(initial=(99,))
        def select(m):
            e = m.engine
            m.calls[0](control.peek() // 2)
        def write(e, r, value):
            out.propose_push(r, value)
        def change(e, r):
            control.propose_revise(r, read(e, r, control) + 1)
        c = Circuit([control, out], [(select, [action(write, pushes=(1,))]),
                                     (one, [action(change, revises=(0,))])])
        s = c.sim
        s.step()
        self.assertFalse(s.is_dirty(1))  # Module read is not a Rule edge.
        before = (s.stats.read_registrations, s.stats.read_clears)
        s.step()
        self.assertEqual(s.rule_calls[1], 1)
        self.assertEqual(s.stats.cache_hits, 1)
        self.assertEqual((s.stats.read_registrations - before[0], s.stats.read_clears - before[1]), (1, 1))
        s.step()
        self.assertEqual(s.rule_calls[1], 2)
        self.assertEqual(out.proposal(1).push, 1)

    def test_cache_hit_keeps_rule_edges_without_traversal_or_renewal(self):
        control, source, out = Queue(initial=(0,)), Queue(initial=(7,)), Queue(initial=(0,))
        def select(m):
            control.peek()
            m.calls[0]()
        def drive(m):
            m.calls[0](m.engine.tick)
            if m.engine.tick == 1:
                m.calls[1]()
        c = Circuit([control, source, out], [(select, [action(
            lambda e, r: out.propose_push(r, read(e, r, source)), pushes=(2,))]),
            (drive, [action(lambda e, r, tick: control.propose_revise(r, tick + 1), revises=(0,)),
                     action(lambda e, r: source.propose_revise(r, 8), revises=(1,))])])
        s = c.sim
        s.step()
        class NoIteration(list):
            def __iter__(self):
                raise AssertionError('cache hit traversed Rule reads')
        s.rules[1].read_slots = NoIteration(s.rules[1].read_slots)
        s.step()  # Cache hit; then source changes after all Work.
        self.assertEqual(s.rule_calls[1], 1)
        self.assertEqual(s.stats.cache_hits, 1)
        self.assertTrue(s.is_dirty(1))  # The retained Rule edge still receives notification.
        s.rules[1].read_slots = s.rules[1].read_slots[:]
        s.step()
        self.assertEqual(s.rule_calls[1], 2)
        self.assertEqual(out.proposal(1).push, 8)

    def test_dynamic_array_edges_replace_old_reads(self):
        index, a, b, out = Queue(initial=(0,)), Queue(initial=(10,)), Queue(initial=(20,)), Queue(initial=(0,))
        def compute(e, r):
            out.propose_push(r, read(e, r, (a, b)[read(e, r, index)]))
        def driver(m):
            if m.engine.tick == 0:
                m.calls[0]()
            elif m.engine.tick == 1:
                m.calls[1]()
        c = Circuit([index, a, b, out], [(one, [action(compute, pushes=(3,))]),
            (driver, [action(lambda e, r: index.propose_revise(r, 1), revises=(0,)),
                      action(lambda e, r: a.propose_revise(r, 11), revises=(1,))])])
        s = c.sim
        s.step()
        s.step()  # New index reads b; simultaneous change to a must not dirty Rule 1.
        self.assertEqual(s.rules[1].read_slots, [0, 2])
        self.assertFalse(s.is_dirty(1))
        self.assertNotIn((2, 0), s.events)
        s.step()
        self.assertEqual(s.rule_calls[1], 2)
        self.assertEqual(out.proposal(1).push, 20)

    def test_empty_abort_clears_partial_effects_keeps_reads_and_commit_subscription(self):
        inp, out = Queue(), Queue(capacity=2)
        def consume(e, r):
            out.propose_push(r, 9)  # Must disappear on missing input.
            read(e, r, inp)
            inp.propose_pop(r)
        def once(m):
            if m.engine.tick == 0:
                m.calls[0]()
        c = Circuit([inp, out], [(one, [action(consume, pops=(0,), pushes=(1,))]),
                                 (once, [action(lambda e, r: inp.propose_push(r, 7), pushes=(0,))])])
        s = c.sim
        s.step()
        self.assertEqual(out.current, ())
        self.assertEqual(s.rules[1].read_slots, [0])
        self.assertFalse(s.rules[1].complete)
        s.step()
        self.assertEqual(out.current, (9,))
        self.assertEqual(s.rules[1].read_slots, [0])
        s.step()  # Successful pop woke the Rule, whose input is now empty.
        self.assertEqual(s.rule_calls[1], 3)
        self.assertEqual(out.current, (9,))
        self.assertEqual(s.rules[1].participants, [])

    def test_unselected_rule_removes_edges_and_control_read_expires(self):
        flag, q, out = Queue(initial=(0,)), Queue(initial=(3,)), Queue(initial=(0,))
        def choose(m):
            if m.engine.tick == 0:
                flag.peek()
                m.calls[0]()
            m.calls[1]()
        def old(e, r):
            out.propose_push(r, read(e, r, q))
        c = Circuit([flag, q, out], [(choose, [action(old, pushes=(2,)), action(lambda e, r: None)]),
            (one, [action(lambda e, r: flag.propose_revise(r, 1), revises=(0,))])])
        s = c.sim
        s.step()
        s.step()
        self.assertEqual(s.rules[1].read_slots, [])
        self.assertEqual(s.rules[1].participants, [])
        self.assertFalse(s.is_dirty(1))
        self.assertFalse(s.is_reader(0, 0))
        self.assertFalse(s.is_reader(0, 1))

    def test_early_missing_module_read_preserves_prior_independent_rule(self):
        missing, out = Queue(), Queue()
        def select(m):
            m.calls[0]()
            missing.peek()
            m.calls[1]()
        c = Circuit([missing, out], [(select, [action(lambda e, r: out.propose_push(r, 8), pushes=(1,)),
                                              action(lambda e, r: self.fail('unreachable'))])])
        self.assertEqual(c.sim.step(), (1,))
        self.assertEqual(out.current, (8,))
        self.assertTrue(c.sim.is_reader(0, 0))

    def test_capacity_retry_does_not_rerun_work(self):
        out = Queue(initial=(7,))
        def producer(e, r):
            out.propose_push(r, 9)
        def delayed(m):
            if m.engine.tick == 0:
                m.calls[0]()
            else:
                m.calls[1]()
        c = Circuit([out], [(one, [action(producer, pushes=(0,))]),
            (delayed, [action(lambda e, r: e.request_wakeup(r, 1, 2)),
                       action(lambda e, r: out.propose_pop(r), pops=(0,))])])
        s = c.sim
        s.step()
        s.step()
        self.assertEqual(set(s.step()), {1, 3})
        self.assertEqual(out.current, (9,))
        self.assertEqual(s.module_calls[0], 1)
        self.assertEqual(s.rule_calls[1], 1)
        self.assertFalse(s.is_dirty(1))

    def test_equal_revise_vs_equal_payload_replacement(self):
        for replace in (False, True):
            with self.subTest(replace=replace):
                q, out = Queue(initial=(7,)), Queue(initial=(0,))
                def observe(e, r):
                    out.propose_push(r, read(e, r, q))
                def change(e, r):
                    if replace:
                        q.propose_pop(r)
                        q.propose_push(r, 7)
                    else:
                        q.propose_revise(r, 7)
                mutation = action(change, pops=(0,), pushes=(0,)) if replace else action(change, revises=(0,))
                c = Circuit([q, out], [(one, [action(observe, pushes=(1,))]), (one, [mutation])])
                c.sim.step()
                self.assertEqual(c.sim.is_dirty(1), replace)
                self.assertEqual(q.state_version, int(replace))
                if replace:
                    self.assertFalse(c.sim.arbitrate_rule(1))
                c.sim.step()
                self.assertEqual(c.sim.rule_calls[1], 2 if replace else 1)

    def test_no_effect_queue_change_recomputes_without_firing(self):
        q = Queue(initial=(1,))
        c = Circuit([q], [(one, [action(lambda e, r: read(e, r, q))]),
                         (one, [action(lambda e, r: q.propose_revise(r, read(e, r, q) + 1), revises=(0,))])])
        c.sim.step()
        c.sim.step()
        self.assertEqual(c.sim.rule_calls[1], 2)
        self.assertEqual(c.sim.stats.cache_hits, 0)
        self.assertEqual(c.sim.rules[1].accepted_tick, -1)

    def test_no_effect_reuse_then_value_and_argument_invalidation(self):
        control, value, out = Queue(initial=(0,)), Queue(initial=(7,)), Queue()
        def select(m):
            m.calls[0](control.peek() // 2)
        def consume(e, r, mode):
            v = value.peek()
            if mode:
                out.propose_push(r, v)
        def drive(m):
            m.calls[0](m.engine.tick)
        def change(e, r, tick):
            control.propose_revise(r, tick + 1)
            if tick == 1:
                value.propose_revise(r, 8)
        c = Circuit([control, value, out], [
            (select, [action(consume, pushes=(2,))]),
            (drive, [action(change, revises=(0, 1))])])
        s = c.sim
        s.step()
        self.assertTrue(s.rules[1].complete)
        self.assertFalse(s.rules[1].has_effects())
        self.assertFalse(s.is_dirty(1))
        s.step()  # Equal derived argument reuses the no-effect result.
        self.assertEqual(s.rule_calls[1], 1)
        self.assertEqual(s.rules[1].accepted_tick, -1)
        self.assertTrue(s.is_dirty(1))  # The retained value subscription fires at Xfer.
        s.step()
        self.assertEqual(out.peek(), 8)
        self.assertEqual(s.rule_calls[1], 2)
        self.assertFalse(s.rules[1].complete)

    def test_timer_wakeup_preserves_clean_no_effect_result(self):
        value = Queue(initial=(7,))
        def select(m):
            m.calls[0]()
            m.calls[1]()
        c = Circuit([value], [(select, [
            action(lambda e, r: value.peek()),
            action(lambda e, r: e.request_wakeup(r, 0, 1))])])
        for _ in range(3):
            c.sim.step()
        self.assertEqual(c.sim.rule_calls[1], 1)
        self.assertEqual(c.sim.stats.cache_hits, 2)
        self.assertEqual(c.sim.rules[1].accepted_tick, -1)
        self.assertFalse(c.sim.is_dirty(1))
        self.assertEqual(c.sim.rule_calls[2], 3)

    def test_explicit_resources_reject_unbound_reads_and_modifications(self):
        q = Queue(initial=(1,))
        with self.assertRaisesRegex(ValueError, 'modifies undeclared'):
            Circuit([q], [(one, [action(lambda e, r: None, revises=(0,))])], [[]])
        c = Circuit([q], [(one, [action(lambda e, r: read(e, r, q))])], [[]])
        with self.assertRaisesRegex(ValueError, 'undeclared Module Queue'):
            c.sim.step()


class AutomaticReadTests(unittest.TestCase):
    def test_fixed_mapping_covers_sparse_modules_and_aliases(self):
        queues = [Queue(initial=(i,)) for i in range(3)]
        c = Circuit(queues, [(one, [action(lambda e, r: queues[2].peek())]),
                             (one, [action(lambda e, r: queues[1].peek())])], [[2, 0, 2], [1]])
        maps = [id(q.module_slots) for q in queues]
        self.assertEqual([q.module_slots for q in queues], [[0, -1], [-1, 0], [1, -1]])
        c.sim.step()
        self.assertEqual(c.sim.rules[1].read_slots, [1])
        self.assertEqual(c.sim.rules[2].read_slots, [0])
        self.assertEqual(maps, [id(q.module_slots) for q in queues])
        self.assertFalse(c.sim.is_reader(0, 0))  # Declared does not mean read.

    def test_all_read_interfaces_subscribe_module_and_rule_before_fill(self):
        expectations = {'empty': (True, False), 'full': (False, True), 'size': (0, 1),
                        'try_peek': (None, 7), 'peek': (None, 7), 'current': ((), (7,))}
        for interface, expected in expectations.items():
            for in_rule in (False, True):
                with self.subTest(interface=interface, in_rule=in_rule):
                    q, seen = Queue(), []
                    def observe():
                        try:
                            value = q.current if interface == 'current' else getattr(q, interface)()
                        except NeedInput:
                            seen.append(None)
                            raise
                        seen.append(value)
                    groups = [(one, [action(lambda e, r: observe())])] if in_rule else [(
                        lambda m: observe(), [])]
                    groups.append((one, [action(lambda e, r: q.propose_push(r, 7), pushes=(0,))]))
                    s = Circuit([q], groups).sim
                    s.step()
                    self.assertTrue(s.is_reader(0, 0))
                    if in_rule:
                        self.assertEqual(s.rules[1].read_slots, [0])
                    s.step()
                    self.assertEqual(seen, list(expected))
                    self.assertIsNone(s.active_module)
                    self.assertIsNone(s.active_rule)

    def test_context_restored_after_abort_complete_and_cache_hit(self):
        control, missing, value, after, out = (Queue(initial=(0,)), Queue(),
            Queue(initial=(7,)), Queue(initial=(8,)), Queue(initial=(9,)))
        def select(m):
            control.peek()
            m.calls[0]()  # Missing input aborts.
            self.assertIsNone(m.engine.active_rule)
            after.peek()
            m.calls[1]()  # Completes on tick 0, cache hit on tick 1.
            self.assertIsNone(m.engine.active_rule)
            after.size()
        def blocked(e, r):
            self.assertEqual(e.active_rule, r)
            out.propose_push(r, value.peek())
        s = Circuit([control, missing, value, after, out], [
            (select, [action(lambda e, r: missing.peek()), action(blocked, pushes=(4,))]),
            (one, [action(lambda e, r: control.propose_revise(r, control.peek() + 1), revises=(0,))])]).sim
        s.step()
        s.step()
        self.assertEqual(s.rules[1].read_slots, [1])
        self.assertEqual(s.rules[2].read_slots, [2])
        self.assertEqual(s.stats.cache_hits, 1)
        self.assertEqual(s.modules[0].control_reads[3], s.modules[0].read_gen)
        self.assertEqual(s.modules[0].rule_readers[3], 0)
        self.assertIsNone(s.active_rule)
        self.assertIsNone(s.active_module)

    def test_peek_and_modification_register_once_and_push_is_not_a_read(self):
        for operation in ('pop', 'revise'):
            for peek_first in (False, True):
                with self.subTest(operation=operation, peek_first=peek_first):
                    target, out = Queue(initial=(1,)), Queue()
                    def execute(e, r):
                        if peek_first:
                            target.peek()
                        if operation == 'pop':
                            target.propose_pop(r)
                        else:
                            target.propose_revise(r, 2)
                            target.propose_revise(r, 2)
                        out.propose_push(r, 3)
                    entry = action(execute, pops=(0,), pushes=(1,)) if operation == 'pop' else action(
                        execute, revises=(0,), pushes=(1,))
                    s = Circuit([target, out], [(one, [entry])]).sim
                    s.step()
                    self.assertEqual(s.rules[1].read_slots, [0])
                    self.assertEqual(s.stats.read_registrations, 1)
                    self.assertFalse(s.is_reader(0, 1))
                    self.assertEqual(out.current, (3,))

    def test_observation_outside_work_has_no_scheduling_effects(self):
        from copy import deepcopy
        from dataclasses import asdict
        q = Queue(initial=(1,))
        s = Circuit([q], [(one, [action(lambda e, r: q.peek())])]).sim
        s.step()
        before = deepcopy((s.modules, s.rules, s.events, asdict(s.stats)))
        for _ in range(3):
            self.assertEqual((q.peek(), q.try_peek(), q.empty(), q.full(), q.size(), q.current),
                             (1, 1, False, True, 1, (1,)))
            s.snapshot()
        self.assertEqual((s.modules, s.rules, s.events, asdict(s.stats)), before)
        # An unbound Queue also remains usable for construction and inspection.
        self.assertEqual(Queue(initial=(9,)).peek(), 9)

    def test_exception_and_missing_completion_clear_context(self):
        for in_rule in (False, True):
            with self.subTest(in_rule=in_rule):
                q = Queue(initial=(1,))
                def fail():
                    q.peek()
                    raise ValueError('model failure')
                groups = [(one, [action(lambda e, r: fail())])] if in_rule else [(lambda m: fail(), [])]
                s = Circuit([q], groups).sim
                with self.assertRaisesRegex(ValueError, 'model failure'):
                    s.step()
                self.assertIsNone(s.active_module)
                self.assertIsNone(s.active_rule)
                self.assertTrue(s.is_reader(0, 0) if in_rule else s.modules[0].control_reads[0] != 0)
                with self.assertRaisesRegex(RuntimeError, 'continuation is forbidden'):
                    s.step()
        def unfinished(m):
            m.engine.begin_rule(1)
        s = Circuit([], [(unfinished, [action(lambda e, r: None)])]).sim
        with self.assertRaisesRegex(RuntimeError, 'omitted complete/abort'):
            s.step()
        self.assertIsNone(s.active_rule)
        self.assertIsNone(s.active_module)

    def test_nested_rules_rejected_and_legacy_registration_deduplicated(self):
        s = Circuit([], [(one, [action(lambda e, r: e.begin_rule(2)), action(lambda e, r: None)])]).sim
        with self.assertRaisesRegex(RuntimeError, 'nested Rule'):
            s.step()
        self.assertIsNone(s.active_rule)
        q = Queue(initial=(1,))
        def legacy(e, r):
            e.record_read(0, q.qid, r)
            q.peek()
        s = Circuit([q], [(one, [action(legacy)])]).sim
        s.step()
        self.assertEqual(s.rules[1].read_slots, [0])
        self.assertEqual(s.stats.read_registrations, 1)

    def test_undeclared_automatic_access_rejected_for_module_and_rule(self):
        for in_rule in (False, True):
            with self.subTest(in_rule=in_rule):
                q = Queue(initial=(1,))
                groups = [(one, [action(lambda e, r: q.peek())])] if in_rule else [(lambda m: q.size(), [])]
                s = Circuit([q], groups, [[]]).sim
                with self.assertRaisesRegex(ValueError, 'undeclared Module Queue'):
                    s.step()
                self.assertIsNone(s.active_rule)
                self.assertIsNone(s.active_module)


def cyclic_circuit():
    left, right = Queue(initial=(1,)), Queue(initial=(2,))
    def move(src, dst):
        def execute(e, r):
            dst.propose_push(r, read(e, r, src))
            src.propose_pop(r)
        return execute
    return Circuit([left, right], [(one, [action(move(left, right), pops=(0,), pushes=(1,))]),
                                   (one, [action(move(right, left), pops=(1,), pushes=(0,))])])


if __name__ == '__main__':
    unittest.main()
