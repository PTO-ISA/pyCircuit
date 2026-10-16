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

DYNAMIC_OWNERS = ("left", "right", "spectator")
DYNAMIC_STATES = (
    "count",
    "before",
    "after",
    "old_seen",
    "first_gate",
    "second_gate",
)


def dynamic_stimuli():
    """Return bounded public dynamic-guard stimuli; never DUT results."""

    rows = [
        ("R", "x", "z", "x", "z", "z", "x", "x", "z"),
        ("S", "0", "x", "1", "1", "1", "1", "1", "1"),
        ("S", "1", "0", "1", "1", "1", "1", "1", "1"),
        ("S", "1", "x", "0", "x", "1", "1", "1", "1"),
        ("S", "0", "z", "0", "x", "1", "1", "1", "1"),
        ("S", "1", "0", "0", "x", "1", "1", "1", "1"),
        ("S", "0", "0", "1", "0", "1", "1", "1", "1"),
        ("S", "1", "0", "1", "0", "1", "1", "1", "1"),
        ("S", "0", "0", "1", "1", "0", "x", "1", "1"),
        ("S", "1", "0", "1", "1", "0", "x", "1", "1"),
        ("S", "0", "0", "x", "0", "1", "1", "1", "1"),
        ("S", "1", "0", "x", "0", "1", "1", "1", "1"),
        ("T", "1", "0", "x", "0", "1", "1", "1", "1"),
        ("R", "z", "x", "z", "z", "z", "z", "z", "z"),
        ("S", "0", "0", "1", "1", "1", "1", "1", "1"),
        ("S", "1", "0", "1", "x", "1", "1", "1", "1"),
        ("T", "1", "0", "1", "x", "1", "1", "1", "1"),
        ("R", "x", "z", "x", "x", "x", "x", "x", "x"),
        ("S", "0", "0", "z", "0", "1", "1", "1", "1"),
        ("S", "1", "0", "z", "0", "1", "1", "1", "1"),
        ("T", "1", "0", "z", "0", "1", "1", "1", "1"),
        ("R", "x", "x", "x", "x", "x", "x", "x", "x"),
        ("S", "0", "0", "1", "z", "1", "1", "1", "1"),
        ("S", "1", "0", "1", "z", "1", "1", "1", "1"),
        ("T", "1", "0", "1", "z", "1", "1", "1", "1"),
        ("R", "x", "x", "x", "x", "x", "x", "x", "x"),
        ("S", "0", "0", "1", "1", "z", "0", "1", "1"),
        ("S", "1", "0", "1", "1", "z", "0", "1", "1"),
        ("T", "1", "0", "1", "1", "z", "0", "1", "1"),
        ("R", "z", "z", "z", "z", "z", "z", "z", "z"),
        ("S", "0", "0", "1", "1", "1", "1", "x", "0"),
        ("S", "1", "0", "1", "1", "1", "1", "x", "0"),
        ("T", "1", "0", "1", "1", "1", "1", "x", "0"),
        ("R", "x", "z", "x", "x", "x", "x", "x", "x"),
        ("S", "0", "0", "1", "1", "1", "1", "1", "1"),
        ("D", "1", "0", "x", "0", "1", "1", "1", "1"),
        ("S", "1", "0", "x", "0", "1", "1", "1", "1"),
        ("T", "1", "0", "x", "0", "1", "1", "1", "1"),
        ("R", "z", "x", "z", "z", "z", "z", "z", "z"),
        ("S", "0", "0", "1", "1", "1", "1", "1", "1"),
        ("F", "1", "0", "1", "1", "1", "1", "1", "1"),
        ("S", "1", "x", "0", "x", "1", "1", "1", "1"),
        ("S", "0", "z", "0", "x", "1", "1", "1", "1"),
        ("S", "1", "1", "x", "z", "z", "x", "x", "z"),
        ("S", "1", "0", "x", "z", "z", "x", "x", "z"),
        ("S", "0", "x", "x", "z", "z", "x", "x", "z"),
        ("S", "1", "0", "1", "1", "1", "1", "1", "1"),
        ("R", "x", "z", "x", "x", "x", "x", "x", "x"),
        ("S", "x", "0", "1", "1", "1", "1", "1", "1"),
        ("T", "x", "0", "1", "1", "1", "1", "1", "1"),
        ("R", "z", "x", "z", "z", "z", "z", "z", "z"),
        ("S", "z", "1", "1", "1", "1", "1", "1", "1"),
        ("T", "z", "1", "1", "1", "1", "1", "1", "1"),
        ("R", "x", "z", "x", "x", "x", "x", "x", "x"),
        ("S", "0", "0", "1", "1", "1", "1", "1", "1"),
        ("S", "1", "x", "1", "1", "1", "1", "1", "1"),
        ("T", "1", "x", "1", "1", "1", "1", "1", "1"),
        ("R", "z", "x", "z", "z", "z", "z", "z", "z"),
        ("S", "0", "0", "1", "1", "1", "1", "1", "1"),
        ("S", "1", "z", "1", "1", "1", "1", "1", "1"),
        ("T", "1", "z", "1", "1", "1", "1", "1", "1"),
        ("R", "x", "z", "x", "x", "x", "x", "x", "x"),
    ]
    return [" ".join(row) for row in rows]


