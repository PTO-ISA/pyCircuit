"""Run: python3 -m unittest discover -s gfsim/experiment -v"""

import random
import unittest

from construction import assemble
from engine import NeedInput, Queue, RuleEntry
from models import BranchMove, Drain, Move, Push, RandomProcessor, ReadSwitch, Revise


class PartialMove(Move):
    def work_move(self):
        e = self.engine
        if not e.begin_rule(self.rid):
            return
        try:
            e.record_read(self.mid, self.source.qid, self.rid)
            value = self.source.peek()
            self.source.propose_pop(self.rid)
            e.record_read(self.mid, self.config.qid, self.rid)
            self.target.propose_push(self.rid, (value, self.config.peek()))
        except NeedInput:
            e.abort_rule(self.rid)
            return
        e.complete_rule(self.rid)


class DynamicMove(Move):
    def work_move(self):
        e = self.engine
        if not e.begin_rule(self.rid):
            return
        try:
            e.record_read(self.mid, self.index.qid, self.rid)
            source = self.data[self.index.peek()]
            e.record_read(self.mid, source.qid, self.rid)
            value = source.peek()
            source.propose_pop(self.rid)
            self.target.propose_push(self.rid, value)
        except NeedInput:
            e.abort_rule(self.rid)
            return
        e.complete_rule(self.rid)


class PeekPush(Move):
    def work_move(self):
        e = self.engine
        if not e.begin_rule(self.rid):
            return
        try:
            e.record_read(self.mid, self.source.qid, self.rid)
            self.target.propose_push(self.rid, self.source.peek())
        except NeedInput:
            e.abort_rule(self.rid)
            return
        e.complete_rule(self.rid)


class TwoRules:
    def __init__(self, mid, queues):
        self.mid, self.queues = mid, queues

    def Work(self):
        self.work_a()
        self.work_a()  # Same instance/arguments: deduplicated by begin_rule.
        self.work_b()

    def work_a(self):
        if self.engine.begin_rule(1):
            self.queues[0].propose_push(1, 1)
            self.engine.complete_rule(1)

    def work_b(self):
        if self.engine.begin_rule(2):
            self.queues[1].propose_push(2, 2)
            self.engine.complete_rule(2)

    def arbitrate_a(self):
        return self.engine.arbitrate_rule(1)

    def arbitrate_b(self):
        return self.engine.arbitrate_rule(2)


class ModulePartial(TwoRules):
    def Work(self):
        self.work_a()
        self.engine.record_read(self.mid, self.queues[2].qid)
        self.queues[2].peek()
        self.work_b()


class Faulty(Drain):
    def work_drain(self):
        if not self.engine.begin_rule(self.rid):
            return
        self.queue.propose_pop(self.rid)
        if self.fault == "overlap":
            self.queue.propose_revise(self.rid, 2)
        elif self.fault != "unclosed":
            self.queue.propose_pop(self.rid)
        if self.fault != "unclosed":
            self.engine.complete_rule(self.rid)


class AtomicQueues:
    """Generated member example with several input/output Queues."""
    def __init__(self, mid, rid, queues, pops, pushes):
        self.mid, self.rid, self.queues = mid, rid, queues
        self.pops, self.pushes = pops, pushes

    def Work(self):
        self.work()

    def work(self):
        e = self.engine
        if not e.begin_rule(self.rid):
            return
        try:
            values = []
            for qid in self.pops:
                e.record_read(self.mid, qid, self.rid)
                values.append(self.queues[qid].peek())
                self.queues[qid].propose_pop(self.rid)
            value = tuple(values) if values else self.rid
            for qid in self.pushes:
                self.queues[qid].propose_push(self.rid, value)
        except NeedInput:
            e.abort_rule(self.rid)
            return
        e.complete_rule(self.rid)

    def arbitrate(self):
        return self.engine.arbitrate_rule(self.rid)


