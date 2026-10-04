"""Compare a preserved checkout/build with this build on identical Ripes5 programs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parent.parent


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def measured(command, **kwargs):
    """Process wall time and peak RSS; runner run_ns remains the fixed-loop timer."""
    with tempfile.NamedTemporaryFile() as rss:
        start = time.perf_counter_ns()
        wrapper = ('import resource,subprocess,sys; p=subprocess.run(sys.argv[2:]); '
                   'open(sys.argv[1],"w").write(str(resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss)); '
                   'sys.exit(p.returncode)')
        p = subprocess.run([sys.executable, '-c', wrapper, rss.name, *map(str, command)],
                           capture_output=True, text=True, timeout=300, **kwargs)
        wall = time.perf_counter_ns() - start
        if p.returncode:
            raise RuntimeError(f'{command}: {p.stderr}')
        return p.stdout, dict(process_ns=wall, max_rss_kib=int(Path(rss.name).read_text()))


def summarize(samples):
    return {key: statistics.median(s[key] for s in samples) for key in samples[0]}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--baseline-build', type=Path, required=True)
    p.add_argument('--baseline-source', type=Path, required=True, help='root containing preserved pycircuit/')
    p.add_argument('--build', type=Path, required=True)
    p.add_argument('--cxx', required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--iterations', type=int, default=12000)
    p.add_argument('--repeats', type=int, default=7)
    p.add_argument('--cpu', type=int)
    a = p.parse_args()
    cpu = min(os.sched_getaffinity(0)) if a.cpu is None else a.cpu
    os.sched_setaffinity(0, {cpu})
    a.output.mkdir(parents=True, exist_ok=True)
    sys.path.insert(0, str(ROOT / 'pycircuit/examples/ripes5'))
    from benchmark_programs import suite, check_result
    from benchmark_support import state
    from verify import v
    builds = dict(before=a.baseline_build.resolve(), mlir=a.build.resolve())
    binaries = {k: b / 'acpy-ripes5-compiled' for k, b in builds.items()}
    flags = {k: (b / 'CMakeFiles/acpy-model-compiled.dir/flags.make').read_text() for k, b in builds.items()}
    assert flags['before'] == flags['mlir'].replace(str(builds['mlir']), str(builds['before'])), 'nonuniform model build flags'
    result = dict(host=platform.platform(), cpu=cpu, compiler=subprocess.check_output([a.cxx, '--version'], text=True),
                  flags=flags, binary_sha256={k: sha(b) for k,b in binaries.items()},
                  scope='Fixed N step() calls after K=1024 ticks; construction, loading, warmup and observation excluded. RSS is peak whole-process RSS.',
                  repeats=a.repeats, compilation={}, programs=[])
    for label, root in dict(before=a.baseline_source.resolve(), mlir=ROOT).items():
        out = a.output / label
        build = a.baseline_build if label == 'before' else a.build
        env = dict(os.environ, ACPY_MLIR_COMPILER=str(build.resolve() / 'mlir/acir-compile'))
        command = [sys.executable, '-m', 'pycircuit', 'compile', str(root / 'pycircuit/examples/ripes5/model.py'), '--top', 'CPU', '--output', str(out.resolve())]
        samples = []
        for _ in range(3):
            _, front = measured(command, cwd=root, env=env)
            _, cpp = measured([a.cxx, '-std=gnu++20', '-O3', '-DNDEBUG', '-I', root/'gfsim/cpp/include', '-c', out/'model.cpp', '-o', out/'model.o'])
            samples.append(dict(frontend_and_emission_ns=front['process_ns'], cpp_compile_ns=cpp['process_ns'],
                                frontend_rss_kib=front['max_rss_kib'], cpp_rss_kib=cpp['max_rss_kib']))
        files = [out / n for n in ('model.cpp','model.hpp','ac_support.hpp')]
        result['compilation'][label] = dict(samples=samples, median=summarize(samples),
                                            generated_bytes=sum(f.stat().st_size for f in files), files={f.name: sha(f) for f in files})
    for case in suite(a.iterations):
        tokens = v.numeric_input(case).split()
        # Protocol v1 has a candidate-cache flag; current runners use v2.
        payloads = dict(mlir=' '.join(tokens), before=' '.join(['1', *tokens[1:4], '1', *tokens[4:]]))
        gates = {}
        for label, binary in binaries.items():
            # Bounded-memory gate: retain only boundary/final state and a trace digest.
            with tempfile.TemporaryFile(mode='w+t') as source, tempfile.TemporaryFile(mode='w+t') as errors:
                source.write(payloads[label]); source.seek(0)
                proc = subprocess.Popen([str(binary)], stdin=source, stdout=subprocess.PIPE, stderr=errors, text=True)
                digest = hashlib.sha256(); boundary = final = None
                for line in proc.stdout:
                    row = json.loads(line)
                    digest.update((json.dumps(row, sort_keys=True)+'\n').encode())
                    if row['cycle'] == 1024: boundary = row
                    final = row
                proc.stdout.close()
                assert proc.wait() == 0
                check_result(case, final)
                gates[label] = dict(boundary=boundary, final=final, sha256=digest.hexdigest())
        assert gates['before'] == gates['mlir'], case['name']
        gate = gates['mlir']; final = gate['final']; boundary = gate['boundary']
        n = final['cycle'] - 1024
        assert n > 0 and boundary is not None
        def sample(label):
            text, metrics = measured([binaries[label], '--benchmark-fixed', '1024', str(n)], input=payloads[label])
            data = json.loads(text)
            assert state(data['final_state']) == state(final)
            assert data['measured_cycles'] == n and data['retired_delta'] == final['retired']-boundary['retired']
            counters = {k: data[k] for k in ('construct_ns', 'module_work', 'rule_work', 'queue_checks',
                        'arbitration_attempts', 'delta_rounds', 'accepted', 'events', 'due_events',
                        'cache_hits', 'reader_checks', 'dfs_visits') if k in data}
            return dict(metrics, run_ns=data['run_ns'], ns_per_tick=data['run_ns']/n, **counters,
                        instructions_per_second=data['retired_delta']*1e9/data['run_ns'])
        for label in binaries: sample(label)
        samples = {k: [] for k in binaries}
        for i in range(a.repeats):
            for label in (list(binaries) if i%2==0 else list(reversed(binaries))): samples[label].append(sample(label))
        report = dict(program=case['name'], cycles=final['cycle'], measured_cycles=n, trace_sha256=gate['sha256'],
                      samples=samples, median={k: summarize(s) for k,s in samples.items()})
        result['programs'].append(report)
        print(case['name'], report['median'], flush=True)
    (a.output/'results.json').write_text(json.dumps(result, indent=2)+'\n')


if __name__ == '__main__':
    main()
