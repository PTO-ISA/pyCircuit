"""Memory: explicit generated-style Rule and runtime calls."""

from .records import Busy, Completed, LOAD, STORE, NO_MEMORY, ModelError


class Memory:
    def __init__(self, mid, rid, source, output, data, busy, latency):
        self.mid, self.rid, self.engine = mid, rid, None
        self.source = source
        self.output = output
        self.data = data
        self.busy = busy
        self.latency = latency

    def Work(self):
        self.work_memory(self.engine.tick)

    def work_memory(self, now):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid, (now,)):
            return

        busy = self.busy.peek()
        if busy.valid:
            if now < busy.due:
                e.abort_rule(rid)  # The already-published completion event remains live.
                return
            inst = busy.instruction
        else:
            inst = self.source.try_peek()
            if inst is None:
                e.abort_rule(rid)
                return
            self.source.propose_pop(rid)
            if inst.memory != NO_MEMORY and self.latency > 1:
                self.busy.propose_revise(rid, Busy(True, now + self.latency - 1, inst))
                e.request_wakeup(rid, self.mid, self.latency - 1)
                e.complete_rule(rid)
                return
        value = inst.result
        if inst.memory != NO_MEMORY:
            if inst.address % 4 or inst.address // 4 >= len(self.data):
                raise ModelError(f'invalid data address 0x{inst.address:x} at PC 0x{inst.pc:x}')
            cell = self.data[inst.address // 4]
            if inst.memory == LOAD:
                value = cell.peek()
            else:
                cell.propose_revise(rid, inst.store_data)
        self.output.propose_push(rid, Completed(
            inst.pc, inst.word, inst.rd, value,
            inst.memory == STORE, inst.address if inst.memory == STORE else 0,
            inst.store_data if inst.memory == STORE else 0, inst.halt))
        if busy.valid:
            self.busy.propose_revise(rid, Busy())

        e.complete_rule(rid)

    def arbitrate_memory(self):
        return self.engine.arbitrate_rule(self.rid)
