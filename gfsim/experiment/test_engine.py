"""Run with: python3 -m unittest discover -s gfsim/experiment -v"""

import random
import unittest

from engine import Simulator


def transfer(sim, source, target, transform=lambda value: value, name=None):
    module = sim.module()
    def work():
        value = source.peek()
        source.pop()
        target.push(transform(value))
    rule = sim.rule(module, work, pops=(source,), pushes=(target,), name=name)
    module.work = rule
    return module, rule


def drain(sim, queue, enabled=lambda: True, name=None):
    module = sim.module()
    rule = sim.rule(module, queue.pop, pops=(queue,), name=name)
    module.work = lambda: rule() if enabled() else None
    return module, rule


def setter(sim, queue, value):
    module = sim.module()
    rule = sim.rule(module, queue.revise, revises=(queue,))
    module.work = lambda: rule(value)
    return module, rule


class SchedulingTests(unittest.TestCase):
    def test_module_required_read_stops_work_preserving_prior_independent_rules(self):
        sim = Simulator()
        first, second, missing = sim.queue(), sim.queue(), sim.queue()
        module = sim.module()
        before = sim.rule(module, lambda: first.push(1), pushes=(first,))
        after = sim.rule(module, lambda: second.push(2), pushes=(second,))
        def work():
            before()
            missing.peek()
            after()
        module.work = work
        self.assertEqual(sim.step(), (before.name,))
        self.assertEqual(sim.snapshot(), ((1,), (), ()))
        self.assertEqual(module.reads, {missing})
        self.assertNotIn(after, module.selected)

    def test_full_pipeline_consumer_first_and_one_firing(self):
        sim = Simulator()
        queues = [sim.queue(initial=(i,)) for i in range(3)]
        for i in range(2):
            transfer(sim, queues[i], queues[i + 1], name=f"move{i}")
        drain(sim, queues[-1], name="sink")
        self.assertEqual(sim.step(), ("sink", "move1", "move0"))
        self.assertEqual(sim.snapshot(), ((), (0,), (1,)))
        self.assertEqual(sim.stats["module_work"], 3)
        self.assertEqual(sim.stats["rule_work"], 3)
        self.assertEqual(sim.stats["accepted"], 3)
        self.assertEqual(sim.stats["deltas"], 1)

    def test_no_same_tick_data_forwarding(self):
        sim = Simulator()
        queues = [sim.queue(initial=(7,) if i == 0 else ()) for i in range(3)]
        for i in range(2):
            transfer(sim, queues[i], queues[i + 1], name=f"move{i}")
        self.assertEqual(sim.step(), ("move0",))
        self.assertEqual(sim.snapshot(), ((), (7,), ()))
        self.assertEqual(sim.step(), ("move1",))
        self.assertEqual(sim.snapshot(), ((), (), (7,)))

    def test_late_activation_propagates_across_deltas(self):
        sim = Simulator()
        queues = [sim.queue(initial=(i,)) for i in range(4)]
        for i in range(3):
            transfer(sim, queues[i], queues[i + 1], name=f"move{i}")
        sink, _ = drain(sim, queues[-1], lambda: sim.tick == 1, "sink")
        self.assertEqual(sim.step(), ())
        before = sim.stats.copy()
        self.assertEqual(sim.step(wake=(sink,)), ("sink", "move2", "move1", "move0"))
        self.assertEqual(sim.stats["deltas"] - before["deltas"], 4)
        self.assertEqual(sim.stats["rule_work"] - before["rule_work"], 1)
        self.assertEqual(sim.stats["cache_hits"] - before["cache_hits"], 3)

    def test_partial_attempt_cleanup_and_read_empty_subscription(self):
        sim = Simulator()
        source, config, target = sim.queue(initial=(4,)), sim.queue(), sim.queue()
        module = sim.module()
        def work():
            value = source.peek()
            source.pop()
            target.push((value, config.peek()))
        rule = sim.rule(module, work, pops=(source,), pushes=(target,))
        module.work = rule
        driver = sim.module()
        supply = sim.rule(driver, lambda: config.push(9), pushes=(config,))
        driver.work = lambda: supply() if sim.tick == 0 else None
        sim.step()
        self.assertFalse(rule.candidate)
        self.assertNotIn(rule, source.proposals)
        self.assertEqual(module.reads, {source, config})
        self.assertIn(module, sim.next_modules)
        sim.step()
        self.assertEqual(target.current, ((4, 9),))

    def test_read_generations_replace_branch_and_empty_try_peek(self):
        sim = Simulator()
        mode = sim.queue(initial=(0,))
        data = [sim.queue(), sim.queue()]
        module = sim.module(lambda: data[mode.peek()].try_peek())
        setter(sim, mode, 1)
        sim.step()
        sim.step()
        self.assertEqual(module.reads, {mode, data[1]})
        self.assertNotEqual(data[0].readers[module], module.read_gen)
        driver = sim.modules[1]
        self.assertNotIn(module, sim._readers(data[0]))
        self.assertIn(module, sim._readers(data[1]))
        self.assertNotIn(driver, data[0].readers)

    def test_changed_queue_ignores_stale_reader(self):
        sim = Simulator()
        mode = sim.queue(initial=(0,))
        old, new = sim.queue(), sim.queue()
        module = sim.module(lambda: (old if mode.peek() == 0 else new).try_peek())
        setter(sim, mode, 1)
        driver = sim.module()
        supply = sim.rule(driver, lambda which: (old, new)[which].push(5), pushes=(old, new))
        driver.work = lambda: supply(sim.tick - 2) if sim.tick in (2, 3) else None
        sim.step()
        sim.step()
        generation = module.read_gen
        sim.step(wake=(driver,))
        self.assertNotIn(module, sim.next_modules)
        self.assertEqual(module.read_gen, generation)
        sim.step(wake=(driver,))
        self.assertIn(module, sim.next_modules)

    def test_cached_reads_are_published_in_new_module_generation(self):
        sim = Simulator()
        source, target = sim.queue(initial=(5,)), sim.queue(initial=(8,))
        module, rule = transfer(sim, source, target)
        sim.step()
        sim.step(wake=(module,))
        self.assertEqual(module.read_gen, 2)
        self.assertEqual(source.readers[module], 2)
        self.assertEqual(module.reads, {source})
        self.assertNotIn(target, rule.deps)
        self.assertEqual(sim.stats["rule_work"], 1)
        self.assertEqual(sim.stats["cache_hits"], 1)

    def test_unselected_rule_discards_all_slots_and_wait_relationship(self):
        sim = Simulator()
        mode = sim.queue(initial=(0,))
        left, right = sim.queue(initial=(1,)), sim.queue(initial=(2,))
        blocked, free = sim.queue(initial=(99,)), sim.queue()
        module = sim.module()
        def move(source, target):
            value = source.peek()
            source.pop()
            target.push(value)
        a = sim.rule(module, lambda: move(left, blocked), pops=(left,), pushes=(blocked,))
        b = sim.rule(module, lambda: move(right, free), pops=(right,), pushes=(free,))
        module.work = lambda: (a if mode.peek() == 0 else b)()
        setter(sim, mode, 1)
        sim.step()
        self.assertTrue(a.candidate)
        sim.step()
        self.assertFalse(a.candidate)
        self.assertNotIn(a, left.proposals)
        self.assertNotIn(a, blocked.proposals)
        self.assertNotIn(a, blocked.waiters)
        self.assertEqual(free.current, (2,))

    def test_rule_dynamic_read_and_target_change_overwrite_all_old_operations(self):
        sim = Simulator()
        index = sim.queue(initial=(0,))
        data = [sim.queue(initial=(i + 10,)) for i in range(2)]
        output = sim.queue(initial=(99,))
        module = sim.module()
        def work():
            source = data[index.peek()]
            value = source.peek()
            source.pop()
            output.push(value)
        rule = sim.rule(module, work, pops=data, pushes=(output,))
        module.work = rule
        setter(sim, index, 1)
        sim.step()
        sim.step()
        self.assertEqual(rule.deps.keys(), {index, data[1]})
        self.assertNotIn(rule, data[0].proposals)
        self.assertEqual(output.proposals[rule].push, 11)
        self.assertEqual(sim.stats["cache_invalidations"], 1)

    def test_atomic_failure_releases_reservations_for_competitor(self):
        sim = Simulator()
        source, blocked, free = sim.queue(initial=(1,)), sim.queue(initial=(8,)), sim.queue()
        _, loser = transfer(sim, source, blocked, name="loser")
        _, winner = transfer(sim, source, free, name="winner")
        self.assertEqual(sim.step(), ("winner",))
        self.assertTrue(loser.candidate)
        self.assertEqual(blocked.current, (8,))
        self.assertEqual(free.current, (1,))
        self.assertTrue(all(owner is None for owner in source.owners.values()))
        self.assertEqual(winner.accepted_tick, 0)
        sim.step()
        self.assertFalse(loser.candidate)  # Its original pop target disappeared.

    def test_failed_temporary_pop_does_not_offer_capacity(self):
        sim = Simulator()
        source, blocked = sim.queue(initial=(1,)), sim.queue(initial=(2,))
        transfer(sim, source, blocked)
        producer = sim.module()
        rule = sim.rule(producer, lambda: source.push(3), pushes=(source,))
        producer.work = rule
        self.assertEqual(sim.step(), ())
        self.assertEqual(source.current, (1,))
        self.assertTrue(rule.candidate)
        self.assertTrue(all(owner is None for queue in sim.queues for owner in queue.owners.values()))

    def test_rule_parameters_invalidate_even_without_queue_reads(self):
        sim = Simulator()
        output = sim.queue(initial=(9,))
        module = sim.module()
        rule = sim.rule(module, output.push, pushes=(output,))
        module.work = lambda: rule(sim.tick)
        sim.step()
        sim.step(wake=(module,))
        self.assertEqual(rule.deps, {})
        self.assertEqual(output.proposals[rule].push, 1)
        self.assertEqual(sim.stats["cache_invalidations"], 1)

    def test_noop_revise_port_reset_wakes_waiter_without_version_change(self):
        sim = Simulator()
        register = sim.queue(initial=(0,))
        first, _ = setter(sim, register, 0)
        second, rule = setter(sim, register, 0)
        sim.step()
        self.assertEqual(register.state_version, 0)
        self.assertEqual(sim.next_modules, {second})
        self.assertEqual(sim.step(), (rule.name,))
        self.assertEqual(sim.stats["module_work"], 3)
        self.assertEqual(sim.stats["cache_hits"], 1)
        self.assertNotIn(first, sim.next_modules)

    def test_self_pop_push_equal_value_changes_identity_and_invalidates_cache(self):
        sim = Simulator()
        queue, output = sim.queue(initial=(7,)), sim.queue(initial=(8,))
        replace = sim.module()
        def work():
            value = queue.peek()
            queue.pop()
            queue.push(value)
        rule = sim.rule(replace, work, pops=(queue,), pushes=(queue,))
        replace.work = lambda: rule() if sim.tick == 0 else None
        reader = sim.module()
        cached = sim.rule(reader, lambda: output.push(queue.peek()), pushes=(output,))
        reader.work = cached
        sim.step()
        self.assertEqual(queue.current, (7,))
        self.assertEqual(queue.state_version, 1)
        sim.step()
        self.assertEqual(sim.stats["cache_invalidations"], 1)

    def test_revise_tail_then_pop_head_then_push(self):
        sim = Simulator()
        queue = sim.queue(2, (1, 2))
        setter(sim, queue, 9)
        drain(sim, queue)
        producer = sim.module()
        rule = sim.rule(producer, lambda: queue.push(3), pushes=(queue,))
        producer.work = rule
        sim.step()
        self.assertEqual(queue.current, (9, 3))

    def test_business_return_can_drop_input_with_full_output(self):
        sim = Simulator()
        source, output = sim.queue(initial=(0,)), sim.queue(initial=(9,))
        module = sim.module()
        def work():
            value = source.peek()
            source.pop()
            if value == 0:
                return
            output.push(value)
        rule = sim.rule(module, work, pops=(source,), pushes=(output,))
        module.work = rule
        self.assertEqual(sim.step(), (rule.name,))
        self.assertEqual(sim.snapshot(), ((), (9,)))

    def test_type_change_is_not_equal_value_or_equal_parameters(self):
        sim = Simulator()
        queue = sim.queue(initial=((0,),))
        setter(sim, queue, (False,))
        sim.step()
        self.assertEqual(queue.state_version, 1)
        sim = Simulator()
        queue = sim.queue(initial=(9,))
        module = sim.module()
        rule = sim.rule(module, queue.push, pushes=(queue,))
        module.work = lambda: rule(0 if sim.tick == 0 else False)
        sim.step()
        sim.step(wake=(module,))
        self.assertEqual(sim.stats["cache_invalidations"], 1)

    def test_invalid_models_and_duplicate_operations_report_errors(self):
        sim = Simulator()
        a, b = sim.queue(initial=(1,)), sim.queue(initial=(2,))
        transfer(sim, a, b)
        transfer(sim, b, a)
        with self.assertRaisesRegex(ValueError, "cyclic"):
            sim.step()
        for operation, error in (("duplicate", ValueError), ("overlap", ValueError), ("undeclared", RuntimeError)):
            with self.subTest(operation=operation):
                sim = Simulator()
                queue = sim.queue(initial=(1,))
                module = sim.module()
                def work():
                    queue.pop()
                    queue.revise(2) if operation == "overlap" else queue.pop()
                rule = sim.rule(module, work, pops=() if operation == "undeclared" else (queue,), revises=(queue,))
                module.work = rule
                with self.assertRaises(error):
                    sim.step()
                self.assertTrue(all(owner is None for owner in queue.owners.values()))
        with self.assertRaises(TypeError):
            Simulator().queue(initial=([1],))


