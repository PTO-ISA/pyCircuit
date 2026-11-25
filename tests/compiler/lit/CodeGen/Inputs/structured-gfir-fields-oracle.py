"""Independent scalar four-state oracle; writes expected pre-commit snapshots."""

WIDTH = 23


def word(value):
    bits = f"{value:08b}" if isinstance(value, int) else value
    return bits + "0" * (WIDTH - 16) + bits


def mux_unknown(left, right):
    return "".join(
        a if a == b and a != "x" else "x" for a, b in zip(left, right, strict=True)
    )


initial = word(0xA5) + word(0x3C) + word(0x11) * 3
current = initial
previous_clock = 0
# Clock/reset/index/payload/tag. Unknown indices merge only compatible rows;
# masking the high bit proves the depth-three bound and leaves row two intact.
stimuli = [
    (0, 0, "00", 0x22, 0x41),
    (1, 0, "00", 0x22, 0x42),
    (1, 0, "01", 0x33, 0x43),
    (0, 0, "01", 0x33, 0x44),
    (1, 0, "01", 0x33, 0x45),
    (0, 0, "xx", 0x31, 0x46),
    (1, 0, "xx", 0x31, 0x47),
    (0, 0, "zz", "zzzz0001", 0x48),
    (1, 0, "zz", "zzzz0001", 0x49),
    (0, 0, "10", 0x55, 0x4A),
    (1, 1, "00", "xxxxxxxx", 0x4B),
    (1, 0, "00", 0x66, 0x4C),
    (0, 0, "11", "x1z00101", 0x4D),
    (1, 0, "11", "x1z00101", 0x4E),
    (0, 0, "00", 0x77, 0x4F),
    (1, 0, "00", 0x77, 0x50),
]
for clock, reset, raw, data, tag in stimuli:
    data, tag = word(data), word(tag)
    snapshot = current[:WIDTH] + tag + current[2 * WIDTH :]
    rows = [snapshot[i * WIDTH : (i + 1) * WIDTH] for i in range(2, 5)]
    if "x" in raw or "z" in raw:
        selected, in_range = "x" * WIDTH, "x"
    else:
        index = int(raw, 2)
        selected = rows[index] if index < len(rows) else "x" * WIDTH
        in_range = "1" if index < len(rows) else "0"

    low = raw[1]
    for ordinal in range(2):
        if low in "01":
            if ordinal == int(low):
                rows[ordinal] = data
        else:
            rows[ordinal] = mux_unknown(data, rows[ordinal])
    candidate = current[:WIDTH] + data + "".join(rows)
    print(
        clock, reset, raw, data, tag, current, snapshot, candidate, selected, in_range
    )
    if clock and not previous_clock:
        current = initial if reset else candidate
    previous_clock = clock
