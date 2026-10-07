"""Match the checked bits of an opcode and retain its original wire value."""

import pycircuit as ac


@ac.struct
class Instruction:
    opcode: ac.u4
    is_compute: ac.u1


@ac.struct
class Result:
    ready: ac.u1
    valid: ac.u1
    data: Instruction


@ac.rule
def decode(instruction: Instruction) -> Instruction:
    result = instruction
    result.is_compute = (instruction.opcode & 9) == 8
    return result


@ac.module
def MaskedDecodePipeline(  # noqa: N802
    valid: ac.u1, data: Instruction, take: ac.u1
) -> Result:
    ready, available, instruction = ac.queue[Instruction](
        valid, data, stage_ready, depth=1, ready_policy="downstream_pop"  # noqa: F821
    )
    decoded = decode(instruction)
    stage_ready, out_valid, out_data = ac.queue[Instruction](
        available, decoded, take, depth=1, ready_policy="downstream_pop"
    )
    return Result(ready=ready, valid=out_valid, data=out_data)
