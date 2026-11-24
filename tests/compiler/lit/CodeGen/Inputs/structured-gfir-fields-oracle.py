"""Independent scalar four-state oracle; writes expected pre-commit snapshots."""
import sys


def byte(value):
    return f"{value:08b}"


def mux_unknown(left, right):
    return "".join(a if a == b and a != "x" else "x" for a, b in zip(left, right))


initial = byte(0xA5) + byte(0x3C) + byte(0x11) * 3
current = initial
previous_clock = 0
# Clock/reset/index/payload/tag. Unknown indices merge only compatible rows;
# masking the high bit proves the depth-three bound and leaves row two intact.
stimuli = [
    (0, 0, "00", byte(0x22), byte(0x41)),
    (1, 0, "00", byte(0x22), byte(0x42)),
    (1, 0, "01", byte(0x33), byte(0x43)),
    (0, 0, "01", byte(0x33), byte(0x44)),
    (1, 0, "01", byte(0x33), byte(0x45)),
    (0, 0, "xx", byte(0x31), byte(0x46)),
    (1, 0, "xx", byte(0x31), byte(0x47)),
    (0, 0, "zz", "zzzz0001", byte(0x48)),
    (1, 0, "zz", "zzzz0001", byte(0x49)),
    (0, 0, "10", byte(0x55), byte(0x4A)),
    (1, 1, "00", "xxxxxxxx", byte(0x4B)),
    (1, 0, "00", byte(0x66), byte(0x4C)),
    (0, 0, "11", "x1z00101", byte(0x4D)),
    (1, 0, "11", "x1z00101", byte(0x4E)),
    (0, 0, "00", byte(0x77), byte(0x4F)),
    (1, 0, "00", byte(0x77), byte(0x50)),
]
for clock, reset, raw, data, tag in stimuli:
    snapshot = current[:8] + tag + current[16:]
    rows = [snapshot[16 + i * 8 : 24 + i * 8] for i in range(3)]
    low = raw[1]
    for ordinal in range(2):
        if low in "01":
            if ordinal == int(low):
                rows[ordinal] = data
        else:
            rows[ordinal] = mux_unknown(data, rows[ordinal])
    candidate = current[:8] + data + "".join(rows)
    print(clock, reset, raw, data, tag, current, snapshot, candidate)
    if clock and not previous_clock:
        current = initial if reset else candidate
    previous_clock = clock
