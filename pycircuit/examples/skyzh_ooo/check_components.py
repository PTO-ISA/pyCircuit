"""Check generated execution components. This does not accept an OoO CPU."""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess

from pycircuit import compile_source, emit
from pycircuit.ir import load, save
from .assemble import assemble
from .oracle import interpret, load_image

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def requests(output):
    rows = []
    for source in sorted((HERE / 'programs').glob('*.s')):
        image = assemble(source, output / (source.stem + '.hex'))
        trace = interpret(load_image(image), trace=True)['commits']
        for event in trace:
            index = len(rows) + 1
            pc, word = event['pc'], event['word']
            memory, write = event['memory'], event['write']
            # Loads are checked here for their address; memory data is a later stage.
            check_value = write is not None and word & 127 != 3
            row = [1, index, pc, word, event['left'], event['right'], (pc + 4) & 0xffffffff, 0,
                   int(check_value), write[1] if check_value else 0, int(memory is not None),
                   memory[0] if memory else 0, event['right'] if memory else 0, event['next_pc'], 0]
            rows.append(row)
            if index % 17 == 0:
                stale = row.copy()
                stale[0], stale[1] = 0, len(rows) + 1
                rows.append(stale)
    for word, fault, expected_fault, address in ((0, 0, 2, 0), (0, 1, 1, 0),
                                                (0x00202083, 0, 3, 2), (0x00100073, 0, 0, 0)):
        rows.append([1, len(rows) + 1, 0, word, 0, 0, 4, fault,
                     0, 0, int(address != 0), address, 0, 4, expected_fault])
    return str(len(rows)) + '\n' + '\n'.join(' '.join(map(str, row)) for row in rows) + '\n'


def check(output, cxx):
    output.mkdir(parents=True, exist_ok=True)
    input_text = requests(output)
    compiler = shlex.split(cxx)
    include = ROOT / 'gfsim/cpp/include'
    runtime = output / 'runtime.o'
    subprocess.run([*compiler, '-std=c++20', '-O2', '-I', str(include), '-c',
                    str(ROOT / 'gfsim/cpp/src/simulator.cpp'), '-o', str(runtime)], check=True)
    results = []
    for top, runner in (('ExecutionCheck', 'runner.cpp'), ('MemoryHelpers', 'memory_runner.cpp')):
        compiled = output / top / 'compiled'
        model = compile_source(HERE / 'checks/execution.py', top)
        emit(model, compiled)
        save(model, compiled / 'model.acir.json')
        for variant in ('compiled', 'emitted'):
            directory = output / top / variant
            if variant == 'emitted':
                emit(load(compiled / 'model.acir.json'), directory)
            binary = directory / 'run'
            subprocess.run([*compiler, '-std=c++20', '-O2', '-I', str(include), '-I', str(directory),
                            str(directory / 'model.cpp'), str(HERE / 'checks' / runner), str(runtime),
                            '-o', str(binary)], check=True)
            result = subprocess.run([str(binary)], input=input_text, text=True, capture_output=True, check=True)
            results.append(dict(component=top, variant=variant, **json.loads(result.stdout)))
            print(top, variant, result.stdout.strip())
    report = dict(cpu_status='blocked_on_expression', component_checks=results, cpu_end_to_end=False)
    (output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=HERE / 'output/components')
    parser.add_argument('--cxx', default=os.environ.get('ACPY_CXX', 'c++'))
    args = parser.parse_args()
    check(args.output.resolve(), args.cxx)
