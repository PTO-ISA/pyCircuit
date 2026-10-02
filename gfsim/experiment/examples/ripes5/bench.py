"""Preliminary whole-model timing, with snapshots/JSON/review outside run timers."""
import argparse
import json
from pathlib import Path
import platform
import statistics
import subprocess
import sys
import time
from .model import CPU
from .programs import suite, validate
from .run import (HERE, DEFAULT_RUNNER, native_env, verify_runner, run_reference,
                  run_python, compare)


def worker(path, cache, reverse):
    case = json.loads(path.read_text())
    validate(case)
    start = time.perf_counter_ns()
    cpu = CPU(case, cache=cache, reverse=reverse)
    construct_ns = time.perf_counter_ns() - start
    start = time.perf_counter_ns()
    for _ in range(case['max_cycles']):
        wb = cpu.links[3].peek()
        done = wb.valid and wb.pc == case['end_pc']
        cpu.sim.step()
        if done:
            break
    else:
        raise TimeoutError('benchmark marker missing')
    run_ns = time.perf_counter_ns() - start
    print(json.dumps(dict(cycles=cpu.sim.tick, run_ns=run_ns, construct_ns=construct_ns,
                          rule_calls=cpu.sim.rule_calls[1:], cache_hits=cpu.sim.stats.cache_hits)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runner', type=Path, default=DEFAULT_RUNNER)
    parser.add_argument('--output', type=Path, default=HERE / 'review-output' / 'baseline.json')
    parser.add_argument('--repeats', type=int, default=5)
    parser.add_argument('--worker', type=Path, help=argparse.SUPPRESS)
    parser.add_argument('--no-cache', action='store_true', help=argparse.SUPPRESS)
    parser.add_argument('--reverse', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.worker:
        worker(args.worker, not args.no_cache, args.reverse)
        return
    if args.repeats < 1:
        parser.error('repeats must be positive')
    identity = verify_runner(args.runner)
    case = next(c for c in suite() if c['name'] == 'array_sum')
    directory = args.output.parent / 'benchmark-input'
    expected = run_reference(case, args.runner, directory)
    for cache in (True, False):
        for reverse in (False, True):
            compare(case, expected, run_python(case, cache, reverse), directory / 'mismatch.json')
    input_path = directory / 'input.json'
    commands = [('C++ Ripes', [str(args.runner), str(input_path), '--benchmark'])]
    for cache in (True, False):
        for reverse in (False, True):
            commands.append((f'Python GFSim cache={cache} reverse={reverse}',
                             [sys.executable, '-m', 'examples.ripes5.bench', '--worker', str(input_path)]
                             + ([] if cache else ['--no-cache']) + (['--reverse'] if reverse else [])))
    measurements = []
    for name, command in commands:
        samples = []
        for _ in range(args.repeats):
            start = time.perf_counter_ns()
            result = subprocess.run(command, env=native_env(), text=True, capture_output=True, check=True)
            process_ns = time.perf_counter_ns() - start
            samples.append(dict(json.loads(result.stdout), process_ns=process_ns))
        measurements.append(dict(model=name, samples=samples,
                                 median_run_ns=statistics.median(s['run_ns'] for s in samples),
                                 median_process_ns=statistics.median(s['process_ns'] for s in samples)))
    report = dict(reference=identity, python=sys.version, host=platform.platform(),
                  program=case['name'], measurements=measurements,
                  scope='run_ns: clock loop and end-marker check only; process_ns: cold process, imports, input, construction, clock loop and result output. No per-cycle snapshots, JSONL or ReviewTrace. GFSim built-in scheduler counters and Ripes signal machinery remain enabled. C++ Ripes versus Python GFSim; this is not a scheduler performance comparison.')
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(args.output)


if __name__ == '__main__':
    main()
