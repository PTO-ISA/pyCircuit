"""Check native skyzh programs against the independent ISA oracle, not GFSim."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

from .assemble import assemble
from .oracle import interpret, load_image
from .reference.build import COMMIT

HERE = Path(__file__).resolve().parent
UPSTREAM_PROGRAMS = ('control-hazard-1', 'control-hazard-2', 'control-hazard-3',
                     'data-hazard-1', 'data-hazard-2', 'out-of-order-3', 'rename-register-1')


def verify(runner, source, output):
    runner = runner.resolve()
    manifest = json.loads((runner.parent / 'build.json').read_text())
    if manifest['commit'] != COMMIT or manifest['binary_sha256'] != hashlib.sha256(runner.read_bytes()).hexdigest():
        raise ValueError('reference binary does not match its pinned build manifest')
    output.mkdir(parents=True, exist_ok=True)
    images = [assemble(p, output / (p.stem + '.hex')) for p in sorted((HERE / 'programs').glob('*.s'))]
    if source:
        images += [source / 'tests' / (name + '.hex') for name in UPSTREAM_PROGRAMS]
    rows = []
    for image in images:
        row = dict(program=str(image), name=image.stem)
        try:
            expected = interpret(load_image(image), limit=100000)
            result = subprocess.run([str(runner), str(image), '100000'], capture_output=True, text=True, timeout=30)
            if result.returncode not in (0, 2):
                raise RuntimeError(f'native exit {result.returncode}: {result.stderr}')
            actual = json.loads(result.stdout)
            mismatches = [key for key in ('registers', 'memory_changes') if actual[key] != expected[key]]
            row.update(status='pass' if actual['stopped'] and not mismatches else 'reference_mismatch',
                       mismatches=mismatches, native=actual, expected=expected)
        except (ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
            row.update(status='error', error=str(error))
        rows.append(row)
        print(image.stem, row['status'])
    report = dict(cpu_status='blocked_on_expression', reference_build=manifest, reference_cases=rows,
                  excluded_upstream_inputs={'out-of-order-1': 'intentional infinite Fibonacci loop',
                                            'out-of-order-2': 'no termination Store; falls past program text'},
                  passed=sum(r['status'] == 'pass' for r in rows), total=len(rows))
    (output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runner', type=Path, required=True)
    parser.add_argument('--source', type=Path)
    parser.add_argument('--output', type=Path, default=HERE / 'output/reference')
    args = parser.parse_args()
    report = verify(args.runner, args.source, args.output)
    raise SystemExit(0 if report['passed'] == report['total'] else 1)