def pipeline(length, *, empty=False, delayed_sink=False, reference=False):
    queues = [Queue(initial=(7 if empty else i,) if not empty or i == 0 else ()) for i in range(length)]
    modules = [Move(i, i + 1, queues[i], queues[i + 1]) for i in range(length - 1)]
    rules = [None] + [RuleEntry(i, m.work_move, m.arbitrate_move, pops=(i,), pushes=(i + 1,))
                      for i, m in enumerate(modules)]
    if not empty:
        sink = Drain(length - 1, length, queues[-1], (lambda tick: tick == 1) if delayed_sink else None)
        modules.append(sink)
        rules.append(RuleEntry(sink.mid, sink.work_drain, sink.arbitrate_drain, pops=(length - 1,)))
    return assemble(queues, modules, rules, reference)


class SchedulingTests(unittest.TestCase):
    def test_module_required_read_stops_work_preserving_prior_independent_rules(self):
        queues = [Queue(), Queue(), Queue()]
        m = ModulePartial(0, queues)
        sim = assemble(queues, [m], [None,
            RuleEntry(0, m.work_a, m.arbitrate_a, pushes=(0,)),
            RuleEntry(0, m.work_b, m.arbitrate_b, pushes=(1,))])
        self.assertEqual(sim.step(), (1,))
        self.assertEqual(sim.snapshot(), ((1,), (), ()))
        self.assertEqual(sim.module_records[0].reads, {2})
        self.assertNotIn(2, sim.module_records[0].selected)

    def test_full_pipeline_consumer_first_and_one_firing(self):
        sim = pipeline(3)
        self.assertEqual(sim.step(), (3, 2, 1))
        self.assertEqual(sim.snapshot(), ((), (0,), (1,)))
        for counter in ("module_work", "rule_work", "accepted"):
            self.assertEqual(sim.stats[counter], 3)
        self.assertEqual(sim.stats["deltas"], 1)

    def test_no_same_tick_data_forwarding(self):
        sim = pipeline(3, empty=True)
        self.assertEqual(sim.step(), (1,))
        self.assertEqual(sim.snapshot(), ((), (7,), ()))
        self.assertEqual(sim.step(), (2,))
        self.assertEqual(sim.snapshot(), ((), (), (7,)))

    def test_late_activation_propagates_across_deltas(self):
        sim = pipeline(4, delayed_sink=True)
        self.assertEqual(sim.step(), ())
        before = sim.stats.copy()
        self.assertEqual(sim.step(wake=(3,)), (4, 3, 2, 1))
        self.assertEqual(sim.stats["deltas"] - before["deltas"], 4)
        self.assertEqual(sim.stats["rule_work"] - before["rule_work"], 1)
        self.assertEqual(sim.stats["cache_hits"] - before["cache_hits"], 3)

    def test_unspecified_port_competition_preserves_atomicity_across_deltas(self):
        def model():
            x, y = queues = [Queue(), Queue(initial=(9,))]
            consumer = Drain(0, 1, y, enabled=lambda tick: tick == 1)
            multi = AtomicQueues(1, 2, queues, (), (0, 1))
            single = Push(2, 3, (x,), lambda tick: (0, 30), enabled=lambda tick: tick == 1)
            return assemble(queues, [consumer, multi, single], [None,
                RuleEntry(0, consumer.work_drain, consumer.arbitrate_drain, pops=(1,)),
                RuleEntry(1, multi.work, multi.arbitrate, pushes=(0, 1)),
                RuleEntry(2, single.work_push, single.arbitrate_push, pushes=(0,))])

        for wake in ((0, 2), (0, 1, 2)):
            with self.subTest(wake=wake):
                sim = model()
                self.assertEqual(sim.step(), ())
                accepted = set(sim.step(wake=wake))
                self.assertIn(1, accepted)
                self.assertIn(accepted, ({1, 2}, {1, 3}))
                # No winner is specified; either outcome must commit atomically.
                expected = ((2,), (2,)) if 2 in accepted else ((30,), ())
                self.assertEqual(sim.snapshot(), expected)
                self.assertTrue(all(owner is None for q in sim.queues for owner in q.owners.values()))

    def test_partial_attempt_cleanup_and_read_empty_subscription(self):
        source, config, target = queues = [Queue(initial=(4,)), Queue(), Queue()]
        m = PartialMove(0, 1, source, target)
        m.config = config
        driver = Push(1, 2, (config,), lambda tick: (0, 9), lambda tick: tick == 0)
        sim = assemble(queues, [m, driver], [None,
            RuleEntry(0, m.work_move, m.arbitrate_move, pops=(0,), pushes=(2,)),
            RuleEntry(1, driver.work_push, driver.arbitrate_push, pushes=(1,))])
        sim.step()
        self.assertFalse(sim.rule_records[1].complete)
        self.assertEqual(source.proposal(1).status, "empty")
        self.assertEqual(sim.module_records[0].reads, {0, 1})
        self.assertIn(0, sim.next_modules)
        sim.step()
        self.assertEqual(target.current, ((4, 9),))

    def switch_model(self, with_stimuli=False):
        mode, old, new = queues = [Queue(initial=(0,)), Queue(), Queue()]
        m = ReadSwitch(0, mode, (old, new))
        setter = Revise(1, 1, (mode,), lambda tick: (1,))
        modules = [m, setter]
        rules = [None, RuleEntry(1, setter.work_revise, setter.arbitrate_revise, revises=(0,))]
        if with_stimuli:
            driver = Push(2, 2, (old, new), lambda tick: (tick - 2, 5), lambda tick: tick in (2, 3))
            modules.append(driver)
            rules.append(RuleEntry(2, driver.work_push, driver.arbitrate_push, pushes=(1, 2)))
        return assemble(queues, modules, rules)

    def test_read_generations_replace_branch_and_empty_try_peek(self):
        sim = self.switch_model()
        sim.step()
        sim.step()
        record = sim.module_records[0]
        self.assertEqual(record.reads, {0, 2})
        self.assertNotEqual(sim.queues[1].readers[0], record.read_gen)
        self.assertNotIn(0, sim._readers(1))
        self.assertIn(0, sim._readers(2))
        self.assertNotIn(1, sim.queues[1].readers)

    def test_changed_queue_ignores_stale_reader(self):
        sim = self.switch_model(with_stimuli=True)
        sim.step()
        sim.step()
        generation = sim.module_records[0].read_gen
        sim.step(wake=(2,))
        self.assertNotIn(0, sim.next_modules)
        self.assertEqual(sim.module_records[0].read_gen, generation)
        sim.step(wake=(2,))
        self.assertIn(0, sim.next_modules)

    def blocked_move(self):
        queues = [Queue(initial=(5,)), Queue(initial=(8,))]
        m = Move(0, 1, *queues)
        return assemble(queues, [m], [None,
            RuleEntry(0, m.work_move, m.arbitrate_move, pops=(0,), pushes=(1,))])

    def test_cached_reads_are_published_in_new_module_generation(self):
        sim = self.blocked_move()
        sim.step()
        sim.step(wake=(0,))
        record = sim.module_records[0]
        self.assertEqual(record.read_gen, 2)
        self.assertEqual(sim.queues[0].readers[0], 2)
        self.assertEqual(record.reads, {0})
        self.assertNotIn(1, sim.rule_records[1].deps)
        self.assertEqual(sim.stats["rule_work"], 1)
        self.assertEqual(sim.stats["cache_hits"], 1)

    def test_fixed_task_buffers_survive_empty_ticks_and_duplicate_wakes(self):
        sim = self.blocked_move()
        buffers = (*sim.module_tasks, *sim.rule_tasks, sim.next_modules)
        storage = [(id(b.ids), id(b.tags), len(b.ids), len(b.tags)) for b in buffers]
        sim.step()
        sim.step()  # No tasks; the following explicit wake must still enter once.
        before = sim.stats.copy()
        sim.step(wake=(0, 0, 0))
        self.assertEqual(sim.stats["module_work"] - before["module_work"], 1)
        self.assertEqual(sim.stats["arbitrations"] - before["arbitrations"], 1)
        self.assertEqual(sim.stats["cache_hits"] - before["cache_hits"], 1)
        self.assertEqual(storage, [(id(b.ids), id(b.tags), len(b.ids), len(b.tags)) for b in buffers])

    def test_waiter_swap_removal_survives_branch_cancellation_and_acceptance(self):
        queue = Queue(initial=(9,))
        a = Push(0, 1, (queue,), lambda tick: (0, 10), enabled=lambda tick: tick < 2)
        b = Push(1, 2, (queue,), lambda tick: (0, 20), enabled=lambda tick: tick == 0)
        c = Push(2, 3, (queue,), lambda tick: (0, 30))
        drain = Drain(3, 4, queue, enabled=lambda tick: tick >= 2)
        sim = assemble([queue], [a, b, c, drain], [None,
            *(RuleEntry(m.mid, m.work_push, m.arbitrate_push, pushes=(0,)) for m in (a, b, c)),
            RuleEntry(3, drain.work_drain, drain.arbitrate_drain, pops=(0,))])
        self.assertEqual(sim.step(), ())
        self.assertEqual(queue.waiters, [1, 2, 3])
        self.assertEqual(sim.step(wake=(1,)), ())  # Cancel the middle waiter.
        self.assertEqual(set(queue.waiters), {1, 3})
        for position, rid in enumerate(queue.waiters):
            self.assertEqual(sim.rule_records[rid].waiting_position, position)
        self.assertEqual(sim.rule_records[2].waiting_position, -1)
        self.assertEqual(sim.step(wake=(3,)), (4, 3))  # Cancel A, then accept moved C.
        self.assertEqual(queue.current, (30,))
        self.assertEqual(queue.waiters, [])
        self.assertTrue(all(r.waiting is None and r.waiting_position == -1
                            for r in sim.rule_records[1:]))

    def test_unselected_rule_discards_all_slots_and_wait_relationship(self):
        mode, left, right, blocked, free = queues = [Queue(initial=(0,)), Queue(initial=(1,)),
            Queue(initial=(2,)), Queue(initial=(99,)), Queue()]
        m = BranchMove(0, 1, 2, mode, (left, right), (blocked, free))
        setter = Revise(1, 3, (mode,), lambda tick: (1,))
        sim = assemble(queues, [m, setter], [None,
            RuleEntry(0, m.work_a, m.arbitrate_a, pops=(1,), pushes=(3,)),
            RuleEntry(0, m.work_b, m.arbitrate_b, pops=(2,), pushes=(4,)),
            RuleEntry(1, setter.work_revise, setter.arbitrate_revise, revises=(0,))])
        sim.step()
        self.assertTrue(sim.rule_records[1].complete)
        sim.step()
        self.assertFalse(sim.rule_records[1].complete)
        self.assertEqual(left.proposal(1).status, "empty")
        self.assertEqual(blocked.proposal(1).status, "empty")
        self.assertNotIn(1, blocked.waiters)
        self.assertEqual(free.current, (2,))

    def test_rule_dynamic_read_and_target_change_overwrite_all_old_operations(self):
        index, a, b, output = queues = [Queue(initial=(0,)), Queue(initial=(10,)),
                                       Queue(initial=(11,)), Queue(initial=(99,))]
        m = DynamicMove(0, 1, a, output)
        m.index, m.data = index, (a, b)
        setter = Revise(1, 2, (index,), lambda tick: (1,))
        sim = assemble(queues, [m, setter], [None,
            RuleEntry(0, m.work_move, m.arbitrate_move, pops=(1, 2), pushes=(3,)),
            RuleEntry(1, setter.work_revise, setter.arbitrate_revise, revises=(0,))])
        sim.step()
        sim.step()
        self.assertEqual(sim.rule_records[1].deps.keys(), {0, 2})
        self.assertEqual(a.proposal(1).status, "empty")
        self.assertEqual(output.proposal(1).push, 11)
        self.assertEqual(sim.stats["cache_invalidations"], 1)

    def test_atomic_failure_releases_reservations_for_competitor(self):
        source, blocked, free = queues = [Queue(initial=(1,)), Queue(initial=(8,)), Queue()]
        loser, winner = Move(0, 1, source, blocked), Move(1, 2, source, free)
        sim = assemble(queues, [loser, winner], [None,
            RuleEntry(0, loser.work_move, loser.arbitrate_move, pops=(0,), pushes=(1,)),
            RuleEntry(1, winner.work_move, winner.arbitrate_move, pops=(0,), pushes=(2,))])
        self.assertEqual(sim.step(), (2,))
        self.assertTrue(sim.rule_records[1].complete)
        self.assertEqual(blocked.current, (8,))
        self.assertEqual(free.current, (1,))
        self.assertTrue(all(owner is None for owner in source.owners.values()))
        self.assertEqual(sim.rule_records[2].accepted_tick, 0)
        sim.step()
        self.assertFalse(sim.rule_records[1].complete)

    def test_failed_temporary_pop_does_not_offer_capacity(self):
        source, blocked = queues = [Queue(initial=(1,)), Queue(initial=(2,))]
        consumer = Move(0, 1, source, blocked)
        producer = Push(1, 2, (source,), lambda tick: (0, 3))
        sim = assemble(queues, [consumer, producer], [None,
            RuleEntry(0, consumer.work_move, consumer.arbitrate_move, pops=(0,), pushes=(1,)),
            RuleEntry(1, producer.work_push, producer.arbitrate_push, pushes=(0,))])
        self.assertEqual(sim.step(), ())
        self.assertEqual(source.current, (1,))
        self.assertTrue(sim.rule_records[2].complete)
        self.assertTrue(all(owner is None for q in queues for owner in q.owners.values()))

    def parameter_model(self, value):
        queue = Queue(initial=(9,))
        m = Push(0, 1, (queue,), lambda tick: (0, value(tick)))
        return assemble([queue], [m], [None, RuleEntry(0, m.work_push, m.arbitrate_push, pushes=(0,))])

    def test_rule_parameters_invalidate_even_without_queue_reads(self):
        sim = self.parameter_model(lambda tick: tick)
        sim.step()
        sim.step(wake=(0,))
        self.assertEqual(sim.rule_records[1].deps, {})
        self.assertEqual(sim.queues[0].proposal(1).push, 1)
        self.assertEqual(sim.stats["cache_invalidations"], 1)

    def test_noop_revise_port_reset_wakes_waiter_without_version_change(self):
        register = Queue(initial=(0,))
        a, b = Revise(0, 1, (register,), lambda tick: (0,)), Revise(1, 2, (register,), lambda tick: (0,))
        sim = assemble([register], [a, b], [None,
            RuleEntry(0, a.work_revise, a.arbitrate_revise, revises=(0,)),
            RuleEntry(1, b.work_revise, b.arbitrate_revise, revises=(0,))])
        sim.step()
        self.assertEqual(register.state_version, 0)
        self.assertEqual(set(sim.next_modules), {1})
        self.assertEqual(sim.step(), (2,))
        self.assertEqual(sim.stats["module_work"], 3)
        self.assertEqual(sim.stats["cache_hits"], 1)
        self.assertNotIn(0, sim.next_modules)

    def test_self_pop_push_equal_value_changes_identity_and_invalidates_cache(self):
        queue, output = queues = [Queue(initial=(7,)), Queue(initial=(8,))]
        replace = Move(0, 1, queue, queue, enabled=lambda tick: tick == 0)
        reader = PeekPush(1, 2, queue, output)
        sim = assemble(queues, [replace, reader], [None,
            RuleEntry(0, replace.work_move, replace.arbitrate_move, pops=(0,), pushes=(0,)),
            RuleEntry(1, reader.work_move, reader.arbitrate_move, pushes=(1,))])
        sim.step()
        self.assertEqual(queue.current, (7,))
        self.assertEqual(queue.state_version, 1)
        sim.step()
        self.assertEqual(sim.stats["cache_invalidations"], 1)

    def test_revise_tail_then_pop_head_then_push(self):
        queue = Queue(2, (1, 2))
        revise, pop, push = Revise(0, 1, (queue,), lambda tick: (9,)), Drain(1, 2, queue), Push(2, 3, (queue,), lambda tick: (0, 3))
        sim = assemble([queue], [revise, pop, push], [None,
            RuleEntry(0, revise.work_revise, revise.arbitrate_revise, revises=(0,)),
            RuleEntry(1, pop.work_drain, pop.arbitrate_drain, pops=(0,)),
            RuleEntry(2, push.work_push, push.arbitrate_push, pushes=(0,))])
        sim.step()
        self.assertEqual(queue.current, (9, 3))

    def test_business_return_can_drop_input_with_full_output(self):
        queues = [Queue(initial=(0,)), Queue(initial=(9,))]
        m = Move(0, 1, *queues, drop_zero=True)
        sim = assemble(queues, [m], [None, RuleEntry(0, m.work_move, m.arbitrate_move, pops=(0,), pushes=(1,))])
        self.assertEqual(sim.step(), (1,))
        self.assertEqual(sim.snapshot(), ((), (9,)))

    def test_type_change_is_not_equal_value_or_equal_parameters(self):
        queue = Queue(initial=((0,),))
        m = Revise(0, 1, (queue,), lambda tick: ((False,),))
        sim = assemble([queue], [m], [None, RuleEntry(0, m.work_revise, m.arbitrate_revise, revises=(0,))])
        sim.step()
        self.assertEqual(queue.state_version, 1)
        sim = self.parameter_model(lambda tick: 0 if tick == 0 else False)
        sim.step()
        sim.step(wake=(0,))
        self.assertEqual(sim.stats["cache_invalidations"], 1)

    def test_invalid_models_and_duplicate_operations_report_errors(self):
        a, b = Queue(initial=(1,)), Queue(initial=(2,))
        x, y = Move(0, 1, a, b), Move(1, 2, b, a)
        with self.assertRaisesRegex(ValueError, "cyclic"):
            assemble([a, b], [x, y], [None,
                RuleEntry(0, x.work_move, x.arbitrate_move, pops=(0,), pushes=(1,)),
                RuleEntry(1, y.work_move, y.arbitrate_move, pops=(1,), pushes=(0,))])
        for fault, error in (("duplicate", ValueError), ("overlap", ValueError), ("undeclared", RuntimeError)):
            with self.subTest(fault=fault):
                queue = Queue(initial=(1,))
                m = Faulty(0, 1, queue)
                m.fault = fault
                sim = assemble([queue], [m], [None,
                    RuleEntry(0, m.work_drain, m.arbitrate_drain,
                              pops=() if fault == "undeclared" else (0,), revises=(0,))])
                with self.assertRaises(error):
                    sim.step()
                self.assertTrue(all(owner is None for owner in queue.owners.values()))
        with self.assertRaises(TypeError):
            Queue(initial=([1],))

    def test_unclosed_generated_rule_is_reported_and_cleaned(self):
        queue = Queue(initial=(1,))
        m = Faulty(0, 1, queue)
        m.fault = "unclosed"
        sim = assemble([queue], [m], [None, RuleEntry(0, m.work_drain, m.arbitrate_drain, pops=(0,))])
        with self.assertRaisesRegex(RuntimeError, "complete or abort"):
            sim.step()
        self.assertEqual(queue.proposal(1).status, "empty")

    def test_multiple_rule_members_of_one_module_and_duplicate_call(self):
        queues = [Queue(), Queue()]
        m = TwoRules(0, queues)
        sim = assemble(queues, [m], [None,
            RuleEntry(0, m.work_a, m.arbitrate_a, pushes=(0,)),
            RuleEntry(0, m.work_b, m.arbitrate_b, pushes=(1,))])
        self.assertEqual(sim.step(), (1, 2))
        self.assertEqual(sim.snapshot(), ((1,), (2,)))
        self.assertEqual(sim.stats["module_work"], 1)
        self.assertEqual(sim.stats["rule_work"], 2)
        self.assertEqual(sim.stats["rule_entries"], 3)
        self.assertEqual(sim.module_records[0].selected, [1, 2])
        self.assertEqual([r.call_args for r in sim.rule_records[1:]], [(), ()])

    def test_same_function_code_on_distinct_instances_and_preallocated_slots(self):
        queues = [Queue(initial=(10,)), Queue(), Queue(initial=(20,)), Queue()]
        a, b = Move(0, 1, *queues[:2]), Move(1, 2, *queues[2:])
        sim = assemble(queues, [a, b], [None,
            RuleEntry(0, a.work_move, a.arbitrate_move, pops=(0,), pushes=(1,)),
            RuleEntry(1, b.work_move, b.arbitrate_move, pops=(2,), pushes=(3,))])
        self.assertIs(sim.tables.rules[1].work.__func__, sim.tables.rules[2].work.__func__)
        self.assertIsNot(sim.tables.rules[1].work.__self__, sim.tables.rules[2].work.__self__)
        slot_ids = [[id(slot) for slot in q.slots] for q in queues]
        self.assertEqual([q.source_index for q in queues], [{1: 0}, {1: 0}, {2: 0}, {2: 0}])
        self.assertEqual(sim.step(), (1, 2))
        self.assertEqual(sim.snapshot(), ((), (10,), (), (20,)))
        self.assertEqual(slot_ids, [[id(slot) for slot in q.slots] for q in queues])
        self.assertTrue(all(slot.status == "empty" for q in queues for slot in q.slots))
        self.assertEqual(sim.module_records[0].reads, {0})
        self.assertEqual(sim.module_records[1].reads, {2})

    def test_queue_reads_are_pure_until_explicitly_recorded(self):
        sim = self.blocked_move()
        self.assertEqual(sim.queues[0].peek(), 5)
        self.assertFalse(sim.module_records[0].reads)
        self.assertFalse(sim.queues[0].readers)
        sim.step()
        self.assertEqual(sim.module_records[0].reads, {0})
        self.assertEqual(sim.rule_records[1].deps, {0: 0})


