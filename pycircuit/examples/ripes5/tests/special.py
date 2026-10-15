"""Compatibility CLI for documented reference differences in the shared manifest."""
import argparse
from pathlib import Path
from .cases import load_cases, source_for
from .__main__ import main as standard_main

# Retain the historical report generator's list interface.
CASES = [(case['name'], case['isa'], source_for(case), case['expected_reason'],
          case['reference']['classification'], case['reference']['evidence'])
         for case in load_cases() if case['group'] == 'reference']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', action='append', choices=[case[0] for case in CASES])
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    command = ['run', '--check', 'ripes', '--sanitize']
    if args.case:
        for name in args.case:
            command += ['--case', name]
    else:
        command += ['--group', 'reference']
    if args.output:
        command += ['--output', str(args.output)]
    return standard_main(command)


if __name__ == '__main__':
    raise SystemExit(main())
