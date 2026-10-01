"""MEM: one-cycle accesses, or explicit request storage and timed completion."""

from .stage import Stage
from .records import Busy, Completed, LOAD, STORE, NO_MEMORY, ModelError


class Memory(Stage):
    def __init__(self, mid, rid, source, output, data, busy, latency):
        super().__init__(mid, rid)
        self.source, self.output, self.data = source, output, data
        self.busy, self.latency = busy, latency

    def arguments(self):
        return (self.engine.tick,)

    def finish(self, inst):
        value = inst.result
        if inst.memory != NO_MEMORY:
            if inst.address % 4 or inst.address // 4 >= len(self.data):
                raise ModelError(f'invalid data address 0x{inst.address:x} at PC 0x{inst.pc:x}')
            cell = self.data[inst.address // 4]
            if inst.memory == LOAD:
                value = self.observe(cell)
            else:
                cell.propose_revise(self.rid, inst.store_data)
        self.output.propose_push(self.rid, Completed(
            inst.pc, inst.word, inst.rd, value,
            inst.memory == STORE, inst.address if inst.memory == STORE else 0,
            inst.store_data if inst.memory == STORE else 0, inst.halt))

    def work_stage(self, now):
        busy = self.observe(self.busy)
        if busy.valid:
            if now < busy.due:
                return False               # The already-published event remains live.
            self.finish(busy.instruction)
            self.busy.propose_revise(self.rid, Busy())
            return True
        inst = self.take(self.source)
        if inst is None:
            return False
        if inst.memory != NO_MEMORY and self.latency > 1:
            # This start transaction has no capacity-dependent push; its event
            # and due time therefore share this tick as their acceptance anchor.
            self.busy.propose_revise(self.rid, Busy(True, now + self.latency - 1, inst))
            self.engine.request_wakeup(self.rid, self.mid, self.latency - 1)
        else:
            self.finish(inst)
        return True
