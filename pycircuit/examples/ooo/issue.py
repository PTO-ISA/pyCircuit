from pycircuit import ac
from .types import *

@ac.module
def Wakeup(control, entries, operands, int_results, mem_results):
    @ac.rule
    def wake():
        ctl = control.value
        for i in range(8):
            entry = entries[i].value
            if live(entry, ctl):
                state = operands[i].value
                if not same(state.tag, entry.tag):
                    state = Operands(entry.tag, entry.left, entry.right)
                if not state.left.ready:
                    a = int_results[slot(state.left.producer)].value
                    b = mem_results[slot(state.left.producer)].value
                    if same(a.tag, state.left.producer):
                        state.left.value = a.value
                        state.left.ready = True
                    elif same(b.tag, state.left.producer):
                        state.left.value = b.value
                        state.left.ready = True
                if not state.right.ready:
                    a = int_results[slot(state.right.producer)].value
                    b = mem_results[slot(state.right.producer)].value
                    if same(a.tag, state.right.producer):
                        state.right.value = a.value
                        state.right.ready = True
                    elif same(b.tag, state.right.producer):
                        state.right.value = b.value
                        state.right.ready = True
                operands[i].value = state
    @ac.work
    def work():
        wake()

@ac.signal
def oldest_ready(control, entries, operands, issued, is_memory) -> Pick:
    ctl = control.value
    pick = Pick()
    lane = ac.u32(is_memory)
    for i in range(8):
        entry = entries[i].value
        state = operands[i].value
        ready = live(entry, ctl) and same(entry.tag, state.tag) and state.left.ready and state.right.ready
        ready = ready and ac.u32(memory_op(entry.ins.op)) == lane and not same(issued[i].value, entry.tag)
        if ready and entry.ins.op == LW:
            # No speculative loads and no store forwarding.
            for j in range(8):
                older = entries[j].value
                if live(older, ctl) and older.tag.sequence < entry.tag.sequence and older.ins.op == SW:
                    ready = False
        if ready and (not pick.valid or entry.tag.sequence < pick.sequence):
            pick = Pick(True, i, entry.tag.sequence)
    return pick

@ac.module
def Issue(control, entries, operands, issued, candidate):
    @ac.rule(capacity=1)
    def issue():
        selected = candidate.value
        if selected.valid and not control.value.stopped:
            entry = entries[selected.index].value
            state = operands[selected.index].value
            # This revise is committed atomically with the request push.
            issued[selected.index].value = entry.tag
            return Request(entry.tag, entry.pc, entry.word, entry.fault, state.left.value, state.right.value)
        return None
    @ac.work
    def work():
        request = issue()
    return request
