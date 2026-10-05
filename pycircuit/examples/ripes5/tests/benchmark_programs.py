"""Bounded throughput workloads, with independently calculated architectural results."""
from .verify import v  # Install the existing example/assembler import path.
from examples.ripes5.tests.programs import DATA_BASE, make_case


def suite(iterations=12000):
    if iterations < 1 or iterations % 2:
        raise ValueError('iterations must be positive and even')
    cases = []

    def add(name, body, expected, *, registers=None, data=None, final_data=None):
        regs = {30: iterations, **(registers or {})}
        case = make_case(name, 'loop:\n' + body + '\naddi x30,x30,-1\nbne x30,x0,loop\n',
                         registers=regs, data=data or [0] * 8, max_cycles=iterations * 32 + 32)
        final = case['registers'].copy()
        for index, value in {30: 0, **expected}.items():
            final[index] = value & 0xffffffff
        case['expected_registers'] = final
        case['expected_data'] = case['data'].copy() if final_data is None else final_data
        case['iterations'] = iterations
        cases.append(case)

    add('independent_integer', '\n'.join(f'addi x{i},x{i},{i}' for i in range(1, 9)),
        {i: i * iterations for i in range(1, 9)})
    n = iterations
    add('forwarding_chain', '''
        addi x1,x1,1
        addi x2,x1,3
        add x3,x2,x1
        sub x4,x3,x2
        xor x5,x4,x1
        add x6,x3,x4
        addi x7,x6,7
        xor x8,x7,x6
    ''', {1: n, 2: n + 3, 3: 2*n + 3, 4: n, 5: 0,
          6: 3*n + 3, 7: 3*n + 10, 8: (3*n + 10) ^ (3*n + 3)})
    add('branch_flush', '''
        xor x1,x1,x2
        beq x1,x0,even
        addi x3,x3,3
        jal x0,join
        sw x0,0(x10)
    even:
        addi x3,x3,1
        beq x1,x0,join
        sw x0,0(x10)
    join:
        addi x4,x4,1
    ''', {1: 0, 3: 2*n, 4: n}, registers={2: 1, 10: DATA_BASE}, data=[99] * 8)
    add('load_use', '''
        lw x1,0(x10)
        add x2,x2,x1
        lw x3,4(x10)
        add x4,x4,x3
        lw x5,8(x10)
        add x6,x6,x5
        lw x7,12(x10)
        add x8,x8,x7
    ''', {1: 3, 2: 3*n, 3: 5, 4: 5*n, 5: 7, 6: 7*n, 7: 11, 8: 11*n},
        registers={10: DATA_BASE}, data=[3, 5, 7, 11, 0, 0, 0, 0])
    add('consecutive_memory', '''
        lw x1,0(x10)
        lw x2,4(x10)
        lw x3,8(x10)
        lw x4,12(x10)
        addi x1,x1,1
        addi x2,x2,2
        addi x3,x3,3
        addi x4,x4,4
        sw x1,0(x10)
        sw x2,4(x10)
        sw x3,8(x10)
        sw x4,12(x10)
    ''', {i: i*n for i in range(1, 5)}, registers={10: DATA_BASE},
        final_data=[n, 2*n, 3*n, 4*n, 0, 0, 0, 0])
    return cases


def check_result(case, row):
    for key in ('registers', 'data'):
        if row[key] != case['expected_' + key]:
            raise AssertionError(f'{case["name"]}: incorrect final {key}: {row[key]}')
