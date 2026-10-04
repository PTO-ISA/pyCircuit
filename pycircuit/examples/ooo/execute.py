from pycircuit import ac
from .types import *

@ac.module
def Integer(requests, control):
    @ac.rule(capacity=1)
    def execute(request):
        req = request.value
        if req.tag.epoch == control.value.epoch and not control.value.stopped:
            return calculate(req)
        return None
    completed = execute(requests)
    return completed

@ac.module
def Memory(requests, control, pending, data, data_base):
    @ac.rule(capacity=1)
    def execute(request):
        state = pending.value
        ctl = control.value
        if state.remaining != 0:
            if state.result.tag.epoch != ctl.epoch or ctl.stopped:
                pending.value = Pending()
            elif state.remaining == 1:
                pending.value = Pending()
                return state.result
            else:
                state.remaining = state.remaining - 1
                pending.value = state
        else:
            req = request.value
            if req.tag.epoch == ctl.epoch and not ctl.stopped:
                result = calculate(req)
                address = result.address
                valid = address >= data_base and (address - data_base) % 4 == 0
                valid = valid and (address - data_base) // 4 < len(data)
                if not valid:
                    result.fault = 3
                elif decode(req.word).op == LW:
                    result.value = data[(address - data_base) // 4].value
                # Accept, wait, publish: three execution ticks; no overlapping job.
                pending.value = Pending(2, result)
        return None
    completed = execute(requests)
    return completed

@ac.module
def Writeback(completed, control, entries, results, clock, wb_period, wb_closed):
    @ac.rule
    def writeback(message):
        result = message.value
        if result.tag.epoch == control.value.epoch and not control.value.stopped:
            if same(entries[slot(result.tag)].value.tag, result.tag):
                results[slot(result.tag)].value = result
    # Optional deterministic test port for completion backpressure.
    if wb_period == 0 or clock.value % ac.u64(wb_period) >= ac.u64(wb_closed):
        writeback(completed)
