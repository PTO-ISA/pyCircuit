"""Runtime for explicit generated methods and preconstructed ID-indexed tables."""

from collections import Counter
from dataclasses import dataclass, field


class NeedInput(Exception):
    """A required current element is absent."""


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


class IdList:
    """Fixed ID buffer and membership tags; clear advances a generation."""
    def __init__(self, capacity):
        self.ids, self.tags = [0] * capacity, [0] * capacity
        self.count, self.generation = 0, 1

    def __bool__(self):
        return self.count != 0

    def __iter__(self):
        return (self.ids[i] for i in range(self.count))

    def __contains__(self, identity):
        return self.tags[identity] == self.generation

    def add(self, identity):
        if identity not in self:
            self.tags[identity] = self.generation
            self.ids[self.count] = identity
            self.count += 1

    def extend(self, identities):
        for identity in identities:
            self.add(identity)

    def clear(self):
        self.count = 0
        self.generation += 1

    def sort(self, key=None):
        self.ids[:self.count] = sorted(self, key=key)


@dataclass(frozen=True)
class RuleEntry:
    module_id: int
    work: object
    arbitrate: object
    pops: tuple = ()
    pushes: tuple = ()
    revises: tuple = ()


@dataclass(frozen=True)
class StaticTables:
    module_work: tuple
    rules: tuple                 # rules[0] is unused; RuleId is the index.
    producers: tuple             # QueueId -> tuple of possible push RuleIds.
    rank: tuple                  # RuleId -> fixed arbitration priority.


@dataclass
class ModuleRecord:
    worked_tick: int = -1
    read_gen: int = 0
    reads: set = field(default_factory=set)
    selected: list = field(default_factory=list)
    previous_selected: list = field(default_factory=list)
    collecting: bool = False


@dataclass
class RuleRecord:
    deps: dict = field(default_factory=dict)
    participants: list = field(default_factory=list)
    args: tuple = ()
    call_args: tuple = ()
    selected_tick: int = -1
    complete: bool = False
    executing: bool = False
    prepared_epoch: object = None
    accepted_tick: int = -1
    waiting: object = None
    waiting_position: int = -1


@dataclass
class ProposalSlot:
    pop: bool = False
    push: object = None
    revise: object = None
    status: str = "empty"


def clear_slot(slot):
    slot.pop, slot.push, slot.revise, slot.status = False, None, None, "empty"


