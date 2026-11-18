# ruff: noqa: N802, N803, F823, F841 -- pyCircuit declarations and captured state.
"""Stable modeled-event heap with bounded native memory and serial maintenance.

Capacity is 1024 events. Each event has a 64-bit timestamp and opaque payload;
32-bit insertion order breaks equal-time ties. push/start_pop begin operations,
then busy remains true until the caller finishes maintenance using advance().
These solver microsteps do not advance the caller's modeled hardware time.
"""

import pycircuit as ac


@ac.struct
class Event:
    when: ac.u64
    payload: ac.u64


@ac.module
def EventQueue():
    # A row occupies 20 bytes. A 40-byte read fetches adjacent heap children.
    # The extra 20 bytes allow a read at the last row; only live rows are used.
    memory = ac.byte_mem[ac.bits[320]](depth=20500)
    count: ac.u16 = 0
    sequence: ac.u32 = 0
    phase: ac.u3 = 0
    position: ac.u16 = 0
    saved: ac.bits[160] = 0
    popped: ac.bits[160] = 0
    has_result: bool = False

    @ac.method
    def busy() -> bool:
        return phase != 0

    @ac.method
    def empty() -> bool:
        assert phase == 0, "event queue operation has completed"
        return count == 0

    @ac.method
    def size() -> ac.u16:
        assert phase == 0, "event queue operation has completed"
        return count

    @ac.method
    def push(when: ac.u64, payload: ac.u64) -> None:
        assert phase == 0, "event queue operation has completed"
        assert count < 1024, "event queue capacity"
        assert sequence != 4294967295, "event insertion order must not wrap"
        saved = ac.concat(when, sequence, payload)
        sequence = sequence + 1
        position = count
        phase = 1

    @ac.method
    def start_pop() -> None:
        assert phase == 0, "event queue operation has completed"
        assert count != 0, "event queue is nonempty"
        has_result = False
        phase = 2

    @ac.method
    def result() -> Event:
        assert phase == 0 and has_result, "event extraction has completed"
        return Event(when=popped[96:160], payload=popped[0:64])

    @ac.method
    def advance() -> None:
        if phase == 1:
            # Lift the parent into the hole, or install the incoming event.
            move: bool = False
            parent: ac.u16 = 0
            previous: ac.bits[160] = 0
            if position != 0:
                parent = (position - 1) >> 1
                address: ac.u16 = parent * 20
                row = memory.read(address[0:15])
                previous = row[0:160]
                move = saved[64:160] < previous[64:160]
            value = previous if move else saved
            padding: ac.bits[160] = 0
            address: ac.u16 = position * 20
            mask: ac.u40 = 1048575
            memory.write(address[0:15], ac.concat(padding, value), mask=mask)
            if move:
                position = parent
            else:
                count = count + 1
                phase = 0
        elif phase == 2:
            root_address: ac.u15 = 0
            row = memory.read(root_address)
            popped = row[0:160]
            if count == 1:
                count = 0
                has_result = True
                phase = 0
            else:
                phase = 3
        elif phase == 3:
            address: ac.u16 = (count - 1) * 20
            row = memory.read(address[0:15])
            saved = row[0:160]
            position = 0
            phase = 4
        elif phase == 4:
            remaining: ac.u16 = count - 1
            left: ac.u16 = (position << 1) + 1
            child: ac.u16 = left
            candidate: ac.bits[160] = 0
            move: bool = False
            if left < remaining:
                address: ac.u16 = left * 20
                pair = memory.read(address[0:15])
                candidate = pair[0:160]
                right: ac.u16 = left + 1
                other = pair[160:320]
                if right < remaining and other[64:160] < candidate[64:160]:
                    child = right
                    candidate = other
                move = candidate[64:160] < saved[64:160]
            value = candidate if move else saved
            padding: ac.bits[160] = 0
            address: ac.u16 = position * 20
            mask: ac.u40 = 1048575
            memory.write(address[0:15], ac.concat(padding, value), mask=mask)
            if move:
                position = child
            else:
                count = remaining
                has_result = True
                phase = 0
