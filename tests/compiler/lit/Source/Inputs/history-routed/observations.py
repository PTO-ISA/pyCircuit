"""Decode actual hardware samples and transfers, independently of expected rows."""

from copy import deepcopy

from drivers import DEPTHS, QUEUE_NAMES

FIELDS = ("sequence_id", "opcode", "route", "waits_for", "cycles", "value")
WIDTHS = (8, 8, 2, 8, 16, 64)
PUBLIC = ("output", "scheduled", "route_1_done", "route_3_done", "completed")
OBSERVED = PUBLIC[1:]


def item(value):
    result = {}
    for field, width in reversed(tuple(zip(FIELDS, WIDTHS, strict=True))):
        result[field] = value & ((1 << width) - 1)
        value >>= width
    assert value == 0
    return result


def heads(packed):
    value = int(packed, 16)
    remaining = 21934 - 130
    result = {}
    for name, depth in zip(QUEUE_NAMES, DEPTHS, strict=True):
        remaining -= depth.bit_length()
        count = (value >> remaining) & ((1 << depth.bit_length()) - 1)
        assert count <= depth
        remaining -= depth * 106
        result[name] = (
            item((value >> (remaining + (depth - 1) * 106)) & ((1 << 106) - 1))
            if count
            else None
        )
    assert remaining == 1384 + 10944
    return result


def public(packed):
    value = int(packed, 16)
    assert value < (1 << 536)
    remaining = 535
    result = {"ready": bool(value >> remaining)}
    for name in PUBLIC:
        remaining -= 1
        available = bool((value >> remaining) & 1)
        remaining -= 106
        view = {"available": available}
        if available:
            view["head"] = item((value >> remaining) & ((1 << 106) - 1))
        result[name] = view
    assert remaining == 0
    return result


def stimulus(rows):
    lines = [str(len(rows))]
    for row in rows:
        frame = row["frame"]
        data = frame["data"]
        high = (
            (data["sequence_id"] << 34)
            | (data["opcode"] << 26)
            | (data["route"] << 24)
            | (data["waits_for"] << 16)
            | data["cycles"]
        )
        assert not frame["clock_error"], "control-reference rows are not executable"
        lines.append(
            f"{int(frame['valid'])} {int(frame['take'])} {int(frame['host_reset'])} {data['value']:x} {high:x}"
        )
    return "\n".join(lines) + "\n"


def decode(output, frames):
    """Ledgers come only from actual DUT flags and actual old queue heads."""
    lines = [line.split() for line in output.splitlines() if line.startswith("ROW ")]
    assert len(lines) == len(frames)
    pops = dict.fromkeys(QUEUE_NAMES, 0)
    pushes = dict.fromkeys(QUEUE_NAMES, 0)
    received = 0
    counts = dict.fromkeys(OBSERVED, 0)
    last = dict.fromkeys(OBSERVED)
    rows = []
    for line, frame in zip(lines, frames, strict=True):
        assert len(line) == 11, line
        attempt, committed, failed = map(int, line[1:4])
        before, after, public_bits = line[4:7]
        pop_mask, push_mask = map(int, line[7:9])
        work = line[10]
        received_delta = []
        observation_delta = {name: [] for name in OBSERVED}
        if frame["host_reset"]:
            assert committed and not failed and pop_mask == push_mask == 0
            pops = dict.fromkeys(QUEUE_NAMES, 0)
            pushes = dict.fromkeys(QUEUE_NAMES, 0)
            received = 0
            counts = dict.fromkeys(OBSERVED, 0)
            last = dict.fromkeys(OBSERVED)
        elif committed:
            old_heads = heads(work)
            for name in OBSERVED:
                head = old_heads[name]
                pending = {"pops": pops[name], "item": head}
                if head is not None and last[name] != pending:
                    observation_delta[name].append(head)
                    counts[name] += 1
                    last[name] = pending
            for i, name in enumerate(QUEUE_NAMES):
                if pop_mask & (1 << i):
                    assert old_heads[name] is not None, (attempt, name, "empty pop")
                    pops[name] += 1
                    if name == "output":
                        received_delta.append(old_heads[name])
                        received += 1
                if push_mask & (1 << i):
                    pushes[name] += 1
        else:
            assert failed and pop_mask == push_mask == 0
        row = {
            "attempt": attempt,
            "committed": bool(committed),
            "failed": bool(failed),
            "packed_before": before,
            "packed_after": after,
            "after_flow": {
                "pops": deepcopy(pops),
                "pushes": deepcopy(pushes),
                "received_count": received,
                "received_delta": received_delta,
                "observation_counts": deepcopy(counts),
                "observation_delta": observation_delta,
                "observer_last": deepcopy(last),
            },
        }
        if public_bits != "-":
            row["public"] = public(public_bits)
        rows.append(row)
    return rows
