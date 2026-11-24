"""Independent two-counter clock histories, committed only on global success."""

values = [0, 0]
histories = [0, 0]
# east clock/reset, west clock/reset, east/west admission.
# The late west fault is retried at exactly the same two high levels.
stimuli = [
    (0, 0, 0, 0, "1", "1"),
    (1, 0, 0, 0, "1", "1"),
    (1, 0, 1, 0, "1", "1"),
    (0, 0, 1, 0, "1", "1"),
    (0, 0, 0, 0, "1", "1"),
    (1, 0, 1, 0, "1", "x"),
    (1, 0, 1, 0, "1", "1"),
    (1, 0, 1, 0, "1", "x"),  # unknown held guard is not a sampled fault
    (1, 0, 0, 0, "1", "1"),
    (0, 0, 0, 0, "1", "1"),
    (1, 1, 0, 0, "1", "1"),  # only east resets
    (1, 0, 1, 1, "1", "x"),  # only west resets; its guard is masked
    (0, 0, 0, 0, "1", "1"),
    (0, 0, 1, 0, "1", "1"),
    (1, 0, 1, 0, "1", "1"),
    (0, 0, 0, 0, "1", "1"),
]
for clock_e, reset_e, clock_w, reset_w, allow_e, allow_w in stimuli:
    clocks, resets, allows = [clock_e, clock_w], [reset_e, reset_w], [allow_e, allow_w]
    rising = [bool(c and not old) for c, old in zip(clocks, histories, strict=True)]
    accepted = not any(
        edge and not reset and allow in ("x", "z")
        for edge, reset, allow in zip(rising, resets, allows, strict=True)
    )
    print(
        clock_e,
        reset_e,
        clock_w,
        reset_w,
        allow_e,
        allow_w,
        f"{values[0]:x}",
        f"{values[1]:x}",
        int(accepted),
    )
    if accepted:
        for i, step in enumerate([1, 10]):
            if rising[i]:
                if resets[i]:
                    values[i] = 0
                elif allows[i] == "1":
                    values[i] = (values[i] + step) % 256
        histories = clocks
