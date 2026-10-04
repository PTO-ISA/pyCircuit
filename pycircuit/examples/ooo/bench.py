"""Pinned-CPU repeated timings; model construction and JSON are excluded."""
import argparse
import json
import os
from pathlib import Path
import statistics
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from pycircuit.examples.ooo.programs import suite
from pycircuit.examples.ooo.verify import HERE, architecture, run, sha


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compiled', type=Path, required=True)
    p.add_argument('--emitted', type=Path, required=True)
    p.add_argument('--acceptance', type=Path, default=HERE / 'output/Release/comparison.json')
    p.add_argument('--output', type=Path, default=HERE / 'timing.json')
    args = p.parse_args()
    acceptance = json.loads(args.acceptance.read_text())
    assert acceptance['full_acceptance'], 'full acceptance must precede timing'
    for binary in (args.compiled, args.emitted):
        assert acceptance['binaries'][str(binary)] == sha(binary), 'binary changed since acceptance'
    affinity = sorted(os.sched_getaffinity(0))
    os.sched_setaffinity(0, {affinity[0]})
    results = []
    for case in suite():
        if case['name'] not in ('latency', 'congestion', 'mixed_2026'):
            continue
        configs = [(name, binary, reverse) for name, binary in
                   (('compiled', args.compiled), ('emitted', args.emitted))
                   for reverse in (False, True)]
        samples = [[] for _ in configs]
        for name, binary, reverse in configs:
            rows = run(binary, case, reverse)
            architecture(case, rows, HERE / 'output/timing-mismatch.json')
            run(binary, case, reverse, benchmark=True)  # warm-up
        for repeat in range(7):
            # Rotate execution order while remaining strictly serial on one CPU.
            for index in [(repeat + n) % len(configs) for n in range(len(configs))]:
                name, binary, reverse = configs[index]
                samples[index].append(run(binary, case, reverse, benchmark=True)[0])
        for (name, binary, reverse), values in zip(configs, samples):
            per_cycle = [v['run_ns'] / v['cycles'] for v in values]
            results.append(dict(program=case['name'], binary=name, reverse=reverse,
                                cycles=values[0]['cycles'], ipc=values[0]['retired'] / values[0]['cycles'],
                                median_ns_per_cycle=statistics.median(per_cycle),
                                min_ns_per_cycle=min(per_cycle), max_ns_per_cycle=max(per_cycle), samples=values))
    args.output.write_text(json.dumps(dict(cpu=affinity[0], machine=os.uname().machine,
        method='serial, one warm-up, seven rotated samples; steady_clock inside runner tick loop',
        includes='sim.step, terminal Queue observation, first Signal initialization',
        excludes='input, construction, snapshots, JSON',
        acceptance_sha256=sha(args.acceptance), results=results), indent=2) + '\n')
    print(f'{len(results)} timing groups saved to {args.output}')


if __name__ == '__main__':
    main()
