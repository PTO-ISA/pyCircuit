"""Build an observer around the pinned, unmodified skyzh core (no GTest needed)."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess

COMMIT = '8989a09c357a69b68612f653380d60816f5176c2'
SOURCES = ('Common/Session.cpp', 'Common/utils.cpp', 'Common/ReservationStation.cpp',
           'Common/ReorderBuffer.cpp', 'Pipeline/Issue.cpp', 'Pipeline/OoOExecute.cpp',
           'Module/ALUUnit.cpp', 'Module/LoadStoreUnit.cpp', 'Module/CommitUnit.cpp',
           'Module/BranchPrediction.cpp')


def build(source, output, cxx):
    source, output = source.resolve(), output.resolve()
    def git(*args):
        return subprocess.check_output(['git', '-C', str(source), *args], text=True).strip()
    if git('rev-parse', 'HEAD') != COMMIT or git('status', '--porcelain', '--untracked-files=no'):
        raise ValueError('reference must be a clean checkout of ' + COMMIT)
    output.mkdir(parents=True, exist_ok=True)
    runner = Path(__file__).with_name('runner.cpp')
    binary = output / 'skyzh-reference'
    command = [*shlex.split(cxx), '-std=gnu++20', '-O3', '-DNDEBUG', '-I', str(source / 'src'),
               *(str(source / 'src' / name) for name in SOURCES), str(runner), '-o', str(binary)]
    subprocess.run(command, check=True)
    manifest = dict(commit=COMMIT, source=str(source), command=command,
                    compiler=subprocess.check_output([*shlex.split(cxx), '--version'], text=True),
                    binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                    adapter_sha256=hashlib.sha256(runner.read_bytes()).hexdigest(),
                    rob_slots=8, rob_usable=7, memory_bytes=0x400000)
    (output / 'build.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(binary)
    return binary


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=Path('/tmp/skyzh-reference-build'))
    parser.add_argument('--cxx', default=os.environ.get('CXX', 'c++'))
    args = parser.parse_args()
    build(args.source, args.output, args.cxx)
