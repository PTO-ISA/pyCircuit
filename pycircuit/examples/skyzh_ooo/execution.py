"""Execution lanes, operand forwarding and their reservation-station state."""
from .types import ac, Completion, CompletionLane, Station, Operand, LSUState
from .logic import alu_value, sign_extend, next_rob


class LSUEffect:
    complete: bool = False
    rob: ac.u32 = 0
    value: ac.u32 = 0
    address_valid: bool = False
    address: ac.u32 = 0
    advance: bool = False
    state: LSUState


class LSUEffects:
    lanes: ac.array[LSUEffect, 10]


@ac.signal
def alu(stations) -> Completion:
    result = Completion()
    for i in range(4):
        if not stations[i].empty():
            slot = stations[i].value
            if slot.left.tag == 0 and slot.right.tag == 0:
                result.lanes[i] = CompletionLane(complete=True, rob=slot.rob,
                                                value=alu_value(slot.op, slot.left.value, slot.right.value))
    return result


@ac.signal
def lsu(stations, stages, rob, head, memory) -> LSUEffects:
    result = LSUEffects()
    for i in range(4, 10):
        if not stations[i].empty():
            slot = stations[i].value
            state = stages[i].value
            effect = LSUEffect(rob=slot.rob, state=state)
            if state.phase == 0:
                if slot.left.tag == 0:
                    effect.address_valid = True
                    effect.address = slot.left.value + slot.address
                    effect.advance = True
                    effect.state.phase = 1
            elif i < 7:
                if slot.right.tag == 0:
                    effect.complete = True
                    effect.value = slot.right.value
                    effect.advance = True
                    effect.state.phase = 0
            elif state.phase == 1:
                blocked = False
                older = head.value
                for step in range(8):
                    if older != slot.rob:
                        entry = rob[older - 1].value
                        if entry.ins.opcode == 0x23 and entry.dest == slot.address:
                            blocked = True
                        older = next_rob(older)
                if not blocked:
                    address = slot.address
                    width = 1 << (slot.op & 3)
                    assert address + width <= 0x400000 and address % width == 0
                    page = memory[address >> 8].value
                    word = page.words[(address >> 2) & 63]
                    value = word >> ((address & 3) * 8)
                    if slot.op == 0 or slot.op == 4:
                        # Native aarch64 plain char is unsigned (known R05).
                        value = value & 255
                    elif slot.op == 1 or slot.op == 5:
                        value = value & 65535
                        if slot.op == 1:
                            value = sign_extend(value, 16)
                    effect.state.buffer = value
                    effect.state.phase = 2
                    effect.advance = True
            else:
                effect.complete = True
                effect.value = state.buffer
                effect.advance = True
                effect.state.phase = 0
            result.lanes[i] = effect
    return result


@ac.signal
def broadcast(integer, memory) -> Completion:
    result = Completion()
    alu_results = integer.value
    memory_results = memory.value
    for i in range(4):
        result.lanes[i] = alu_results.lanes[i]
    for i in range(4, 10):
        effect = memory_results.lanes[i]
        result.lanes[i] = CompletionLane(complete=effect.complete, rob=effect.rob,
                                        value=effect.value,
                                        address_valid=effect.address_valid and i < 7,
                                        address=effect.address if i < 7 else ac.u32(0))
    return result


def forwarded(slot: Station, completion: Completion) -> Station:
    result = slot
    for i in range(10):
        done = completion.lanes[i]
        if done.complete and done.rob != 0:
            if result.left.tag == done.rob:
                result.left = Operand(value=done.value)
            if result.right.tag == done.rob:
                result.right = Operand(value=done.value)
    return result


@ac.module
def ReservationStation(slot, stage, index, allocation, completion, memory_lanes, retirement):
    @ac.rule
    def update(previous) -> Station:
        issue = allocation.value
        events = completion.value
        effect = memory_lanes.value.lanes[index]
        commit = retirement.value
        if effect.advance:
            stage.value = effect.state
        # Flush overrides lane progress, completion and new allocation.
        if commit.flush:
            stage.value.phase = 0
            if not previous.empty():
                discarded = previous.value
            return None
        # Old occupied slots complete or absorb broadcasts; freed slots wait a tick.
        if not slot.empty():
            if events.lanes[index].complete:
                consumed = previous.value
            else:
                updated = forwarded(slot.value, events)
                if index >= 7 and effect.address_valid:
                    updated.address = effect.address
                slot.value = updated
            return None
        for i in range(issue.count):
            incoming = issue.allocations[i]
            if incoming.station == index:
                # Broadcast sees the tag assigned by this cycle's dispatch.
                return forwarded(incoming.slot, events)
        return None
    slot = update(slot)


@ac.module
def ExecutionCluster(stations, stages, rob, head, memory, allocation, retirement):
    integer = alu(stations)
    memory_lanes = lsu(stations, stages, rob, head, memory)
    completion = broadcast(integer, memory_lanes)
    for i in range(10):
        ReservationStation(stations[i], stages[i], i, allocation, completion, memory_lanes, retirement)
    return completion