class Queue:
    def __init__(self, capacity=1, initial=()):
        if type(capacity) is not int or capacity < 1 or len(initial) > capacity:
            raise ValueError("invalid queue capacity or initial contents")
        self.capacity, self._current = capacity, [immutable(v) for v in initial]
        self.engine, self.qid = None, -1
        self.source_index, self.allowed_ops, self.slots = {}, (), []
        self.state_version = 0
        self.readers, self.waiters = {}, []
        self.owners = dict.fromkeys(("pop", "push", "revise"))
        self.accepted = []
        self.used_tick = -1

    @property
    def current(self):
        return tuple(self._current)

    def try_peek(self):
        return self._current[0] if self._current else None

    def peek(self):
        value = self.try_peek()
        if value is None:
            raise NeedInput()
        return value

    def size(self):
        return len(self._current)

    def empty(self):
        return self.size() == 0

    def full(self):
        return self.size() == self.capacity

    def proposal(self, rule_id):
        return self.slots[self.source_index[rule_id]]

    def _propose(self, rule_id, operation, value):
        if rule_id not in self.source_index:
            raise RuntimeError("undeclared Queue source")
        index = self.source_index[rule_id]
        record = self.engine.rule_records[rule_id]
        if not record.executing or operation not in self.allowed_ops[index]:
            raise RuntimeError("operation must belong to an executing, declared Rule")
        if operation != "push":
            mid = self.engine.tables.rules[rule_id].module_id
            self.engine.record_read(mid, self.qid, rule_id)  # Target dependency.
            if not self._current:
                raise NeedInput()
        slot = self.slots[index]
        if slot.status == "empty":
            slot.status = "pending"
            record.participants.append(self.qid)
        already_set = slot.pop if operation == "pop" else (
            slot.push is not None if operation == "push" else slot.revise is not None)
        if already_set:
            raise ValueError("duplicate operation on the same queue")
        if operation == "pop":
            slot.pop = True
        elif operation == "push":
            slot.push = value
        else:
            slot.revise = value

    def propose_pop(self, rule_id):
        self._propose(rule_id, "pop", True)

    def propose_push(self, rule_id, value):
        self._propose(rule_id, "push", immutable(value))

    def propose_revise(self, rule_id, value):
        self._propose(rule_id, "revise", immutable(value))

    def reserve(self, rule_id):
        self.engine.stats["reservation_checks"] += 1
        slot = self.proposal(rule_id)
        operations = [op for op, present in zip(self.owners,
                      (slot.pop, slot.push is not None, slot.revise is not None)) if present]
        if len(self._current) == 1 and (
            (slot.pop and slot.revise is not None)
            or (slot.pop and self.owners["revise"] is not None)
            or (slot.revise is not None and self.owners["pop"] is not None)
        ):
            raise ValueError("revise/pop of the same old element is unsupported")
        if any(self.owners[op] is not None for op in operations):
            return False
        pop_owner = self.owners["pop"]
        accepted_pop = pop_owner is not None and self.proposal(pop_owner).status == "accepted"
        if slot.push is not None and not (
            len(self._current) < self.capacity or slot.pop or accepted_pop
        ):
            return False
        for op in operations:
            self.owners[op] = rule_id
        slot.status = "reserved"
        self.engine.stats["reservations"] += 1
        return True

    def release(self, rule_id):
        slot = self.proposal(rule_id)
        if slot.status == "reserved":
            for op, owner in self.owners.items():
                if owner == rule_id:
                    self.owners[op] = None
            slot.status = "pending"

    def accept(self, rule_id):
        assert self.proposal(rule_id).status == "reserved"
        self.proposal(rule_id).status = "accepted"
        self.accepted.append(rule_id)
        if self.used_tick != self.engine.tick:
            self.used_tick = self.engine.tick
            self.engine.used_queues.append(self.qid)

    def xfer(self):
        changed = False
        for rule_id in self.accepted:
            value = self.proposal(rule_id).revise
            if value is not None:
                changed |= not same_value(self._current[-1], value)
                self._current[-1] = value
        if self.owners["pop"] is not None:
            self._current.pop(0)
            changed = True
        if self.owners["push"] is not None:
            self._current.append(self.proposal(self.owners["push"]).push)
            changed = True  # Equal payloads still replace element identity.
        assert len(self._current) <= self.capacity
        self.state_version += bool(changed)
        for rule_id in self.accepted:
            clear_slot(self.proposal(rule_id))
        self.accepted.clear()
        self.owners = dict.fromkeys(self.owners)
        return changed


