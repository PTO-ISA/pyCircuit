# ruff: noqa: F823, F841, N802 -- captured hardware source.
from rv32i_core.types import make_decoded
import pycircuit as ac
from rv32i_core.types import Decoded


# ndf: implements=RV32-SPEC-ISA-001,RV32-SPEC-EEI-001
def compute_Decoder(instruction: ac.u32) -> Decoded:
    opcode: ac.u7 = instruction[0:7]
    funct3: ac.u3 = instruction[12:15]
    funct7: ac.u7 = instruction[25:32]
    zero: ac.u1 = 0
    imm_i: ac.u32 = instruction[20:32]
    imm_s: ac.u32 = ac.concat(instruction[25:32], instruction[7:12])
    imm_b: ac.u32 = ac.concat(
        instruction[31:32],
        instruction[7:8],
        instruction[25:31],
        instruction[8:12],
        zero,
    )
    imm_j: ac.u32 = ac.concat(
        instruction[31:32],
        instruction[12:20],
        instruction[20:21],
        instruction[21:31],
        zero,
    )
    if instruction[31:32]:
        imm_i = imm_i | 4294963200
        imm_s = imm_s | 4294963200
        imm_b = imm_b | 4294959104
        imm_j = imm_j | 4292870144
    result = make_decoded()
    result.rs1 = instruction[15:20]
    result.rs2 = instruction[20:25]
    result.rd = instruction[7:12]
    result.tval = instruction if instruction[0:2] == 3 else instruction & 65535
    if opcode == 55:
        result.operation = 10
        result.immediate = instruction & 4294963200
        result.writes = 1
        result.trap = 0
    elif opcode == 23:
        result.operation = 11
        result.immediate = instruction & 4294963200
        result.writes = 1
        result.trap = 0
    elif opcode == 111:
        result.operation = 12
        result.jump = 1
        result.immediate = imm_j
        result.writes = 1
        result.trap = 0
    elif opcode == 103:
        if funct3 == 0:
            result.operation = 12
            result.jump = 2
            result.immediate = imm_i
            result.use1 = 1
            result.writes = 1
            result.trap = 0
    elif opcode == 99:
        if (
            funct3 == 0
            or funct3 == 1
            or funct3 == 4
            or (funct3 == 5)
            or (funct3 == 6)
            or (funct3 == 7)
        ):
            result.branch = 1
            result.condition = funct3
            result.immediate = imm_b
            result.use1 = 1
            result.use2 = 1
            result.trap = 0
    elif opcode == 3:
        if funct3 == 0 or funct3 == 1 or funct3 == 2 or (funct3 == 4) or (funct3 == 5):
            result.load = 1
            result.use1 = 1
            result.writes = 1
            result.immediate_rhs = 1
            result.immediate = imm_i
            result.size = funct3[0:2]
            result.signed_load = ~funct3[2:3]
            result.trap = 0
    elif opcode == 35:
        if funct3 == 0 or funct3 == 1 or funct3 == 2:
            result.store = 1
            result.use1 = 1
            result.use2 = 1
            result.immediate_rhs = 1
            result.immediate = imm_s
            result.size = funct3[0:2]
            result.trap = 0
    elif opcode == 19 or opcode == 51:
        result.use1 = 1
        result.use2 = opcode == 51
        result.immediate_rhs = opcode == 19
        result.immediate = imm_i
        result.writes = 1
        result.trap = 0
        if funct3 == 0:
            if opcode == 51 and funct7 == 32:
                result.operation = 1
            elif opcode == 51 and funct7 != 0:
                result.trap = 1
        elif funct3 == 1:
            result.operation = 2
            if funct7 != 0:
                result.trap = 1
        elif funct3 == 2:
            result.operation = 3
            if opcode == 51 and funct7 != 0:
                result.trap = 1
        elif funct3 == 3:
            result.operation = 4
            if opcode == 51 and funct7 != 0:
                result.trap = 1
        elif funct3 == 4:
            result.operation = 5
            if opcode == 51 and funct7 != 0:
                result.trap = 1
        elif funct3 == 5:
            if funct7 == 0:
                result.operation = 6
            elif funct7 == 32:
                result.operation = 7
            else:
                result.trap = 1
        elif funct3 == 6:
            result.operation = 8
            if opcode == 51 and funct7 != 0:
                result.trap = 1
        else:
            result.operation = 9
            if opcode == 51 and funct7 != 0:
                result.trap = 1
    elif opcode == 15:
        if funct3 == 0:
            result.trap = 0
    elif opcode == 115:
        if instruction == 115:
            result.cause = 11
            result.tval = 0
        elif instruction == 1048691:
            result.cause = 3
            result.tval = 0
    return result
