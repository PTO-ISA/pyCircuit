"""Independent bounded recurrence: each reached iteration commits its carries."""

current, history = 2, 0
stimuli = [
    (0, 0, "000"),
    (1, 0, "000"),
    (1, 0, "001"),
    (0, 0, "001"),
    (1, 0, "001"),
    (0, 0, "100"),
    (1, 0, "100"),
    (0, 0, "xz1"),
    (1, 0, "xz1"),
    (0, 0, "0x0"),
    (1, 0, "0x0"),
    (1, 0, "00z"),
    (1, 0, "010"),
    (1, 0, "000"),
    (0, 1, "000"),
    (1, 1, "0z0"),
    (1, 0, "0z0"),
    (0, 0, "000"),
    (1, 0, "000"),
    (0, 0, "000"),
]
for clock, reset, stops in stimuli:
    left, right, candidate = 1, 7, current
    valid, natural = True, True
    for ordinal, stop in enumerate(reversed(stops)):
        next_left = (right + ordinal + 1) % 256
        candidate = (candidate + next_left) % 256
        left, right = next_left, left
        if stop in ("x", "z"):
            valid = False
            break
        if stop == "1":
            natural = False
            break
    expected = (candidate << 18) | (left << 10) | (right << 2) | (int(natural) << 1) | 1
    fault = clock and not history and not reset and not valid
    print(
        clock,
        reset,
        stops,
        f"{current:x}",
        int(valid),
        f"{expected:x}",
        int(not fault),
    )
    if fault:
        continue
    if clock and not history:
        current = 2 if reset else candidate
    history = clock
