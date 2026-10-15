"""Small manifest-driven entry: list, build or run complete CPU programs."""
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import os
from pathlib import Path
import sys

from pycircuit.tests.support import cpp_compiler
from .cases import load_cases, MANIFEST
from .paths import ROOT, EXAMPLE, NATIVE_BUILD
from .program import LLVM
from .runner import run_case, write_json


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('list', 'build', 'run'))
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument('--case', action='append', help='case name; repeat to select several')
    selection.add_argument('--group', help='upstream, algorithms, boundary, legacy or reference')
    selection.add_argument('--all', action='store_true', help='all validation cases; audit probes remain explicit')
    selection.add_argument('--elf', type=Path, help='run an existing bare-metal ELF without reassembling it')
    parser.add_argument('--isa', choices=('I', 'IM', 'IMC'), help='required with --elf')
    parser.add_argument('--expect-status', type=lambda value: int(value, 0))
    parser.add_argument('--expect-output')
    parser.add_argument('--max-cycles', type=int, help='cycle bound for --elf (default: 5000)')
    parser.add_argument('--check', choices=('program', 'ripes', 'compiler', 'all'), default='ripes')
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args(argv)
    if args.elf and not args.isa:
        parser.error('--elf requires --isa')
    if not args.elf and (args.isa or args.expect_status is not None or args.expect_output is not None or args.max_cycles is not None):
        parser.error('--isa/--expect-status/--expect-output/--max-cycles apply to --elf inputs')
    if args.sanitize and args.action != 'run':
        parser.error('--sanitize requires run')
    cases = load_cases()
    if args.elf:
        cases = [dict(name='elf_program', elf=str(args.elf.resolve()), language='elf', isa=args.isa,
                      group='external', expected_reason='exit', expected_status=args.expect_status,
                      expected_output=args.expect_output, max_cycles=args.max_cycles if args.max_cycles is not None else 5000)]
    elif args.case:
        missing = set(args.case) - {case['name'] for case in cases}
        if missing:
            parser.error('unknown cases: ' + ', '.join(sorted(missing)))
        cases = [case for case in cases if case['name'] in args.case]
    elif args.group:
        cases = [case for case in cases if case['group'] == args.group]
        if not cases:
            parser.error('unknown or empty group: ' + args.group)
    elif args.all:
        cases = [case for case in cases if case['group'] != 'audit']
    elif args.action != 'list':
        cases = [case for case in cases if case.get('smoke')]
    if args.action == 'list':
        for case in cases:
            print(f'{case["name"]:32} {case["isa"]:3} {case["group"]:10} {case.get("source", case.get("upstream", case.get("elf")))}')
        return 0
    output = (args.output or ROOT / 'pycircuit/builds/benchmarks/ripes5-standard' /
              datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')).resolve()
    output.mkdir(parents=True, exist_ok=True)
    sources = [MANIFEST, EXAMPLE / 'model/model.py', EXAMPLE / 'model/isa.py']
    sources += [Path(__file__), *(Path(__file__).parent / name for name in
                 ('cases.py', 'runner.py', 'program.py', 'oracle.py', 'e2e.py', 'upstream.py'))]
    provenance = dict(schema_version=1, python=sys.version,
                      llvm_directory=str(LLVM), cxx=cpp_compiler(),
                      mlir_compiler=os.environ.get('ACPY_MLIR_COMPILER'),
                      source_sha256={str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in sources})
    if (NATIVE_BUILD / 'provenance.json').exists():
        import json
        provenance['native_build'] = json.loads((NATIVE_BUILD / 'provenance.json').read_text())
    write_json(output / 'provenance.json', provenance)
    write_json(output / 'selected-cases.json', dict(schema_version=1, cases=cases))
    print('Results: ' + str(output), flush=True)
    results = []
    for case in cases:
        result = run_case(case, output / case['name'], check=args.check,
                          sanitize=args.sanitize, build_only=args.action == 'build')
        results.append(result)
        write_json(output / 'summary.json', dict(schema_version=1, results=results,
                                               counts=dict(Counter(row['status'] for row in results))))
        print(f'{case["name"]}: {result["status"]}', flush=True)
        if result['status'] == 'failed':
            print('  ' + str(result['failure']['detail'])[:500], flush=True)
    return int(any(row['status'] == 'failed' for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
