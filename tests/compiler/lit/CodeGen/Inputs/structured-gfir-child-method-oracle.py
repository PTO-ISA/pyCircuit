"""Independent committed scalar and two ordered candidate snapshots."""

value, history = 0, 0
# Repeated high samples after failures must still be the original rising edge.
stimuli = [
    (0, 0, "1"),
    (1, 0, "1"),
    (1, 0, "1"),
    (0, 0, "0"),
    (1, 0, "0"),
    (0, 0, "1"),
    (1, 0, "x"),
    (1, 0, "z"),
    (1, 0, "1"),
    (0, 0, "1"),
    (1, 1, "1"),
    (1, 0, "1"),
    (0, 0, "x"),
    (1, 1, "z"),
    (1, 0, "z"),
    (0, 0, "1"),
    (1, 0, "1"),
    (0, 0, "0"),
    (1, 0, "0"),
    (0, 0, "1"),
    (1, 0, "1"),
    (0, 0, "1"),
]
for clock, reset, allow in stimuli:
    rising = bool(clock and not history)
    accepted = not (rising and not reset and allow in ("x", "z"))
    available = allow == "1"
    first = (value + 3) % 256
    second = (first + 5) % 256
    print(
        clock,
        reset,
        allow,
        f"{value:x}",
        int(available),
        f"{first:x}",
        f"{second:x}",
        int(accepted),
    )
    if not accepted:
        continue
    if rising:
        if reset:
            value = 0
        elif available:
            value = second
    history = clock
