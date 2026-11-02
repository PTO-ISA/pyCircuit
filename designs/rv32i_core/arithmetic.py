# ruff: noqa: F823, F841, N802 -- captured hardware source.
from rv32i_core.types import make_packet
import pycircuit as ac
from rv32i_core.types import Decoded, Packet


# ndf: implements=RV32-SPEC-ISA-001
def compute_Arithmetic(
    control: Decoded, pc: ac.u32, instruction: ac.u32, first: ac.u32, second: ac.u32
) -> Packet:
    rhs: ac.u32 = control.immediate if control.immediate_rhs else second
    amount: ac.u5 = rhs[0:5]
    wide_amount: ac.u6 = amount
    ones: ac.u32 = 4294967295
    fill: ac.u32 = ones << 32 - wide_amount if first[31:32] and amount != 0 else 0
    signed_less: ac.u1 = first ^ 2147483648 < rhs ^ 2147483648
    value: ac.u32 = first + rhs
    if control.operation == 1:
        value = first - rhs
    elif control.operation == 2:
        value = first << amount
    elif control.operation == 3:
        value = 1 if signed_less else 0
    elif control.operation == 4:
        value = 1 if first < rhs else 0
    elif control.operation == 5:
        value = first ^ rhs
    elif control.operation == 6:
        value = first >> amount
    elif control.operation == 7:
        value = first >> amount | fill
    elif control.operation == 8:
        value = first | rhs
    elif control.operation == 9:
        value = first & rhs
    elif control.operation == 10:
        value = control.immediate
    elif control.operation == 11:
        value = pc + control.immediate
    elif control.operation == 12:
        value = pc + 4
    less: ac.u1 = first ^ 2147483648 < second ^ 2147483648
    taken: ac.u1 = 0
    if control.branch:
        if control.condition == 0:
            taken = first == second
        elif control.condition == 1:
            taken = first != second
        elif control.condition == 4:
            taken = less
        elif control.condition == 5:
            taken = ~less
        elif control.condition == 6:
            taken = first < second
        else:
            taken = first >= second
    target: ac.u32 = pc + control.immediate
    if control.jump == 2:
        target = first + control.immediate & 4294967294
    redirect: ac.u1 = taken or control.jump != 0
    result = make_packet()
    result.valid = 1
    result.pc = pc
    result.instruction = instruction
    result.next_pc = target if redirect else pc + 4
    result.rd = control.rd
    result.writes = control.writes
    result.result = value
    result.load = control.load
    result.store = control.store
    result.address = first + control.immediate
    result.store_data = second
    result.size = control.size
    result.signed_load = control.signed_load
    result.redirect = redirect
    result.trap = control.trap
    result.cause = control.cause
    result.tval = control.tval
    if control.trap and control.cause == 3:
        result.tval = pc
    if redirect and target[0:2] != 0:
        result.trap = 1
        result.cause = 0
        result.tval = target
    if control.load or control.store:
        if (
            control.size == 1
            and result.address[0:1] != 0
            or (control.size == 2 and result.address[0:2] != 0)
        ):
            result.trap = 1
            result.cause = 6 if control.store else 4
            result.tval = result.address
    if result.trap:
        result.redirect = 0
        result.writes = 0
        result.next_pc = pc
    return result
