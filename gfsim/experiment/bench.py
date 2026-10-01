"""End-to-end workloads; cache on/off, identical inputs and activation semantics."""

import argparse
from dataclasses import asdict
import json
import platform
from statistics import median
from time import perf_counter

from examples.pipeline.model import pipeline
from examples.memory.model import memory
from engine import Stats


def measure(build, ticks, repeat):
    modes, signatures = {}, []
    for cache in (True, False):
        elapsed, signature = [], None
        for _ in range(repeat):
            circuit = build(cache)
            sim = circuit.sim
            sim.step()  # Construction and initial subscription are outside the timing.
            sim.stats = Stats()
            start = perf_counter()
            trace = []
            for _ in range(ticks):
                trace.append(tuple(sorted(sim.step())))
            elapsed.append(perf_counter() - start)
            current = (trace, sim.snapshot(), sorted(sim.events))
            if signature is not None:
                assert signature == current
            signature = current
        signatures.append(signature)
        modes["cached" if cache else "uncached"] = {
            "seconds_median": median(elapsed), "counters": asdict(sim.stats),
        }
    assert signatures[0] == signatures[1]
    modes["uncached_over_cached"] = (modes["uncached"]["seconds_median"] /
                                      modes["cached"]["seconds_median"])
    modes["layout"] = {"modules": len(sim.modules), "rules": len(sim.rules) - 1,
                        "queues": len(sim.queues),
                        "reader_slots": len(sim.modules) * len(sim.queues),
                        "reader_bytes_if_uint64": len(sim.modules) * len(sim.queues) * 8}
    modes["completed_outputs"] = circuit.output.size()
    return modes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ticks", type=int, default=150)
    parser.add_argument("--length", type=int, default=16)
    parser.add_argument("--depth", type=int, default=512)
    parser.add_argument("--work", type=int, default=2000)
    parser.add_argument("--repeat", type=int, default=3)
    args = parser.parse_args()
    if min(args.ticks, args.length, args.depth, args.repeat) < 1 or args.work < 0:
        parser.error("sizes and repeat must be positive; work must be nonnegative")
    values = tuple(range(args.ticks + 10))
    requests = tuple((i, (i * 17) % (2 * args.depth), i % 2 == 0, i)
                     for i in range(args.ticks))
    workloads = {
        "full_pipeline": lambda cache: pipeline(values, length=args.length, period=1,
                                                  prefill=True, cache=cache),
        "backpressure_compute": lambda cache: pipeline(values, length=2, period=20,
             iterations=args.work, control=tuple(range(args.ticks + 2)), cache=cache),
        "sparse_banked_table": lambda cache: memory(requests, depth=args.depth,
                                                      latency=4, period=3, cache=cache),
    }
    report = {"python": platform.python_version(), "machine": platform.machine(),
              "parameters": vars(args), "workloads": {}}
    for name, build in workloads.items():
        report["workloads"][name] = measure(build, args.ticks, args.repeat)
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
