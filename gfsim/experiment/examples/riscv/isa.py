"""Small assembler and CPU decoder. The sequential oracle has its own decoder."""

import re

from .records import (Instruction, INVALID, ADD, ADDI, SUB, AND, OR, XOR,
                      SLT, LUI, LW, SW, BEQ, BNE, JAL, JALR, HALT)


def sext(value, width):
    sign = 1 << (width - 1)
    return (value ^ sign) - sign


def decode(word):
    opcode, f3, f7 = word & 127, (word >> 12) & 7, word >> 25
    rd, rs1, rs2 = (word >> 7) & 31, (word >> 15) & 31, (word >> 20) & 31
    op, immediate = INVALID, 0
    if opcode == 0x33:
        op = {(0, 0): ADD, (0, 32): SUB, (7, 0): AND,
              (6, 0): OR, (4, 0): XOR, (2, 0): SLT}.get((f3, f7), INVALID)
    elif opcode in (0x13, 0x03, 0x67):
        op = {(0x13, 0): ADDI, (0x03, 2): LW, (0x67, 0): JALR}.get((opcode, f3), INVALID)
        immediate, rs2 = sext(word >> 20, 12), 0
    elif opcode == 0x37:
        op, immediate, rs1, rs2 = LUI, word & 0xfffff000, 0, 0
    elif opcode == 0x23 and f3 == 2:
        op, immediate, rd = SW, sext(((word >> 25) << 5) | ((word >> 7) & 31), 12), 0
    elif opcode == 0x63 and f3 in (0, 1):
        bits = ((word >> 31) << 12) | (((word >> 7) & 1) << 11)
        bits |= (((word >> 25) & 63) << 5) | (((word >> 8) & 15) << 1)
        op, immediate, rd = (BEQ if f3 == 0 else BNE), sext(bits, 13), 0
    elif opcode == 0x6f:
        bits = ((word >> 31) << 20) | (((word >> 12) & 255) << 12)
        bits |= (((word >> 20) & 1) << 11) | (((word >> 21) & 1023) << 1)
        op, immediate, rs1, rs2 = JAL, sext(bits, 21), 0, 0
    elif word == 0x00100073:
        op, rd, rs1, rs2 = HALT, 0, 0, 0
    return Instruction(word, op, rd, rs1, rs2, immediate)


def sources(instruction):
    op = instruction.op
    first = op in (ADD, ADDI, SUB, AND, OR, XOR, SLT, LW, SW, BEQ, BNE, JALR)
    second = op in (ADD, SUB, AND, OR, XOR, SLT, SW, BEQ, BNE)
    return instruction.rs1 if first else 0, instruction.rs2 if second else 0


def register(text):
    if not re.fullmatch(r'x(?:[0-9]|[12][0-9]|3[01])', text):
        raise ValueError(f'invalid register {text!r}')
    return int(text[1:])


def immediate(text, width):
    value = int(text, 0)
    if not -(1 << (width - 1)) <= value < (1 << (width - 1)):
        raise ValueError(f'immediate {value} does not fit signed {width} bits')
    return value & ((1 << width) - 1)


def encode(line, pc, labels):
    parts = re.split(r'[\s,()]+', line.strip().lower())
    parts = [p for p in parts if p]
    name, args = parts[0], parts[1:]
    if name == 'halt' and not args:
        return 0x00100073
    if name == 'nop' and not args:
        return 0x00000013
    if name == '.word' and len(args) == 1:
        word = int(args[0], 0)
        if not 0 <= word <= 0xffffffff:
            raise ValueError('.word must be an unsigned 32-bit integer')
        return word
    r_ops = {'add': (0, 0), 'sub': (0, 32), 'and': (7, 0),
             'or': (6, 0), 'xor': (4, 0), 'slt': (2, 0)}
    if name in r_ops and len(args) == 3:
        rd, rs1, rs2 = map(register, args)
        f3, f7 = r_ops[name]
        return (f7 << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | 0x33
    if name == 'addi' and len(args) == 3:
        rd, rs1, imm = register(args[0]), register(args[1]), immediate(args[2], 12)
        return (imm << 20) | (rs1 << 15) | (rd << 7) | 0x13
    if name in ('lw', 'sw', 'jalr') and len(args) == 3:
        reg, imm, base = register(args[0]), immediate(args[1], 12), register(args[2])
        if name == 'sw':
            return ((imm >> 5) << 25) | (reg << 20) | (base << 15) | (2 << 12) | ((imm & 31) << 7) | 0x23
        return (imm << 20) | (base << 15) | ((2 if name == 'lw' else 0) << 12) | (reg << 7) | (0x03 if name == 'lw' else 0x67)
    if name == 'lui' and len(args) == 2:
        rd, imm = register(args[0]), int(args[1], 0)
        if not 0 <= imm < (1 << 20):
            raise ValueError('LUI requires an unsigned 20-bit immediate')
        return (imm << 12) | (rd << 7) | 0x37
    if name in ('beq', 'bne', 'jal'):
        if len(args) != (2 if name == 'jal' else 3):
            raise ValueError(f'wrong operands: {line}')
        target = args[-1]
        offset = labels[target] - pc if target in labels else int(target, 0)
        width = 21 if name == 'jal' else 13
        imm = immediate(str(offset), width)
        if offset % 2:
            raise ValueError('branch/jump offset must be even')
        if name == 'jal':
            return (((imm >> 20) & 1) << 31) | (((imm >> 1) & 1023) << 21) | (((imm >> 11) & 1) << 20) | (((imm >> 12) & 255) << 12) | (register(args[0]) << 7) | 0x6f
        return (((imm >> 12) & 1) << 31) | (((imm >> 5) & 63) << 25) | (register(args[1]) << 20) | (register(args[0]) << 15) | ((name == 'bne') << 12) | (((imm >> 1) & 15) << 8) | (((imm >> 11) & 1) << 7) | 0x63
    raise ValueError(f'unsupported instruction or operands: {line}')


def assemble(source):
    labels, lines = {}, []
    for number, raw in enumerate(source.splitlines(), 1):
        line = raw.split('#', 1)[0].strip().lower()
        while ':' in line:
            label, line = line.split(':', 1)
            label, line = label.strip(), line.strip()
            if not re.fullmatch(r'[a-z_][a-z_0-9]*', label) or label in labels:
                raise ValueError(f'line {number}: invalid or duplicate label {label!r}')
            labels[label] = 4 * len(lines)
        if line:
            lines.append((number, line))
    words = []
    for pc, (number, line) in enumerate(lines):
        try:
            words.append(encode(line, pc * 4, labels))
        except ValueError as error:
            raise ValueError(f'line {number}: {error}') from error
    return tuple(words)
