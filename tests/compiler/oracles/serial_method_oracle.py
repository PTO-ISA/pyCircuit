"""Independent cycle model for the autonomous serial Method source fixture.

This checker consumes actual native/RTL traces. It never contributes values to
the generated design, compiler, driver or storage transition implementation.
"""

import argparse
from pathlib import Path

OWNERS = (
    "count",
    "before",
    "middle",
    "saved",
    "after",
    "old_seen",
    "payload",
    "copied",
    "held",
    "identity",
)


def expected_trace(stimuli, width, initial, first, second, bias, last, mode="local"):
    mask = (1 << width) - 1

    def bits(value):
        return format(value & mask, f"0{width}b")

    initial_state = dict.fromkeys(OWNERS, bits(0))
    initial_state.update(
        count=bits(initial), payload=bits(9), held=bits(9), identity=bits(11)
    )
    active = {"local": [True, True], "cross": [True, True, False], "root": [False]}[
        mode
    ]
    states = [initial_state.copy() for _ in active]
    external_writes = {
        "count",
        "before",
        "middle",
        "saved",
        "after",
        "copied",
        "identity",
    }
    clock = 0
    lines = []
    for row, line in enumerate(stimuli.splitlines()):
        level, reset, discard, inject_count, inject_payload = line.split()
        level, reset, discard = map(int, (level, reset, discard))
        if inject_count != "-":
            states[0]["count"] = inject_count
        if inject_payload != "-":
            states[0]["payload"] = inject_payload
        tokens = ["WORK", str(row)]
        candidates = []
        for state, advancing in zip(states, active, strict=True):
            q = int(state["count"], 2)
            candidate = state.copy()
            candidate["old_seen"] = bits(q)
            if advancing:
                candidate.update(
                    count=bits(q + first + second + bias + last),
                    before=bits(q),
                    middle=bits(q + first),
                    saved=bits(q + first + second),
                    after=bits(q + first + second + bias + last),
                    copied=state["payload"],
                )
            for owner in OWNERS:
                enable = int(
                    owner == "old_seen" or (advancing and owner in external_writes)
                )
                # The uniform physical interface requires known-zero D/E for
                # an occurrence with no external writer; Q still holds.
                data = (
                    bits(0)
                    if not advancing and owner in external_writes
                    else candidate[owner]
                )
                tokens.extend((state[owner], data, str(enable), str(clock)))
            candidates.append(candidate)
        lines.append(" ".join(tokens))
        if not discard:
            if level and not clock:
                states = [initial_state.copy() for _ in active] if reset else candidates
            clock = level
    return lines


def check(actual, expected, label):
    assert len(actual) == len(expected), (
        label,
        "frame count",
        len(actual),
        len(expected),
    )
    for row, (observed, wanted) in enumerate(zip(actual, expected, strict=True)):
        assert observed == wanted, (label, "frame", row, observed, wanted)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--stimuli", type=Path, required=True)
    parser.add_argument("--trace", type=Path, action="append", required=True)
    parser.add_argument("--mode", choices=("local", "cross", "root"), default="local")
    for name in ("width", "initial", "first", "second", "bias", "last"):
        parser.add_argument("--" + name, type=int, required=True)
    args = parser.parse_args()
    expected = expected_trace(
        args.stimuli.read_text(),
        args.width,
        args.initial,
        args.first,
        args.second,
        args.bias,
        args.last,
        args.mode,
    )
    for path in args.trace:
        check(
            [
                line
                for line in path.read_text().splitlines()
                if line.startswith("WORK ")
            ],
            expected,
            str(path),
        )
    print(  # noqa: T201 - standalone independent oracle result
        f"serial Method independent oracle: {len(expected)} frames, {len(args.trace)} traces"
    )


if __name__ == "__main__":
    main()
