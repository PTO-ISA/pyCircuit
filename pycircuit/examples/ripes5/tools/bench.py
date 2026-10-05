"""Verified, one-CPU, serial three-way fixed-window measurements (15 samples)."""
import argparse
import json
import os
from pathlib import Path
import platform
import shlex
import statistics
import time
from ..tests.verify import ROOT, v, sha, fingerprint, verify
from ..tests.benchmark_programs import suite, check_result
from .benchmark_support import fixed_sample, stream_verify
from ..tests.test_benchmark import verify as verify_fixed


def summary(samples):
    result = {}
    for key in ('ns_per_cycle', 'instructions_per_second', 'run_ns', 'process_ns'):
        values = [row[key] for row in samples]
        q1, _, q3 = statistics.quantiles(values, n=4, method='inclusive')
        result[key] = dict(median=statistics.median(values), q1=q1, q3=q3)
    return result


def check_build(path, binaries):
    manifest = json.loads(path.read_text())
    if not manifest['version'].startswith('14.') or manifest['lto'] or manifest['standard'] != 'c++20':
        raise AssertionError('requires uniform GCC 14 / C++20 / no LTO build')
    for label, binary in binaries.items():
        if manifest['binaries'][label]['sha256'] != sha(binary):
            raise AssertionError(f'{label}: binary differs from build manifest; rebuild')
    for source, digest in manifest['local_source_sha256'].items():
        if sha(ROOT / source) != digest:
            raise AssertionError(f'local source changed since build: {source}; rebuild')
    for build in manifest['builds'].values():
        for source, digest in build['translation_units_sha256'].items():
            if sha(source) != digest:
                raise AssertionError(f'source changed since build: {source}')
        for entry in build['commands']:
            flags = shlex.split(entry['command'])
            if not all(flag in flags for flag in shlex.split(manifest['flags'])):
                raise AssertionError(f'missing release flags: {entry}')
            if any(flag.startswith('-flto') for flag in flags):
                raise AssertionError('LTO enabled')
            for prefix, expected in (('-O', '-O3'), ('-std=', '-std=c++20'),
                                     ('-march=', '-march=armv8-a'), ('-mtune=', '-mtune=generic')):
                if [flag for flag in flags if flag.startswith(prefix)] != [expected]:
                    raise AssertionError(f'inconsistent effective flags: {entry}')
            if any(flag.startswith('-mcpu=') for flag in flags):
                raise AssertionError('unexpected CPU architecture override')
    return manifest


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--generated-runner', type=Path, required=True)
    p.add_argument('--cpp-runner', type=Path, required=True)
    p.add_argument('--runner', type=Path, default=v.DEFAULT_RUNNER)
    p.add_argument('--manifest', type=Path, help='defaults to generated runner directory/build-manifest.json')
    p.add_argument('--output', type=Path, default=ROOT / 'reference/benchmarks/ripes5/timing-fixed.json')
    p.add_argument('--evidence', type=Path, default=ROOT / 'reference/benchmarks/ripes5')
    p.add_argument('--cpu', type=int)
    p.add_argument('--warmup-cycles', type=int, default=1024)
    p.add_argument('--repeats', type=int, default=15)
    p.add_argument('--verify-only', action='store_true')
    a = p.parse_args()
    if a.warmup_cycles < 0 or a.repeats < 4:
        p.error('requires K >= 0 and at least four samples')
    available = sorted(os.sched_getaffinity(0))
    cpu = available[0] if a.cpu is None else a.cpu
    if cpu not in available:
        p.error('CPU outside affinity set')
    os.sched_setaffinity(0, {cpu})
    binaries = dict(generated=a.generated_runner.resolve(), handwritten=a.cpp_runner.resolve(), native=a.runner.resolve())
    manifest_path = (a.manifest or a.generated_runner.parent / 'build-manifest.json').resolve()
    manifest = check_build(manifest_path, binaries)
    os.environ['RIPES_QT_PREFIX'] = manifest['qt_prefix']
    identity = v.verify_runner(a.runner)
    a.evidence.mkdir(parents=True, exist_ok=True)
    # All 13 programs x 2 Module orders remain a mandatory prerequisite.
    emitted = Path(manifest['binaries']['emitted']['path'])
    if sha(emitted) != manifest['binaries']['emitted']['sha256']:
        raise AssertionError('emitted runner changed since build')
    acceptance = verify(a.generated_runner, emitted, a.cpp_runner, a.runner,
                        a.evidence / 'acceptance', acir=a.generated_runner.parent / 'compiled/model.acir.mlir')
    fixed = verify_fixed(binaries, a.evidence / 'fixed-runner-tests.json')
    reports = []
    for case in suite():
        print(f'{case["name"]}: checking three models and native observation toggle', flush=True)
        directory = a.evidence / case['name']
        gate = stream_verify(case, binaries, directory, a.warmup_cycles)
        check_result(case, gate['final'])
        if gate['cycles'] - a.warmup_cycles < 100000:
            raise AssertionError('requires at least 100,000 measured cycles')
        reports.append(dict(name=case['name'], input=case, verification=gate, cycles=gate['cycles'],
                            warmup_cycles=a.warmup_cycles, measured_cycles=gate['cycles'] - a.warmup_cycles))
    reference_dir = ROOT / 'gfsim/experiment/examples/ripes5/reference'
    report = dict(schema=2, reference=identity, host=platform.platform(),
                  affinity=dict(available=available, selected=cpu),
                  timestamp_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
                  warmups=1, repeats=a.repeats, gf_config=dict(reverse=False),
                  native_config=dict(port_notifications=False, clock_notifications=False, reverse_history=False),
                  binaries_sha256={k: sha(b) for k, b in binaries.items()},
                  compiler_source_sha256=fingerprint(), existing_source_sha256=v.fingerprint(),
                  adapter_source_sha256={p.name: sha(p) for p in reference_dir.iterdir()
                                         if p.suffix in ('.cpp', '.py', '.cmake', '.json')},
                  build_manifest=dict(path=str(manifest_path), sha256=sha(manifest_path)),
                  build_configuration={k: manifest[k] for k in ('compiler', 'compiler_version', 'flags', 'standard', 'lto')},
                  scope='Only N calls to step()/clockUnguarded(), after K warmup cycles; stop at marker retirement. '
                        'Model checks/statistics retained. Construction, loading, warmup, host address/marker checks '
                        'and snapshots excluded. process_ns measures the subprocess separately.',
                  quantiles='inclusive linear interpolation (statistics.quantiles, n=4)',
                  acceptance=dict(configurations=acceptance['configurations'], full_acceptance=acceptance['full_acceptance'],
                                  comparison_sha256=sha(a.evidence / 'acceptance/comparison.json'), fixed_runner=fixed),
                  programs=reports)
    if a.verify_only:
        target = a.evidence / 'verification.json'
    else:
        for program in reports:
            case = program['input']
            gate = program['verification']
            path = (a.evidence / case['name'] / 'input.json').resolve()
            k, n = program['warmup_cycles'], program['measured_cycles']
            def sample(label):
                return fixed_sample(binaries[label], case, path, k, n, gate['boundary'], gate['final'], native=label == 'native')
            jobs = list(binaries)
            warmup = {label: sample(label) for label in jobs}
            samples, orders = {label: [] for label in jobs}, []
            for i in range(a.repeats):
                offset = i % len(jobs)
                order = jobs[offset:] + jobs[:offset]
                orders.append(order)
                for label in order:
                    samples[label].append(sample(label))
            program['orders'] = orders
            program['warmup_order'] = jobs
            program['measurements'] = [dict(model=label, warmup=warmup[label], samples=samples[label],
                                            summary=summary(samples[label])) for label in jobs]
            medians = {label: statistics.median(s['ns_per_cycle'] for s in samples[label]) for label in jobs}
            program['ratios_of_medians'] = dict(native_over_generated=medians['native'] / medians['generated'],
                                                 native_over_handwritten=medians['native'] / medians['handwritten'],
                                                 generated_over_handwritten=medians['generated'] / medians['handwritten'])
            print(f'{case["name"]}: {n} measured cycles, three models, 1 warmup + {a.repeats} samples', flush=True)
        target = a.output
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(report, indent=2) + '\n')
    print(f'Evidence: {target}', flush=True)


if __name__ == '__main__':
    main()