def random_model(reference, schedule):
    sim = Simulator(reference)
    mode, index, bias = (sim.queue(initial=(0,)) for _ in range(3))
    inputs = [sim.queue(24, tuple(range(i * 30, i * 30 + 24))) for i in range(2)]
    outputs = [sim.queue(2, ((-1, i), (-2, i))) for i in range(2)]
    module = sim.module()
    def work(offset):
        i = index.peek()
        value = inputs[i].peek()
        inputs[i].pop()
        if (value + offset) % 7 == 0:
            return  # Complete discard path requires no output capacity.
        outputs[(value + offset) % 2].push((value, offset))
    rules = [sim.rule(module, work, pops=inputs, pushes=outputs) for _ in range(2)]
    module.work = lambda: rules[mode.peek()](bias.peek())
    driver = sim.module()
    def configure(m, i, b):
        mode.revise(m)
        index.revise(i)
        bias.revise(b)
    configure_rule = sim.rule(driver, configure, revises=(mode, index, bias))
    driver.work = lambda: configure_rule(*schedule[sim.tick][:3])
    sinks = [drain(sim, output, lambda i=i: schedule[sim.tick][3 + i])[0]
             for i, output in enumerate(outputs)]
    return sim, (driver, *sinks)


class DifferentialTests(unittest.TestCase):
    def test_fixed_seed_random_branches_backpressure_and_cache(self):
        for seed in range(12):
            rng = random.Random(seed)
            schedule = tuple((rng.randrange(2), rng.randrange(2), rng.randrange(5),
                              rng.random() < .25, rng.random() < .25) for _ in range(100))
            indexed, wake_indexed = random_model(False, schedule)
            reference, wake_reference = random_model(True, schedule)
            for tick in range(len(schedule)):
                with self.subTest(seed=seed, tick=tick):
                    self.assertEqual(indexed.step(wake_indexed), reference.step(wake_reference))
                    self.assertEqual(indexed.snapshot(), reference.snapshot())
                    self.assertEqual([q.state_version for q in indexed.queues],
                                     [q.state_version for q in reference.queues])
                    self.assertEqual({m.index for m in indexed.next_modules},
                                     {m.index for m in reference.next_modules})
                    self.assertTrue(all(owner is None for q in indexed.queues for owner in q.owners.values()))
                    self.assertTrue(all(p.status == "pending" for q in indexed.queues for p in q.proposals.values()))


if __name__ == "__main__":
    unittest.main()
