import argparse
from pathlib import Path
from .frontend import compile_source
from .backend import analyze
from .ir import CompileError, load


def main():
    parser = argparse.ArgumentParser(description='ACPy → source MLIR → transactions → RTL → cycle traces')
    commands = parser.add_subparsers(dest='command', required=True)
    for name in ('compile', 'analyze', 'run'):
        sub = commands.add_parser(name)
        sub.add_argument('source', type=Path)
        sub.add_argument('--output', type=Path, required=True)
        sub.add_argument('--emit-cpp', action='store_true')
        sub.add_argument('--no-opt', action='store_true')
        sub.add_argument('--stop-after', choices=('source', 'transaction', 'rtl'), default='rtl')
        sub.add_argument('--cycles', type=int, default=0)
        sub.add_argument('--config', action='append', default=[], metavar='NAME=VALUE')
        if name == 'compile':
            sub.add_argument('--top', required=True)
    args = parser.parse_args()
    try:
        if args.cycles < 0:
            raise CompileError('cycles must be nonnegative')
        config = {}
        for entry in args.config:
            key, value = entry.split('=', 1)
            config[key] = int(value, 0)
        model = compile_source(args.source, args.top) if args.command == 'compile' else load(args.source)
        analyze(model, args.output, optimize=not args.no_opt, stop_after=args.stop_after,
                cycles=args.cycles, config=config, emit_cpp=args.emit_cpp)
    except (CompileError, SyntaxError, OSError, ValueError) as error:
        parser.exit(1, f'{error}\n')


if __name__ == '__main__':
    main()
