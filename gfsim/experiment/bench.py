"""RISC-V end-to-end traces and isolated Python simulation-loop timing."""

from pathlib import Path
from dataclasses import asdict
import argparse
import hashlib
import json
import platform
import statistics
import time
import engine
from examples.riscv.model import build_cpu
from examples.riscv.reference import Interpreter

PROGRAMS = {
    'alu': 'addi x1,x0,1\n' + 'addi x1,x1,1\nadd x2,x1,x1\n' * 24 + 'halt',
    'sum': (Path(__file__).parent / 'examples/riscv/programs/sum.s').read_text(),
    'control': '''
addi x1,x0,7
sw x1,0(x0)
lw x2,0(x0)
add x3,x2,x1
beq x3,x1,wrong
jal x5,func
sw x3,8(x0)
jal x0,done
wrong:
sw x1,4(x0)
func:
addi x3,x3,1
jalr x0,0(x5)
done:
halt
''',
}

def capture(program, *, latency, cache, reverse):
    cpu = build_cpu(program, memory_latency=latency, data_words=64, cache=cache, reverse=reverse)
    oracle = Interpreter(cpu.words, data_words=64)
    rows = [{'queues': cpu.sim.snapshot(), 'accepted': [], 'events': []}]
    for _ in range(5000):
        inst = cpu.step()
        if inst is not None:
            assert inst == oracle.step()
            assert cpu.register_values() == tuple(oracle.registers)
        rows.append({'queues': cpu.sim.snapshot(), 'accepted': sorted(cpu.sim.last_accepted),
                     'events': sorted(cpu.sim.events)})
        if cpu.halted:
            break
    assert cpu.halted and oracle.halted and cpu.memory_values() == tuple(oracle.memory)
    return rows


def collect(repeat=5):
    traces, results = {}, {}
    for name, program in PROGRAMS.items():
        for latency in (1, 3, 5):
            for cache in (True, False):
                for reverse in (False, True):
                    tag = f'{name}-lat{latency}-cache{int(cache)}-reverse{int(reverse)}'
                    traces[tag] = capture(program, latency=latency, cache=cache, reverse=reverse)
                    samples = []
                    for _ in range(repeat):
                        cpu = build_cpu(program, memory_latency=latency, data_words=64, cache=cache, reverse=reverse)
                        start = time.perf_counter_ns()
                        for tick in range(5000):
                            cpu.sim.step()
                            if cpu.halted:
                                break
                        samples.append(time.perf_counter_ns() - start)
                        assert cpu.halted
                    results[tag] = {'samples_ns': samples, 'median_ns': statistics.median(samples), 'cycles': cpu.sim.tick, 'stats': asdict(cpu.sim.stats)}
    return traces, results

def compare_traces(expected, actual):
    if expected.keys() != actual.keys():
        raise AssertionError('configuration sets differ')
    for tag, rows in expected.items():
        if len(rows) != len(actual[tag]):
            raise AssertionError(f'{tag}: cycle count {len(rows)} != {len(actual[tag])}')
        for tick, (old, new) in enumerate(zip(rows, actual[tag])):
            if old != new:
                fields = [key for key in old if old[key] != new[key]]
                raise AssertionError(f'{tag}: first difference at snapshot {tick}, fields {fields}: '
                                     f'old={old!r}, new={new!r}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path(__file__).parent / 'review-output/dirty')
    parser.add_argument('--compare', type=Path, help='previous traces.json')
    parser.add_argument('--repeat', type=int, default=5)
    args = parser.parse_args()
    if args.repeat < 1:
        parser.error('--repeat must be positive')
    args.output.mkdir(parents=True, exist_ok=True)
    traces, results = collect(args.repeat)
    encoded = json.dumps(traces)
    (args.output / 'traces.json').write_text(encoded + '\n')
    report = {
        'python': platform.python_version(), 'machine': platform.machine(),
        'engine_sha256': hashlib.sha256(Path(engine.__file__).read_bytes()).hexdigest(),
        'scope': f'{args.repeat} samples; construction, snapshots, oracle and JSON outside timer; '
                 'clock loop plus HALT check inside timer; built-in counters enabled',
        'workloads': results,
    }
    (args.output / 'timing.json').write_text(json.dumps(report, indent=2) + '\n')
    if args.compare:
        compare_traces(json.loads(args.compare.read_text()), json.loads(encoded))
        print(f'All {len(traces)} configurations match previous snapshots, accepted Rules and events.')
    print(f'Saved {len(traces)} configurations to {args.output}')


if __name__ == '__main__':
    main()
