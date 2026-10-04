"""Report expression blockers; exit 2 while any requested capability is absent."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
from tempfile import TemporaryDirectory

from pycircuit import compile_source, emit

HERE = Path(__file__).resolve().parent


def diagnose(cxx):
    results = []
    with TemporaryDirectory(prefix='skyzh-expression-') as directory:
        for name in ('output_feedback', 'output_capacities', 'cpp_keyword'):
            source = HERE / 'repro' / (name + '.py')
            row = dict(probe=name, source_sha256=hashlib.sha256(source.read_bytes()).hexdigest())
            try:
                model = compile_source(source, 'Probe')
                emit(model, Path(directory) / name)
                if name == 'output_capacities':
                    constants = {op['id']: op.get('value') for op in model['construction']}
                    capacities = [constants[r['capacity']] for r in model['resources'] if r['name'] in ('rob', 'reservation')]
                    if capacities != [12, 1]:
                        raise ValueError(f'expected separate capacities [12, 1], got {capacities}')
                generated = Path(directory) / name
                command = [*shlex.split(cxx), '-std=c++20', '-fsyntax-only', '-I', str(generated),
                           '-I', str(HERE.parents[2] / 'gfsim/cpp/include'), str(generated / 'model.cpp')]
                result = subprocess.run(command, text=True, capture_output=True)
                if result.returncode:
                    raise ValueError('\n'.join(result.stderr.replace(str(generated), '<generated>').splitlines()[:12]))
                row.update(status='emitted', note='Simulation regressions are in pycircuit/tests/test_compiler.py.')
            except Exception as error:
                row.update(status='blocked', error_type=type(error).__name__, error=str(error))
            results.append(row)
    return dict(cpu_status='see_verify_for_full_cpu_acceptance', probes=results)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=HERE / 'output/expression.json')
    parser.add_argument('--cxx', default=os.environ.get('ACPY_CXX', 'c++'))
    args = parser.parse_args()
    report = diagnose(args.cxx)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n')
    print(json.dumps(report, indent=2, ensure_ascii=False))
    return 2 if any(row['status'] == 'blocked' for row in report['probes']) else 0


if __name__ == '__main__':
    raise SystemExit(main())
