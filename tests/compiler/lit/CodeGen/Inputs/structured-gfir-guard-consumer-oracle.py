"""Independent lexical guard admission and committed counter/clock oracle."""

count = 0
return_count = 0
history = 0
# clk rst producer-guard actual-payload consumer-first-guard
stimuli = [
    (0, 0, "0", "x", "1"),
    (1, 0, "0", "z", "1"),
    (0, 0, "0", "x", "1"),
    (1, 0, "0", "x", "x"),  # earlier reached X precedes unavailable capture
    (1, 0, "1", "1", "1"),  # same-high retry
    (1, 0, "1", "x", "1"),  # held, available unknown has no sampled fault
    (0, 0, "1", "z", "1"),
    (1, 0, "1", "x", "1"),
    (1, 0, "1", "z", "1"),
    (1, 0, "1", "1", "1"),  # failed X/Z attempts kept clock and count
    (0, 0, "1", "0", "1"),
    (1, 0, "1", "0", "1"),  # available known false
    (0, 0, "1", "1", "0"),
    (1, 0, "1", "1", "0"),  # independent guard blocks consumer
    (0, 0, "x", "1", "0"),
    (1, 0, "x", "1", "0"),  # producer checks cannot disappear with unused output
    (1, 0, "0", "x", "0"),
    (0, 0, "1", "z", "1"),
    (1, 1, "1", "z", "1"),  # reset masks sampled unknown payload
    (1, 0, "1", "z", "1"),
    (0, 0, "1", "1", "1"),
    (1, 0, "1", "1", "1"),
    (0, 0, "0", "z", "1"),
]
for clock, reset, offer, payload, enable in stimuli:
    # In these samples the first reached write must block before the X guard.
    late = "x" if offer == "0" and enable == "1" else "1"
    rising = bool(clock and not history)
    fault = False
    if rising and not reset:
        # Producer and consumer are separate transactions/check sequences.
        fault = offer in ("x", "z") or enable in ("x", "z")
        if enable == "1" and offer == "1" and payload in ("x", "z"):
            fault = True
    print(
        clock,
        reset,
        offer,
        payload,
        enable,
        late,
        f"{count:x}",
        f"{return_count:x}",
        int(not fault),
    )
    if fault:
        continue
    if rising:
        if reset:
            count = return_count = 0
        elif offer == "1" and enable == "1":
            return_count = (return_count + 1) % 256
            if payload == "1":
                count = (count + 1) % 256
    history = clock
