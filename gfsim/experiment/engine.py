"""Single-threaded scheduling experiment; callbacks must be pure except proposals."""

from collections import Counter
from heapq import heappop, heappush


class NeedInput(Exception):
    """A required current element is absent; this attempt is incomplete."""


def immutable(value):
    if type(value) not in (int, bool, tuple):
        raise TypeError("values must be integers, booleans or immutable tuples")
    if isinstance(value, tuple):
        for item in value:
            immutable(item)
    return value


def same_value(left, right):
    if type(left) is not type(right):
        return False
    if isinstance(left, tuple):
        return len(left) == len(right) and all(same_value(a, b) for a, b in zip(left, right))
    return left == right


class Proposal:
    def __init__(self):
        self.pop = False
        self.push = self.revise = None
        self.status = "pending"


class Queue:
    def __init__(self, sim, capacity, initial, name):
        if type(capacity) is not int or capacity < 1 or len(initial) > capacity:
            raise ValueError("invalid queue capacity or initial contents")
        self.sim, self.capacity, self.name = sim, capacity, name
        self._current = [immutable(v) for v in initial]
        self.state_version = 0
        self.readers, self.proposals = {}, {}
        self.producers, self.waiters = set(), set()
        self.owners = dict.fromkeys(("pop", "push", "revise"))
        self.accepted = []

    @property
    def current(self):
        if self.sim.current_module is not None:
            raise RuntimeError("Work must use tracked queue reads")
        return tuple(self._current)

    def _read(self):
        module = self.sim.current_module
        if module is None:
            raise RuntimeError("queue reads require a Work context")
        module.reads.add(self)
        if not module.collecting:
            self.readers[module] = module.read_gen
        rule = self.sim.current_rule
        if rule is not None:
            rule.deps[self] = self.state_version

    def try_peek(self):
        self._read()
        return self._current[0] if self._current else None

    def peek(self):
        value = self.try_peek()
        if value is None:
            raise NeedInput()
        return value

    def size(self):
        self._read()
        return len(self._current)

    def empty(self):
        return self.size() == 0

    def full(self):
        return self.size() == self.capacity

    def _propose(self, operation, value):
        rule = self.sim.current_rule
        if rule is None or self not in rule.resources[operation]:
            raise RuntimeError("operation must belong to a declared Rule resource")
        if operation != "push":
            self._read()  # pop/revise targets are also computation dependencies.
            if not self._current:
                raise NeedInput()
        if rule not in self.proposals:
            self.proposals[rule] = Proposal()
            rule.participants.append(self)
        proposal = self.proposals[rule]
        already_set = proposal.pop if operation == "pop" else getattr(proposal, operation) is not None
        if already_set:
            raise ValueError("duplicate operation on the same queue")
        setattr(proposal, operation, value)

    def pop(self):
        self._propose("pop", True)

    def push(self, value):
        self._propose("push", immutable(value))

    def revise(self, value):
        self._propose("revise", immutable(value))

    def reserve(self, rule):
        self.sim.stats["reservation_checks"] += 1
        proposal = self.proposals[rule]
        operations = [op for op in self.owners
                      if (proposal.pop if op == "pop" else getattr(proposal, op) is not None)]
        if len(self._current) == 1 and (
            (proposal.pop and proposal.revise is not None)
            or (proposal.pop and self.owners["revise"] is not None)
            or (proposal.revise is not None and self.owners["pop"] is not None)
        ):
            raise ValueError("revise/pop of the same old element is unsupported")
        if any(self.owners[op] is not None for op in operations):
            return False
        pop_owner = self.owners["pop"]
        accepted_pop = (pop_owner is not None
                        and self.proposals[pop_owner].status == "accepted")
        if proposal.push is not None and not (
            len(self._current) < self.capacity or proposal.pop or accepted_pop
        ):
            return False
        for op in operations:
            self.owners[op] = rule
        proposal.status = "reserved"
        self.sim.stats["reservations"] += 1
        return True

    def release(self, rule):
        proposal = self.proposals.get(rule)
        if proposal is not None and proposal.status == "reserved":
            for op, owner in self.owners.items():
                if owner is rule:
                    self.owners[op] = None
            proposal.status = "pending"

    def accept(self, rule):
        assert self.proposals[rule].status == "reserved"
        self.proposals[rule].status = "accepted"
        self.accepted.append(rule)
        self.sim.used_queues.add(self)

    def xfer(self):
        changed = False
        for rule in self.accepted:
            value = self.proposals[rule].revise
            if value is not None:
                changed |= not same_value(self._current[-1], value)
                self._current[-1] = value
        if self.owners["pop"] is not None:
            self._current.pop(0)
            changed = True
        if self.owners["push"] is not None:
            self._current.append(self.proposals[self.owners["push"]].push)
            changed = True  # Element identity changes even for equal payloads.
        assert len(self._current) <= self.capacity
        self.state_version += bool(changed)
        for rule in self.accepted:
            del self.proposals[rule]
        self.accepted.clear()
        self.owners = dict.fromkeys(self.owners)
        return changed


