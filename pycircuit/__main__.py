import argparse
from pathlib import Path
from .frontend import compile_source
from .backend import emit
from .ir import CompileError, load, save


def main():
    p = argparse.ArgumentParser(description='ACPy → ACIR → GFSim C++')
    subs = p.add_subparsers(dest='command', required=True)
    for name in ('compile', 'emit'):
        sub = subs.add_parser(name)
        sub.add_argument('source', type=Path)
        sub.add_argument('--no-opt', action='store_true')
        sub.add_argument('--output', type=Path, required=True)
        if name == 'compile':
            sub.add_argument('--top', required=True)
    args = p.parse_args()
    try:
        model = compile_source(args.source, args.top) if args.command == 'compile' else load(args.source)
        args.output.mkdir(parents=True, exist_ok=True)
        save(model, args.output / 'model.acir.mlir')
        emit(model, args.output, optimize=not args.no_opt)
    except (CompileError, SyntaxError) as error:
        p.exit(1, f'{error}\n')


if __name__ == '__main__':
    main()
