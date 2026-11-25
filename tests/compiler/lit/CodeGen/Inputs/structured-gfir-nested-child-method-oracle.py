"""Two actual committed state objects and ordered candidate snapshots."""

total, rows, history = 0, [0, 0], 0
# Failed samples retain clock history, so the next high remains a rising edge.
stimuli = [
    (0, 0, "1", 0),
    (1, 0, "1", 0),
    (1, 0, "1", 1),
    (0, 0, "0", 1),
    (1, 0, "0", 1),
    (0, 0, "1", 1),
    (1, 0, "x", 1),
    (1, 0, "z", 0),
    (1, 0, "1", 1),
    (0, 0, "1", 0),
    (1, 1, "1", 0),
    (1, 0, "1", 1),
    (0, 0, "x", 0),
    (1, 1, "z", 0),
    (1, 0, "z", 1),
    (0, 0, "1", 1),
    (1, 0, "1", 1),
    (0, 0, "0", 0),
    (1, 0, "0", 0),
    (0, 0, "1", 0),
    (1, 0, "1", 0),
    (0, 0, "1", 1),
]
for clock, reset, allow, index in stimuli:
    rising = bool(clock and not history)
    accepted = not (rising and not reset and allow in ("x", "z"))
    available = allow == "1"
    first_sum, first_row = (total + 3) % 256, (rows[index] + 3) % 256
    second_sum, second_row = (total + 8) % 256, (rows[index] + 8) % 256
    candidate = (first_sum << 24) | (first_row << 16) | (second_sum << 8) | second_row
    print(
        clock,
        reset,
        allow,
        index,
        f"{total:x}",
        f"{(rows[0] << 8) | rows[1]:x}",
        f"{rows[index]:x}",
        f"{candidate:x}",
        int(available),
        int(accepted),
    )
    if not accepted:
        continue
    if rising:
        if reset:
            total, rows = 0, [0, 0]
        elif available:
            total, rows[index] = second_sum, second_row
    history = clock