class Module:
    def __init__(self, sim, work, name):
        self.sim, self.work, self.name = sim, work, name
        self.index = len(sim.modules)
        self.read_gen, self.worked_tick = 0, -1
        self.reads, self.selected = set(), {}
        self.collecting = False

    def run(self):
        old_selected = self.selected
        self.reads, self.selected = set(), {}
        self.worked_tick = self.sim.tick
        self.sim.stats["module_work"] += 1
        self.collecting, self.sim.current_module = True, self
        try:
            self.work()
        except NeedInput:
            self.sim.stats["module_incomplete"] += 1
        finally:
            self.collecting, self.sim.current_module = False, None
        for rule in old_selected.keys() - self.selected.keys():
            rule.discard()
        generation = self.read_gen + 1
        for queue in self.reads:
            queue.readers[self] = generation
        self.read_gen = generation


class Rule:
    def __init__(self, module, work, name, pops, pushes, revises):
        self.module, self.sim, self.work, self.name = module, module.sim, work, name
        self.pops, self.pushes, self.revises = map(frozenset, (pops, pushes, revises))
        self.resources = dict(zip(("pop", "push", "revise"), (self.pops, self.pushes, self.revises)))
        self.deps, self.participants = {}, []
        self.candidate, self.args, self.waiting = False, (), None
        self.accepted_tick, self.prepared_epoch = -1, None

    def __call__(self, *args):
        if self.sim.current_module is not self.module or self.sim.current_rule is not None:
            raise RuntimeError("only the owning Module Work may call a Rule")
        immutable(args)
        if self in self.module.selected:
            if not same_value(args, self.module.selected[self]):
                raise ValueError("one Rule instance cannot have multiple calls/parameters")
            return
        self.module.selected[self] = args
        self.prepare(args)

    def unwait(self):
        if self.waiting is not None:
            self.waiting.waiters.remove(self)
            self.waiting = None

    def discard(self):
        self.unwait()
        for queue in self.participants:
            proposal = queue.proposals.get(self)
            assert proposal is None or proposal.status != "accepted"
            queue.release(self)
            queue.proposals.pop(self, None)
        self.participants.clear()
        self.deps.clear()
        self.candidate = False

    def prepare(self, args):
        self.prepared_epoch = (self.sim.tick, self.sim.delta)
        valid = self.candidate and same_value(args, self.args) and not self.sim.reference
        if valid:
            for queue, version in self.deps.items():
                self.sim.stats["version_checks"] += 1
                if queue.state_version != version:
                    valid = False
                    break
        if valid:
            self.sim.stats["cache_hits"] += 1
            self.module.reads.update(self.deps)
            if not self.module.collecting:
                for queue in self.deps:
                    queue.readers[self.module] = self.module.read_gen
            return
        if self.candidate and not self.sim.reference:
            self.sim.stats["cache_invalidations"] += 1
        self.discard()
        self.args = args
        self.sim.stats["rule_work"] += 1
        old_module = self.sim.current_module
        self.sim.current_module, self.sim.current_rule = self.module, self
        try:
            self.work(*args)
            self.candidate = bool(self.participants)
        except NeedInput:
            self.sim.stats["incomplete"] += 1
            self.discard()  # Module already collected even the failed read.
        except Exception:
            self.discard()
            raise
        finally:
            self.sim.current_module, self.sim.current_rule = old_module, None

    def arbitrate(self):
        self.sim.stats["arbitrations"] += 1
        self.unwait()
        try:
            for queue in self.participants:
                if not queue.reserve(self):
                    for participant in self.participants:
                        participant.release(self)
                    self.waiting = queue
                    queue.waiters.add(self)
                    return False
        except Exception:
            for queue in self.participants:
                queue.release(self)
            raise
        for queue in self.participants:
            queue.accept(self)
        self.accepted_tick = self.sim.tick
        self.sim.stats["accepted"] += 1
        return True


