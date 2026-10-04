"""Assemble a program, run the generated model, and check every commit."""
import argparse
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[2]))
from gfsim.experiment.examples.ripes5.isa import assemble
from pycircuit.examples.ooo.verify import architecture, context, microarchitecture, run


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('program', type=Path)
    p.add_argument('--runner', required=True, type=Path)
    p.add_argument('--data', type=Path, help='JSON array of initial 32-bit data words')
    p.add_argument('--registers', type=Path, help='JSON array of 32 initial registers; x0 must be zero')
    p.add_argument('--data-base', type=int, default=4096)
    p.add_argument('--max-cycles', type=int, default=12000)
    p.add_argument('--reverse', action='store_true')
    p.add_argument('--wb-period', type=int, default=0)
    p.add_argument('--wb-closed', type=int, default=0)
    p.add_argument('--output', type=Path, default=HERE / 'output/custom')
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    regs = json.loads(args.registers.read_text()) if args.registers else [0] * 31 + [args.data_base]
    data = json.loads(args.data.read_text()) if args.data else [7, 23] + list(range(2, 64))
    if len(regs) != 32 or not data or any(type(v) is not int or not 0 <= v <= 0xffffffff for v in regs + data):
        p.error('registers and data must be unsigned 32-bit integer arrays (32 registers)')
    case = dict(words=assemble(args.program.read_text()), registers=regs, data=data, base=args.data_base,
                max_cycles=args.max_cycles, wb_period=args.wb_period, wb_closed=args.wb_closed)
    try:
        rows = run(args.runner, case, args.reverse)
    except AssertionError as error:
        rows = error.rows
        (args.output / 'trace.jsonl').write_text(''.join(json.dumps(row) + '\n' for row in rows))
        context(args.output / 'run-mismatch.json', rows, max(0, len(rows) - 1), str(error))
        p.exit(1, str(error) + '\n')
    (args.output / 'trace.jsonl').write_text(''.join(json.dumps(row) + '\n' for row in rows))
    retired = architecture(case, rows, args.output / 'architecture-mismatch.json')
    coverage = microarchitecture(rows, args.output / 'microarchitecture-mismatch.json')
    summary = dict(cycles=rows[-1]['cycle'], retired=retired, ipc=retired / rows[-1]['cycle'],
                   fault=rows[-1]['retire']['fault'], registers=rows[-1]['registers'],
                   data=rows[-1]['data'], coverage=coverage)
    (args.output / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(json.dumps(summary, indent=2))


if __name__ == '__main__':
    main()
