"""Pinned-CPU, serial old/new Python and native Ripes whole-model timing."""
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

from .programs import suite
from .run import HERE, DEFAULT_RUNNER, native_env, verify_runner, run_reference, run_python, compare
from .evidence import (DEFAULT_EVIDENCE, preserve, baseline_source, fingerprint,
                       worker_command, source_env)


def measure(command, env, cycles):
    start = time.perf_counter_ns()
    result = subprocess.run(command, env=env, text=True, capture_output=True, check=True, timeout=120)
    process_ns = time.perf_counter_ns() - start
    sample = json.loads(result.stdout)
    if sample['cycles'] != cycles:
        raise AssertionError(f'benchmark cycles changed: {sample["cycles"]} != {cycles}')
    return dict(sample, process_ns=process_ns, ns_per_cycle=sample['run_ns'] / cycles)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runner', type=Path, default=DEFAULT_RUNNER)
    parser.add_argument('--output', type=Path, default=DEFAULT_EVIDENCE / 'timing.json')
    parser.add_argument('--evidence', type=Path, default=DEFAULT_EVIDENCE)
    parser.add_argument('--repeats', type=int, default=7)
    parser.add_argument('--cpu', type=int, help='default: lowest CPU in this process affinity set')
    parser.add_argument('--case', action='append', help='repeat to select programs')
    args = parser.parse_args()
    if args.repeats < 1:
        parser.error('repeats must be positive')
    available = sorted(os.sched_getaffinity(0))
    cpu = min(available) if args.cpu is None else args.cpu
    if cpu not in available:
        parser.error(f'CPU must be in {available}')
    os.sched_setaffinity(0, {cpu})  # Children inherit the same single-CPU affinity.
    identity = verify_runner(args.runner)
    baseline = preserve(args.evidence)
    roots = dict(before=baseline_source(args.evidence), after=HERE.parents[1])
    names = args.case or ['array_sum', 'mixed_2026', 'memory_loop_256']
    cases = {case['name']: case for case in suite()}
    if len(set(names)) != len(names) or any(name not in cases for name in names):
        parser.error('unknown or duplicate case')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    programs = []
    for name in names:
        case = cases[name]
        directory = args.output.parent / 'benchmark-input' / name
        expected = run_reference(case, args.runner, directory)
        cycles = expected[-1]['cycle']
        jobs = [('native', [str(args.runner.resolve()), str(directory.resolve() / 'input.json'), '--benchmark'], native_env())]
        for version, root in roots.items():
            for cache in (True, False):
                for reverse in (False, True):
                    tag = f'{version}-cache{int(cache)}-reverse{int(reverse)}'
                    command = worker_command(directory.resolve() / 'input.json', cache, reverse)
                    env = source_env(root)
                    # All snapshots, JSONL and comparisons precede the measurements.
                    if version == 'before':
                        trace_path = directory.resolve() / f'{tag}.jsonl'
                        subprocess.run(command + ['--trace', str(trace_path)], env=env,
                                       capture_output=True, text=True, check=True)
                        rows = [json.loads(line) for line in trace_path.read_text().splitlines()]
                    else:
                        rows = run_python(case, cache, reverse)
                    compare(case, expected, rows, directory / f'{tag}.mismatch.json')
                    jobs.append((tag, command, env))
        # One untallied warmup per group, then rotate the serial measurement order.
        warmup = {tag: measure(command, env, cycles) for tag, command, env in jobs}
        samples = {tag: [] for tag, _, _ in jobs}
        orders = []
        for repetition in range(args.repeats):
            offset = repetition % len(jobs)
            order = jobs[offset:] + jobs[:offset]
            orders.append([tag for tag, _, _ in order])
            for tag, command, env in order:
                samples[tag].append(measure(command, env, cycles))
        measurements = []
        for tag, command, _ in jobs:
            group = samples[tag]
            measurements.append(dict(model=tag, command=command, warmup=warmup[tag], samples=group,
                median_run_ns=statistics.median(s['run_ns'] for s in group),
                median_process_ns=statistics.median(s['process_ns'] for s in group),
                median_ns_per_cycle=statistics.median(s['ns_per_cycle'] for s in group)))
        by_name = {m['model']: m for m in measurements}
        ratios = []
        for cache in (True, False):
            for reverse in (False, True):
                tag = f'cache{int(cache)}-reverse{int(reverse)}'
                old, new, native = (by_name[key] for key in (f'before-{tag}', f'after-{tag}', 'native'))
                ratios.append(dict(cache=cache, reverse=reverse,
                    before_over_after_run=old['median_run_ns'] / new['median_run_ns'],
                    before_over_after_process=old['median_process_ns'] / new['median_process_ns'],
                    before_over_native_run=old['median_run_ns'] / native['median_run_ns'],
                    after_over_native_run=new['median_run_ns'] / native['median_run_ns']))
        programs.append(dict(name=name, cycles=cycles, input_sha256=hashlib.sha256(
            (directory / 'input.json').read_bytes()).hexdigest(), orders=orders,
            measurements=measurements, ratios=ratios))
        print(f'{name}: {cycles} cycles; 9 groups, one warmup + {args.repeats} samples', flush=True)
    report = dict(reference=identity, binary_sha256=hashlib.sha256(args.runner.read_bytes()).hexdigest(),
        baseline=baseline, current_sha256=fingerprint(roots['after']),
        harness_sha256={name: hashlib.sha256((HERE / name).read_bytes()).hexdigest()
                       for name in ('bench.py', 'worker.py', 'evidence.py')},
        python=sys.version, python_executable=sys.executable, host=platform.platform(),
        timestamp_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
        affinity=dict(available=available, selected=cpu), repeats=args.repeats, warmups=1,
        programs=programs,
        scope='run_ns: clock loop, original runtime checks and marker check; Python first-step Signal initialization included. Construction, snapshots, JSON and review excluded. process_ns: cold child process, imports, input, construction, loop, result output and exit. One CPU, serial children, rotated order. Built-in Rule/Signal counters and native signal machinery retained. Ratios compare complete Python and C++ models, not schedulers.')
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(args.output)


if __name__ == '__main__':
    main()
