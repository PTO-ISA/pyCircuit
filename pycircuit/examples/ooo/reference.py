"""Sequential RV32 oracle: independent decode and execution, no model imports."""
MASK = (1 << 32) - 1


def signed(value, width=32):
    return value - (1 << width) if value & (1 << (width - 1)) else value


def interpret(words, registers, data, base=4096, limit=20000):
    regs, mem = list(registers), list(data)
    pc, trace = 0, []
    for _ in range(limit):
        fault = 1 if pc % 4 or pc // 4 >= len(words) else 0
        word = 0 if fault else words[pc // 4]
        opcode, f3, f7 = word & 127, (word >> 12) & 7, word >> 25
        rd, rs1, rs2 = (word >> 7) & 31, (word >> 15) & 31, (word >> 20) & 31
        a, b = regs[rs1], regs[rs2]
        next_pc, value, store, halt = (pc + 4) & MASK, None, None, False
        if fault:
            pass
        elif word == 0x00100073:
            halt = True
        elif opcode == 0x33 and (f3, f7) in ((0, 0), (0, 32), (7, 0), (6, 0), (4, 0), (2, 0)):
            value = {(0, 0): a + b, (0, 32): a - b, (7, 0): a & b,
                     (6, 0): a | b, (4, 0): a ^ b, (2, 0): int(signed(a) < signed(b))}[f3, f7]
        elif opcode == 0x13 and f3 == 0:
            value = a + signed(word >> 20, 12)
        elif opcode == 0x37:
            value = word & 0xfffff000
        elif (opcode == 3 and f3 == 2) or (opcode == 0x23 and f3 == 2):
            imm = signed(word >> 20, 12) if opcode == 3 else signed(((word >> 25) << 5) | ((word >> 7) & 31), 12)
            address = (a + imm) & MASK
            if address < base or (address - base) % 4 or (address - base) // 4 >= len(mem):
                fault = 3
            elif opcode == 3:
                value = mem[(address - base) // 4]
            else:
                mem[(address - base) // 4] = b
                store = [address, b]
        elif opcode == 0x63 and f3 in (0, 1):
            imm = ((word >> 31) << 12) | (((word >> 7) & 1) << 11) | (((word >> 25) & 63) << 5) | (((word >> 8) & 15) << 1)
            if (a == b) == (f3 == 0):
                next_pc = (pc + signed(imm, 13)) & MASK
        elif opcode == 0x6f:
            imm = ((word >> 31) << 20) | (((word >> 12) & 255) << 12) | (((word >> 20) & 1) << 11) | (((word >> 21) & 1023) << 1)
            value, next_pc = next_pc, (pc + signed(imm, 21)) & MASK
        elif opcode == 0x67 and f3 == 0:
            value, next_pc = next_pc, ((a + signed(word >> 20, 12)) & MASK) & ~1
        else:
            fault = 2
        write = None
        if value is not None and rd and not fault:
            regs[rd] = value & MASK
            write = [rd, regs[rd]]
        trace.append(dict(pc=pc, word=word, write=write, store=store, fault=fault, halt=halt,
                          registers=list(regs), data=list(mem)))
        if halt or fault:
            return trace
        pc = next_pc
    raise AssertionError('reference exceeded instruction limit')