def random_model(reference, schedule):
    mode, index, bias = controls = [Queue(initial=(0,)) for _ in range(3)]
    inputs = [Queue(24, tuple(range(i * 30, i * 30 + 24))) for i in range(2)]
    outputs = [Queue(2, ((-1, i), (-2, i))) for i in range(2)]
    m = RandomProcessor(0, 1, 2, mode, index, bias, inputs, outputs)
    driver = Revise(1, 3, controls, lambda tick: schedule[tick][:3])
    sinks = [Drain(2 + i, 4 + i, output, lambda tick, i=i: schedule[tick][3 + i])
             for i, output in enumerate(outputs)]
    rules = [None,
        RuleEntry(0, m.work_a, m.arbitrate_a, pops=(3, 4), pushes=(5, 6)),
        RuleEntry(0, m.work_b, m.arbitrate_b, pops=(3, 4), pushes=(5, 6)),
        RuleEntry(1, driver.work_revise, driver.arbitrate_revise, revises=(0, 1, 2)),
        *(RuleEntry(s.mid, s.work_drain, s.arbitrate_drain, pops=(5 + i,)) for i, s in enumerate(sinks))]
    return assemble(controls + inputs + outputs, [m, driver, *sinks], rules, reference)


class DifferentialTests(unittest.TestCase):
    def test_independent_atomic_commit_model(self):
        rng = random.Random(8261)
        for case in range(300):
            nq, nr = rng.randrange(1, 9), rng.randrange(1, 13)
            capacities = [rng.randrange(1, 4) for _ in range(nq)]
            contents = tuple(tuple(rng.randrange(10) for _ in range(rng.randrange(c + 1)))
                             for c in capacities)
            queues = [Queue(c, v) for c, v in zip(capacities, contents)]
            modules, rules, operations = [], [None], {}
            for rid in range(1, nr + 1):
                split = rng.randrange(nq + 1)
                pops = tuple(q for q in range(split) if rng.random() < .3)
                pushes = tuple(q for q in range(split, nq) if rng.random() < .3)
                operations[rid] = pops, pushes
                m = AtomicQueues(rid - 1, rid, queues, pops, pushes)
                modules.append(m)
                rules.append(RuleEntry(m.mid, m.work, m.arbitrate, pops=pops, pushes=pushes))
            sim = assemble(queues, modules, rules)
            order = sorted(operations, key=sim.tables.rank.__getitem__)
            versions = [0] * nq
            for tick in range(10):
                # Whole-candidate checks and functional commit: no runtime reserve/xfer calls.
                accepted, popped, pushed = [], set(), {}
                for rid in order:
                    pops, pushes = operations[rid]
                    if not (pops or pushes) or any(not contents[q] for q in pops):
                        continue
                    if popped.intersection(pops) or pushed.keys() & set(pushes):
                        continue
                    if any(len(contents[q]) == capacities[q] and q not in pops and q not in popped
                           for q in pushes):
                        continue
                    accepted.append(rid)
                    popped.update(pops)
                    value = tuple(contents[q][0] for q in pops) if pops else rid
                    pushed.update((q, value) for q in pushes)
                versions = [v + int(q in popped or q in pushed) for q, v in enumerate(versions)]
                contents = tuple((c[1:] if q in popped else c) + ((pushed[q],) if q in pushed else ())
                                 for q, c in enumerate(contents))
                with self.subTest(case=case, tick=tick):
                    self.assertEqual(sim.step(wake=range(nr)), tuple(accepted))
                    self.assertEqual(sim.snapshot(), contents)
                    self.assertEqual([q.state_version for q in queues], versions)

    def test_fixed_seed_random_branches_backpressure_and_cache(self):
        for seed in range(12):
            rng = random.Random(seed)
            schedule = tuple((rng.randrange(2), rng.randrange(2), rng.randrange(5),
                              rng.random() < .25, rng.random() < .25) for _ in range(100))
            indexed, reference = random_model(False, schedule), random_model(True, schedule)
            for tick in range(len(schedule)):
                with self.subTest(seed=seed, tick=tick):
                    self.assertEqual(indexed.step((1, 2, 3)), reference.step((1, 2, 3)))
                    self.assertEqual(indexed.snapshot(), reference.snapshot())
                    self.assertEqual([q.state_version for q in indexed.queues],
                                     [q.state_version for q in reference.queues])
                    self.assertEqual(set(indexed.next_modules), set(reference.next_modules))
                    self.assertEqual([m.reads for m in indexed.module_records],
                                     [m.reads for m in reference.module_records])
                    self.assertTrue(all(owner is None for q in indexed.queues for owner in q.owners.values()))
                    self.assertTrue(all(p.status in ("pending", "empty") for q in indexed.queues for p in q.slots))


if __name__ == "__main__":
    unittest.main()
