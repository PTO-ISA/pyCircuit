"""Independent clock, guard and four-counter model with old-Q peer feedback."""

# Plan effective child admission; encode raw allows using reference peer parity.
# clk rst left-admit left-payload right-admit right-payload left-enable right-enable
stimuli = [
    (0, 0, "0", "x", "1", "1", "1", "1"),
    (1, 0, "0", "z", "1", "1", "1", "1"),
    (0, 0, "1", "1", "0", "x", "1", "1"),
    (1, 0, "1", "1", "0", "z", "1", "1"),
    (0, 0, "1", "0", "1", "1", "1", "1"),
    (1, 0, "1", "0", "1", "1", "1", "1"),  # available false
    (0, 0, "0", "z", "1", "1", "x", "1"),
    (1, 0, "0", "z", "1", "1", "x", "1"),  # earlier X survives
    (1, 0, "1", "1", "0", "x", "1", "1"),  # same-high retry
    (1, 0, "1", "x", "1", "z", "1", "1"),  # held sample
    (0, 0, "1", "z", "1", "x", "1", "1"),  # falling sample
    (1, 0, "1", "x", "1", "1", "1", "1"),  # left reached X
    (1, 0, "1", "z", "1", "1", "1", "1"),  # left reached Z
    (1, 0, "1", "1", "1", "z", "1", "1"),  # right reached Z
    (1, 0, "1", "1", "1", "1", "1", "1"),  # retry all four owners
    (0, 0, "1", "x", "1", "z", "0", "0"),
    (1, 0, "1", "x", "1", "z", "0", "0"),  # earlier false hides payload
    (0, 0, "x", "1", "1", "1", "0", "0"),
    (1, 0, "x", "1", "1", "1", "0", "0"),  # child's own check remains
    (1, 0, "0", "x", "1", "1", "1", "1"),
    (0, 0, "1", "z", "1", "x", "1", "1"),
    (1, 1, "1", "z", "1", "x", "1", "1"),  # reset eligibility
    (1, 0, "1", "z", "1", "x", "1", "1"),
    (0, 0, "1", "1", "0", "z", "1", "1"),
    (1, 0, "1", "1", "0", "z", "1", "1"),
    (0, 0, "0", "x", "1", "1", "1", "1"),
    (1, 0, "0", "x", "1", "1", "1", "1"),
    (0, 0, "0", "z", "0", "x", "1", "1"),
]
history = 0
issued = [0, 0]
seen = [0, 0]
for clock, reset, *signals in stimuli:
    left_allow, left_payload, right_allow, right_payload, *enable = signals
    allow = [left_allow, right_allow]
    payload = [left_payload, right_payload]
    rising = bool(clock and not history)
    fault = False
    if rising and not reset:
        fault = any(value in ("x", "z") for value in allow + enable)
        fault |= any(
            gate == "1" and admission == "1" and value in ("x", "z")
            for gate, admission, value in zip(enable, allow, payload, strict=True)
        )
    available = [int(value == "1") for value in allow]
    raw_allow = [
        value if value in ("x", "z") else str(int(value) ^ (issued[1 - index] & 1))
        for index, value in enumerate(allow)
    ]
    print(
        clock,
        reset,
        raw_allow[0],
        left_payload,
        raw_allow[1],
        right_payload,
        *enable,
        *available,
        *(f"{value:x}" for value in issued + seen),
        int(not fault),
    )
    if fault:
        continue
    if rising:
        if reset:
            issued = [0, 0]
            seen = [0, 0]
        else:
            for index in range(2):
                issued[index] = (issued[index] + available[index]) % 256
                admitted = allow[index] == enable[index] == payload[index] == "1"
                seen[index] = (seen[index] + int(admitted)) % 256
    history = clock
