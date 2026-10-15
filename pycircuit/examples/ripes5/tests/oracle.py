"""Sequential ISA oracle driven by LLVM disassembly, independent of ACPy decode.

The oracle uses Python unbounded integers and byte memory. It has no pipeline,
forwarding, ACPy helpers or compiler IR dependencies.
"""
import re
import subprocess
from .program import LLVM

MASK = (1 << 32) - 1


def signed(n):
    return n - (1 << 32) if n & (1 << 31) else n


def disassemble(elf):
    result = subprocess.run([str(LLVM / 'llvm-objdump'), '-d', '--mattr=+m,+c', '-M', 'no-aliases,numeric', str(elf)], check=True, capture_output=True, text=True)
    instructions = {}
    for line in result.stdout.splitlines():
        match = re.match(r'\s*([0-9a-f]+):\s+([0-9a-f]{4}(?:[0-9a-f]{4})?)\s+([^\s]+)\s*(.*)', line)
        if match:
            pc, raw, name, operands = match.groups()
            instructions[int(pc, 16)] = (int(raw, 16), len(raw) // 2, name, [x.strip().split(' <')[0] for x in operands.split('#')[0].split(',') if x.strip()])
    return instructions


def execute(image, instructions):
    regs = image.registers[:]
    memory = bytearray(image.text.ljust(4096, b'\0') + image.ram.ljust(4096, b'\0'))
    pc = image.entry
    retired, events, coverage = [], [], {}
    output = ''
    status, reason = None, None
    def index(token):
        return int(token.removeprefix('x'))
    def integer(token):
        return int(token, 0)
    def r(token):
        return regs[index(token)]
    def address(token):
        match = re.fullmatch(r'(-?(?:0x[0-9a-f]+|[0-9]+))\((x\d+)\)', token)
        if not match:
            raise ValueError('invalid LLVM memory operand: ' + token)
        return (r(match[2]) + integer(match[1])) & MASK
    def checked(addr, size, store=False):
        if not (0 <= addr < 4096 and addr + size <= 4096 and not store or 4096 <= addr and addr + size <= 8192):
            raise ValueError(f'oracle memory bounds: {addr:#x}+{size}')
    for _ in range(image.max_cycles):
        if pc not in instructions:
            raise ValueError(f'no instruction at {pc:#x}')
        raw, length, mnemonic, args = instructions[pc]
        coverage[mnemonic] = coverage.get(mnemonic, 0) + 1
        name = mnemonic
        next_pc = pc + length
        write = None
        def put(dest, value):
            nonlocal write
            idx = index(dest)
            if idx:
                regs[idx] = value & MASK
                write = [idx, regs[idx]]
        # LLVM's C names are interpreted directly; no binary decompressor is shared.
        if name.startswith('c.'):
            name = name[2:]
            if name == 'nop':
                name, args = 'addi', ['x0', 'x0', '0']
            elif name == 'li':
                name, args = 'addi', [args[0], 'x0', args[1]]
            elif name == 'mv':
                name, args = 'add', [args[0], 'x0', args[1]]
            elif name in ('addi', 'andi', 'slli', 'srli', 'srai'):
                args = [args[0], args[0], args[1]]
            elif name in ('add', 'sub', 'xor', 'or', 'and'):
                args = [args[0], args[0], args[1]]
            elif name == 'addi4spn':
                name, args = 'addi', [args[0], 'x2', args[-1]]
            elif name == 'addi16sp':
                name, args = 'addi', ['x2', 'x2', args[-1]]
            elif name in ('lwsp', 'swsp'):
                name = name[:2]
            elif name in ('beqz', 'bnez'):
                name, args = ('beq' if name == 'beqz' else 'bne'), [args[0], 'x0', args[1]]
            elif name in ('j', 'jal'):
                name, args = 'jal', ['x1' if name == 'jal' else 'x0', args[0]]
            elif name in ('jr', 'jalr'):
                name, args = 'jalr', ['x1' if name == 'jalr' else 'x0', '0(' + args[0] + ')']
        if name in ('lui', 'auipc'):
            put(args[0], (pc if name == 'auipc' else 0) + (integer(args[1]) << 12))
        elif name == 'jal':
            target = integer(args[1]); put(args[0], next_pc); next_pc = target
        elif name == 'jalr':
            if len(args) == 3:
                target = (r(args[1]) + integer(args[2])) & ~1
            else:
                target = address(args[1]) & ~1
            put(args[0], next_pc); next_pc = target
        elif name in ('beq', 'bne', 'blt', 'bge', 'bltu', 'bgeu'):
            a, b = r(args[0]), r(args[1])
            take = dict(beq=a == b, bne=a != b, blt=signed(a) < signed(b), bge=signed(a) >= signed(b), bltu=a < b, bgeu=a >= b)[name]
            if take:
                next_pc = integer(args[2])
        elif name in ('lb', 'lbu', 'lh', 'lhu', 'lw'):
            size = {'lb':1, 'lbu':1, 'lh':2, 'lhu':2, 'lw':4}[name]
            addr = address(args[1]); checked(addr, size)
            put(args[0], int.from_bytes(memory[addr:addr+size], 'little', signed=name in ('lb','lh','lw')))
        elif name in ('sb', 'sh', 'sw'):
            size = {'sb':1, 'sh':2, 'sw':4}[name]
            addr = address(args[1]); checked(addr, size, True)
            memory[addr:addr+size] = (r(args[0]) & ((1 << (size*8))-1)).to_bytes(size, 'little')
        elif name in ('ecall', 'ebreak', 'unimp', '<unknown>'):
            if name == 'ecall':
                number, arg = regs[17], regs[10]
                events.append([True, pc, 1, number, arg])
                if number == 1:
                    output += str(signed(arg))
                elif number == 11:
                    output += chr(arg & 255)
                elif number == 4:
                    addr = arg
                    while True:
                        checked(addr, 1)
                        if memory[addr] == 0:
                            break
                        output += chr(memory[addr]); addr += 1
                elif number in (10, 93):
                    status, reason = arg if number == 93 else 0, 'exit'
                else:
                    raise ValueError(f'unsupported oracle ecall {number}')
            else:
                reason = 'break' if name == 'ebreak' else 'illegal'
        elif name == 'fence':
            pass
        else:
            immediate = name in ('addi', 'slli', 'slti', 'sltiu', 'xori', 'srli', 'srai', 'ori', 'andi')
            operation = 'sltu' if name == 'sltiu' else name[:-1] if immediate else name
            a = r(args[1]); b = integer(args[2]) & MASK if immediate else r(args[2])
            sa, sb = signed(a), signed(b)
            if operation == 'add': value = a + b
            elif operation == 'sub': value = a - b
            elif operation == 'sll': value = a << (b & 31)
            elif operation == 'srl': value = a >> (b & 31)
            elif operation == 'sra': value = sa >> (b & 31)
            elif operation == 'slt': value = int(sa < sb)
            elif operation == 'sltu' or name == 'sltiu': value = int(a < b)
            elif operation == 'xor': value = a ^ b
            elif operation == 'or': value = a | b
            elif operation == 'and': value = a & b
            elif operation == 'mul': value = a * b
            elif operation == 'mulh': value = sa * sb >> 32
            elif operation == 'mulhsu': value = sa * b >> 32
            elif operation == 'mulhu': value = a * b >> 32
            elif operation in ('div', 'rem'):
                quotient = (-1 if (sa < 0) != (sb < 0) else 1) * (abs(sa) // abs(sb)) if b else -1
                value = quotient if operation == 'div' else (sa - quotient * sb if b else sa)
            elif operation == 'divu': value = a // b if b else MASK
            elif operation == 'remu': value = a % b if b else a
            else: raise ValueError(f'oracle unsupported {mnemonic} {args}')
            put(args[0], value)
        retired.append(dict(pc=pc, word=raw, write=write))
        if pc == image.end_pc or reason is not None:
            return dict(registers=regs, ram=list(memory[4096:]), retired=retired, events=events, output=output, status=status, reason=reason, coverage=coverage)
        pc = next_pc & MASK
    raise ValueError('oracle did not terminate within bound')
