"""Run all four GFSim configurations against one native Ripes trace per input."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
from .model import CPU
from .programs import suite, validate

HERE = Path(__file__).resolve().parent
DEFAULT_RUNNER = HERE / 'reference' / 'build' / 'ripes5-reference'


def native_env():
    env = os.environ.copy()
    qt = env.get('RIPES_QT_PREFIX', '/tmp/gfsim-ripes-qt')
    env['LD_LIBRARY_PATH'] = qt + '/lib:' + env.get('LD_LIBRARY_PATH', '')
    env['QT_QPA_PLATFORM'] = 'offscreen'
    return env


def write_jsonl(path, rows):
    path.write_text(''.join(json.dumps(row, sort_keys=True) + '\n' for row in rows))


def run_python(case, cache=True, reverse=False, html=None):
    cpu = CPU(case, cache=cache, reverse=reverse)
    observer = None
    if html:
        from review import ReviewTrace
        observer = ReviewTrace(cpu.sim, title=f'Ripes5 / {case["name"]}', queue_names=cpu.names,
                               signal_names={0: 'ex_result', 1: 'load_use_stall'})
    rows = [cpu.snapshot()]
    try:
        for _ in range(case['max_cycles']):
            row = cpu.step()
            rows.append(row)
            if row['retire'] and row['retire']['pc'] == case['end_pc']:
                return rows
        raise TimeoutError('end marker did not retire')
    finally:
        if observer:
            observer.write_html(html)


def differences(expected, actual, path=''):
    if type(expected) is not type(actual):
        return [dict(field=path, expected=expected, actual=actual)]
    if isinstance(expected, dict):
        result = []
        for key in sorted(expected.keys() | actual.keys()):
            if key not in expected or key not in actual:
                result.append(dict(field=f'{path}.{key}', expected=expected.get(key), actual=actual.get(key)))
            else:
                result += differences(expected[key], actual[key], f'{path}.{key}')
        return result
    if isinstance(expected, list) and len(expected) == len(actual):
        return [diff for i, (a, b) in enumerate(zip(expected, actual))
                for diff in differences(a, b, f'{path}[{i}]')]
    return [] if expected == actual else [dict(field=path, expected=expected, actual=actual)]


def compare(case, expected, actual, report, radius=3):
    # Only remove explicitly non-normalized native diagnostics. Never shift rows.
    expected = [{k: v for k, v in row.items() if k != 'raw'} for row in expected]
    for i in range(max(len(expected), len(actual))):
        diffs = differences(expected[i] if i < len(expected) else None,
                            actual[i] if i < len(actual) else None)
        if diffs:
            lo, hi = max(0, i - radius), i + radius + 1
            report.write_text(json.dumps(dict(
                name=case['name'], first_row=i, differences=diffs,
                instructions=[dict(pc=j * 4, word=f'{word:08x}') for j, word in enumerate(case['words'])],
                source=case.get('source', ''),
                ripes=expected[lo:hi], gfsim=actual[lo:hi]), indent=2) + '\n')
            raise AssertionError(f'{case["name"]}: first difference at row {i}: {diffs[:3]}; {report}')


def run_reference(case, runner, directory):
    validate(case)
    directory.mkdir(parents=True, exist_ok=True)
    input_path = directory / 'input.json'
    input_path.write_text(json.dumps(case, indent=2) + '\n')
    result = subprocess.run([str(runner), str(input_path)], env=native_env(),
                            text=True, capture_output=True, timeout=120)
    (directory / 'ripes.raw.jsonl').write_text(result.stdout)
    (directory / 'ripes.stderr.txt').write_text(result.stderr)
    if result.returncode:
        raise RuntimeError(f'Ripes failed ({result.returncode}): {result.stderr}')
    rows = [json.loads(line) for line in result.stdout.splitlines()]
    if not rows or (rows[-1].get('retire') or {}).get('pc') != case['end_pc']:
        raise RuntimeError('Ripes did not reach marker retirement')
    return rows


def verify_runner(runner):
    if not runner.is_file():
        raise FileNotFoundError(f'Build the original Ripes reference first: {runner}; see {HERE / "README.md"}')
    result = subprocess.run([str(runner), '--identity'], env=native_env(),
                            text=True, capture_output=True, check=True)
    identity = json.loads(result.stdout)
    lock = json.loads((HERE / 'reference' / 'version.json').read_text())
    if identity['ripes'] != lock['ripes'] or identity['vsrtl'] != lock['submodules']['external/VSRTL']:
        raise RuntimeError(f'reference version mismatch: {identity}')
    return identity


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runner', type=Path, default=DEFAULT_RUNNER)
    parser.add_argument('--output', type=Path, default=HERE / 'review-output')
    parser.add_argument('--case', help='one built-in program name')
    parser.add_argument('--input', type=Path, help='custom input using the same JSON schema')
    parser.add_argument('--html', action='store_true')
    args = parser.parse_args()
    identity = verify_runner(args.runner)
    cases = [json.loads(args.input.read_text())] if args.input else suite()
    if args.case:
        cases = [c for c in cases if c['name'] == args.case]
    if not cases:
        parser.error('no matching cases')
    results = []
    for case in cases:
        directory = args.output / case['name']
        reference = run_reference(case, args.runner, directory)
        for cache in (True, False):
            for reverse in (False, True):
                tag = f'cache{int(cache)}-reverse{int(reverse)}'
                rows = run_python(case, cache, reverse,
                                  directory / f'{tag}.html' if args.html else None)
                write_jsonl(directory / f'{tag}.jsonl', rows)
                compare(case, reference, rows, directory / f'{tag}.mismatch.json')
        results.append(dict(name=case['name'], cycles=reference[-1]['cycle'],
                            retired=reference[-1]['retired'], configurations=4))
        print(f'{case["name"]}: {reference[-1]["cycle"]} cycles, four configurations matched')
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / 'summary.json').write_text(json.dumps(dict(
        reference=identity, binary_sha256=hashlib.sha256(args.runner.read_bytes()).hexdigest(),
        results=results), indent=2) + '\n')


if __name__ == '__main__':
    main()