class Simulator:
    def __init__(self, reference=False):
        self.reference = reference
        self.queues, self.modules, self.rules = [], [], []
        self.tick, self.delta = 0, 0
        self.current_module = self.current_rule = None
        self.next_modules, self.used_queues = set(), set()
        self.stats, self.rank = Counter(), None

    def _register(self, collection, obj):
        if self.rank is not None:
            raise RuntimeError("registration is frozen after the first step")
        collection.append(obj)
        return obj

    def queue(self, capacity=1, initial=(), name=None):
        return self._register(self.queues, Queue(self, capacity, initial, name or f"q{len(self.queues)}"))

    def module(self, work=None, name=None):
        return self._register(self.modules, Module(self, work, name or f"m{len(self.modules)}"))

    def rule(self, module, work, *, pops=(), pushes=(), revises=(), name=None):
        if module.sim is not self:
            raise ValueError("Module belongs to another simulator")
        rule = Rule(module, work, name or f"r{len(self.rules)}", pops, pushes, revises)
        if any(q.sim is not self for q in rule.pops | rule.pushes | rule.revises):
            raise ValueError("Queue belongs to another simulator")
        self._register(self.rules, rule)
        for queue in rule.pushes:
            queue.producers.add(rule)
        return rule

    def _freeze(self):
        edges = [set() for _ in self.rules]
        indegree = [0] * len(self.rules)
        indices = {rule: i for i, rule in enumerate(self.rules)}
        for i, consumer in enumerate(self.rules):
            for queue in consumer.pops:
                for producer in queue.producers:
                    j = indices[producer]
                    if i != j and j not in edges[i]:
                        edges[i].add(j)
                        indegree[j] += 1
        ready, order = [], []
        for i, degree in enumerate(indegree):
            if degree == 0:
                heappush(ready, i)
        while ready:
            i = heappop(ready)
            order.append(self.rules[i])
            for j in edges[i]:
                indegree[j] -= 1
                if indegree[j] == 0:
                    heappush(ready, j)
        if len(order) != len(self.rules):
            raise ValueError("cyclic static capacity dependencies are unsupported")
        self.rank = {rule: i for i, rule in enumerate(order)}
        self.next_modules.update(self.modules)

    def _readers(self, queue):
        if self.reference:
            self.stats["module_scans"] += len(self.modules)
            return {m for m in self.modules if queue in m.reads}
        self.stats["reader_checks"] += len(queue.readers)
        return {m for m, gen in queue.readers.items() if gen == m.read_gen}

    def _waiters(self, queue):
        if self.reference:
            self.stats["rule_scans"] += len(self.rules)
            return {r for r in self.rules if r.waiting is queue}
        self.stats["waiter_checks"] += len(queue.waiters)
        return queue.waiters

    def _producers(self, queue):
        if self.reference:
            self.stats["rule_scans"] += len(self.rules)
            return {r for r in self.rules if queue in r.pushes}
        self.stats["producer_checks"] += len(queue.producers)
        return queue.producers

    def step(self, wake=()):
        wake = set(wake)
        if any(module.sim is not self for module in wake):
            raise ValueError("wake target belongs to another simulator")
        if self.rank is None:
            self._freeze()
        modules, rules = self.next_modules | wake, set()
        self.next_modules = set()
        self.delta, accepted = 0, []
        while modules or rules:
            self.stats["deltas"] += 1
            batch_modules, modules = modules, set()
            batch_rules, rules = rules, set()
            for module in sorted(batch_modules, key=lambda m: m.index):
                if module.worked_tick != self.tick:
                    module.run()
                    batch_rules.update(module.selected)
            for rule in sorted(batch_rules, key=self.rank.__getitem__):
                if rule.accepted_tick == self.tick or rule not in rule.module.selected:
                    continue
                if rule.prepared_epoch != (self.tick, self.delta):
                    rule.prepare(rule.module.selected[rule])
                if not rule.candidate or not rule.arbitrate():
                    continue
                accepted.append(rule)
                for queue in rule.pops & set(rule.participants):
                    if not queue.proposals[rule].pop:
                        continue
                    for producer in self._producers(queue):
                        module = producer.module
                        if module.worked_tick != self.tick:
                            modules.add(module)
                        elif (producer not in batch_rules and producer in module.selected
                              and producer.candidate and producer.accepted_tick != self.tick):
                            rules.add(producer)
            self.delta += 1
        changed = {queue for queue in self.used_queues if queue.xfer()}
        # Work never runs during Xfer; publish notifications after all commits.
        for queue in changed:
            self.next_modules.update(self._readers(queue))
        for queue in self.used_queues:
            self.next_modules.update(rule.module for rule in self._waiters(queue))
        for rule in accepted:
            rule.candidate = False
            rule.participants.clear()
            rule.deps.clear()
        self.used_queues.clear()
        self.tick += 1
        self.stats["ticks"] += 1
        return tuple(rule.name for rule in accepted)

    def snapshot(self):
        return tuple(queue.current for queue in self.queues)
