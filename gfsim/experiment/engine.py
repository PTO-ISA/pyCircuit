"""Single-thread GFSim runtime. IDs, arrays and vectors mirror generated C++."""

from bisect import bisect_left
from dataclasses import dataclass, field
from heapq import heappop, heappush


class NeedInput(Exception):
    """Required old state is absent; generated code must abort this candidate."""


class CapacityCycle(RuntimeError):
    pass


def value_copy(value):
    record = isinstance(value, tuple) and isinstance(getattr(type(value), '_fields', None), tuple)
    if type(value) not in (int, bool, tuple) and not record:
        raise TypeError("expected a scalar or fixed immutable aggregate")
    if isinstance(value, tuple):
        for item in value:
            value_copy(item)
    return value  # Immutable aggregates have C++ value semantics without copying.


def same_value(a, b):
    if type(a) is not type(b):
        return False
    if isinstance(a, tuple):
        return len(a) == len(b) and all(same_value(x, y) for x, y in zip(a, b))
    return a == b


def assign_field(old, path, value):
    """Constant tuple indices stand for generated C++ member-pointer paths."""
    if not path:
        return value
    fields = list(old)
    fields[path[0]] = assign_field(fields[path[0]], path[1:], value)
    return tuple(fields) if type(old) is tuple else type(old)(*fields)


class IdTasks:
    def __init__(self, capacity):
        self.ids, self.tags = [0] * capacity, [0] * capacity
        self.count, self.cursor, self.generation = 0, 0, 1

    def add(self, identity):
        if self.tags[identity] != self.generation:
            self.tags[identity] = self.generation
            self.ids[self.count] = identity
            self.count += 1

    def __bool__(self):
        return self.cursor < self.count

    def take(self):
        identity = self.ids[self.cursor]
        self.cursor += 1
        return identity

    def clear(self):
        self.count = self.cursor = 0
        self.generation += 1


@dataclass(frozen=True)
class RuleEntry:
    module_id: int
    work: object
    arbitrate: object
    pops: tuple = ()
    pushes: tuple = ()
    revises: tuple = ()


@dataclass
class ModuleRecord:
    worked_tick: int = -1
    read_gen: int = 0
    selected: list = field(default_factory=list)
    previous: list = field(default_factory=list)


@dataclass
class RuleRecord:
    deps: list = field(default_factory=list)
    participants: list = field(default_factory=list)
    wake_requests: list = field(default_factory=list)
    candidate_args: tuple = ()
    call_args: tuple = ()
    complete: bool = False
    executing: bool = False
    selected_tick: int = -1
    accepted_tick: int = -1

    def has_effects(self):
        return bool(self.participants or self.wake_requests)


@dataclass
class Proposal:
    pop: bool = False
    push: object = None
    revises: list = field(default_factory=list)
    status: int = 0  # Empty, Pending, Accepted.

    def clear(self):
        self.pop, self.push, self.status = False, None, 0
        self.revises.clear()


@dataclass
class Stats:
    module_work: int = 0
    rule_work: int = 0
    cache_hits: int = 0
    version_checks: int = 0
    dep_searches: int = 0
    reader_checks: int = 0
    dfs_visits: int = 0
    capacity_edges: int = 0
    queue_checks: int = 0
    accepted: int = 0
    events: int = 0
    due_events: int = 0
    max_stack: int = 0


