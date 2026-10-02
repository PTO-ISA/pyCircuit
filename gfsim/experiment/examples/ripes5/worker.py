"""Standalone worker; PYTHONPATH selects the preserved or current experiment."""
import argparse
import json
from pathlib import Path
import time

from examples.ripes5.model import CPU
from examples.ripes5.programs import validate
from examples.ripes5.run import run_python, write_jsonl


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('--no-cache', action='store_true')
    parser.add_argument('--reverse', action='store_true')
    parser.add_argument('--trace', type=Path)
    args = parser.parse_args()
    case = json.loads(args.input.read_text())
    validate(case)
    if args.trace:
        write_jsonl(args.trace, run_python(case, not args.no_cache, args.reverse))
        return
    start = time.perf_counter_ns()
    cpu = CPU(case, cache=not args.no_cache, reverse=args.reverse)
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
    signals = {s.helper.__name__: dict(initial=1, xfer=s.read_gen - 1, total=s.read_gen)
               for s in cpu.sim.signals}
    print(json.dumps(dict(cycles=cpu.sim.tick, run_ns=run_ns, construct_ns=construct_ns,
                          rule_calls=cpu.sim.rule_calls[1:], module_calls=cpu.sim.module_calls,
                          events=cpu.sim.stats.events,
                          change_notifications=cpu.sim.stats.change_notifications,
                          signals=signals,
                          signal_work=cpu.sim.stats.signal_work,
                          cache_hits=cpu.sim.stats.cache_hits)))


if __name__ == '__main__':
    main()
