"""Three reproducible workloads; report counters and median time as JSON."""

import argparse
import json
from statistics import median
from time import perf_counter

from construction import assemble
from engine import Queue, RuleEntry
from models import Drain, Move, Revise


def pipeline(reference, args):
    queues = [Queue(args.ticks + 2, tuple(range(args.ticks + 2)))]
    queues.extend(Queue(initial=(i,)) for i in range(args.size))
    modules = [Move(i, i + 1, queues[i], queues[i + 1]) for i in range(args.size)]
    rules = [None] + [RuleEntry(i, m.work_move, m.arbitrate_move, pops=(i,), pushes=(i + 1,))
                      for i, m in enumerate(modules)]
    sink = Drain(args.size, args.size + 1, queues[-1])
    modules.append(sink)
    rules.append(RuleEntry(sink.mid, sink.work_drain, sink.arbitrate_drain, pops=(args.size,)))
    return assemble(queues, modules, rules, reference), (sink.mid,)


def backpressure(reference, args):
    source = Queue(args.ticks + 2, tuple(range(args.ticks + 2)))
    target, control = Queue(initial=(0,)), Queue(initial=(0,))
    compute = Move(0, 1, source, target, control=control, iterations=args.work)
    driver = Revise(1, 2, (control,), lambda tick: (tick % 2,))
    sink = Drain(2, 3, target, lambda tick: tick % 20 == 0)
    rules = [None,
        RuleEntry(0, compute.work_move, compute.arbitrate_move, pops=(0,), pushes=(1,)),
        RuleEntry(1, driver.work_revise, driver.arbitrate_revise, revises=(2,)),
        RuleEntry(2, sink.work_drain, sink.arbitrate_drain, pops=(1,))]
    return assemble([source, target, control], [compute, driver, sink], rules, reference), (1, 2)


def sparse(reference, args):
    queues = [Queue() for _ in range(args.idle)]
    modules = [Drain(i, i + 1, queue) for i, queue in enumerate(queues)]
    rules = [None] + [RuleEntry(i, m.work_drain, m.arbitrate_drain, pops=(i,))
                      for i, m in enumerate(modules)]
    source, target = Queue(args.ticks + 2, tuple(range(args.ticks + 2))), Queue(initial=(-1,))
    mover = Move(args.idle, args.idle + 1, source, target)
    sink = Drain(args.idle + 1, args.idle + 2, target)
    rules.extend((RuleEntry(mover.mid, mover.work_move, mover.arbitrate_move,
                            pops=(args.idle,), pushes=(args.idle + 1,)),
                  RuleEntry(sink.mid, sink.work_drain, sink.arbitrate_drain, pops=(args.idle + 1,))))
    return assemble([*queues, source, target], [*modules, mover, sink], rules, reference), (sink.mid,)


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
