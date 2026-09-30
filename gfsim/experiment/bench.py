"""Three reproducible workloads; report counters and median time as JSON."""

import argparse
import json
from statistics import median
from time import perf_counter

from engine import Simulator


def mover(sim, source, target, work=0):
    module = sim.module()
    def compute():
        value = source.peek()
        result = sum((value * 17 + i) % 97 for i in range(work)) if work else value
        source.pop()
        target.push(result)
    rule = sim.rule(module, compute, pops=(source,), pushes=(target,))
    module.work = rule
    return module


def sink(sim, queue, period=1):
    module = sim.module()
    rule = sim.rule(module, queue.pop, pops=(queue,))
    module.work = lambda: rule() if sim.tick % period == 0 else None
    return module


def pipeline(reference, args):
    sim = Simulator(reference)
    source = sim.queue(args.ticks + 2, tuple(range(args.ticks + 2)))
    stages = [sim.queue(initial=(i,)) for i in range(args.size)]
    for left, right in zip((source, *stages), stages):
        mover(sim, left, right)
    return sim, (sink(sim, stages[-1]),)


def backpressure(reference, args):
    sim = Simulator(reference)
    source = sim.queue(args.ticks + 2, tuple(range(args.ticks + 2)))
    target, control = sim.queue(initial=(0,)), sim.queue(initial=(0,))
    module = mover(sim, source, target, args.work)
    compute = module.work
    def controlled():
        if control.peek() >= 0:
            compute()
    module.work = controlled
    driver = sim.module()
    toggle = sim.rule(driver, control.revise, revises=(control,))
    driver.work = lambda: toggle(sim.tick % 2)
    return sim, (driver, sink(sim, target, period=20))


def sparse(reference, args):
    sim = Simulator(reference)
    for _ in range(args.idle):
        queue = sim.queue()
        module = sim.module()
        rule = sim.rule(module, queue.pop, pops=(queue,))
        module.work = rule  # Empty reads subscribe once, then sleep.
    source = sim.queue(args.ticks + 2, tuple(range(args.ticks + 2)))
    target = sim.queue(initial=(-1,))
    mover(sim, source, target)
    return sim, (sink(sim, target),)


def measure(builder, reference, args):
    timings, stats, trace, final = [], None, None, None
    for _ in range(args.repeat):
        sim, wake = builder(reference, args)
        sim.step(wake)  # Exclude registration, topology and initial subscriptions.
        sim.stats.clear()
        started = perf_counter()
        run_trace = tuple(sim.step(wake) for _ in range(args.ticks))
        timings.append(perf_counter() - started)
        run_final = sim.snapshot()
        if stats is not None:
            assert stats == dict(sim.stats) and trace == run_trace and final == run_final
        stats, trace, final = dict(sim.stats), run_trace, run_final
    return {"seconds_median": median(timings), "counters": stats}, (trace, final)


def positive(value):
    number = int(value)
    if number < 1:
        raise argparse.ArgumentTypeError("must be positive")
    return number


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ticks", type=positive, default=200)
    parser.add_argument("--size", type=positive, default=32)
    parser.add_argument("--idle", type=positive, default=1000)
    parser.add_argument("--work", type=positive, default=2000)
    parser.add_argument("--repeat", type=positive, default=3)
    args = parser.parse_args()
    report = {"parameters": vars(args), "workloads": {}}
    for builder in (pipeline, backpressure, sparse):
        indexed, indexed_result = measure(builder, False, args)
        reference, reference_result = measure(builder, True, args)
        assert indexed_result == reference_result, builder.__name__
        report["workloads"][builder.__name__] = {
            "indexed": indexed, "reference": reference,
            "reference_over_indexed": reference["seconds_median"] / indexed["seconds_median"],
        }
    print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
