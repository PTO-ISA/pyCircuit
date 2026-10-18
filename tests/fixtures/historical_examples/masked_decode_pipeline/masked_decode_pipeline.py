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


def decode(instruction: Instruction) -> Instruction:
    result = instruction
    result.is_compute = (instruction.opcode & 9) == 8
    return result


@ac.module
def MaskedDecodePipeline(  # noqa: N802
    valid: ac.u1, data: Instruction, take: ac.u1
) -> Result:
    ingress = ac.queue[Instruction](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = ac.queue[Instruction](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @ac.rule
    def transfer() -> None:
        instruction = ingress.pop()
        decoded = decode(instruction)
        egress.push(decoded)

    transfer()
    return Result(ready=ingress.ready, valid=egress.valid, data=egress.data)
