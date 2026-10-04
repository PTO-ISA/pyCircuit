from pycircuit import ac
from .types import *

@ac.module
def Commit(control, entries, int_results, mem_results, registers, data, data_base, retirement, flush):
    @ac.rule
    def commit():
        ctl = control.value
        if not ctl.stopped:
            tag = Tag(ctl.epoch, ctl.head)
            entry = entries[slot(tag)].value
            result = mem_results[slot(tag)].value if memory_op(entry.ins.op) else int_results[slot(tag)].value
            if same(entry.tag, tag) and same(result.tag, tag):
                ins = entry.ins
                event = Retirement(count=retirement.value.count + 1, tag=tag, pc=entry.pc,
                                   word=entry.word, fault=result.fault, halt=ins.op == HALT)
                if result.fault == 0:
                    if ins.rd != 0:
                        registers[ins.rd].value = result.value
                        event.rd = ins.rd
                        event.value = result.value
                    if ins.op == SW:
                        data[(result.address - data_base) // 4].value = result.store_value
                        event.store = True
                        event.address = result.address
                        event.store_value = result.store_value
                retirement.value = event
                ctl.head = ctl.head + 1
                if ins.op == HALT or result.fault != 0:
                    ctl.stopped = True
                elif result.redirect:
                    ctl.epoch = ctl.epoch + 1
                    ctl.head = 1
                    ctl.target = result.target
                    flush.value = Flush(flush.value.count + 1, tag, result.target)
                control.value = ctl
    @ac.work
    def work():
        commit()

@ac.module
def Clock(control, clock):
    @ac.rule
    def tick():
        if not control.value.stopped:
            clock.value = clock.value + 1
    @ac.work
    def work():
        tick()