def dynamic_entry_stimuli():
    """Return bounded candidate-at-guard-entry stimuli; never DUT results."""

    rows = [
        ("R", "x", "z", "x", "z", "z", "x", "x", "z"),
        ("S", "0", "x", "0", "1", "x", "1", "1", "1"),
        ("S", "1", "0", "0", "1", "x", "1", "1", "1"),
        ("S", "0", "0", "1", "0", "0", "1", "1", "0"),
        ("S", "1", "0", "1", "0", "0", "1", "1", "0"),
        ("S", "0", "0", "1", "1", "1", "1", "1", "1"),
        ("S", "1", "0", "1", "1", "1", "1", "1", "1"),
        ("S", "0", "0", "0", "x", "1", "1", "1", "1"),
        ("S", "1", "0", "0", "x", "1", "1", "1", "1"),
        ("T", "0", "0", "0", "0", "0", "0", "0", "0"),
        ("R", "x", "z", "x", "z", "z", "x", "x", "z"),
        ("S", "0", "0", "1", "1", "1", "1", "0", "z"),
        ("S", "1", "0", "1", "1", "1", "1", "0", "z"),
        ("T", "0", "0", "0", "0", "0", "0", "0", "0"),
        ("R", "x", "z", "x", "z", "z", "x", "x", "z"),
        ("S", "0", "0", "0", "x", "1", "1", "1", "1"),
        ("D", "1", "0", "0", "x", "1", "1", "1", "1"),
        ("S", "1", "0", "0", "x", "1", "1", "1", "1"),
        ("T", "0", "0", "0", "0", "0", "0", "0", "0"),
        ("R", "x", "z", "x", "z", "z", "x", "x", "z"),
        ("S", "0", "0", "0", "x", "x", "z", "1", "1"),
        ("S", "1", "1", "0", "x", "x", "z", "1", "1"),
        ("S", "1", "0", "0", "x", "x", "z", "1", "1"),
        ("S", "0", "x", "0", "x", "x", "z", "1", "1"),
        ("S", "1", "0", "0", "1", "0", "1", "x", "1"),
        ("S", "0", "0", "0", "1", "0", "1", "x", "1"),
        ("F", "1", "0", "0", "1", "0", "1", "x", "1"),
        ("S", "1", "x", "1", "0", "x", "0", "1", "0"),
        ("R", "x", "z", "x", "z", "z", "x", "x", "z"),
    ]
    return [" ".join(row) for row in rows]


def nominal_stimuli():
    """Known, invalid Enum and X/Z carriers with equal and differing leaves."""
    payloads = [
        ("00000101", "00001001", "00000000", "00000101"),
        ("10101010", "10101010", "00000011", "00000011"),
        ("zz00xx11", "zz00xx11", "zzzzzzzz", "zzzzzzzz"),
        ("10z0x101", "10z0x001", "00000z01", "00000x01"),
        ("11111111", "00000000", "11111111", "11111110"),
    ]
    return [" ".join((flag, *values)) for values in payloads for flag in "01xz"]


def nominal_expected(stimuli):
    """Immutable source values; per-bit conditional truth table is independent."""

    def choose(flag, left, right):
        if flag == "1":
            return left
        if flag == "0":
            return right
        return "".join(a if a == b else "x" for a, b in zip(left, right, strict=True))

    def assemble(flag, pair, replacement):
        a, ta, b, tb = pair
        data, tag = replacement
        # Reconstructed left before the branch; both branch arms start here.
        yes = (data, tag, b, tb)
        no = (b, ta, b, tag)
        joined = tuple(choose(flag, x, y) for x, y in zip(yes, no, strict=True))
        return joined, pair, choose(flag, tag, ta)

    lines = []
    for row, text in enumerate(stimuli):
        flag, a, b, ta, tb = text.split()
        original = (a, ta, b, tb)
        first, saved, direct = assemble(flag, original, (a, tb))
        repeated, _, _ = assemble(flag, first, (b, ta))
        payload = (
            "".join(first)
            + "".join(saved)
            + direct
            + "".join(repeated)
            + "".join(original)
        )
        lines.append(f"NOMINAL {row} {payload}")
    return lines


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


