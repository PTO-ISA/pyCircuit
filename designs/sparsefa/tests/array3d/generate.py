"""Independent rational arithmetic and explicit-advance state oracle.

No DUT import, source evaluation, generated artifact, or compiler is used here.
"""

import copy
import json
from fractions import Fraction
from pathlib import Path

FULL = (1 << 256) - 1
OPS = {"idle": 0, "begin": 1, "advance": 2, "take": 3}
ACTIONS = {"bubble": 0, "group": 1, "end": 2, "abort": 3}


def decode(word):
    if not 0 <= word <= 0xFFFFFFFF:
        raise ValueError("expected uint32")
    exponent = (word >> 23) & 255
    if exponent == 255:
        raise ValueError("nonfinite input is outside the numeric contract")
    if exponent == 0:
        return Fraction(0)
    value = Fraction((word & 0x7FFFFF) + (1 << 23)) * Fraction(2) ** (exponent - 150)
    return -value if word >> 31 else value


def encode(value, zero_sign=0):
    sign = 0x80000000 if value < 0 else 0
    if not value:
        return zero_sign & 0x80000000
    value = abs(value)
    if value < Fraction(2) ** -126:
        return sign
    maximum = Fraction((1 << 24) - 1) * Fraction(2) ** 104
    if value >= maximum:
        return sign | 0x7F7FFFFF
    exponent = value.numerator.bit_length() - value.denominator.bit_length()
    if value < Fraction(2) ** exponent:
        exponent -= 1
    significand = int(value / Fraction(2) ** (exponent - 23))
    return sign | ((exponent + 127) << 23) | (significand - (1 << 23))


def mul(a, b):
    return encode(decode(a) * decode(b), a ^ b)


def add(a, b):
    # The profile picks the raw larger magnitude, left on ties. This also
    # defines the zero sign when subnormal operands have both flushed to zero.
    sign_source = a if (a & 0x7FFFFFFF) >= (b & 0x7FFFFFFF) else b
    return encode(decode(a) + decode(b), sign_source)


def level(values):
    assert len(values) in (16, 8, 4, 2)
    return [add(values[i], values[i + 1]) for i in range(0, len(values), 2)]


def tree(values):
    values = list(values)
    assert len(values) == 16
    for _ in range(4):
        values = level(values)
    return values[0]


