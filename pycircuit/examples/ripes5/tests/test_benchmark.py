"""Exercise fixed-window boundaries, legacy calls, errors and notification toggles."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile
from .verify import v
from ..tools.benchmark_support import fixed_sample, prepare
from examples.ripes5.tests.run import native_env


def verify(binaries, output):
    checks = dict(fixed_windows=0, invalid_arguments=0, legacy_calls=0, observation_traces=0)
    with tempfile.TemporaryDirectory() as temporary:
        directory = Path(temporary)
        for case in v.suite():
            path = prepare(case, directory)
            traces = {}
            for label, binary in binaries.items():
                native = label == 'native'
                command = [str(binary.resolve())] + ([str(path)] if native else [])
                payload = None if native else v.numeric_input(case)
                result = subprocess.run(command, input=payload, capture_output=True, text=True,
                                        env=native_env(), check=True, timeout=30)
                traces[label] = [json.loads(line) for line in result.stdout.splitlines()]
                legacy = subprocess.run(command + ['--benchmark'], input=payload, capture_output=True,
                                        text=True, env=native_env(), check=True, timeout=30)
                assert json.loads(legacy.stdout)['cycles'] == traces[label][-1]['cycle']
                checks['legacy_calls'] += 2
                if native:
                    observed = subprocess.run(command + ['--observe'], capture_output=True, text=True,
                                              env=native_env(), check=True, timeout=30)
                    assert [json.loads(line) for line in observed.stdout.splitlines()] == traces[label]
                    checks['observation_traces'] += 1
                if case['name'] not in ('forward_priority', 'array_sum', 'load_branch_wrong_path'):
                    continue
                rows = traces[label]
                total = rows[-1]['cycle']
                for end in (1, 4, total - 1, total):
                    for warmup in sorted({0, end // 2, end - 1}):
                        fixed_sample(binary, case, path, warmup, end - warmup, rows[warmup], rows[end], native=native)
                        checks['fixed_windows'] += 1
                limited = dict(case, max_cycles=total)
                limited_path = prepare(limited, directory / 'limit')
                fixed_sample(binary, limited, limited_path, total - 1, 1, rows[-2], rows[-1], native=native)
                checks['fixed_windows'] += 1
                invalid = [[], ['0'], ['0', '0'], ['-1', '1'], ['1', '-1'], ['x', '1'],
                           ['0', '1x'], [str(2**64), '1'], ['0', str(2**64)],
                           [str(2**64 - 1), '2'], [str(case['max_cycles']), '1'],
                           ['0', str(case['max_cycles'] + 1)], ['0', '1', 'extra']]
                for args in invalid:
                    rejected = subprocess.run(command + ['--benchmark-fixed'] + args, input=payload,
                                              capture_output=True, text=True, env=native_env(), timeout=10)
                    assert rejected.returncode and rejected.stderr and not rejected.stdout, args
                    checks['invalid_arguments'] += 1
            for label, rows in traces.items():
                if label != 'native':
                    v.compare(case, traces['native'], rows, directory / 'mismatch.json')
    report = dict(passed=True, **checks)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'Fixed runner checks: {checks}', flush=True)
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--generated-runner', type=Path, required=True)
    p.add_argument('--cpp-runner', type=Path, required=True)
    p.add_argument('--runner', type=Path, default=v.DEFAULT_RUNNER)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    verify(dict(generated=a.generated_runner, handwritten=a.cpp_runner, native=a.runner), a.output)


if __name__ == '__main__':
    main()