class Queue:
    def __init__(self, capacity=1, initial=()):
        if capacity < 1 or len(initial) > capacity:
            raise ValueError("invalid Queue capacity")
        self.capacity = capacity
        self.data = [None] * capacity
        for i, value in enumerate(initial):
            self.data[i] = value_copy(value)
        self.head, self.count = 0, len(initial)
        self.qid, self.engine, self.state_version = -1, None, 0
        self.readers, self.sources, self.slots = [], [], []
        self.pop_rule, self.push_rule = None, None
        self.accepted, self.accepted_pop, self.used_tick = [], False, -1

    @property
    def current(self):
        return tuple(self.data[(self.head + i) % self.capacity] for i in range(self.count))

    def empty(self):
        return self.count == 0

    def full(self):
        return self.count == self.capacity

    def size(self):
        return self.count

    def try_peek(self):
        return None if self.empty() else self.data[self.head]

    def peek(self):
        if self.empty():
            raise NeedInput()
        return self.data[self.head]

    def proposal(self, rid):
        index = bisect_left(self.sources, rid)
        if index == len(self.sources) or self.sources[index] != rid:
            raise ValueError("undeclared Queue source")
        return self.slots[index]

    def _prepare(self, rid, reads_target=False):
        e, record = self.engine, self.engine.rules[rid]
        if not record.executing:
            raise RuntimeError("proposal outside Rule Work")
        if reads_target:
            e.record_read(e.entries[rid].module_id, self.qid, rid)
            if self.empty():
                raise NeedInput()
        slot = self.proposal(rid)
        if slot.status == 0:
            record.participants.append(self.qid)
            slot.status = 1
        return slot

    def propose_pop(self, rid):
        slot = self._prepare(rid, True)
        if slot.pop:
            raise ValueError("duplicate pop in one Rule")
        slot.pop = True

    def propose_push(self, rid, value):
        slot = self._prepare(rid)
        if slot.push is not None:
            raise ValueError("duplicate push in one Rule")
        slot.push = value_copy(value)

    def propose_revise(self, rid, value, path=()):
        self._prepare(rid, True).revises.append((path, value_copy(value)))

    def has_push_space(self, slot):
        return not self.full() or self.accepted_pop or (slot.pop and not self.empty())

    def can_accept(self, rid):
        slot = self.proposal(rid)
        return (slot.status == 1 and (not (slot.pop or slot.revises) or not self.empty())
                and (slot.push is None or self.has_push_space(slot)))

    def accept(self, rid):
        slot = self.proposal(rid)
        slot.status = 2
        self.accepted_pop |= slot.pop
        self.accepted.append(rid)
        if self.used_tick != self.engine.tick:
            self.used_tick = self.engine.tick
            self.engine.used_queues.append(self.qid)

    def xfer(self):
        changed = False
        for rid in self.accepted:
            for path, value in self.proposal(rid).revises:
                tail = (self.head + self.count - 1) % self.capacity
                old = self.data[tail]
                self.data[tail] = assign_field(old, path, value)
                changed |= not same_value(old, self.data[tail])
        if self.accepted_pop:
            self.data[self.head] = None
            self.head = (self.head + 1) % self.capacity
            self.count -= 1
            changed = True
        for rid in self.accepted:
            slot = self.proposal(rid)
            if slot.push is not None:
                self.data[(self.head + self.count) % self.capacity] = slot.push
                self.count += 1
                changed = True
            slot.clear()
        self.state_version += int(changed)
        self.accepted.clear()
        self.accepted_pop = False
        return changed