def _dynamic_initial_state():
    return {
        owner: {
            "count": 5,
            "before": 0,
            "after": 0,
            "old_seen": 0,
            "first_gate": "1",
            "second_gate": "1",
        }
        for owner in DYNAMIC_OWNERS
    }


def _dynamic_probe_tokens(states, clocks, sampling):
    tokens = []
    for owner in DYNAMIC_OWNERS:
        for name in DYNAMIC_STATES:
            value = states[owner][name]
            tokens.extend(
                (
                    value if isinstance(value, str) else format(value & 0x7F, "07b"),
                    clocks[owner][name],
                )
            )
    return [*tokens, "H", *sampling]


def _dynamic_guard(first, second):
    if first in "xz":
        return False, True
    if first == "0":
        return False, False
    if second in "xz":
        return False, True
    return second == "1", False


def _dynamic_advances(states, mode):
    decisions = {}
    if mode == "dynamic-entry":
        fault = False
        for owner in DYNAMIC_OWNERS:
            candidate = states[owner]["second_gate"]
            decisions[owner] = candidate == "1"
            fault |= candidate in "xz"
        return decisions, fault
    if mode == "dynamic-independent":
        fault = False
        for owner in DYNAMIC_OWNERS:
            fire, unknown = _dynamic_guard(
                states[owner]["first_gate"], states[owner]["second_gate"]
            )
            decisions[owner] = fire
            fault |= unknown
        return decisions, fault

    left, fault = _dynamic_guard(
        states["left"]["first_gate"], states["left"]["second_gate"]
    )
    parent = left
    if not fault and left:
        right, fault = _dynamic_guard(
            states["right"]["first_gate"], states["right"]["second_gate"]
        )
        parent = right
    decisions["left"] = parent and not fault
    decisions["right"] = parent and not fault
    spectator, spectator_fault = _dynamic_guard(
        states["spectator"]["first_gate"],
        states["spectator"]["second_gate"],
    )
    decisions["spectator"] = spectator
    return decisions, fault or spectator_fault


