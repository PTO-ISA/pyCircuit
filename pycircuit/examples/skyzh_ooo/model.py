"""Queue ROB, reservation stations, rename, three execution lanes and precise commit.

Every state change is an ordinary Rule proposal. The host only loads and observes.
"""
from pycircuit import ac
from .logic import *
from .execute import Integer


class CPUControl:
    epoch: ac.u64 = 1
    head: ac.u64 = 1
    stored: ac.u64 = 0
    target: ac.u32 = 0
    stopped: bool = False


class Front:
    epoch: ac.u64 = 0
    pc: ac.u32 = 0


class Tail:
    sequence: ac.u64 = 1
    epoch: ac.u64 = 0
    last_store: ac.u64 = 0


class Operand:
    ready: bool = False
    value: ac.u32 = 0
    producer: Tag


class Station:
    request: Request
    left: Operand
    right: Operand
    store_dependency: ac.u64 = 0


class MemoryMessage:
    request: Request
    result: Completion


class Predictor:
    history: ac.u32 = 0
    counters: ac.array[ac.u32, 4]


class Retirement:
    count: ac.u64 = 0
    tag: Tag
    pc: ac.u32 = 0
    word: ac.u32 = 0
    rd: ac.u32 = 0
    value: ac.u32 = 0
    address: ac.u32 = 0
    width: ac.u32 = 0
    store_value: ac.u32 = 0
    fault: ac.u32 = 0
    target: ac.u32 = 0
    redirect: bool = False
    halted: bool = False


def predictor_initial(index: ac.u32) -> Predictor:
    return Predictor(counters=[1, 1, 1, 1])


