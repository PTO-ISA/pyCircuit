"""Data-only independent rational oracle and protocol schedule for the array."""

import json
from fractions import Fraction
from pathlib import Path

FULL = (1 << 256) - 1
OPS = {"idle": 0, "begin": 1, "step": 2, "end": 3, "take": 4, "abort": 5}


def decode(word):
    exponent = (word >> 23) & 255
    if exponent == 255:
        raise ValueError("nonfinite input outside contract")
    if not exponent:
        return Fraction(0)
    value = Fraction((word & 0x7FFFFF) + (1 << 23)) * Fraction(2) ** (exponent - 150)
    return -value if word >> 31 else value


def encode(value, zero_sign=0):
    sign = (1 << 31) if value < 0 else 0
    if not value:
        return zero_sign & (1 << 31)
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


def mac(old, left, right):
    product = encode(decode(left) * decode(right), left ^ right)
    a, b = decode(old), decode(product)
    return encode(a + b, old if abs(a) >= abs(b) else product)


class Schedule:
    def __init__(self):
        self.instructions, self.actions, self.results = [], [], []
        self.state = "idle"

    def add(self, name, **fields):
        item = {
            "cycle": len(self.instructions),
            "op": OPS[name],
            "tag": 0,
            "length": 0,
            "cell_mask": 0,
            "index": 0,
            "lhs": [0] * 16,
            "rhs": [0] * 16,
            "row_mask": 0,
            "col_mask": 0,
        }
        item.update(fields)
        accepted = False
        if name == "begin" and self.state == "idle" and 1 <= item["length"] <= 64:
            accepted = True
            self.state, self.tag, self.length = "active", item["tag"], item["length"]
            self.mask, self.steps, self.useful = item["cell_mask"], 0, 0
            self.rows = [[0] * 16 for _ in range(16)]
            self.begin = item["cycle"]
        elif (
            name == "step"
            and self.state == "active"
            and item["tag"] == self.tag
            and item["index"] == self.steps
            and self.steps < self.length
        ):
            accepted = True
            for r in range(16):
                for c in range(16):
                    if (
                        (self.mask >> (16 * r + c)) & 1
                        and (item["row_mask"] >> r) & 1
                        and (item["col_mask"] >> c) & 1
                    ):
                        self.rows[r][c] = mac(
                            self.rows[r][c], item["lhs"][r], item["rhs"][c]
                        )
                        self.useful += 1
            self.steps += 1
        elif (
            name == "end"
            and self.state == "active"
            and item["tag"] == self.tag
            and self.steps == self.length
        ):
            accepted = True
            self.state, self.end = "result", item["cycle"]
        elif name == "take" and self.state == "result":
            accepted = True
            # take has no tag argument; its action carries the returned tag.
            item["tag"] = self.tag
            self.results.append(
                {
                    "tag": self.tag,
                    "steps": self.steps,
                    "useful_macs": self.useful,
                    "rows": self.rows,
                    "begin_cycle": self.begin,
                    "end_cycle": self.end,
                    "take_cycle": item["cycle"],
                }
            )
            self.state = "idle"
        elif name == "abort" and self.state != "idle" and item["tag"] == self.tag:
            accepted, self.state = True, "idle"
        item["accepted"] = accepted
        self.instructions.append(item)
        if accepted:
            self.actions.append([item["cycle"], item["op"], item["tag"], item["index"]])

    def tile(self, tag, count, row_mask=65535, cell_mask=FULL, hold=0, bubbles=False):
        self.add("begin", tag=tag, length=count, cell_mask=cell_mask)
        for k in range(count):
            if bubbles and k % 2 == 0:
                self.add("idle")
            left = [
                encode(
                    Fraction((r + 1) * (-1 if (k + r) % 3 == 0 else 1), 1 << (k % 3))
                )
                for r in range(16)
            ]
            right = [encode(Fraction(2 * c + 1, 1 << (k % 2))) for c in range(16)]
            self.add(
                "step",
                tag=tag,
                index=k,
                lhs=left,
                rhs=right,
                row_mask=row_mask,
                col_mask=65535 if not bubbles or k % 2 else 0xAAAA,
            )
        self.add("end", tag=tag)
        for _ in range(hold):
            self.add("idle")
        self.add("take")


def generate():
    s = Schedule()
    for tag, k in enumerate((1, 4, 16, 64), 1):
        s.tile(tag, k)
    for row in range(16):
        s.tile(100 + row, 2, row_mask=1 << row)
    s.tile(
        200,
        4,
        cell_mask=sum(
            1 << (r * 16 + c)
            for r in range(16)
            for c in range(16)
            if (r + 2 * c) % 3 == 0
        ),
        hold=3,
        bubbles=True,
    )
    s.tile(201, 2, cell_mask=0, hold=2)
    # Known-invalid requests cannot advance the operation or create output.
    s.add("take")
    s.add("abort", tag=900)
    s.add("begin", tag=900, length=0, cell_mask=FULL)
    s.add("begin", tag=900, length=65, cell_mask=FULL)
    s.add("begin", tag=901, length=2, cell_mask=FULL)
    s.add("begin", tag=902, length=1, cell_mask=FULL)
    s.add("end", tag=901)
    beat = {
        "lhs": [encode(Fraction(2))] * 16,
        "rhs": [encode(Fraction(3))] * 16,
        "row_mask": 65535,
        "col_mask": 65535,
    }
    s.add("step", tag=902, index=0, **beat)
    s.add("step", tag=901, index=1, **beat)
    s.add("step", tag=901, index=0, **beat)
    s.add("step", tag=901, index=0, **beat)
    s.add("abort", tag=902)
    s.add("abort", tag=901)
    s.add("end", tag=901)
    s.add("take")
    s.tile(903, 1, hold=3)
    s.add("begin", tag=904, length=1, cell_mask=FULL)
    s.add("step", tag=904, index=0, **beat)
    s.add("end", tag=904)
    s.add("begin", tag=905, length=1, cell_mask=FULL)
    s.add("step", tag=904, index=1, **beat)
    s.add("abort", tag=904)
    s.add("take")
    s.tile(906, 1)
    # Nonfused witness: rounded product and cancellation differ from fused sum.
    s.add("begin", tag=950, length=3, cell_mask=FULL)
    for k, (a, b) in enumerate(
        ((0xBF800000, 0x3F800000), (0x3F800001, 0x3F7FFFFE), (0x00800000, 0x3F000000))
    ):
        s.add(
            "step",
            tag=950,
            index=k,
            lhs=[a] * 16,
            rhs=[b] * 16,
            row_mask=65535,
            col_mask=65535,
        )
    s.add("end", tag=950)
    s.add("take")
    return {
        "schema": 1,
        "operations": OPS,
        "instructions": s.instructions,
        "accepted_actions": s.actions,
        "results": s.results,
        "total_results": len(s.results),
        "cycles": len(s.instructions) + 2,
        "not_run": [
            "Queue take+push backpressure",
            "reset midoperation",
            "X/Z failure injection",
            "NaN/Inf outside finite input domain",
            "RTL simulation",
            "full attention integration",
        ],
    }


if __name__ == "__main__":
    path = Path(__file__).with_name("cases.json")
    path.write_text(json.dumps(generate(), indent=2) + "\n")
    print(path)  # noqa: T201 -- CLI generated data location.
