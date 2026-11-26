"""Independent scalar four-state assertion mask and sampled counter oracle."""


def invert(bit):
    return {"0": "1", "1": "0"}.get(bit, "x")


def conjunction(left, right):
    if "0" in (left, right):
        return "0"
    return "1" if left == right == "1" else "x"


count, history = 0, 0
# clk, raw strict reset, assertion predicate, independent late guard.
stimuli = [
    (0, "0", "1", "1"),
    (1, "0", "1", "1"),
    (1, "0", "0", "1"),
    (1, "0", "x", "1"),
    (1, "0", "z", "1"),
    (1, "1", "z", "1"),
    (0, "0", "0", "1"),
    (0, "0", "1", "1"),
    (0, "x", "1", "1"),
    (0, "x", "0", "1"),
    (0, "z", "z", "1"),
    (0, "1", "x", "1"),
    (1, "0", "0", "x"),  # all guards precede all assertions
    (1, "0", "0", "1"),
    (1, "0", "1", "1"),  # same-high retry
    (0, "0", "1", "1"),
    (1, "1", "0", "x"),
    (1, "0", "x", "1"),
    (1, "0", "1", "1"),
    (0, "0", "1", "1"),
    (1, "0", "1", "1"),
    (0, "0", "1", "1"),
    (1, "0", "0", "0"),  # late false guard suppresses the earlier assertion
    (1, "0", "x", "0"),
    (0, "0", "z", "0"),
]
for clock, reset, predicate, allow in stimuli:
    rising = bool(clock and not history)
    guard_fault = rising and reset == "0" and allow in ("x", "z")
    # The writer admits every value except a case-equal zero. Any X/Z in
    # the independent guard still follows the ordinary edge checks.
    fire = allow != "0"
    error = conjunction(
        "1" if fire else "0", conjunction(invert(reset), invert(predicate))
    )
    outcome = "G" if guard_fault else "A" if error != "0" else "C"
    print(clock, reset, predicate, allow, f"{count:x}", outcome)
    if outcome != "C":
        continue
    if rising and reset == "1":
        count = 0
    elif rising and fire:
        count = (count + 1) % 256
    history = clock
