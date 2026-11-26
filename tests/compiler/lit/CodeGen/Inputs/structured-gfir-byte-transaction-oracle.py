"""Independent byte storage/clock oracle including failed sample rollback."""

memory = [0, 0, 0]
clock_history = 0
counter = 0
# clock reset before after raddr write waddr data mask
stimuli = [
    (0, 0, 1, 1, "0", 0, 0, 0, 3),
    (1, 0, 1, 1, "0", 1, 0, 0xABCD, 3),
    (0, 0, 1, 1, "0", 0, 0, 0, 3),
    (0, 0, 0, 1, "x", 0, 0, 0, 3),  # earlier false suppresses read
    (0, 0, 1, 0, "x", 0, 0, 0, 3),  # later false cannot erase reached read
    (0, 1, 1, 0, "z", 0, 0, 0, 3),  # even reset/off-edge validates read
    (1, 0, 1, 0, "z", 1, 0, 0xBEEF, 3),  # write/counter must roll back
    (1, 0, 1, 1, "0", 1, 0, 0xBEEF, 3),  # same high edge retry, old ABCD
    (0, 0, 1, 1, "0", 0, 0, 0, 3),
    (1, 0, 1, 1, "0", 1, 0, 0x1234, 1),
    (1, 0, 1, 1, "0", 1, 0, 0xFFFF, 3),  # held: no write
    (0, 0, 1, 1, "0", 0, 0, 0, 3),
    (1, 0, 1, 1, "0", 1, 0, 0x5600, 2),
    (0, 0, 1, 1, "0", 0, 0, 0, 3),
    (1, 0, 1, 1, "2", 1, 2, 0xCAFE, 3),  # trailing lane clips
    (0, 0, 1, 1, "1", 0, 0, 0, 3),
    (1, 0, 1, 1, "2", 0, 0, 0, 3),
    (0, 0, 1, 1, "3", 0, 0, 0, 3),
    (1, 0, 1, 1, "3", 1, 3, 0x9999, 3),  # fully OOB read/write
    (0, 0, 1, 1, "0", 0, 0, 0, 3),
    (1, 1, 1, 1, "0", 1, 0, 0xFFFF, 3),  # reset keeps RAM, blocks write
    (0, 0, 1, 1, "0", 0, 0, 0, 3),
    (1, 0, 1, 1, "0", 1, 0, 0xFFFF, 3),
    (0, 0, 1, 1, "0", 0, 0, 0, 3),  # data-dependent late guard blocks
]
for clock, reset, before, after, address, write, waddr, data, mask in stimuli:
    failed = bool(before and address in ("x", "z"))
    base = int(address, 16) if address not in ("x", "z") else 0
    observed = sum(
        memory[base + lane] << (8 * lane)
        for lane in range(2)
        if base + lane < len(memory)
    )
    available = bool(before and after and observed != 0xFFFF)
    inputs = [clock, reset, before, after]
    rest = [write, waddr, data, mask, int(available), observed, counter]
    print(
        "F" if failed else "C",
        *(f"{x:x}" for x in inputs),
        address,
        *(f"{x:x}" for x in rest),
    )
    if failed:
        continue
    if clock and not clock_history:
        if reset:
            counter = 0
        else:
            counter = (counter + 1) % 256
            if write:
                for lane in range(2):
                    if mask & (1 << lane) and waddr + lane < len(memory):
                        memory[waddr + lane] = (data >> (8 * lane)) & 255
    clock_history = clock
