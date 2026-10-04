from pycircuit import ac
from .types import *

@ac.module
def Fetch(control, front, words):
    @ac.rule(capacity=1)
    def fetch():
        ctl = control.value
        if not ctl.stopped:
            current = front.value.pc if front.value.epoch == ctl.epoch else ctl.target
            valid = current % 4 == 0 and current // 4 < len(words)
            word = words[current // 4] if valid else ac.u32(0)
            front.value = Front(ctl.epoch, current + 4)
            return Fetched(ctl.epoch, current, word, 0 if valid else 1)
        return None
    fetched = fetch()
    return fetched

@ac.signal
def room(control, tail) -> bool:
    ctl = control.value
    end = tail.value
    return not ctl.stopped and end.epoch == ctl.epoch and end.sequence - ctl.head < 8

@ac.module
def Dispatch(fetched, control, tail, has_room, entries, rename, registers, int_results, mem_results):
    @ac.rule
    def dispatch(message):
        ctl = control.value
        end = tail.value
        if end.epoch != ctl.epoch:
            tail.value = Tail(ctl.epoch, 1)
        elif not ctl.stopped:
            # Consume stale fetches after the epoch reset; only current ones allocate.
            item = message.value
            if item.epoch == ctl.epoch and has_room.value:
                ins = decode(item.word)
                entry = Entry(tag=Tag(ctl.epoch, end.sequence), pc=item.pc,
                              word=item.word, fault=item.fault, ins=ins)
                if ins.op == INVALID:
                    entry.fault = 2 if entry.fault == 0 else entry.fault
                    entry.ins = Instruction()
                left = Operand()
                right = Operand()
                # A committed producer is read from the architectural register file.
                if ins.rs1 != 0:
                    producer = rename[ins.rs1].value
                    left.value = registers[ins.rs1].value
                    if producer.epoch == ctl.epoch and producer.sequence >= ctl.head:
                        a = int_results[slot(producer)].value
                        b = mem_results[slot(producer)].value
                        left = Operand(False, 0, producer)
                        if same(a.tag, producer):
                            left = Operand(True, a.value, producer)
                        elif same(b.tag, producer):
                            left = Operand(True, b.value, producer)
                if ins.rs2 != 0:
                    producer = rename[ins.rs2].value
                    right.value = registers[ins.rs2].value
                    if producer.epoch == ctl.epoch and producer.sequence >= ctl.head:
                        a = int_results[slot(producer)].value
                        b = mem_results[slot(producer)].value
                        right = Operand(False, 0, producer)
                        if same(a.tag, producer):
                            right = Operand(True, a.value, producer)
                        elif same(b.tag, producer):
                            right = Operand(True, b.value, producer)
                entry.left = left
                entry.right = right
                entries[slot(entry.tag)].value = entry
                if entry.ins.rd != 0:
                    rename[entry.ins.rd].value = entry.tag
                tail.value = Tail(ctl.epoch, end.sequence + 1)

    # Do not consume a current-epoch fetch when full. A redirect changes epoch
    # and resets tail before dispatch resumes; stale fetches are then drained.
    if tail.value.epoch != control.value.epoch or has_room.value:
        dispatch(fetched)
