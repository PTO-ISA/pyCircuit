"""Serial, pinned-CPU timing of C++ GFSim, current Python GFSim and native Ripes."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import sys
import time

from verify import (HERE, EXPERIMENT, DEFAULT_RUNNER, suite, numeric_input, verify_runner,
                    run_cpp, run_python, run_reference, compare, fingerprint)
from examples.ripes5.run import native_env


def measure(command, env, cycles, stdin=None):
    start = time.perf_counter_ns()
    result = subprocess.run(command, env=env, input=stdin, text=True, capture_output=True, check=True, timeout=120)
    process_ns = time.perf_counter_ns() - start
    sample = json.loads(result.stdout)
    if sample['cycles'] != cycles:
        raise AssertionError(f'benchmark cycles changed: {sample["cycles"]} != {cycles}')
    return dict(sample, process_ns=process_ns, ns_per_cycle=sample['run_ns'] / cycles)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp-runner', type=Path, required=True)
    parser.add_argument('--runner', type=Path, default=DEFAULT_RUNNER)
    parser.add_argument('--output', type=Path, default=HERE / 'timing.json')
    parser.add_argument('--cpu', type=int)
    args = parser.parse_args()
    available = sorted(os.sched_getaffinity(0))
    cpu = available[0] if args.cpu is None else args.cpu
    if cpu not in available:
        parser.error('CPU must be in the current affinity set')
    os.sched_setaffinity(0, {cpu})
    identity = verify_runner(args.runner)
    env = native_env()
    python_env = dict(env, PYTHONPATH=str(EXPERIMENT))
    worker = EXPERIMENT / 'examples/ripes5/worker.py'
    cases = {case['name']: case for case in suite()}
    programs = []
    for name in ('array_sum', 'mixed_2026', 'memory_loop_256'):
        case = cases[name]
        directory = HERE / 'output' / 'benchmark-input' / name
        native = run_reference(case, args.runner, directory)
        cycles = native[-1]['cycle']
        jobs = [('native', [str(args.runner.resolve()), str((directory / 'input.json').resolve()), '--benchmark'], env, None)]
        # Equivalence before timing for every measured configuration.
        for cache in (True, False):
            for reverse in (False, True):
                tag = f'cache{int(cache)}-reverse{int(reverse)}'
                compare(case, native, run_python(case, cache, reverse), directory / f'{tag}.python-mismatch.json')
                compare(case, native, run_cpp(case, args.cpp_runner, cache, reverse), directory / f'{tag}.cpp-mismatch.json')
                jobs.append((f'cpp-{tag}', [str(args.cpp_runner.resolve()), '--benchmark'], env, numeric_input(case, cache, reverse)))
                command = [sys.executable, str(worker), str(directory / 'input.json')]
                if not cache: command.append('--no-cache')
                if reverse: command.append('--reverse')
                jobs.append((f'python-{tag}', command, python_env, None))
        warmup = {tag: measure(cmd, env, cycles, stdin) for tag, cmd, env, stdin in jobs}
        samples = {tag: [] for tag, *_ in jobs}
        orders = []
        for repetition in range(7):
            order = jobs[repetition:] + jobs[:repetition]
            orders.append([tag for tag, *_ in order])
            for tag, cmd, env, stdin in order:
                samples[tag].append(measure(cmd, env, cycles, stdin))
        measurements = [dict(model=tag, command=cmd, warmup=warmup[tag], samples=samples[tag],
                             median_ns_per_cycle=statistics.median(s['ns_per_cycle'] for s in samples[tag]),
                             median_run_ns=statistics.median(s['run_ns'] for s in samples[tag]))
                        for tag, cmd, *_ in jobs]
        programs.append(dict(name=name, cycles=cycles, orders=orders, measurements=measurements))
        print(f'{name}: {cycles} cycles; 9 groups, one warmup + seven samples', flush=True)
    report = dict(reference=identity, cpp_binary_sha256=hashlib.sha256(args.cpp_runner.read_bytes()).hexdigest(),
                  native_binary_sha256=hashlib.sha256(args.runner.read_bytes()).hexdigest(), source_sha256=fingerprint(),
                  host=platform.platform(), python=sys.version, python_executable=sys.executable,
                  timestamp_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
                  affinity=dict(available=available, selected=cpu), warmups=1, repeats=7,
                  scope='run_ns: clock loop, runtime counters and marker check; first-step Signal initialization included. '
                        'Construction, snapshots, trace and JSON excluded. process_ns includes process startup, imports, input and output. '
                        'Children run serially on one CPU with rotated order. Native has no GFSim scheduler counters.',
                  programs=programs)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
