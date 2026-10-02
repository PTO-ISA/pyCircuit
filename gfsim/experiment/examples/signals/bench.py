"""Simulation-only timing; construction and the independent oracle are not timed."""
from dataclasses import asdict
from pathlib import Path
import argparse
import json
import platform
import statistics
import time

from .model import build, workload
from .reference import Reference


def measure(repeat=7, cycles=1200):
    results = {}
    for frequent in (False, True):
        _, script = workload(cycles, frequent)
        words = (tuple(range(cycles // 8)), tuple(range(1000, 1000 + cycles // 8)))
        sim, ref = build(words=words, script=script, cycles=cycles), Reference(words, script)
        for _ in range(cycles):
            sim.step()
            assert sim.snapshot() == ref.step()
            assert tuple(signal.value for signal in sim.signals) == ref.signals()
        samples = []
        for sample in range(repeat + 1):
            sim = build(words=words, script=script, cycles=cycles)
            start = time.perf_counter_ns()
            for _ in range(cycles):
                sim.step()
            elapsed = time.perf_counter_ns() - start
            if sample:  # One warmup before measured runs.
                samples.append(elapsed)
        results['frequent' if frequent else 'sparse'] = {
            'cycles': cycles, 'samples_ns': samples, 'median_ns': statistics.median(samples),
            'stats': asdict(sim.stats),
            'unconditional_evaluations': len(sim.signals) * (cycles + 1)}
    return {'python': platform.python_version(), 'machine': platform.machine(),
            'scope': 'Only sim.step loop; counters on; observer off; one warmup',
            'workloads': results}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path(__file__).parent / 'results.json')
    parser.add_argument('--repeat', type=int, default=7)
    args = parser.parse_args()
    if args.repeat < 1:
        parser.error('--repeat must be positive')
    result = measure(args.repeat)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    for name, row in result['workloads'].items():
        print(f"{name}: {row['median_ns'] / 1e6:.3f} ms; "
              f"{row['stats']['signal_work']} / {row['unconditional_evaluations']} evaluations")


if __name__ == '__main__':
    main()
