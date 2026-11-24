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
]
for clock, reset, predicate, allow in stimuli:
    rising = bool(clock and not history)
    guard_fault = rising and reset == "0" and allow in ("x", "z")
    # Assertion path is final admission of its own guard-free Rule: true.
    error = conjunction(invert(reset), invert(predicate))
    outcome = "G" if guard_fault else "A" if error != "0" else "C"
    print(clock, reset, predicate, allow, f"{count:x}", outcome)
    if outcome != "C":
        continue
    if rising:
        count = 0 if reset == "1" else (count + 1) % 256
    history = clock