class Simulator:
    def __init__(self, queues, modules, entries, cache=True):
        self.queues, self.module_objects, self.entries = queues, modules, entries
        self.modules = [ModuleRecord() for _ in modules]
        self.rules = [None] + [RuleRecord() for _ in entries[1:]]
        self.module_tasks, self.rule_tasks = IdTasks(len(modules)), IdTasks(len(entries))
        self.visited, self.visiting = [-1] * len(entries), [False] * len(entries)
        self.used_queues, self.events = [], []
        self.tick, self.active_module, self.cache = 0, None, cache
        self.stats = Stats()
        # Per-object counters are observation only, never drive scheduling.
        self.module_calls, self.rule_calls = [0] * len(modules), [0] * len(entries)
        self.last_accepted = ()
        self.failed = False
        self.observer = None  # Optional review trace; never consulted by scheduling.

    def record_read(self, mid, qid, rid=None):
        if mid != self.active_module:
            raise RuntimeError("read outside owning Module Work")
        queue, module = self.queues[qid], self.modules[mid]
        if self.observer is not None:
            self.observer.read(mid, qid, rid)
        queue.readers[mid] = module.read_gen + 1
        if rid is not None:
            record = self.rules[rid]
            for old_qid, _ in record.deps:
                self.stats.dep_searches += 1
                if old_qid == qid:
                    return
            record.deps.append((qid, queue.state_version))

    def _discard(self, rid):
        record = self.rules[rid]
        for qid in record.participants:
            self.queues[qid].proposal(rid).clear()
        record.deps.clear()
        record.participants.clear()
        record.wake_requests.clear()
        record.complete = record.executing = False

    def begin_rule(self, rid, args=()):
        record, entry = self.rules[rid], self.entries[rid]
        if entry.module_id != self.active_module:
            raise RuntimeError("Rule outside owning Module Work")
        if record.selected_tick == self.tick:
            return False  # Same-Rule, same-tick arguments are a model precondition.
        record.selected_tick, record.call_args = self.tick, value_copy(args)
        self.modules[entry.module_id].selected.append(rid)
        valid = (self.cache and record.complete and record.has_effects()
                 and same_value(args, record.candidate_args))
        if valid:
            for qid, version in record.deps:
                self.stats.version_checks += 1
                if self.queues[qid].state_version != version:
                    valid = False
                    break
        if valid:
            if self.observer is not None:
                self.observer.rule_call(rid, True)
            self.stats.cache_hits += 1
            for qid, _ in record.deps:
                self.record_read(entry.module_id, qid)
            return False
        if self.observer is not None:
            self.observer.rule_call(rid, False)
        self._discard(rid)
        record.candidate_args, record.executing = args, True
        self.stats.rule_work += 1
        self.rule_calls[rid] += 1
        return True

    def complete_rule(self, rid):
        self.rules[rid].complete, self.rules[rid].executing = True, False

    def abort_rule(self, rid):
        self._discard(rid)

    def request_wakeup(self, rid, mid, delay):
        if not self.rules[rid].executing or delay < 1:
            raise ValueError("future event requires executing Rule and positive delay")
        self.rules[rid].wake_requests.append((mid, delay))

    def _wakeup(self, mid, tick):
        heappush(self.events, (tick, mid))
        self.stats.events += 1

    def _work(self, mid):
        module = self.modules[mid]
        module.selected, module.previous = module.previous, module.selected
        module.selected.clear()
        module.worked_tick, self.active_module = self.tick, mid
        self.stats.module_work += 1
        self.module_calls[mid] += 1
        try:
            self.module_objects[mid].Work()
        except NeedInput:
            pass  # Prior independent Rules survive; attempted reads remain subscribed.
        self.active_module = None
        for rid in module.selected:
            if self.rules[rid].executing:
                raise RuntimeError("generated Rule omitted complete/abort")
            self.rule_tasks.add(rid)
        for rid in module.previous:
            if self.rules[rid].selected_tick != self.tick:
                self._discard(rid)
        module.previous.clear()
        module.read_gen += 1

    def _pending(self, rid):
        record = self.rules[rid]
        return record.complete and record.has_effects() and record.accepted_tick != self.tick

    def arbitrate_rule(self, rid):
        record = self.rules[rid]
        for qid in record.participants:
            self.stats.queue_checks += 1
            if not self.queues[qid].can_accept(rid):
                return False
        for qid in record.participants:
            self.queues[qid].accept(rid)
        record.accepted_tick = self.tick
        self.stats.accepted += 1
        for mid, delay in record.wake_requests:
            self._wakeup(mid, self.tick + delay)
        return True

    def _visit(self, root, accepted):
        if not self._pending(root) or self.visited[root] == self.tick:
            return
        stack = [(root, 0)]  # Frame: RuleId, next participant; C++ vector<Frame>.
        self.visited[root], self.visiting[root] = self.tick, True
        while stack:
            self.stats.max_stack = max(self.stats.max_stack, len(stack))
            rid, cursor = stack[-1]
            record = self.rules[rid]
            if cursor < len(record.participants):
                stack[-1] = (rid, cursor + 1)
                queue = self.queues[record.participants[cursor]]
                slot, child = queue.proposal(rid), queue.pop_rule
                if (slot.push is None or queue.has_push_space(slot) or child is None
                        or child == rid or not self._pending(child)
                        or not queue.proposal(child).pop):
                    continue
                self.stats.capacity_edges += 1
                if self.observer is not None:
                    self.observer.capacity_edge(rid, child, queue.qid)
                if self.visited[child] == self.tick:
                    if self.visiting[child]:
                        raise CapacityCycle(f"tick {self.tick}: capacity cycle {stack} -> {child}")
                    continue
                self.visited[child], self.visiting[child] = self.tick, True
                stack.append((child, 0))
                continue
            self.stats.dfs_visits += 1
            success = self.entries[rid].arbitrate()
            if self.observer is not None:
                self.observer.decision(rid, success)
            self.visiting[rid] = False
            stack.pop()
            if success:
                accepted.append(rid)
                for qid in record.participants:
                    queue = self.queues[qid]
                    producer = queue.push_rule
                    if (queue.proposal(rid).pop and producer is not None
                            and self._pending(producer)
                            and queue.proposal(producer).push is not None):
                        self.rule_tasks.add(producer)

    def step(self):
        if self.failed:
            raise RuntimeError("simulation has failed; continuation is forbidden")
        try:
            if self.observer is not None:
                self.observer.start()
            result = self._step()
            if self.observer is not None:
                self.observer.finish()
            return result
        except Exception as error:
            self.failed = True
            if self.observer is not None:
                self.observer.finish(error)
            raise

    def _step(self):
        if self.tick == 0:
            for mid in range(len(self.modules)):
                self.module_tasks.add(mid)
        while self.events and self.events[0][0] <= self.tick:
            _, mid = heappop(self.events)
            self.stats.due_events += 1
            self.module_tasks.add(mid)
        while self.module_tasks:
            self._work(self.module_tasks.take())
        if self.observer is not None:
            self.observer.prepared()
        accepted = []
        while self.rule_tasks:
            self._visit(self.rule_tasks.take(), accepted)
        changed = [qid for qid in self.used_queues if self.queues[qid].xfer()]
        for qid in changed:
            for mid, generation in enumerate(self.queues[qid].readers):
                self.stats.reader_checks += 1
                if generation and generation == self.modules[mid].read_gen:
                    self._wakeup(mid, self.tick + 1)
        for rid in accepted:
            self._discard(rid)
        self.used_queues.clear()
        self.module_tasks.clear()
        self.rule_tasks.clear()
        self.last_accepted = tuple(accepted)
        self.tick += 1
        return self.last_accepted

    def snapshot(self):
        return tuple(queue.current for queue in self.queues)
