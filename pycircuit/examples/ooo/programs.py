"""Reproducible assembly fixtures; the assembler is shared, the oracle is not."""
import random
from pathlib import Path
from gfsim.experiment.examples.ripes5.isa import assemble
HERE = Path(__file__).resolve().parent


def mixed(seed):
    rng = random.Random(seed)
    lines = ['addi x20, x0, 4', 'loop:']
    for i in range(48):
        rd, a, b = (rng.randrange(1, 16) for _ in range(3))
        kind = rng.randrange(7)
        if kind < 2:
            lines.append(f'addi x{rd}, x{a}, {rng.randrange(-32, 33)}')
        elif kind < 4:
            lines.append(f'{rng.choice(["add", "sub", "and", "or", "xor", "slt"])} x{rd}, x{a}, x{b}')
        elif kind == 4:
            lines.append(f'lw x{rd}, {4 * rng.randrange(16)}(x31)')
        elif kind == 5:
            lines.append(f'sw x{a}, {4 * rng.randrange(16)}(x31)')
        else:
            lines += [f'beq x{a}, x{b}, skip_{i}', f'addi x{rd}, x{rd}, 1', f'skip_{i}:']
    lines += ['addi x20, x20, -1', 'bne x20, x0, loop', 'halt']
    return '\n'.join(lines) + '\n'


def generated_programs():
    congestion = ['lw x1, 0(x31)']
    congestion += [f'addi x{2 + i % 26}, x0, {i + 1}' for i in range(40)]
    congestion += [f'lw x{2 + i}, {4 * i}(x31)' for i in range(12)]
    congestion += ['add x18, x1, x2', 'halt']
    return {'congestion': '\n'.join(congestion) + '\n',
            **{f'mixed_{seed}': mixed(seed) for seed in (7, 2026, 65537)}}


def suite():
    generated = generated_programs()
    for name, source in generated.items():
        saved = (HERE / 'programs' / f'{name}.s').read_text()
        if saved != source:
            raise AssertionError(f'{name}: saved assembly differs from deterministic generator')
    cases = []
    for path in sorted((HERE / 'programs').glob('*.s')):
        regs, data = [0] * 32, [7, 23] + list(range(2, 64))
        regs[31] = 4096
        if path.stem == 'dual_issue':
            data[0] = 4096
        period, closed = (23, 17) if path.stem == 'congestion' else (0, 0)
        cases.append(dict(name=path.stem, words=assemble(path.read_text()), registers=regs,
                          data=data, base=4096, max_cycles=12000, wb_period=period, wb_closed=closed))
    # Re-run recovery and memory under blocked writeback to stress retained operands.
    for original in list(cases):
        if original['name'] in ('recovery', 'memory', 'rename', 'mixed_2026'):
            cases.append(dict(original, name=original['name'] + '_blocked', wb_period=19, wb_closed=13))
    return cases


if __name__ == '__main__':
    for name, source in generated_programs().items():
        (HERE / 'programs' / f'{name}.s').write_text(source)
