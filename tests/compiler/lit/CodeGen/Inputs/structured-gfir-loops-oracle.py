"""Independent scalar recurrence and sampled-clock oracle for fixed loops."""

current = 2
previous_clock = 0
# Asymmetric carries expose sequential rather than simultaneous step updates.
stimuli = [
    (0, 0, 1, 7),
    (1, 0, 1, 7),
    (1, 0, 9, 3),
    (0, 0, 250, 253),
    (1, 0, 250, 253),
    (0, 0, 18, 90),
    (1, 1, 18, 90),
    (1, 0, 3, 41),
    (0, 0, 3, 41),
    (1, 0, 3, 41),
    (0, 0, 255, 0),
    (1, 0, 255, 0),
    (0, 0, 0, 255),
    (1, 0, 0, 255),
]
for clock, reset, x, y in stimuli:
    left, right = x, y
    candidate = current
    for ordinal in range(3):
        next_left = (right + ordinal + 1) % 256
        candidate = (candidate + next_left) % 256
        left, right = next_left, left
    # range(0) returns both initial carries, never overwrites the candidate.
    fields = [current, candidate, left, right, left, right]
    expected = 0
    for field in fields:
        expected = (expected << 8) | field
    expected = (expected << 2) | 3  # both loops complete naturally
    print(*(f"{value:x}" for value in [clock, reset, x, y, expected]))
    if clock and not previous_clock:
        current = 2 if reset else candidate
    previous_clock = clock