@ac.module
def Fetch(control, front, words, predictor):
    @ac.rule
    def fetch():
        ctl = control.value
        if ctl.stopped:
            return None
        state = front.value
        pc = state.pc if state.epoch == ctl.epoch else ctl.target
        request = Request(tag=Tag(epoch=ctl.epoch), pc=pc, predicted_next=pc + 4)
        if pc % 4 != 0 or pc // 4 >= len(words):
            request.fault = 1
        else:
            request.word = words[pc // 4]
            ins = decode(request.word)
            if ins.kind == BRANCH:
                prediction = predictor[(pc >> 2) & 63].value
                if prediction.counters[prediction.history] >= 2:
                    request.predicted_next = pc + ins.immediate
            elif ins.kind == JAL:
                request.predicted_next = pc + ins.immediate
        front.value = Front(epoch=ctl.epoch, pc=request.predicted_next)
        return request
    fetched = fetch()
    return fetched


@ac.module
def Dispatch(fetched, control, tail, rob, stations, rename, registers, results):
    s0 = stations[0]
    s1 = stations[1]
    s2 = stations[2]
    s3 = stations[3]
    s4 = stations[4]
    s5 = stations[5]
    s6 = stations[6]
    s7 = stations[7]
    s8 = stations[8]
    s9 = stations[9]

    @ac.rule
    def allocate(message):
        req = message.value
        ctl = control.value
        if req.tag.epoch != ctl.epoch or ctl.stopped:
            return None
        ins = decode(req.word)
        begin = ac.u32(4) if ins.kind == LOAD else ac.u32(7) if ins.kind == STORE else ac.u32(0)
        end = ac.u32(7) if ins.kind == LOAD else ac.u32(10) if ins.kind == STORE else ac.u32(4)
        selected = ac.u32(10)
        for i in range(begin, end):
            if selected == 10 and stations[i].empty():
                selected = i
        # Returning no outputs would still pop the input, so Work selects only
        # when there is a slot. This assertion records the invariant locally.
        assert selected < 10
        state = tail.value
        req.tag = Tag(epoch=ctl.epoch, sequence=state.sequence)
        left = Operand(ready=True)
        right = Operand(ready=True)
        if ins.rs1 != 0:
            tag = rename[ins.rs1].value
            left.value = registers[ins.rs1].value
            if tag.epoch == ctl.epoch and tag.sequence >= ctl.head:
                value = results[ac.u32(tag.sequence % 12)].value
                left = Operand(ready=value.tag == tag, value=value.value, producer=tag)
        if ins.rs2 != 0:
            tag = rename[ins.rs2].value
            right.value = registers[ins.rs2].value
            if tag.epoch == ctl.epoch and tag.sequence >= ctl.head:
                value = results[ac.u32(tag.sequence % 12)].value
                right = Operand(ready=value.tag == tag, value=value.value, producer=tag)
        station = Station(request=req, left=left, right=right,
                          store_dependency=state.last_store if state.epoch == ctl.epoch else ac.u64(0))
        if ins.rd != 0:
            rename[ins.rd].value = req.tag
        tail.value = Tail(sequence=state.sequence + 1, epoch=ctl.epoch,
                          last_store=state.sequence if ins.kind == STORE else station.store_dependency)
        return (req, station if selected == 0 else None, station if selected == 1 else None,
                station if selected == 2 else None, station if selected == 3 else None,
                station if selected == 4 else None, station if selected == 5 else None,
                station if selected == 6 else None, station if selected == 7 else None,
                station if selected == 8 else None, station if selected == 9 else None)

    if not fetched.empty():
        incoming = fetched.value
        ins = decode(incoming.word)
        begin = ac.u32(4) if ins.kind == LOAD else ac.u32(7) if ins.kind == STORE else ac.u32(0)
        end = ac.u32(7) if ins.kind == LOAD else ac.u32(10) if ins.kind == STORE else ac.u32(4)
        available = False
        for i in range(begin, end):
            available = available or stations[i].empty()
        if available or incoming.tag.epoch != control.value.epoch or control.value.stopped:
            rob, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9 = allocate(fetched)


@ac.module
def Wakeup(control, stations, results):
    @ac.rule
    def wake():
        ctl = control.value
        for i in range(10):
            if not stations[i].empty():
                slot = stations[i].value
                if slot.request.tag.epoch == ctl.epoch:
                    if not slot.left.ready:
                        done = results[ac.u32(slot.left.producer.sequence % 12)].value
                        if done.tag == slot.left.producer:
                            stations[i].value.left = Operand(ready=True, value=done.value, producer=done.tag)
                    if not slot.right.ready:
                        done = results[ac.u32(slot.right.producer.sequence % 12)].value
                        if done.tag == slot.right.producer:
                            stations[i].value.right = Operand(ready=True, value=done.value, producer=done.tag)
    wake()


@ac.module
def Issue(control, stations, begin, end, load_lane):
    @ac.rule
    def issue(message):
        slot = message.value
        if slot.request.tag.epoch == control.value.epoch and not control.value.stopped:
            req = slot.request
            req.left = slot.left.value
            req.right = slot.right.value
            return req
        return None

    ctl = control.value
    selected = ac.u32(10)
    oldest = ac.u64(0xffffffffffffffff)
    for i in range(begin, end):
        if not stations[i].empty():
            slot = stations[i].value
            stale = slot.request.tag.epoch != ctl.epoch or ctl.stopped
            ready = slot.left.ready and slot.right.ready
            ready = ready and (not load_lane or slot.store_dependency <= ctl.stored)
            if (stale or ready) and slot.request.tag.sequence < oldest:
                selected = i
                oldest = slot.request.tag.sequence
    if selected != 10:
        requested = issue(stations[selected])
    return requested


@ac.module
def LoadAddress(requests, control):
    @ac.rule
    def address(message):
        req = message.value
        if req.tag.epoch == control.value.epoch and not control.value.stopped:
            return MemoryMessage(request=req, result=calculate(req))
        return None
    addressed = address(requests)
    return addressed


@ac.module
def LoadRead(addressed, control, data):
    @ac.rule
    def read(message):
        packet = message.value
        if packet.request.tag.epoch == control.value.epoch and not control.value.stopped:
            ins = decode(packet.request.word)
            if packet.result.address // 4 >= len(data):
                packet.result.fault = 4
            if packet.result.fault == 0:
                word = data[packet.result.address // 4].value
                packet.result.value = load_value(word, packet.result.address, ins.width, ins.unsigned_load)
            return packet
        return None
    loaded = read(addressed)
    return loaded


@ac.module
def LoadReturn(loaded, control):
    @ac.rule
    def respond(message):
        packet = message.value
        if packet.request.tag.epoch == control.value.epoch and not control.value.stopped:
            return packet.result
        return None
    completed = respond(loaded)
    return completed


@ac.module
def Writeback(control, integer, load, store, results, clock, period, closed):
    completed = [integer, load, store]
    @ac.rule
    def write(message):
        result = message.value
        if result.tag.epoch == control.value.epoch and not control.value.stopped:
            results[ac.u32(result.tag.sequence % 12)].value = result
    if period == 0 or clock.value % period >= closed:
        selected = ac.u32(3)
        oldest = ac.u64(0xffffffffffffffff)
        for i in range(3):
            if not completed[i].empty():
                done = completed[i].value
                if done.tag.sequence < oldest:
                    selected = i
                    oldest = done.tag.sequence
        if selected != 3:
            write(completed[selected])


@ac.module
def Commit(rob, control, registers, data, results, retirement, predictor):
    @ac.rule
    def commit(message):
        req = message.value
        ctl = control.value
        if req.tag.epoch != ctl.epoch or ctl.stopped:
            return
        done = results[ac.u32(req.tag.sequence % 12)].value
        assert done.tag == req.tag
        ins = decode(req.word)
        retired = Retirement(count=retirement.value.count + 1, tag=req.tag, pc=req.pc, word=req.word,
                             value=done.value, target=done.target, fault=done.fault, redirect=done.redirect)
        if ins.kind == STORE:
            if done.address // 4 >= len(data):
                retired.fault = 4
            if retired.fault == 0:
                index = done.address // 4
                data[index].value = store_value(data[index].value, done.address, ins.width, done.store_value)
                retired.address = done.address
                retired.width = ins.width
                retired.store_value = done.store_value & (0xff if ins.width == 1 else 0xffff if ins.width == 2 else 0xffffffff)
                ctl.stored = req.tag.sequence
                if done.address == 0x30004 and (done.store_value & 255) != 0:
                    retired.halted = True
        elif ins.rd != 0 and retired.fault == 0:
            registers[ins.rd].value = done.value
            retired.rd = ins.rd
        if ins.kind == BRANCH:
            index = (req.pc >> 2) & 63
            pred = predictor[index].value
            taken = done.target != req.pc + 4
            count = pred.counters[pred.history]
            pred.counters[pred.history] = (count + 1 if count < 3 else 3) if taken else (count - 1 if count > 0 else 0)
            pred.history = ((pred.history << 1) | ac.u32(taken)) & 3
            predictor[index].value = pred
        ctl.head = req.tag.sequence + 1
        if done.redirect:
            ctl.epoch = ctl.epoch + 1
            ctl.target = done.target
            ctl.stored = 0
        if ins.kind == HALT or retired.fault != 0 or retired.halted:
            ctl.stopped = True
            retired.halted = True
        control.value = ctl
        retirement.value = retired

    if not rob.empty():
        req = rob.value
        ctl = control.value
        if req.tag.epoch != ctl.epoch or ctl.stopped:
            commit(rob)
        elif results[ac.u32(req.tag.sequence % 12)].value.tag == req.tag:
            commit(rob)


@ac.module
def Clock(control, clock):
    @ac.rule
    def tick():
        if not control.value.stopped:
            clock.value = clock.value + 1
    tick()


@ac.module
def CPU(words: ac.vector[ac.u32], initial_data: ac.vector[ac.u32],
        wb_period: ac.u64, wb_closed: ac.u64):
    control = ac.queue[CPUControl](initial=CPUControl())
    front = ac.queue[Front](initial=Front())
    tail = ac.queue[Tail](initial=Tail())
    rob = ac.queue[Request](capacity=12)
    stations = ac.array(ac.queue[Station], shape=(10,))
    registers = ac.array(ac.queue[ac.u32], shape=(32,), initial=0)
    rename = ac.array(ac.queue[Tag], shape=(32,), initial=Tag())
    results = ac.array(ac.queue[Completion], shape=(12,), initial=Completion())
    predictor = ac.array(ac.queue[Predictor], shape=(64,), initial=predictor_initial)
    data = [ac.queue[ac.u32](initial=value) for value in initial_data]
    retirement = ac.queue[Retirement](initial=Retirement())
    clock = ac.queue[ac.u64](initial=0)
    fetched = Fetch(control, front, words, predictor)
    dispatch = Dispatch(fetched, control, tail, rob, stations, rename, registers, results)
    wakeup = Wakeup(control, stations, results)
    # Fixed port lists preserve each lane's possible consumers in static IR.
    # Dynamic scanning and selection remain inside Issue.
    int_requests = Issue(control, [stations[0], stations[1], stations[2], stations[3]], 0, 4, False)
    load_requests = Issue(control, [stations[4], stations[5], stations[6]], 0, 3, True)
    store_requests = Issue(control, [stations[7], stations[8], stations[9]], 0, 3, False)
    int_completed = Integer(int_requests, control)
    store_completed = Integer(store_requests, control)
    addressed = LoadAddress(load_requests, control)
    loaded = LoadRead(addressed, control, data)
    load_completed = LoadReturn(loaded, control)
    writeback = Writeback(control, int_completed, load_completed, store_completed, results, clock, wb_period, wb_closed)
    commit = Commit(rob, control, registers, data, results, retirement, predictor)
    timer = Clock(control, clock)
