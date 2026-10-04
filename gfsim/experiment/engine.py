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
    resource_ids: tuple = ()
    rule_ids: tuple = ()
    control_reads: list = field(default_factory=list)
    rule_readers: list = field(default_factory=list)  # Flat [resource][64-bit word].
    dirty_words: list = field(default_factory=list)
    word_count: int = 0


@dataclass
class RuleRecord:
    read_slots: list = field(default_factory=list)
    word_index: int = 0
    bit: int = 0
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
    reader_lookups: int = 0
    read_registrations: int = 0
    read_clears: int = 0
    change_notifications: int = 0
    dirty_marks: int = 0
    reader_checks: int = 0
    dfs_visits: int = 0
    capacity_edges: int = 0
    queue_checks: int = 0
    accepted: int = 0
    events: int = 0
    due_events: int = 0
    max_stack: int = 0
    signal_work: int = 0
    signal_changes: int = 0


class ReadResource:
    """Shared read metadata; only Queue owns storage/proposal operations."""
    def __init__(self):
        self.resource_id, self.engine, self.state_version = -1, None, 0
        self.readers, self.reader_slots, self.module_slots = [], [], []

    def _record_read(self):
        e = self.engine
        if e is not None:
            if e.active_signal is not None:
                e._check_signal_input(self.resource_id)
            elif e.active_module is not None:
                e.record_read(e.active_module, self.resource_id, e.active_rule)


class Signal(ReadResource):
    """A cached pure function of Queue current; immutable throughout Work."""
    def __init__(self, helper):
        super().__init__()
        self.helper, self.sid = helper, -1
        self.input_qids, self.evaluations = (), 0
        self.dependents = {}  # Fixed Module -> Rule dirty mask (empty for Work only).
        self._value, self.initialized = None, False

    @property
    def value(self):
        self._record_read()
        if not self.initialized:
            raise RuntimeError("Signal read before initialization")
        return self._value


