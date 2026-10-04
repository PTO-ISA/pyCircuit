"""Verified, serial fixed-tick comparisons with the unmodified pinned skyzh core."""
import argparse
import json
import os
from pathlib import Path
import platform
from pycircuit.benchmark_migration import measured, summarize, sha
from .assemble import assemble
from .oracle import interpret, load_image
from .verify import check
from .reference.build import COMMIT

HERE = Path(__file__).resolve().parent


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--generated', type=Path, required=True)
    p.add_argument('--reference', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--repeats', type=int, default=7)
    p.add_argument('--cpu', type=int)
    a = p.parse_args()
    cpu = min(os.sched_getaffinity(0)) if a.cpu is None else a.cpu
    os.sched_setaffinity(0, {cpu})
    a.output.mkdir(parents=True, exist_ok=True)
    binaries = dict(generated=a.generated.resolve(), reference=a.reference.resolve())
    reference_build = json.loads((a.reference.parent/'build.json').read_text())
    assert reference_build['commit'] == COMMIT and reference_build['binary_sha256'] == sha(a.reference)
    flags = (a.generated.parent/'CMakeFiles/acpy-skyzh-model-compiled.dir/flags.make').read_text()
    expected_flags = {'-std=gnu++20', '-O3', '-DNDEBUG'}
    generated_flags = next(line.split('=', 1)[1].split() for line in flags.splitlines() if line.startswith('CXX_FLAGS ='))
    compiler = next(line.removeprefix('# compile CXX with ') for line in flags.splitlines() if line.startswith('# compile CXX with '))
    assert set(generated_flags) == expected_flags, 'benchmark requires the uniform Release configuration'
    assert compiler == reference_build['command'][0] and expected_flags <= set(reference_build['command']), 'nonuniform reference compiler/flags'
    source = a.generated.parent/'compiled'
    result = dict(host=platform.platform(), cpu=cpu, repeats=a.repeats, reference_build=reference_build,
                  generated_flags=flags, generated_sources_sha256={n: sha(source/n) for n in ('model.cpp','model.hpp','ac_support.hpp','model.acir.mlir')},
                  binaries_sha256={k: sha(b) for k,b in binaries.items()},
                  scope='Fixed N calls to step()/tick(), from initial state (K=0). Construction, image loading, host termination tests and observations excluded; first Signal initialization included. RSS is whole-process peak.',
                  microarchitecture=dict(generated=dict(rob=12, memory_bytes=262144), reference=dict(rob=7, memory_bytes=4194304)), programs=[])
    for name, original in [('window', 'addi x2, x0, 64'), ('branches', 'addi x2, x0, 32')]:
        source = a.output/(name+'.s')
        source.write_text((HERE/'programs'/source.name).read_text().replace(original, 'lui x2, 1'))
        image = assemble(source, a.output/(name+'.hex'))
        oracle = interpret(load_image(image), trace=True)
        text, _ = measured([binaries['generated'], image, '1000000'])
        rows = [json.loads(line) for line in text.splitlines()]
        check(rows, oracle)
        cycles = {'generated': rows[-1]['cycles']}
        text, _ = measured([binaries['reference'], image, '1000000'])
        native = json.loads(text)
        for key in ('registers','memory_changes'): assert native[key] == oracle[key], (name,key)
        assert native['stopped']
        cycles['reference'] = native['cycles']
        def sample(label):
            command = [binaries[label], image, '1000000', '--fixed', cycles[label]]
            if label=='generated': command.append('--benchmark')
            text, metrics = measured(command)
            row = json.loads(text)
            assert row['stopped'] and row['cycles'] == cycles[label]
            for key in ('registers','memory_changes'): assert row[key] == oracle[key], (name,label,key)
            ns = row['run_ns'] if label=='generated' else row['elapsed_ns']
            return dict(metrics, run_ns=ns, ns_per_tick=ns/cycles[label],
                        instructions_per_second=oracle['instructions']*1e9/ns)
        for label in binaries: sample(label)
        samples = {k: [] for k in binaries}
        for i in range(a.repeats):
            for label in (list(binaries) if i%2==0 else list(reversed(binaries))): samples[label].append(sample(label))
        report = dict(program=name, iterations=4096, instructions=oracle['instructions'], cycles=cycles,
                      ipc={k: oracle['instructions']/n for k,n in cycles.items()}, image_sha256=sha(image),
                      samples=samples, median={k:summarize(s) for k,s in samples.items()})
        result['programs'].append(report)
        print(name, report['median'], flush=True)
    (a.output/'results.json').write_text(json.dumps(result, indent=2)+'\n')


if __name__ == '__main__':
    main()
