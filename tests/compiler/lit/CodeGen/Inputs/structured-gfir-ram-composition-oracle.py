"""Independent byte RAM, old-data response lifetime and optional FIFO model."""

import sys

mixed = sys.argv[1] == "fifo"
# clk reset active addr data mask late select-response explicit-discard
selected = [
    (0, 0, "0", "x", "zzzzzzzz", "z", "1", "0", 0),
    (1, 0, "0", "z", "xxxxxxxx", "x", "1", "0", 0),
    (0, 0, "1", "0", "00010001", "1", "0", "1", 0),
    (1, 0, "1", "0", "00010001", "1", "0", "1", 0),
    (0, 0, "1", "0", "00010001", "1", "1", "1", 0),
    (1, 0, "1", "0", "00010001", "1", "1", "1", 0),
    (0, 0, "1", "0", "00100010", "1", "x", "1", 0),
    (1, 0, "1", "0", "00100010", "1", "x", "1", 0),
    (1, 0, "1", "1", "00110011", "1", "z", "1", 0),
    (1, 0, "1", "0", "01000100", "1", "1", "1", 0),
    (1, 0, "0", "z", "xxxxxxxx", "z", "1", "1", 0),
    (0, 0, "0", "z", "xxxxxxxx", "z", "1", "1", 0),
    (1, 0, "0", "z", "xxxxxxxx", "z", "1", "1", 0),
    (0, 0, "0", "x", "zzzzzzzz", "x", "1", "1", 0),
    (1, 0, "0", "x", "zzzzzzzz", "x", "1", "1", 0),
    (0, 0, "1", "0", "01010101", "0", "1", "0", 0),
    (1, 0, "1", "0", "01010101", "0", "1", "0", 0),
    (0, 0, "1", "0", "01100110", "1", "1", "1", 0),
    (1, 0, "1", "0", "01100110", "1", "1", "1", 0),
    (0, 1, "0", "z", "xxxxxxxx", "z", "1", "0", 0),
    (1, 1, "0", "z", "xxxxxxxx", "z", "1", "0", 0),
    (1, 0, "0", "z", "xxxxxxxx", "z", "1", "1", 0),
    (0, 0, "1", "0", "01110111", "1", "1", "1", 0),
    (1, 0, "1", "0", "01110111", "1", "1", "1", 0),
    (0, 0, "0", "z", "xxxxxxxx", "z", "1", "1", 0),
    (1, 0, "1", "1", "10001000", "0", "1", "1", 0),
    (0, 0, "0", "x", "xxxxxxxx", "x", "1", "1", 0),
]
fifo = [
    (0, 0, "1", "0", "00010001", "1", "x", "0", 0),
    (1, 0, "1", "0", "00010001", "1", "x", "0", 0),
    (0, 0, "1", "0", "00100010", "1", "0", "1", 0),
    (1, 0, "1", "0", "00100010", "1", "0", "1", 0),
    (0, 0, "1", "0", "00100010", "1", "x", "1", 0),
    (1, 0, "1", "0", "00100010", "1", "x", "1", 0),
    (1, 0, "1", "0", "00110011", "1", "z", "1", 0),
    (1, 0, "1", "0", "01000100", "1", "1", "1", 0),
    (1, 0, "1", "0", "10011001", "1", "1", "1", 0),
    (0, 0, "0", "0", "xxxxxxxx", "1", "1", "1", 0),
    (1, 0, "0", "0", "xxxxxxxx", "1", "1", "1", 1),
    (1, 0, "0", "0", "xxxxxxxx", "1", "0", "1", 0),
    (0, 0, "0", "0", "xxxxxxxx", "1", "1", "1", 0),
    (1, 0, "0", "0", "xxxxxxxx", "1", "1", "1", 0),
    (0, 0, "0", "x", "xxxxxxxx", "z", "x", "1", 0),
    (1, 0, "0", "x", "xxxxxxxx", "z", "x", "1", 0),
    (0, 0, "0", "z", "xxxxxxxx", "x", "z", "1", 0),
    (1, 0, "0", "z", "xxxxxxxx", "x", "z", "1", 0),
    (0, 0, "1", "0", "01010101", "1", "1", "1", 0),
    (1, 0, "1", "0", "01010101", "1", "1", "1", 0),
    (0, 1, "1", "0", "01100110", "1", "1", "0", 0),
    (1, 1, "1", "0", "01100110", "1", "1", "0", 0),
    (1, 0, "1", "0", "01110111", "1", "1", "1", 0),
    (0, 0, "1", "0", "01110111", "1", "1", "1", 0),
    (1, 0, "1", "0", "01110111", "1", "1", "1", 0),
    (0, 0, "0", "0", "xxxxxxxx", "1", "1", "1", 0),
    (1, 0, "0", "0", "xxxxxxxx", "1", "1", "1", 0),
    (0, 0, "0", "x", "xxxxxxxx", "z", "x", "1", 0),
]
memory, queue = [0, 0], []
live, response, idle, count, history = False, 0, 0, 0, 0
stimuli = fifo if mixed else selected
for clock, reset, active, addr, data, mask, late, use, discard in stimuli:
    present = bool(queue)
    head = queue[0] if present else 0
    operation = (present if mixed else active == "1") and late == "1"
    ready = not present or operation
    valid = use == "0" or live
    value = 0x7E if use == "0" else response
    rising = clock and not history
    fault = rising and not reset and late in ("x", "z") and (not mixed or present)
    print(
        clock,
        reset,
        active,
        addr,
        data,
        mask,
        late,
        use,
        int(valid),
        f"{value:08b}" if valid else "00000000",
        f"{count:08b}",
        int(ready) if mixed else 0,
        int(present),
        f"{head:08b}",
        int(not fault),
        discard,
    )
    if fault or discard:
        continue
    if rising:
        if reset:
            queue.clear()
            live, idle, count = False, 0, 0
        else:
            if operation:
                address = int(addr)
                payload = head if mixed else int(data, 2)
                response = memory[address]  # Old data, before the same-edge write.
                live, idle = True, 0
                if mask == "1":
                    memory[address] = payload
                count = (count + 1) % 256
            elif live:
                idle += 1
                if idle >= 2:
                    live = False
            if mixed:
                if operation:
                    queue.pop(0)
                if active == "1" and ready:
                    queue.append(int(data, 2))
                assert len(queue) <= 1
    history = clock
