import argparse
from pathlib import Path
from .frontend import compile_source
from .backend import analyze
from .ir import CompileError, load


def main():
    p = argparse.ArgumentParser(description='ACPy → MLIR → conditional static rule graph')
    subs = p.add_subparsers(dest='command', required=True)
    for name in ('compile', 'analyze'):
        sub = subs.add_parser(name)
        sub.add_argument('source', type=Path)
        sub.add_argument('--no-opt', action='store_true')
        sub.add_argument('--output', type=Path, required=True)
        if name == 'compile': sub.add_argument('--top', required=True)
    args = p.parse_args()
    try:
        model = compile_source(args.source, args.top) if args.command == 'compile' else load(args.source)
        graph = analyze(model, args.output, optimize=not args.no_opt)
        if graph['diagnostics']:
            messages = []
            for diagnostic in graph['diagnostics']:
                details = []
                for index in diagnostic['edges']:
                    edge = graph['edges'][index]
                    details.append(f"{edge['resource']} {edge['operations']} at {edge['locations']} if {edge['condition']} sources={edge.get('provenance', [])}")
                messages.append(diagnostic['message'] + ': ' + ' → '.join(diagnostic['rules']) + '\n  ' + '\n  '.join(details))
            p.exit(1, '\n'.join(messages) + '\n')
    except (CompileError, SyntaxError, OSError) as error:
        p.exit(1, f'{error}\n')


if __name__ == '__main__': main()