class Queue(ReadResource):
    def __init__(self, capacity=1, initial=()):
        super().__init__()
        if capacity < 1 or len(initial) > capacity:
            raise ValueError("invalid Queue capacity")
        self.capacity = capacity
        self.data = [None] * capacity
        for i, value in enumerate(initial):
            self.data[i] = value_copy(value)
        self.head, self.count = 0, len(initial)
        self.qid = -1
        self.sources, self.slots = [], []
        self.signal_readers = []  # Fixed SignalId links.
        self.pop_rule, self.push_rule = None, None
        self.accepted, self.accepted_pop, self.used_tick = [], False, -1

    @property
    def current(self):
        self._record_read()
        return tuple(self.data[(self.head + i) % self.capacity] for i in range(self.count))

    def empty(self):
        self._record_read()
        return self.count == 0

    def full(self):
        self._record_read()
        return self.count == self.capacity

    def size(self):
        self._record_read()
        return self.count

    def try_peek(self):
        self._record_read()
        return None if self.count == 0 else self.data[self.head]

    def peek(self):
        self._record_read()
        if self.count == 0:
            raise NeedInput()
        return self.data[self.head]

    def proposal(self, rid):
        index = bisect_left(self.sources, rid)
        if index == len(self.sources) or self.sources[index] != rid:
            raise ValueError("undeclared Queue source")
        return self.slots[index]

    def _prepare(self, rid, reads_target=False):
        e, record = self.engine, self.engine.rules[rid]
        if not record.executing or e.active_rule != rid:
            raise RuntimeError("proposal outside Rule Work")
        if reads_target:
            e.record_read(e.entries[rid].module_id, self.qid, rid)
            if self.count == 0:
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
        # Arbitration inspects storage directly; it does not subscribe to current.
        return self.count < self.capacity or self.accepted_pop or (slot.pop and self.count != 0)

    def can_accept(self, rid):
        slot = self.proposal(rid)
        return (slot.status == 1 and (not (slot.pop or slot.revises) or self.count != 0)
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
    def __init__(self, queues, modules, entries, cache=True, *, signals=()):
        self.queues, self.module_objects, self.entries = queues, modules, entries
        self.signals, self.resources = list(signals), list(queues) + list(signals)
        self.modules = [ModuleRecord() for _ in modules]
        self.rules = [None] + [RuleRecord() for _ in entries[1:]]
        self.module_tasks, self.rule_tasks = IdTasks(len(modules)), IdTasks(len(entries))
        self.visited, self.visiting = [-1] * len(entries), [False] * len(entries)
        self.used_queues, self.events = [], []
        self.tick, self.active_module, self.cache = 0, None, cache
        self.active_rule = None  # Work context only; never a persistent model signal.
        self.active_signal = None
        self.signal_tasks = IdTasks(len(signals))
        self.stats = Stats()
        # Per-object counters are observation only, never drive scheduling.
        self.module_calls, self.rule_calls = [0] * len(modules), [0] * len(entries)
        self.last_accepted = ()
        self.failed = False
        self.observer = None  # Optional review trace; never consulted by scheduling.

    def record_read(self, mid, resource_id, rid=None):
        if mid != self.active_module:
            raise RuntimeError("read outside owning Module Work")
        resource, module = self.resources[resource_id], self.modules[mid]
        if rid is not None:
            record = self.rules[rid]
            if self.entries[rid].module_id != mid or not record.executing or self.active_rule != rid:
                raise RuntimeError("read outside owning Rule Work")
        if isinstance(resource, Signal):
            if mid not in resource.dependents:
                raise ValueError("undeclared Module Signal access")
            if rid is not None:
                mask = resource.dependents[mid]
                if not mask or not mask[record.word_index] & record.bit:
                    raise ValueError("undeclared Rule Signal access")
            if self.observer is not None:
                self.observer.read(mid, resource_id, rid)
            return
        self.stats.reader_lookups += 1
        slot = resource.module_slots[mid]
        if slot < 0:
            raise ValueError(f"undeclared Module {type(resource).__name__} access")
        if self.observer is not None:
            self.observer.read(mid, resource_id, rid)
        if rid is None:
            module.control_reads[slot] = module.read_gen + 1
        else:
            offset = slot * module.word_count + record.word_index
            if not module.rule_readers[offset] & record.bit:
                module.rule_readers[offset] |= record.bit
                record.read_slots.append(slot)
                self.stats.read_registrations += 1

    def is_dirty(self, rid):
        record = self.rules[rid]
        return bool(self.modules[self.entries[rid].module_id].dirty_words[record.word_index]
                    & record.bit)

    def is_reader(self, mid, slot):
        module = self.modules[mid]
        generation = module.control_reads[slot]
        if generation and generation == module.read_gen:
            return True
        start = slot * module.word_count
        return any(module.rule_readers[start + word] for word in range(module.word_count))

    def _clear_reads(self, rid):
        record = self.rules[rid]
        module = self.modules[self.entries[rid].module_id]
        for slot in record.read_slots:
            module.rule_readers[slot * module.word_count + record.word_index] &= ~record.bit
            self.stats.read_clears += 1
        record.read_slots.clear()

    def _clear_candidate(self, rid):
        record = self.rules[rid]
        for qid in record.participants:
            self.queues[qid].proposal(rid).clear()
        record.participants.clear()
        record.wake_requests.clear()
        record.complete = record.executing = False

    def _cancel_rule(self, rid):
        self._clear_candidate(rid)
        self._clear_reads(rid)
        record = self.rules[rid]
        self.modules[self.entries[rid].module_id].dirty_words[record.word_index] &= ~record.bit

    def _notify_changed(self, resource_id):
        resource = self.resources[resource_id]
        if isinstance(resource, Signal):
            for mid in resource.dependents:
                self._wakeup(mid, self.tick + 1, changed_signal=resource_id)
            return
        for mid, slot in zip(resource.readers, resource.reader_slots):
            self.stats.reader_checks += 1
            self._wakeup(mid, self.tick + 1, changed_slot=slot)

    def _check_signal_input(self, qid):
        if qid >= len(self.queues):
            raise RuntimeError("Signal helpers cannot read another Signal")
        signal = self.signals[self.active_signal]
        slot = bisect_left(signal.input_qids, qid)
        if slot == len(signal.input_qids) or signal.input_qids[slot] != qid:
            raise ValueError("undeclared Signal Queue access")

    def _eval_signals(self, initial=False):
        while self.signal_tasks:
            sid = self.signal_tasks.take()
            signal = self.signals[sid]
            self.active_signal = sid
            try:
                value = value_copy(signal.helper())
            finally:
                self.active_signal = None
            changed = not signal.initialized or not same_value(signal._value, value)
            self.stats.signal_work += 1
            signal.evaluations += 1  # Observation only.
            if changed:
                signal._value, signal.initialized = value, True
                signal.state_version += 1
                self.stats.signal_changes += 1
                if not initial:
                    self._notify_changed(signal.resource_id)
            if self.observer is not None:
                self.observer.signal_eval(sid, changed)
        self.signal_tasks.clear()

    def begin_rule(self, rid, args=()):
        record, entry = self.rules[rid], self.entries[rid]
        if entry.module_id != self.active_module:
            raise RuntimeError("Rule outside owning Module Work")
        if self.active_rule is not None:
            raise RuntimeError("nested Rule Work is not supported")
        if record.selected_tick == self.tick:
            return False  # Same-Rule, same-tick arguments are a model precondition.
        record.selected_tick, record.call_args = self.tick, value_copy(args)
        module = self.modules[entry.module_id]
        module.selected.append(rid)
        dirty = module.dirty_words[record.word_index] & record.bit
        if not dirty and not same_value(args, record.candidate_args):
            self.stats.dirty_marks += 1
            module.dirty_words[record.word_index] |= record.bit
            dirty = record.bit
        valid = self.cache and record.complete and not dirty
        if valid:
            if self.observer is not None:
                self.observer.rule_call(rid, True)
            self.stats.cache_hits += 1
            return False
        if self.observer is not None:
            self.observer.rule_call(rid, False)
        self._clear_candidate(rid)
        self._clear_reads(rid)
        module.dirty_words[record.word_index] &= ~record.bit
        record.candidate_args, record.executing = args, True
        self.active_rule = rid
        self.stats.rule_work += 1
        self.rule_calls[rid] += 1
        return True

    def complete_rule(self, rid):
        if self.active_rule != rid:
            raise RuntimeError("complete outside owning Rule Work")
        self.rules[rid].complete, self.rules[rid].executing = True, False
        self.active_rule = None

    def abort_rule(self, rid):
        if self.active_rule != rid:
            raise RuntimeError("abort outside owning Rule Work")
        self._clear_candidate(rid)
        self.active_rule = None

    def request_wakeup(self, rid, mid, delay):
        if not self.rules[rid].executing or delay < 1:
            raise ValueError("future event requires executing Rule and positive delay")
        self.rules[rid].wake_requests.append((mid, delay))

    def _wakeup(self, mid, tick, changed_slot=None, *, changed_signal=None):
        if changed_signal is not None:
            module = self.modules[mid]
            for word, readers in enumerate(self.resources[changed_signal].dependents[mid]):
                self.stats.reader_checks += 1
                self.stats.dirty_marks += (readers & ~module.dirty_words[word]).bit_count()
                module.dirty_words[word] |= readers
            self.stats.change_notifications += 1
            if self.observer is not None:
                self.observer.invalidate(mid, changed_signal)
        if changed_slot is not None:
            module = self.modules[mid]
            generation = module.control_reads[changed_slot]
            live = bool(generation and generation == module.read_gen)
            start = changed_slot * module.word_count
            for word in range(module.word_count):
                readers = module.rule_readers[start + word]
                if readers:
                    live = True
                    self.stats.dirty_marks += (readers & ~module.dirty_words[word]).bit_count()
                    module.dirty_words[word] |= readers
            if not live:
                return
            self.stats.change_notifications += 1
            if self.observer is not None:
                self.observer.invalidate(mid, module.resource_ids[changed_slot])
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
        finally:
            self.active_rule = self.active_module = None
        for rid in module.selected:
            if self.rules[rid].executing:
                raise RuntimeError("generated Rule omitted complete/abort")
            self.rule_tasks.add(rid)
        for rid in module.previous:
            if self.rules[rid].selected_tick != self.tick:
                self._cancel_rule(rid)
        module.previous.clear()
        module.read_gen += 1

    def _pending(self, rid):
        record = self.rules[rid]
        return (record.complete and record.has_effects() and record.accepted_tick != self.tick
                and not self.is_dirty(rid))

    def arbitrate_rule(self, rid):
        if not self._pending(rid):
            return False
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
            for sid in range(len(self.signals)):
                self.signal_tasks.add(sid)
            if self.signals:
                self._eval_signals(initial=True)
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
            self._notify_changed(qid)
            for sid in self.queues[qid].signal_readers:
                self.signal_tasks.add(sid)
        if self.signal_tasks:
            self._eval_signals()
        for rid in accepted:
            self._clear_candidate(rid)
        self.used_queues.clear()
        self.module_tasks.clear()
        self.rule_tasks.clear()
        self.last_accepted = tuple(accepted)
        self.tick += 1
        return self.last_accepted

    def snapshot(self):
        return tuple(queue.current for queue in self.queues)