class Schedule:
    def __init__(self):
        self.instructions, self.events, self.results, self.matrices = [], [], [], []
        self.phase, self.slots = "idle", [None] * 5
        self.rows = [[0] * 16 for _ in range(16)]
        self.retirements = []
        self.matrix_id([[0] * 16 for _ in range(16)])

    def matrix_id(self, matrix):
        assert len(matrix) == 16 and all(len(row) == 16 for row in matrix)
        if matrix not in self.matrices:
            self.matrices.append(copy.deepcopy(matrix))
        return self.matrices.index(matrix)

    def token(self, item):
        lanes = item["k_mask"] & (
            (1 << min(16, self.length - 16 * item["group_index"])) - 1
        )
        left, right = self.matrices[item["left_id"]], self.matrices[item["right_id"]]
        values, active = [], []
        for r in range(16):
            for c in range(16):
                enabled = bool(
                    (self.mask >> (16 * r + c))
                    & (item["row_mask"] >> r)
                    & (item["col_mask"] >> c)
                    & 1
                ) and bool(lanes)
                active.append(enabled)
                values.append(
                    [
                        (
                            mul(left[r][k], right[c][k])
                            if enabled and (lanes >> k) & 1
                            else 0
                        )
                        for k in range(16)
                    ]
                )
        return {
            "values": values,
            "active": active,
            "useful": sum(active) * lanes.bit_count(),
            "index": item["group_index"],
            "accepted_cycle": item["cycle"],
        }

    def tick(self, token, cycle):
        retired = self.slots[4]
        if retired is not None:
            for i, active in enumerate(retired["active"]):
                if active:
                    r, c = divmod(i, 16)
                    self.rows[r][c] = add(self.rows[r][c], retired["values"][i][0])
            self.retired += 1
            self.useful += retired["useful"]
            self.retirements.append(
                {
                    "tag": self.tag,
                    "index": retired["index"],
                    "accepted_cycle": retired["accepted_cycle"],
                    "retire_cycle": cycle,
                }
            )
        new_slots = [token]
        for prior in self.slots[:4]:
            if prior is None:
                new_slots.append(None)
            else:
                next_token = {**prior, "values": [level(v) for v in prior["values"]]}
                new_slots.append(next_token)
        self.slots = new_slots

    def add(self, op, **fields):
        item = {
            "cycle": len(self.instructions),
            "op": op,
            "action": "bubble",
            "tag": 0,
            "length": 0,
            "cell_mask": 0,
            "group_index": 0,
            "left_id": 0,
            "right_id": 0,
            "row_mask": 0,
            "col_mask": 0,
            "k_mask": 0,
        }
        item.update(fields)
        accepted = False
        if op == "begin" and self.phase == "idle" and 1 <= item["length"] <= 64:
            accepted = True
            self.phase, self.tag, self.length, self.mask = (
                "active",
                item["tag"],
                item["length"],
                item["cell_mask"],
            )
            self.groups = (self.length + 15) // 16
            self.accepted = self.retired = self.useful = 0
            self.rows, self.slots = [[0] * 16 for _ in range(16)], [None] * 5
            self.begin_cycle = item["cycle"]
        elif op == "advance":
            matching = self.phase != "idle" and item["tag"] == self.tag
            if matching and item["action"] == "abort":
                accepted, self.phase, self.slots = True, "idle", [None] * 5
            elif self.phase in ("active", "drain"):
                token = None
                if matching and item["action"] == "bubble":
                    accepted = True
                elif (
                    matching
                    and item["action"] == "group"
                    and self.phase == "active"
                    and item["group_index"] == self.accepted
                    and self.accepted < self.groups
                ):
                    accepted, token = True, self.token(item)
                    self.accepted += 1
                elif (
                    matching
                    and item["action"] == "end"
                    and self.phase == "active"
                    and self.accepted == self.groups
                ):
                    accepted, self.phase = True, "drain"
                    self.end_cycle = item["cycle"]
                self.tick(token, item["cycle"])
                if self.phase == "drain" and self.retired == self.groups:
                    self.phase = "result"
                    self.result_cycle = item["cycle"]
        elif op == "take" and self.phase == "result":
            accepted = True
            item["tag"] = self.tag
            self.results.append(
                {
                    "tag": self.tag,
                    "rows": copy.deepcopy(self.rows),
                    "groups": self.groups,
                    "useful_macs": self.useful,
                    "begin_cycle": self.begin_cycle,
                    "end_cycle": self.end_cycle,
                    "result_cycle": self.result_cycle,
                    "take_cycle": item["cycle"],
                }
            )
            self.phase = "idle"
        if op == "advance" or accepted:
            self.events.append(
                [
                    item["cycle"],
                    OPS[op],
                    ACTIONS[item["action"]],
                    item["tag"],
                    item["group_index"],
                    int(accepted),
                ]
            )
        self.instructions.append(item)
        return accepted

    def finish(self, tag, hold=0):
        self.add("advance", tag=tag, action="end")
        while self.phase == "drain":
            self.add("advance", tag=tag)
        for _ in range(hold):
            self.add("idle")
        self.add("take")

    def tile(
        self,
        tag,
        length,
        left_id,
        right_id,
        cell_mask=FULL,
        row_mask=65535,
        col_mask=65535,
        k_mask=65535,
        bubbles=False,
    ):
        self.add("begin", tag=tag, length=length, cell_mask=cell_mask)
        for group in range((length + 15) // 16):
            self.add(
                "advance",
                action="group",
                tag=tag,
                group_index=group,
                left_id=left_id,
                right_id=right_id,
                row_mask=row_mask,
                col_mask=col_mask,
                k_mask=k_mask,
            )
            if bubbles:
                self.add("idle")
                self.add("advance", tag=tag)
        self.finish(tag, hold=2 if bubbles else 0)


def generate():
    s = Schedule()
    left = [
        [encode(Fraction((r + 1) * (k + 1), 16)) for k in range(16)] for r in range(16)
    ]
    right = [
        [encode(Fraction((2 * c + 1) * (17 - k), 8)) for k in range(16)]
        for c in range(16)
    ]
    li, ri = s.matrix_id(left), s.matrix_id(right)
    for tag, length in enumerate((1, 15, 16, 17, 31, 32, 63, 64), 1):
        s.tile(tag, length, li, ri)
    s.tile(9, 33, li, ri)
    nonrect = sum(
        1 << (16 * r + c) for r in range(16) for c in range(16) if (r + 2 * c) % 3
    )
    s.tile(20, 63, li, ri, nonrect, 0xA55A, 0x5AA5, 0xB6D9, True)
    s.tile(21, 64, li, ri, k_mask=0)
    s.tile(22, 17, li, ri, cell_mask=0)
    for r in range(16):
        s.tile(30 + r, 16, li, ri, row_mask=1 << r, col_mask=1 << (15 - r))
    # Protocol cases: invalid actions still move old tokens; no public call holds.
    s.add("begin", tag=70, length=0)
    s.add("begin", tag=70, length=65)
    s.add("begin", tag=70, length=17, cell_mask=FULL)
    s.add("begin", tag=71, length=1, cell_mask=FULL)
    s.add(
        "advance",
        action="group",
        tag=71,
        left_id=li,
        right_id=ri,
        row_mask=65535,
        col_mask=65535,
        k_mask=65535,
    )
    s.add("advance", action="end", tag=70)
    s.add(
        "advance",
        action="group",
        tag=70,
        group_index=0,
        left_id=li,
        right_id=ri,
        row_mask=65535,
        col_mask=65535,
        k_mask=65535,
    )
    s.add("advance", action="group", tag=70, group_index=0)
    s.add("take")
    s.add("idle")
    s.add("advance", tag=999)
    s.add(
        "advance",
        action="group",
        tag=70,
        group_index=1,
        left_id=ri,
        right_id=li,
        row_mask=0xFFFF,
        col_mask=0xFFFF,
        k_mask=0xFFFF,
    )
    s.add("advance", action="group", tag=70, group_index=2)
    s.finish(70, hold=3)
    # Abort at each occupied depth, DRAIN, and RESULT; immediately restart.
    for depth in range(6):
        tag = 80 + 2 * depth
        s.add("begin", tag=tag, length=16, cell_mask=FULL)
        s.add(
            "advance",
            action="group",
            tag=tag,
            left_id=li,
            right_id=ri,
            row_mask=65535,
            col_mask=65535,
            k_mask=65535,
        )
        for _ in range(depth):
            s.add("advance", tag=tag)
        s.add("advance", tag=tag, action="abort")
        s.add("take")
        s.tile(tag + 1, 1, ri, li)
    for state in ("drain", "result"):
        tag = 100 if state == "drain" else 102
        s.add("begin", tag=tag, length=16, cell_mask=FULL)
        s.add(
            "advance",
            action="group",
            tag=tag,
            left_id=li,
            right_id=ri,
            row_mask=65535,
            col_mask=65535,
            k_mask=65535,
        )
        s.add("advance", tag=tag, action="end")
        if state == "result":
            while s.phase == "drain":
                s.add("advance", tag=tag)
            s.add("advance", tag=tag, action="group")
        s.add("advance", tag=tag + 999, action="abort")
        s.add("advance", tag=tag, action="abort")
        s.add("take")
        s.tile(tag + 1, 16, li, ri)
    # Explicit per-level rounding, signed zero, FTZ and saturation vectors.
    special = [
        0x3F800001,
        0xBF800000,
        0x33800000,
        0xB3800000,
        0x80000000,
        0,
        0x00800000,
        0x80800000,
        1,
        0x80000001,
        0x7F7FFFFF,
        0xFF7FFFFF,
        0x3F7FFFFE,
        0x3F000000,
        0x40000000,
        0xC0000000,
    ]
    sl = s.matrix_id([special[r:] + special[:r] for r in range(16)])
    sr = s.matrix_id(
        [
            [0x3F800000 if (c + k) % 2 else 0x3F000000 for k in range(16)]
            for c in range(16)
        ]
    )
    s.tile(110, 64, sl, sr, bubbles=True)
    s.tile(111, 17, sl, sr, k_mask=0x8001)
    one = s.matrix_id([[0x3F800000] * 16 for _ in range(16)])
    rounded = s.matrix_id(
        [[0x4B800000, 0x3F800000, 0xCB800000, 0xBF800000] + [0] * 12 for _ in range(16)]
    )
    s.tile(112, 16, rounded, one)
    nonfused_left = s.matrix_id(
        [[0xBF800000, 0x3F800001] + [0] * 14 for _ in range(16)]
    )
    nonfused_right = s.matrix_id(
        [[0x3F800000, 0x3F7FFFFE] + [0x3F800000] * 14 for _ in range(16)]
    )
    s.tile(113, 16, nonfused_left, nonfused_right)
    negative = s.matrix_id([[0xBF800000] + [0] * 15 for _ in range(16)])
    positive = s.matrix_id([[0x3F800000] + [0] * 15 for _ in range(16)])
    s.add("begin", tag=114, length=48, cell_mask=FULL)
    for index, matrix, mask in ((0, negative, 1), (1, positive, 1), (2, sl, 0)):
        s.add(
            "advance",
            action="group",
            tag=114,
            group_index=index,
            left_id=matrix,
            right_id=one,
            row_mask=65535,
            col_mask=65535,
            k_mask=mask,
        )
    # Finish retirement before END, checking the empty-drain boundary.
    for _ in range(7):
        s.add("advance", tag=114)
    s.finish(114)
    s.add("begin", tag=115, length=16, cell_mask=FULL)
    s.add(
        "advance",
        tag=115,
        action="group",
        left_id=li,
        right_id=ri,
        row_mask=65535,
        col_mask=65535,
        k_mask=65535,
    )
    s.add("advance", tag=115, action="end")
    for _ in range(3):
        s.add("advance", tag=115)
    s.add("take")  # final tree value exists, but accumulation has not committed
    s.add("advance", tag=115)
    s.add("take")
    return {
        "schema_version": 1,
        "contract": {
            "m": 16,
            "n": 16,
            "pk": 16,
            "retire_after_advances": 5,
            "abort": "immediate_flush_no_arithmetic",
        },
        "operand_matrices": s.matrices,
        "instructions": s.instructions,
        "expected_actions": s.events,
        "results": s.results,
        "retirements": s.retirements,
        "total_results": len(s.results),
        "cycles": len(s.instructions) + 2,
        "not_run": [
            "RTL simulation excluded",
            "Queue take+push resource backpressure",
            "mid-operation reset",
            "X/Z fault injection",
            "NaN/Inf outside finite input domain",
        ],
    }


if __name__ == "__main__":
    Path(__file__).with_name("cases.json").write_text(
        json.dumps(generate(), indent=2) + "\n"
    )
