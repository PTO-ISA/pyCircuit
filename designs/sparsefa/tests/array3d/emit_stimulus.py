"""Emit only independent input operands and commands as a pyCircuit System.

No expected action, retirement, arithmetic result or oracle decision enters
bench.py. The compiler captures the DUT; this script never imports it.
"""

import json
from pathlib import Path

OPS = {"idle": 0, "begin": 1, "advance": 2, "take": 3}
ACTIONS = {"bubble": 0, "group": 1, "end": 2, "abort": 3}


def render(cases):
    matrices = cases["operand_matrices"]
    instructions = cases["instructions"]
    if len(matrices) > 16 or len(instructions) > 512:
        raise ValueError("input schedule exceeds the declared ROM extent")
    if not 1 <= cases["cycles"] < 512:
        raise ValueError("cycle horizon exceeds the fixed bench counter")
    for matrix in matrices:
        if len(matrix) != 16 or any(len(row) != 16 for row in matrix):
            raise ValueError("every input operand must contain all 16x16 words")
    lines = [
        "# ruff: noqa: F823, F841, N802 -- captured state and hardware names.",
        '"""Generated input-only stimulus; regenerate with emit_stimulus.py."""',
        "import pycircuit as ac",
        "from fa2.fa2_array3d import Array16x16x16",
        "from fa2.fa2_array3d_interfaces import Array3DAdvance, Array3DBegin, Array3DResult",
        "from fa2.fa2_array_interfaces import Row16",
        "",
        "",
        "@ac.struct",
        "class Instruction:",
        "    op: ac.u2",
        "    action: ac.u2",
        "    tag: ac.u32",
        "    length: ac.u7",
        "    cell_mask: ac.bits[256]",
        "    index: ac.u3",
        "    left_id: ac.u4",
        "    right_id: ac.u4",
        "    row_mask: ac.u16",
        "    col_mask: ac.u16",
        "    k_mask: ac.u16",
        "",
        "",
        "def operands() -> ac.table[256, Row16]:",
        "    data: ac.table[256, Row16] = (",
    ]
    for matrix in matrices:
        for row in matrix:
            fields = ", ".join(f"c{i}={word}" for i, word in enumerate(row))
            lines.append(f"        Row16({fields}),")
    zero_fields = ", ".join(f"c{i}=0" for i in range(16))
    lines += [f"        Row16({zero_fields}),"] * (256 - 16 * len(matrices))
    lines += [
        "    )",
        "    return data",
        "",
        "",
        "def matrix(index: ac.u4) -> ac.table[16, Row16]:",
        "    words = operands()",
        "    base: ac.u8 = index",
        "    base = (base | 0) << 4",
        "    return (",
    ]
    lines += [f"        words[base + {i}]," for i in range(16)]
    lines += [
        "    )",
        "",
        "",
        "def schedule() -> ac.table[512, Instruction]:",
        "    data: ac.table[512, Instruction] = (",
    ]
    for cycle, item in enumerate(instructions):
        if item["cycle"] != cycle:
            raise ValueError("instruction cycles must be dense and ordered")
        fields = [f"op={OPS[item['op']]}", f"action={ACTIONS[item['action']]}"]
        for key in ("tag", "length", "cell_mask"):
            fields.append(f"{key}={item[key]}")
        fields.append(f"index={item['group_index']}")
        for key in ("left_id", "right_id", "row_mask", "col_mask", "k_mask"):
            fields.append(f"{key}={item[key]}")
        lines.append("        Instruction(" + ", ".join(fields) + "),")
    lines += ["        Instruction(),"] * (512 - len(instructions))
    lines += [
        "    )",
        "    return data",
        "",
        "",
        "@ac.rule",
        "def publish(result: Array3DResult) -> None:",
    ]
    for row in range(16):
        lines.append(f"    row{row} = result.rows[{row}]")
        values = ", ".join(f"row{row}.c{i}" for i in range(16))
        lines.append(f'    ac.log("info", "array3d_row", result.tag, {row}, {values})')
    lines += [
        '    ac.log("info", "array3d_result", result.tag, result.groups, result.useful_macs)',
        "",
        "",
        "@ac.system",
        "def Array3DSystem():",
        "    cycle: ac.u9 = 0",
        "    completed: ac.u9 = 0",
        "    array = Array16x16x16()",
        "    program = schedule()",
        "    command = program[cycle]",
        "    left = matrix(command.left_id)",
        "    right = matrix(command.right_id)",
        "",
        "    @ac.rule",
        "    def tick() -> None:",
        f"        if cycle == {len(instructions)}:",
        '            ac.log("info", "array3d_pass", completed)',
        "        cycle = cycle + 1",
        "",
        "    @ac.rule",
        "    def drive() -> None:",
        "        if command.op == 1:",
        "            array.begin(Array3DBegin(tag=command.tag, length=command.length, cell_mask=command.cell_mask))",
        '            ac.log("info", "array3d_action", cycle, 1, command.action, command.tag, command.index, 1)',
        "        elif command.op == 2:",
        "            accepted = array.advance(Array3DAdvance(action=command.action, tag=command.tag, index=command.index, left=left, right=right, row_mask=command.row_mask, col_mask=command.col_mask, k_mask=command.k_mask))",
        '            ac.log("info", "array3d_action", cycle, 2, command.action, command.tag, command.index, accepted)',
        "        elif command.op == 3:",
        "            result = array.take()",
        "            publish(result)",
        "            completed = completed + 1",
        '            ac.log("info", "array3d_action", cycle, 3, command.action, result.tag, command.index, 1)',
        "",
        "    tick()",
        "    drive()",
        "",
    ]
    return "\n".join(lines)


def main():
    here = Path(__file__).resolve().parent
    cases = json.loads((here / "cases.json").read_text())
    (here / "bench.py").write_text(render(cases))


if __name__ == "__main__":
    main()
