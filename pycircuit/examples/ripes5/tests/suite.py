"""Compatibility entry for the historical suite; programs live in cases.json."""
import argparse
import json
from pathlib import Path
from .cases import load_cases, source_for
from .e2e import DEFAULT_OUTPUT
from .upstream import inventory


def prepare(output):
    """Retain the original 53-case ledger for historical report generation."""
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    cases = []
    for case in load_cases():
        if not case.get('legacy_suite'):
            continue
        source = source_for(case, output / case['name'])
        cases.append((case['name'], source, case['isa'], case['expected_status'],
                      case.get('expected_output'), case['language']))
    (output / 'upstream-inventory.json').write_text(json.dumps(inventory(), indent=2) + '\n')
    (output / 'programs.json').write_text(json.dumps([dict(name=n,isa=i,expected_status=s,expected_output=o,language=l) for n,_,i,s,o,l in cases], indent=2) + '\n')
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument('--case', action='append')
    parser.add_argument('--prepare-only', action='store_true')
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--reload', action='store_true')
    args = parser.parse_args()
    from .__main__ import main as standard_main
    command = ['build' if args.prepare_only else 'run', '--output', str(args.output)]
    if args.case:
        for name in args.case:
            command += ['--case', name]
    else:
        for case in load_cases():
            if case.get('legacy_suite'):
                command += ['--case', case['name']]
    if not args.prepare_only:
        command += ['--check', 'all' if args.reload else 'ripes']
        if args.sanitize:
            command += ['--sanitize']
    return standard_main(command)

if __name__ == '__main__':
    raise SystemExit(main())