class Simulator:
    def __init__(self, queues, tables, module_records, rule_records, reference=False):
        self.queues, self.tables = tuple(queues), tables
        self.module_records, self.rule_records = module_records, rule_records
        self.reference, self.tick, self.delta = reference, 0, 0
        self.next_modules = IdList(len(module_records))
        self.next_modules.extend(range(len(module_records)))
        self.module_tasks = (IdList(len(module_records)), IdList(len(module_records)))
        self.rule_tasks = (IdList(len(rule_records)), IdList(len(rule_records)))
        self.used_queues, self.stats = [], Counter()
        for qid, queue in enumerate(queues):
            queue.engine, queue.qid = self, qid

    def record_read(self, module_id, queue_id, rule_id=None):
        module, queue = self.module_records[module_id], self.queues[queue_id]
        if module.worked_tick != self.tick:
            raise RuntimeError("read belongs to an inactive Module")
        if rule_id is not None:
            if self.tables.rules[rule_id].module_id != module_id:
                raise ValueError("read belongs to another Module's Rule")
            self.rule_records[rule_id].deps[queue_id] = queue.state_version
        module.reads.add(queue_id)
        if not module.collecting:
            queue.readers[module_id] = module.read_gen

    def _unwait(self, rule_id):
        record = self.rule_records[rule_id]
        if record.waiting is not None:
            waiters = self.queues[record.waiting].waiters
            last = waiters.pop()
            if record.waiting_position < len(waiters):
                waiters[record.waiting_position] = last
                self.rule_records[last].waiting_position = record.waiting_position
            record.waiting = None
            record.waiting_position = -1

    def _discard(self, rule_id):
        record = self.rule_records[rule_id]
        self._unwait(rule_id)
        for qid in record.participants:
            queue = self.queues[qid]
            assert queue.proposal(rule_id).status != "accepted"
            queue.release(rule_id)
            clear_slot(queue.proposal(rule_id))
        record.participants.clear()
        record.deps.clear()
        record.complete = record.executing = False

    def begin_rule(self, rule_id, args=()):
        self.stats["rule_entries"] += 1
        immutable(args)
        record = self.rule_records[rule_id]
        mid = self.tables.rules[rule_id].module_id
        module = self.module_records[mid]
        selected = record.selected_tick == self.tick
        if module.worked_tick != self.tick or (not module.collecting and not selected):
            raise RuntimeError("Rule was not selected by an active Module")
        if selected and not same_value(args, record.call_args):
            raise ValueError("one Rule instance cannot have multiple calls/parameters")
        if not selected:
            module.selected.append(rule_id)
            record.selected_tick, record.call_args = self.tick, args
        epoch = (self.tick, self.delta)
        if record.accepted_tick == self.tick or record.prepared_epoch == epoch:
            return False
        record.prepared_epoch = epoch
        candidate = record.complete and bool(record.participants)
        valid = candidate and same_value(args, record.args) and not self.reference
        if valid:
            for qid, version in record.deps.items():
                self.stats["version_checks"] += 1
                if self.queues[qid].state_version != version:
                    valid = False
                    break
        if valid:
            self.stats["cache_hits"] += 1
            for qid in record.deps:
                self.record_read(mid, qid)
            return False
        if candidate and not self.reference:
            self.stats["cache_invalidations"] += 1
        self._discard(rule_id)
        record.args, record.executing = args, True
        self.stats["rule_work"] += 1
        return True

    def complete_rule(self, rule_id):
        record = self.rule_records[rule_id]
        if not record.executing:
            raise RuntimeError("Rule is not executing")
        record.complete, record.executing = True, False

    def abort_rule(self, rule_id):
        self.stats["incomplete"] += 1
        self._discard(rule_id)  # Module's collected reads survive cancellation.

    def _closed(self, rule_id):
        if self.rule_records[rule_id].executing:
            self._discard(rule_id)
            raise RuntimeError("generated Rule must complete or abort before returning")

    def _run_module(self, mid):
        module = self.module_records[mid]
        module.selected, module.previous_selected = module.previous_selected, module.selected
        module.selected.clear()
        module.reads.clear()
        module.worked_tick, module.collecting = self.tick, True
        self.stats["module_work"] += 1
        try:
            self.tables.module_work[mid]()
        except NeedInput:
            self.stats["module_incomplete"] += 1
        except Exception:
            for rid in module.selected:
                if self.rule_records[rid].executing:
                    self._discard(rid)
            raise
        finally:
            module.collecting = False
        for rid in module.selected:
            self._closed(rid)
        for rid in module.previous_selected:
            if self.rule_records[rid].selected_tick != self.tick:
                self._discard(rid)
        module.previous_selected.clear()
        generation = module.read_gen + 1
        for qid in module.reads:
            self.queues[qid].readers[mid] = generation
        module.read_gen = generation

    def arbitrate_rule(self, rule_id):
        record = self.rule_records[rule_id]
        if not record.complete or not record.participants or record.accepted_tick == self.tick:
            return False
        self.stats["arbitrations"] += 1
        self._unwait(rule_id)
        try:
            for qid in record.participants:
                if not self.queues[qid].reserve(rule_id):
                    for participant in record.participants:
                        self.queues[participant].release(rule_id)
                    record.waiting = qid
                    record.waiting_position = len(self.queues[qid].waiters)
                    self.queues[qid].waiters.append(rule_id)
                    return False
        except Exception:
            for qid in record.participants:
                self.queues[qid].release(rule_id)
            raise
        for qid in record.participants:
            self.queues[qid].accept(rule_id)
        record.accepted_tick = self.tick
        self.stats["accepted"] += 1
        return True

    def _readers(self, qid):
        if self.reference:
            self.stats["module_scans"] += len(self.module_records)
            return [mid for mid, m in enumerate(self.module_records) if qid in m.reads]
        readers = self.queues[qid].readers
        self.stats["reader_checks"] += len(readers)
        return [mid for mid, gen in readers.items() if gen == self.module_records[mid].read_gen]

    def _waiters(self, qid):
        if self.reference:
            self.stats["rule_scans"] += len(self.rule_records) - 1
            return [rid for rid, r in enumerate(self.rule_records[1:], 1) if r.waiting == qid]
        waiters = self.queues[qid].waiters
        self.stats["waiter_checks"] += len(waiters)
        return waiters

    def _producers(self, qid):
        if self.reference:
            self.stats["rule_scans"] += len(self.tables.rules) - 1
            return [rid for rid, r in enumerate(self.tables.rules[1:], 1) if qid in r.pushes]
        producers = self.tables.producers[qid]
        self.stats["producer_checks"] += len(producers)
        return producers

    def step(self, wake=()):
        wake = tuple(wake)
        if any(type(mid) is not int or not 0 <= mid < len(self.module_records) for mid in wake):
            raise ValueError("invalid ModuleId")
        modules, spare_modules = self.module_tasks
        rules, spare_rules = self.rule_tasks
        for tasks in (*self.module_tasks, *self.rule_tasks):
            tasks.clear()
        modules.extend(self.next_modules)
        modules.extend(wake)
        self.next_modules.clear()
        self.delta, accepted = 0, []
        while modules or rules:
            self.stats["deltas"] += 1
            batch_modules, modules = modules, spare_modules
            batch_rules, rules = rules, spare_rules
            modules.clear()
            rules.clear()
            batch_modules.sort()
            for mid in batch_modules:
                module = self.module_records[mid]
                if module.worked_tick != self.tick:
                    self._run_module(mid)
                    batch_rules.extend(module.selected)
            batch_rules.sort(key=self.tables.rank.__getitem__)
            for rid in batch_rules:
                entry, record = self.tables.rules[rid], self.rule_records[rid]
                if record.accepted_tick == self.tick or record.selected_tick != self.tick:
                    continue
                if record.prepared_epoch != (self.tick, self.delta):
                    entry.work(*record.call_args)
                    self._closed(rid)
                if not record.complete or not record.participants or not entry.arbitrate():
                    continue
                assert record.accepted_tick == self.tick
                accepted.append(rid)
                for qid in record.participants:
                    if not self.queues[qid].proposal(rid).pop:
                        continue
                    for producer in self._producers(qid):
                        mid = self.tables.rules[producer].module_id
                        m, r = self.module_records[mid], self.rule_records[producer]
                        if m.worked_tick != self.tick:
                            modules.add(mid)
                        elif (producer not in batch_rules and r.selected_tick == self.tick and r.complete
                              and r.participants and r.accepted_tick != self.tick):
                            rules.add(producer)
            spare_modules, spare_rules = batch_modules, batch_rules
            self.delta += 1
        changed = [qid for qid in self.used_queues if self.queues[qid].xfer()]
        for qid in changed:  # Publish only after every Queue has committed.
            self.next_modules.extend(self._readers(qid))
        for qid in self.used_queues:
            self.next_modules.extend(self.tables.rules[rid].module_id for rid in self._waiters(qid))
        for rid in accepted:
            record = self.rule_records[rid]
            record.complete = False
            record.participants.clear()
            record.deps.clear()
        self.used_queues.clear()
        self.tick += 1
        self.stats["ticks"] += 1
        return tuple(accepted)

    def snapshot(self):
        return tuple(queue.current for queue in self.queues)
