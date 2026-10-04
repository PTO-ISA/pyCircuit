"""Independent sequential RV32I interpreter; never imports the ACPy decoder."""
from pathlib import Path

MASK = 0xffffffff
STOP = 0x30004


def signed(value, bits=32):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def load_image(path, size=0x400000):
    path = Path(path)
    memory, address = bytearray(size), 0
    for token in path.read_text().split():
        if token.startswith('@'):
            address = int(token[1:], 16)
            continue
        width = 4 if path.suffix == '.hex' else 1
        if not 0 <= address <= size - width:
            raise ValueError(f'image address out of range: {address:#x}')
        memory[address:address + width] = int(token, 16).to_bytes(width, 'little')
        address += width
    return memory


def interpret(image, limit=100000, trace=False):
    memory, initial, regs = bytearray(image), bytes(image), [0] * 32
    pc, commits, written = 0, [], set()
    for count in range(1, limit + 1):
        if pc % 4 or pc > len(memory) - 4:
            raise ValueError(f'fetch fault at {pc:#x}')
        word = int.from_bytes(memory[pc:pc + 4], 'little')
        op, f3, f7 = word & 127, word >> 12 & 7, word >> 25
        rd, rs1, rs2 = word >> 7 & 31, word >> 15 & 31, word >> 20 & 31
        a, b = regs[rs1], regs[rs2]
        original_b, access = b, None
        target, value, store = (pc + 4) & MASK, None, None
        halt = word == 0x00100073
        if halt:
            pass
        elif op in (0x33, 0x13):
            if op == 0x13:
                b = signed(word >> 20, 12) & MASK
            if op == 0x33 and not (f7 == 0 or (f7 == 32 and f3 in (0, 5))):
                raise ValueError(f'illegal arithmetic instruction {word:08x}')
            if op == 0x13 and f3 in (1, 5) and not (f7 == 0 or (f7 == 32 and f3 == 5)):
                raise ValueError(f'illegal shift instruction {word:08x}')
            if f3 == 0:
                value = a - b if op == 0x33 and f7 == 32 else a + b
            elif f3 == 1:
                value = a << (b & 31)
            elif f3 == 2:
                value = int(signed(a) < signed(b))
            elif f3 == 3:
                value = int(a < b)
            elif f3 == 4:
                value = a ^ b
            elif f3 == 5:
                value = (signed(a) if f7 == 32 else a) >> (b & 31)
            elif f3 == 6:
                value = a | b
            else:
                value = a & b
        elif op in (0x37, 0x17):
            value = (word & 0xfffff000) + (pc if op == 0x17 else 0)
        elif op in (3, 0x23):
            legal = f3 in ((0, 1, 2, 4, 5) if op == 3 else (0, 1, 2))
            if not legal:
                raise ValueError(f'illegal memory instruction {word:08x}')
            imm = word >> 20 if op == 3 else ((word >> 25) << 5) | (word >> 7 & 31)
            address, width = (a + signed(imm, 12)) & MASK, 1 << (f3 & 3)
            access = [address, width]
            if address % width or address > len(memory) - width:
                raise ValueError(f'memory fault at {address:#x}')
            if op == 3:
                value = int.from_bytes(memory[address:address + width], 'little', signed=f3 < 4)
            else:
                store = [address, width, b & ((1 << (8 * width)) - 1)]
                memory[address:address + width] = store[2].to_bytes(width, 'little')
                written.update(range(address, address + width))
        elif op == 0x63 and f3 in (0, 1, 4, 5, 6, 7):
            imm = (word >> 31 << 12) | ((word >> 7 & 1) << 11) | ((word >> 25 & 63) << 5) | ((word >> 8 & 15) << 1)
            taken = {0: a == b, 1: a != b, 4: signed(a) < signed(b), 5: signed(a) >= signed(b),
                     6: a < b, 7: a >= b}[f3]
            if taken:
                target = (pc + signed(imm, 13)) & MASK
        elif op == 0x6f:
            imm = (word >> 31 << 20) | ((word >> 12 & 255) << 12) | ((word >> 20 & 1) << 11) | ((word >> 21 & 1023) << 1)
            value, target = target, (pc + signed(imm, 21)) & MASK
        elif op == 0x67 and f3 == 0:
            value, target = target, ((a + signed(word >> 20, 12)) & MASK) & ~1
        else:
            raise ValueError(f'illegal instruction {word:08x} at {pc:#x}')
        write = None
        if value is not None and rd:
            regs[rd] = value & MASK
            write = [rd, regs[rd]]
        if trace:
            commits.append(dict(pc=pc, word=word, left=a, right=original_b,
                                write=write, store=store, memory=access, next_pc=target))
        if halt or memory[STOP]:
            return dict(registers=regs, memory_changes=[[i, memory[i]] for i in sorted(written) if memory[i] != initial[i]],
                        instructions=count, commits=commits, stopped=True)
        pc = target
    raise ValueError('reference instruction limit exceeded')
