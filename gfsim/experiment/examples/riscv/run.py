"""Run with: python -m examples.riscv.run program.s --trace"""

import argparse
from pathlib import Path
from .model import build_cpu
from .reference import Interpreter


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('program', type=Path)
    parser.add_argument('--latency', type=int, default=1)
    parser.add_argument('--max-cycles', type=int, default=10000)
    parser.add_argument('--trace', action='store_true')
    parser.add_argument('--html', type=Path, nargs='?',
                        const=Path(__file__).parent / 'review-output' / 'riscv.html',
                        help='write an offline review; defaults to this example/review-output/')
    args = parser.parse_args()
    cpu = build_cpu(args.program.read_text(), memory_latency=args.latency)
    review = None
    if args.html:
        from review import ReviewTrace
        names = {q.qid: name for q, name in zip(cpu.links, ('IF_ID', 'ID_EX', 'EX_MEM', 'MEM_WB'))}
        names.update({q.qid: f'x{i}' for i, q in enumerate(cpu.registers)})
        names.update({q.qid: f'data[{i}]' for i, q in enumerate(cpu.data)})
        fetch = cpu.stages[0]
        names.update({fetch.pc.qid: 'PC', fetch.control.qid: 'front_control',
                      fetch.redirect.qid: 'redirect', cpu.busy.qid: 'MEM_busy',
                      cpu.retirement.qid: 'retirement'})
        review = ReviewTrace(cpu.sim, title=f'RISC-V / {args.program.name} / latency {args.latency}',
                             queue_names=names)
    oracle = Interpreter(cpu.words)
    try:
        for _ in range(args.max_cycles):
            retired = cpu.step(args.trace)
            if retired is not None:
                expected = oracle.step()
                if retired != expected or cpu.register_values() != tuple(oracle.registers):
                    raise AssertionError(f'retirement mismatch: {retired}, expected {expected}')
            if cpu.halted:
                break
        else:
            raise TimeoutError('program did not HALT')
        if cpu.memory_values() != tuple(oracle.memory):
            raise AssertionError('final memory mismatch')
    finally:
        if review is not None:
            review.write_html(args.html)
            print(f'Review: {args.html}')
    if args.trace:
        print('tick  ID    EX    MEM   WB    busy  accepted RuleIds (IF=1 .. WB=5)')
        for tick, pcs, busy, accepted in cpu.trace:
            cells = ['-' if pc is None else str(pc) for pc in (*pcs, busy)]
            print(f'{tick:4}  ' + ' '.join(f'{cell:5}' for cell in cells) + f' {accepted}')
    print(f'{len(cpu.retired)} instructions retired in {cpu.sim.tick} cycles; reference matched')
    print(' '.join(f'x{i}={value}' for i, value in enumerate(cpu.register_values()) if value))


if __name__ == '__main__':
    main()
