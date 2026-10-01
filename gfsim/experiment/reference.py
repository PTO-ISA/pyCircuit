"""Test-only oracle: forward read sets, fresh Work, fixed-point acceptance.

Uses independent component transaction descriptions and only shares the
NeedInput/CapacityCycle signal types. No runtime storage, cache, DFS, acceptance,
notification or commit code is reused.
"""

from engine import NeedInput, CapacityCycle


class Queue:
    def __init__(self, capacity=1, initial=()):
        self.capacity, self.values = capacity, tuple(initial)
        self.state_version, self.qid, self.engine = 0, -1, None
        self.pop_rule, self.push_rule = None, None

    @property
    def current(self):
        return self.values

    def empty(self):
        return not self.values

    def full(self):
        return len(self.values) == self.capacity

    def size(self):
        return len(self.values)

    def try_peek(self):
        return self.values[0] if self.values else None

    def peek(self):
        if not self.values:
            raise NeedInput()
        return self.values[0]

    def _operation(self, rid, reads=False):
        if reads:
            self.engine.record_read(self.engine.entries[rid].module_id, self.qid, rid)
            if not self.values:
                raise NeedInput()
        return self.engine.candidates[rid]["ops"].setdefault(
            self.qid, {"pop": False, "push": None, "revise": []})

    def propose_pop(self, rid):
        self._operation(rid, True)["pop"] = True

    def propose_push(self, rid, value):
        self._operation(rid)["push"] = value

    def propose_revise(self, rid, value, path=()):
        self._operation(rid, True)["revise"].append((path, value))


class Reference:
    def __init__(self, queues, modules, entries, evaluate):
        self.queues, self.module_objects, self.entries = queues, modules, entries
        self.evaluate = evaluate  # Supplied by the circuit; no example imports here.
        self.tick, self.events = 0, []
        self.reads = [set() for _ in modules]
        self.selected = [set() for _ in modules]
        self.candidates, self.called = {}, set()
        self.module_calls, self.rule_calls = [0] * len(modules), [0] * len(entries)
        for qid, queue in enumerate(queues):
            queue.qid, queue.engine = qid, self
        for module in modules:
            module.engine = self
        for rid, entry in enumerate(entries[1:], 1):
            for qid in entry.pops:
                queues[qid].pop_rule = rid
            for qid in entry.pushes:
                queues[qid].push_rule = rid

    def record_read(self, mid, qid, rid=None):
        self.reads[mid].add(qid)

    def begin_rule(self, rid, args=()):
        if rid in self.called:
            return False
        self.called.add(rid)
        self.selected[self.entries[rid].module_id].add(rid)
        self.candidates[rid] = {"ops": {}, "events": [], "complete": False}
        self.rule_calls[rid] += 1
        return True

    def complete_rule(self, rid):
        self.candidates[rid]["complete"] = True

    def abort_rule(self, rid):
        self.candidates.pop(rid, None)

    def request_wakeup(self, rid, mid, delay):
        self.candidates[rid]["events"].append((mid, delay))

    def _pending(self, rid, accepted):
        c = self.candidates.get(rid)
        return c and c["complete"] and (c["ops"] or c["events"]) and rid not in accepted

    def _blocked(self, rid, popped):
        ops = self.candidates[rid]["ops"]
        return [qid for qid, op in ops.items() if op["push"] is not None
                and self.queues[qid].full() and not op["pop"] and qid not in popped]

    def step(self):
        active = set(range(len(self.module_objects))) if self.tick == 0 else set()
        active.update(mid for tick, mid in self.events if tick <= self.tick)
        self.events = [(tick, mid) for tick, mid in self.events if tick > self.tick]
        self.called = set()
        roots = set()
        for mid in sorted(active):
            previous = self.selected[mid]
            self.selected[mid], self.reads[mid] = set(), set()
            self.module_calls[mid] += 1
            try:
                self.evaluate(self, self.module_objects[mid])
            except NeedInput:
                pass
            for rid in previous - self.selected[mid]:
                self.candidates.pop(rid, None)
            roots.update(self.selected[mid])
        accepted, popped = set(), set()
        reachable = {rid for rid in roots if self._pending(rid, accepted)}
        while True:
            before = (len(reachable), len(accepted))
            for rid in list(reachable - accepted):
                for qid in self._blocked(rid, popped):
                    consumer = self.queues[qid].pop_rule
                    if self._pending(consumer, accepted):
                        op = self.candidates[consumer]["ops"].get(qid)
                        if op and op["pop"]:
                            reachable.add(consumer)
            for rid in sorted(reachable - accepted):
                if self._blocked(rid, popped):
                    continue
                accepted.add(rid)
                for qid, op in self.candidates[rid]["ops"].items():
                    if op["pop"]:
                        popped.add(qid)
                        producer = self.queues[qid].push_rule
                        if self._pending(producer, accepted):
                            pending = self.candidates[producer]["ops"].get(qid)
                            if pending and pending["push"] is not None:
                                reachable.add(producer)
            if before == (len(reachable), len(accepted)):
                break
        # Independent Kahn elimination on remaining necessary capacity edges.
        remaining = reachable - accepted
        edges = {rid: set() for rid in remaining}
        for rid in remaining:
            for qid in self._blocked(rid, popped):
                consumer = self.queues[qid].pop_rule
                if consumer in remaining:
                    op = self.candidates[consumer]["ops"].get(qid)
                    if op and op["pop"]:
                        edges[rid].add(consumer)
        while edges:
            leaves = {rid for rid, deps in edges.items() if not deps}
            if not leaves:
                raise CapacityCycle(f"tick {self.tick}: reference capacity cycle")
            edges = {rid: deps - leaves for rid, deps in edges.items() if rid not in leaves}
        updates = {}
        for rid in accepted:
            c = self.candidates[rid]
            for mid, delay in c["events"]:
                self.events.append((self.tick + delay, mid))
            for qid, op in c["ops"].items():
                updates.setdefault(qid, []).append(op)
        changed = set()
        for qid, operations in updates.items():
            queue = self.queues[qid]
            values, modified = list(queue.values), False
            for op in operations:
                for path, value in op["revise"]:
                    old = values[-1]
                    # Flattened path rebuilding independent of runtime assign_field.
                    parents, leaf = [], old
                    for index in path:
                        parents.append((leaf, index))
                        leaf = leaf[index]
                    new = value
                    for parent, index in reversed(parents):
                        new = parent[:index] + (new,) + parent[index + 1:]
                    modified |= new != old
                    values[-1] = new
            if any(op["pop"] for op in operations):
                values = values[1:]
                modified = True
            for op in operations:
                if op["push"] is not None:
                    values.append(op["push"])
                    modified = True
            queue.values = tuple(values)
            if modified:
                queue.state_version += 1
                changed.add(qid)
        for mid, reads in enumerate(self.reads):
            # Keep duplicate state events, as the spec deduplicates at Work entry.
            for _ in reads & changed:
                self.events.append((self.tick + 1, mid))
        for rid in accepted:
            del self.candidates[rid]
        self.tick += 1
        return tuple(sorted(accepted))

    def snapshot(self):
        return tuple(q.current for q in self.queues)
