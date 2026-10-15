"""Compatibility CLI for the original nine programs in the shared manifest."""
import argparse
from pathlib import Path
from .__main__ import main as standard_main


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    command = ['run', '--group', 'legacy', '--check', 'ripes']
    if args.output:
        command += ['--output', str(args.output)]
    return standard_main(command)


if __name__ == '__main__':
    raise SystemExit(main())
