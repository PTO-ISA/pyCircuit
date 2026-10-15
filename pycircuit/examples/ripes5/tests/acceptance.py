"""Compatibility CLI; the manifest-driven runner is the current entry."""
import argparse
from pathlib import Path
import sys
from .__main__ import main as standard_main


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--jobs', type=int, help='legacy option; current runner executes serially')
    args = parser.parse_args()
    if args.jobs is not None:
        print('--jobs is a compatibility option; the standard runner executes serially.', file=sys.stderr)
    command = ['run', '--all', '--check', 'ripes']
    if args.output:
        command += ['--output', str(args.output)]
    return standard_main(command)


if __name__ == '__main__':
    raise SystemExit(main())
