"""Complete bounded programs; marker is ADDI x0,x0,2047, followed by a safe loop."""
import random
from ..riscv.isa import assemble, decode
from ..riscv.records import INVALID, HALT

MARKER = 0x7ff00013
DATA_BASE = 0x1000


def make_case(name, source, *, registers=None, data=None, max_cycles=2000):
    words = list(assemble(source))
    if MARKER in words:
        raise ValueError('marker is reserved')
    end_pc = len(words) * 4
    words += [MARKER, 0x0000006f, 0x00000013, 0x00000013]
    regs = [0] * 32
    for index, value in (registers or {}).items():
        regs[index] = value & 0xffffffff
    case = dict(name=name, words=words, registers=regs, data_base=DATA_BASE,
                data=list(data) if data is not None else [0] * 64,
                end_pc=end_pc, max_cycles=max_cycles, source=source)
    validate(case)
    return case


def validate(case):
    for name in ('words', 'registers', 'data'):
        if not isinstance(case[name], list) or any(type(v) is not int or not 0 <= v <= 0xffffffff for v in case[name]):
            raise ValueError(f'{name} must be a list of uint32 values')
    words = case['words']
    end = case['end_pc']
    if len(case['registers']) != 32 or case['registers'][0] != 0:
        raise ValueError('32 registers required; x0 must be zero')
    if (type(case['data_base']) is not int or case['data_base'] % 4
            or case['data_base'] < len(words) * 4 or not case['data']
            or case['data_base'] + len(case['data']) * 4 > 2**32):
        raise ValueError('aligned, disjoint nonempty data region required')
    if end % 4 or not 0 <= end < len(words) * 4 or words[end // 4] != MARKER:
        raise ValueError('end_pc must identify the reserved marker')
    if words[end // 4 + 1:] != [0x6f, 0x13, 0x13] or words.count(MARKER) != 1:
        raise ValueError('marker must be unique and followed by the safe loop suffix')
    if any(decode(w).op in (INVALID, HALT) for w in words):
        raise ValueError('unsupported instruction (HALT/ECALL excluded)')
    if type(case['max_cycles']) is not int or case['max_cycles'] < 1:
        raise ValueError('positive max_cycles required')


def suite():
    cases = [
        make_case('alu', '''
            addi x1,x0,7
            addi x2,x0,-3
            lui x3,0x80000
            add x4,x1,x2
            sub x5,x1,x2
            and x6,x1,x2
            or x7,x1,x2
            xor x8,x1,x2
            slt x9,x2,x1
            add x10,x3,x3
        '''),
        make_case('forward_priority', '''
            addi x1,x0,1
            addi x1,x1,2
            addi x1,x1,4
            add x2,x1,x1
            addi x0,x2,99
            add x3,x0,x2
            add x4,x1,x3
        '''),
        make_case('wb_id_store', '''
            lui x10,1
            addi x1,x0,41
            nop
            nop
            addi x2,x1,1
            sw x2,0(x10)
            lw x3,0(x10)
            nop
            add x4,x3,x1
            sw x4,4(x10)
        '''),
        make_case('array_sum', '''
            lui x10,1
            addi x1,x0,1
            addi x2,x0,9
        fill:
            sw x1,0(x10)
            addi x10,x10,4
            addi x1,x1,1
            bne x1,x2,fill
            lui x10,1
            addi x1,x0,8
            addi x3,x0,0
        sum:
            lw x4,0(x10)
            add x3,x3,x4
            addi x10,x10,4
            addi x1,x1,-1
            bne x1,x0,sum
            sw x3,0(x10)
        '''),
        make_case('jumps_branches', '''
            addi x1,x0,3
        loop:
            addi x1,x1,-1
            bne x1,x0,loop
            beq x1,x0,call
            addi x20,x0,99
        call:
            jal x5,subroutine
            addi x6,x5,1
            jal x0,done
        subroutine:
            addi x7,x5,2
            jalr x8,0(x5)
            addi x21,x0,99
        done:
            add x9,x7,x8
        '''),
        make_case('load_branch_wrong_path', '''
            lui x10,1
            addi x1,x0,17
            sw x1,0(x10)
            lw x2,0(x10)
            beq x2,x1,taken
            sw x0,0(x10)
            addi x20,x0,99
        taken:
            addi x3,x2,1
            bne x3,x2,next
            sw x0,0(x10)
            addi x21,x0,99
        next:
            lw x4,0(x10)
            bne x4,x1,bad
            addi x5,x4,1
            jal x0,done
        bad:
            sw x0,0(x10)
        done:
            sw x5,4(x10)
        '''),
        make_case('jalr_forwarded', '''
            lui x10,1
            addi x1,x0,24
            jalr x5,0(x1)
            sw x0,0(x10)
            addi x20,x0,99
            nop
            addi x6,x5,1
            lw x2,0(x10)
            jalr x7,0(x2)
            sw x0,0(x10)
            add x8,x5,x7
        ''', data=[40] + [0] * 63),
        make_case('no_false_load_use', '''
            lw x5,0(x10)
            addi x6,x0,5
            lw x5,0(x10)
            lui x7,0x528
            lw x5,0(x10)
            addi x0,x0,5
        ''', registers={10: DATA_BASE}, data=[9] + [0] * 63),
        make_case('initial_and_x0_load', '''
            add x3,x1,x2
            lw x0,0(x10)
            addi x4,x0,4
            lw x5,0(x10)
            sw x5,4(x10)
            lw x6,4(x10)
            add x7,x6,x3
        ''', registers={1: 0xffffffff, 2: 2, 10: DATA_BASE}, data=[0x80000000] + [0] * 63),
    ]
    for seed in (7, 42, 2026):
        rng = random.Random(seed)
        lines = ['lui x10,1']
        for i in range(80):
            a, b, d = (rng.randrange(0, 10) for _ in range(3))
            choice = rng.randrange(7)
            if choice == 0:
                lines.append(f'addi x{d},x{a},{rng.randrange(-100,101)}')
            elif choice == 1:
                lines.append(f'lw x{d},{4 * rng.randrange(16)}(x10)')
            elif choice == 2:
                lines.append(f'sw x{a},{4 * rng.randrange(16)}(x10)')
            elif choice == 3:
                lines += [f'beq x{a},x{b},skip_{i}', f'addi x{d},x{d},1', f'skip_{i}: nop']
            elif choice == 4:
                lines += [f'jal x11,jump_{i}', 'sw x0,0(x10)', f'jump_{i}: add x12,x11,x0']
            else:
                lines.append(f'{rng.choice(("add", "sub", "and", "or", "xor", "slt"))} x{d},x{a},x{b}')
        cases.append(make_case(f'mixed_{seed}', '\n'.join(lines),
                               data=[rng.getrandbits(32) for _ in range(64)]))
    return cases
