"""Pool literal test operands without changing the independent input schedule.

Only replace the stimulus section of bench.py. No oracle acceptance flag or
expected result is emitted. The DUT is never imported or executed here.
"""
import json
from pathlib import Path


def main():
    here = Path(__file__).resolve().parent
    cases = json.loads((here / "cases.json").read_text())
    vectors = list(dict.fromkeys(tuple(item[key]) for item in cases["instructions"]
                                for key in ("lhs", "rhs")))
    if len(vectors) > 16 or len(cases["instructions"]) > 256:
        raise ValueError("literal stimulus exceeds the declared test ROM extent")
    if vectors[0] != (0,) * 16:
        raise ValueError("padding requires operand zero at index zero")
    bench = here / "bench.py"
    current = bench.read_text()
    header = current[:current.index("@ac.struct\nclass Instruction:")]
    body = current[current.index("@ac.rule\ndef publish("):]
    text = '''@ac.struct
class Instruction:
    op: ac.u3
    tag: ac.u32
    length: ac.u7
    cell_mask: ac.bits[256]
    index: ac.u7
    left_id: ac.u4
    right_id: ac.u4
    row_mask: ac.u16
    col_mask: ac.u16


def operand_words() -> ac.table[16, Row16]:
    data: ac.table[16, Row16] = (
'''
    for words in vectors:
        fields = ", ".join(f"c{i}={value}" for i, value in enumerate(words))
        text += f"        Row16({fields}),\n"
    zero_row = ", ".join(f"c{i}=0" for i in range(16))
    text += f"        Row16({zero_row}),\n" * (16 - len(vectors))
    text += "    )\n    return data\n\n\n"
    text += "def vector(row: Row16) -> ac.table[16, ac.u32]:\n"
    text += "    return (" + ", ".join(f"row.c{i}" for i in range(16)) + ")\n\n\n"
    text += "def schedule() -> ac.table[256, Instruction]:\n"
    text += "    data: ac.table[256, Instruction] = (\n"
    for item in cases["instructions"]:
        fields = [f"{key}={item[key]}" for key in
                  ("op", "tag", "length", "cell_mask", "index")]
        fields += [f"left_id={vectors.index(tuple(item['lhs']))}",
                   f"right_id={vectors.index(tuple(item['rhs']))}"]
        fields += [f"{key}={item[key]}" for key in ("row_mask", "col_mask")]
        text += "        Instruction(" + ", ".join(fields) + "),\n"
    text += "        Instruction(),\n" * (256 - len(cases["instructions"]))
    text += "    )\n    return data\n\n\n"
    bench.write_text(header + text + body)


if __name__ == "__main__":
    main()
