"""Independent select truth table, late admission and two committed owners."""

# clk reset offer data select fallback run explicit-discard
stimuli = [
    (0, 0, "0", "xxxxxxxx", "0", "00010001", "1", 0),
    (1, 0, "0", "xxxxxxxx", "0", "00010001", "1", 0),
    (0, 0, "0", "zzzzzzzz", "1", "00100010", "0", 0),
    (1, 0, "0", "zzzzzzzz", "1", "00100010", "0", 0),
    (0, 0, "0", "xxxxxxxx", "1", "00100010", "1", 0),
    (1, 0, "0", "xxxxxxxx", "1", "00100010", "1", 0),
    (0, 0, "0", "xxxxxxxx", "x", "00100010", "1", 0),
    (1, 0, "0", "zzzzzzzz", "z", "00100010", "1", 0),
    (0, 0, "0", "xxxxxxxx", "1", "00100010", "x", 0),
    (1, 0, "0", "xxxxxxxx", "1", "00100010", "x", 0),
    (1, 0, "0", "zzzzzzzz", "1", "00100010", "z", 0),
    (1, 0, "1", "10xz0101", "x", "100z1101", "1", 0),
    (1, 0, "1", "zzzzzzzz", "0", "00110011", "1", 0),
    (0, 0, "1", "zzzzzzzz", "0", "00110011", "1", 0),
    (1, 0, "1", "zzzzzzzz", "0", "00110011", "1", 0),
    (0, 0, "1", "zz0011xx", "z", "zz001100", "1", 0),
    (1, 0, "1", "zz0011xx", "z", "zz001100", "1", 1),
    (1, 0, "1", "01010101", "1", "00000000", "1", 0),
    (0, 0, "x", "11110000", "0", "01100110", "1", 0),
    (1, 0, "x", "11110000", "0", "01100110", "1", 0),
    (1, 0, "1", "10101010", "0", "01110111", "1", 0),
    (0, 1, "z", "xxxxxxxx", "1", "xxxxxxxx", "z", 0),
    (1, 1, "z", "xxxxxxxx", "1", "xxxxxxxx", "z", 0),
    (1, 0, "0", "zzzzzzzz", "0", "10001000", "1", 0),
    (0, 0, "0", "zzzzzzzz", "0", "10001000", "1", 0),
    (1, 0, "0", "zzzzzzzz", "0", "10001000", "1", 0),
    (0, 0, "0", "zzzzzzzz", "0", "10001000", "1", 0),
]
old = produced = "00000000"
history = 0
for clock, reset, offer, data, selector, fallback, run, discard in stimuli:
    available = selector == "0" or offer == "1"
    valid = available and run == "1"
    if selector == "0":
        chosen = fallback
    elif selector == "1":
        chosen = data
    else:
        # Unknown selection preserves only matching four-state symbols.
        chosen = "".join(a if a == b else "x" for a, b in zip(data, fallback, strict=True))
    fault = clock and not history and not reset and (
        offer in ("x", "z") or run in ("x", "z")
    )
    print(
        clock,
        reset,
        offer,
        data,
        selector,
        fallback,
        run,
        int(valid),
        chosen if valid else "00000000",
        old,
        produced,
        int(not fault),
        discard,
    )
    if fault or discard:
        continue
    if clock and not history:
        if reset:
            old = produced = "00000000"
        else:
            if offer == "1":
                produced = data
            if valid:
                old = chosen
    history = clock
