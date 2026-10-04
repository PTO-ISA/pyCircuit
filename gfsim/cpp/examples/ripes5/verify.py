"""Mandatory three-way cycle comparison using the existing assembler and comparator."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
CPP = HERE.parents[1]
EXPERIMENT = CPP.parent / 'experiment'
sys.path.insert(0, str(EXPERIMENT))
from examples.ripes5.programs import suite, validate
from examples.ripes5.run import (DEFAULT_RUNNER, verify_runner, run_reference, run_python as _run_python,
                                 compare, write_jsonl)


def run_python(case, reverse=False):
    # Historical Python engine stays unchanged; only architectural traces are compared.
    return _run_python(case, True, reverse)


def numeric_input(case, reverse=False):
    validate(case)
    if len(case['words']) > 2**20 or len(case['data']) > 2**20:
        raise ValueError('C++ runner transport allows at most 2**20 words per region')
    values = [2, case['max_cycles'], case['end_pc'], case['data_base'], int(reverse),
              len(case['words']), len(case['data']), *case['words'], *case['registers'], *case['data']]
    return ' '.join(map(str, values)) + '\n'


def run_cpp(case, runner, reverse=False, benchmark=False):
    result = subprocess.run([str(runner.resolve())] + (['--benchmark'] if benchmark else []),
                            input=numeric_input(case, reverse), capture_output=True,
                            text=True, check=True, timeout=120)
    return [json.loads(line) for line in result.stdout.splitlines()]


def fingerprint():
    paths = sorted(p for p in CPP.rglob('*') if p.is_file() and p.suffix in ('.hpp', '.cpp', '.py')
                   and not any(part in ('output', '__pycache__', 'build', 'build-asan') for part in p.relative_to(CPP).parts))
    paths += [EXPERIMENT / 'engine.py', EXPERIMENT / 'construction.py']
    paths += sorted((EXPERIMENT / 'examples/ripes5').glob('*.py'))
    return {str(p.relative_to(CPP.parent)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp-runner', type=Path, required=True)
    parser.add_argument('--runner', type=Path, default=DEFAULT_RUNNER)
    parser.add_argument('--output', type=Path, default=HERE / 'output')
    parser.add_argument('--case')
    args = parser.parse_args()
    identity = verify_runner(args.runner)  # Missing/wrong native reference is a hard failure.
    cases = suite()
    if args.case:
        cases = [c for c in cases if c['name'] == args.case]
    if not cases:
        parser.error('unknown case')
    results = []
    for case in cases:
        directory = args.output / case['name']
        expected = run_reference(case, args.runner, directory)
        configs = []
        for reverse in (False, True):
            tag = f'reverse{int(reverse)}'
            (directory / f'{tag}.input.txt').write_text(numeric_input(case, reverse))
            python = run_python(case, reverse)
            cpp = run_cpp(case, args.cpp_runner, reverse)
            write_jsonl(directory / f'{tag}.python.jsonl', python)
            write_jsonl(directory / f'{tag}.cpp.jsonl', cpp)
            compare(case, expected, python, directory / f'{tag}.native-python-mismatch.json')
            compare(case, expected, cpp, directory / f'{tag}.native-cpp-mismatch.json')
            compare(case, python, cpp, directory / f'{tag}.python-cpp-mismatch.json')
            configs.append(dict(reverse=reverse, rows=len(cpp),
                cpp_sha256=hashlib.sha256((directory / f'{tag}.cpp.jsonl').read_bytes()).hexdigest()))
        results.append(dict(name=case['name'], cycles=expected[-1]['cycle'], configurations=configs))
        print(f'{case["name"]}: C++ / Python / native matched in two configurations', flush=True)
    report = dict(reference=identity, native_binary_sha256=hashlib.sha256(args.runner.read_bytes()).hexdigest(),
                  cpp_binary_sha256=hashlib.sha256(args.cpp_runner.read_bytes()).hexdigest(),
                  source_sha256=fingerprint(), configurations=sum(len(r['configurations']) for r in results),
                  full_acceptance=args.case is None, results=results)
    (args.output / 'comparison.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
