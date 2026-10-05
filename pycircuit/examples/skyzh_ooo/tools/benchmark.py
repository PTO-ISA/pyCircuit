"""Clock-equivalent, serial fixed-tick benchmarks against the pinned native core."""
import argparse
import json
import os
from pathlib import Path
import platform
from pycircuit.benchmark_support import measured, summarize, sha
from ..tests.assemble import assemble
from ..tests.oracle import interpret, load_image
from ..tests.verify import run, compare, inspect, architectural_commits
from ..tests.reference import COMMIT

HERE = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--generated', type=Path, required=True)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--repeats', type=int, default=7)
    parser.add_argument('--iterations', type=int, default=4096)
    parser.add_argument('--cpu', type=int)
    args = parser.parse_args()
    if args.repeats < 1 or not 1 <= args.iterations <= 65535:
        parser.error('positive repeats and 1..65535 iterations required')
    cpu = min(os.sched_getaffinity(0)) if args.cpu is None else args.cpu
    os.sched_setaffinity(0, {cpu})
    args.output.mkdir(parents=True, exist_ok=True)
    binaries = dict(generated=args.generated.resolve(), reference=args.reference.resolve())
    manifest = json.loads(args.reference.with_name('build.json').read_text())
    assert manifest['commit'] == COMMIT and manifest['binary_sha256'] == sha(args.reference)
    flags = (args.generated.parent / 'CMakeFiles/acpy-skyzh-model-compiled.dir/flags.make').read_text()
    expected = {'-std=gnu++20', '-O3', '-DNDEBUG'}
    compiled_flags = next(line.split('=',1)[1].split() for line in flags.splitlines() if line.startswith('CXX_FLAGS ='))
    compiler = next(line.removeprefix('# compile CXX with ') for line in flags.splitlines() if line.startswith('# compile CXX with '))
    assert set(compiled_flags) == expected
    assert compiler == manifest['command'][0] and expected <= set(manifest['command'])
    report = dict(host=platform.platform(), cpu=cpu, repeats=args.repeats,
                  compiler=manifest['compiler'], generated_flags=flags, reference_build=manifest,
                  scope='Same preverified N ticks from initial state. Construction, image loading, termination checks and trace/observation excluded. First Signal initialization included. RSS is process peak.',
                  microarchitecture=dict(rob_slots=8,rob_usable=7,alu_stations=4,load_stations=3,store_stations=3,memory_bytes=0x400000),
                  binaries_sha256={k:sha(v) for k,v in binaries.items()},programs=[])
    generated = args.generated.parent / 'compiled'
    report['generated_code'] = {p.name:dict(bytes=p.stat().st_size,sha256=sha(p)) for p in
                                (generated/'model.cpp',generated/'model.hpp',generated/'model.acir.mlir')}
    for case, old in [('window','addi x2, x0, 64'), ('branches','addi x2, x0, 32')]:
        source = args.output / f'{case}.s'
        source.write_text((HERE/'tests/programs'/f'{case}.s').read_text().replace(old, f'li x2, {args.iterations}'))
        image = assemble(source,args.output/f'{case}.hex')
        oracle = interpret(load_image(image),limit=1000000,trace=True)
        paths = {k:args.output/f'{case}-{k}.jsonl' for k in binaries}
        for name, binary in binaries.items():
            run(binary,image,paths[name])
        trace_hash = compare(paths['generated'],paths['reference'])
        final, raw, coverage = inspect(paths['reference'])
        commits = architectural_commits(raw)
        assert len(commits) == oracle['instructions']
        assert all(all(a[k] == b[k] for k in a) for a,b in zip(commits,oracle['commits']))
        for key in ('registers','memory_changes'): assert final[key] == oracle[key]
        cycles, instructions = final['cycles'],oracle['instructions']
        def sample(name):
            text, metrics = measured([binaries[name],image,'1000000','--fixed',cycles,'--benchmark'])
            row = json.loads(text)
            for key in ('cycles','stopped','registers','memory_changes','predictor_history_hash','predictor_counters_hash'):
                assert row[key] == final[key], (case,name,key)
            return dict(metrics,run_ns=row['run_ns'],construct_ns=row['construct_ns'],
                        ns_per_tick=row['run_ns']/cycles,instructions_per_second=instructions*1e9/row['run_ns'])
        for name in binaries: sample(name)
        samples = {name:[] for name in binaries}
        for repeat in range(args.repeats):
            for name in (list(binaries) if repeat%2==0 else list(reversed(binaries))):
                samples[name].append(sample(name))
        medians = {name:summarize(values) for name,values in samples.items()}
        result = dict(program=case,iterations=args.iterations,cycles=cycles,instructions=instructions,
                      ipc=instructions/cycles,trace_sha256=trace_hash,coverage=coverage,
                      median=medians,samples=samples,
                      slowdown=medians['generated']['ns_per_tick']/medians['reference']['ns_per_tick'])
        report['programs'].append(result)
        print(f'{case}: {cycles} identical clocks, IPC={result["ipc"]:.4f}, '
              f'generated={medians["generated"]["ns_per_tick"]:.1f} ns/tick, '
              f'native={medians["reference"]["ns_per_tick"]:.1f}, ratio={result["slowdown"]:.2f}',flush=True)
    (args.output/'results.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__ == '__main__':
    main()