def dynamic_expected(stimuli, mode, sampling_count):
    expected_count = 4 if mode == "dynamic-atomic" else 3
    assert sampling_count == expected_count, (mode, sampling_count, expected_count)
    states = _dynamic_initial_state()
    clocks = {owner: dict.fromkeys(DYNAMIC_STATES, "0") for owner in DYNAMIC_OWNERS}
    sampling = ["0"] * sampling_count
    cycle = 0
    native_failed = False
    common, native = [], []

    rows = [line.split() for line in stimuli.splitlines() if line.strip()]
    for row, fields in enumerate(rows):
        assert len(fields) == 9, (row, fields)
        op, clock, reset, lf, ls, rf, rs, sf, ss = fields
        assert op in "SRDFT" and clock in "01xz" and reset in "01xz"
        gates = ((lf, ls), (rf, rs), (sf, ss))
        assert all(value in "01xz" for pair in gates for value in pair)
        cycle_before = cycle

        if op != "T":
            for owner, (first, second) in zip(DYNAMIC_OWNERS, gates, strict=True):
                states[owner]["first_gate"] = first
                states[owner]["second_gate"] = second

        pre = _dynamic_probe_tokens(states, clocks, sampling)
        if op == "T":
            assert native_failed, (row, "sticky retry issued while native Ready")
            native.append(" ".join(["NATIVE_PRE", str(row), *pre]))
            native.append(" ".join(["NATIVE_POST", str(row), "sticky", *pre]))
            native.append(f"NATIVE {row} {cycle_before} {cycle_before} invalid_state")
            continue

        common.append(" ".join(["PRE", str(row), *pre]))
        semantic_status = "ok"
        result_status = "direct" if op in "DF" else "ok"

        if op == "R":
            states = _dynamic_initial_state()
            clocks = {
                owner: dict.fromkeys(DYNAMIC_STATES, "0") for owner in DYNAMIC_OWNERS
            }
            sampling = ["0"] * sampling_count
            cycle = 0
            native_failed = False
            semantic_status = "reset"
        elif op == "D":
            semantic_status = "discard"
        else:
            prior_clock = sampling[0]
            physical_fault = clock in "xz"
            rising = clock == "1" and prior_clock == "0"
            if rising and reset in "xz":
                physical_fault = True
            reset_active = rising and reset == "1"
            decisions, guard_fault = _dynamic_advances(states, mode)
            guard_fault = rising and reset == "0" and guard_fault
            if physical_fault:
                semantic_status = "physical"
            elif guard_fault:
                semantic_status = "guard"
            else:
                if rising:
                    if reset_active:
                        states = _dynamic_initial_state()
                    else:
                        updated = {
                            owner: values.copy() for owner, values in states.items()
                        }
                        for owner in DYNAMIC_OWNERS:
                            old_count = states[owner]["count"]
                            updated[owner]["old_seen"] = old_count
                            if decisions[owner]:
                                updated[owner]["count"] = (old_count + 1) & 0x7F
                                updated[owner]["before"] = old_count
                                updated[owner]["after"] = (old_count + 1) & 0x7F
                                if mode == "dynamic-entry":
                                    updated[owner]["first_gate"] = "0"
                        states = updated
                for owner in DYNAMIC_OWNERS:
                    for name in DYNAMIC_STATES:
                        clocks[owner][name] = clock
                sampling = [clock] * sampling_count
                if op == "S":
                    cycle += 1

            if semantic_status in ("guard", "physical"):
                assert op == "S", (row, op, semantic_status)
                result_status = "runtime_failure"
                native_failed = True

        if op == "S" and native_failed and result_status == "ok":
            raise AssertionError((row, "successful Step issued while native Failed"))
        post = _dynamic_probe_tokens(states, clocks, sampling)
        common.append(" ".join(["POST", str(row), semantic_status, *post]))
        native.append(f"NATIVE {row} {cycle_before} {cycle} {result_status}")
    return common, native


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
    parser.add_argument("--native-trace", type=Path, action="append", default=[])
    parser.add_argument(
        "--mode",
        choices=(
            "local",
            "cross",
            "root",
            "dynamic-independent",
            "dynamic-atomic",
            "dynamic-entry",
        ),
        default="local",
    )
    parser.add_argument("--sampling-count", type=int)
    for name in ("width", "initial", "first", "second", "bias", "last"):
        parser.add_argument("--" + name, type=int)
    args = parser.parse_args()
    if args.mode.startswith("dynamic-"):
        if args.sampling_count is None:
            parser.error("--sampling-count is required for dynamic modes")
        if not args.native_trace:
            parser.error("--native-trace is required for dynamic modes")
        if args.width not in (None, 7) or args.initial not in (None, 5):
            parser.error("dynamic fixture has fixed width 7 and initial count 5")
        actual_stimuli = [
            line for line in args.stimuli.read_text().splitlines() if line.strip()
        ]
        expected_stimuli = (
            dynamic_entry_stimuli()
            if args.mode == "dynamic-entry"
            else dynamic_stimuli()
        )
        assert actual_stimuli == expected_stimuli, (
            "dynamic stimulus contract",
            actual_stimuli,
            expected_stimuli,
        )
        common, native = dynamic_expected(
            "\n".join(actual_stimuli), args.mode, args.sampling_count
        )
        for path in args.trace:
            lines = path.read_text().splitlines()
            check(
                [line for line in lines if line.startswith(("PRE ", "POST "))],
                common,
                str(path) + ": common",
            )
            actual_native = [line for line in lines if line.startswith("NATIVE")]
            assert not actual_native, (path, "RTL trace contains native metadata")
        for path in args.native_trace:
            lines = path.read_text().splitlines()
            check(
                [line for line in lines if line.startswith(("PRE ", "POST "))],
                common,
                str(path) + ": common",
            )
            check(
                [line for line in lines if line.startswith("NATIVE")],
                native,
                str(path) + ": native",
            )
        print(  # noqa: T201 - standalone independent oracle result
            "serial Method dynamic oracle: "
            f"{len(actual_stimuli)} rows, "
            f"{len(args.trace) + len(args.native_trace)} traces, "
            f"{args.sampling_count} sampling histories"
        )
        return

    missing = [
        name
        for name in ("width", "initial", "first", "second", "bias", "last")
        if getattr(args, name) is None
    ]
    if missing:
        parser.error("old modes require " + ", ".join("--" + name for name in missing))
    if args.native_trace:
        parser.error("--native-trace is only valid for dynamic modes")
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
