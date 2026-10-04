"""Same-compiler, same-CPU before/after Signal scheduling measurements."""
import argparse
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import time

from benchmark_programs import suite, check_result
from benchmark_support import stream_verify, fixed_sample
from verify import sha, v


def configuration(build):
    text = (build / 'CMakeCache.txt').read_text()
    entries = dict(line.split('=', 1) for line in text.splitlines()
                   if '=' in line and not line.startswith(('#', '//')))
    keys = ('CMAKE_CXX_COMPILER', 'CMAKE_CXX_FLAGS', 'CMAKE_CXX_FLAGS_RELEASE')
    return {key: next(value for k, value in entries.items() if k.split(':')[0] == key)
            for key in keys}


def source_identity(build):
    cache = (build / 'CMakeCache.txt').read_text()
    home = next(line.split('=', 1)[1] for line in cache.splitlines()
                if line.startswith('CMAKE_HOME_DIRECTORY:'))
    root = Path(home).parent
    paths = [p for directory in ('gfsim/cpp/include', 'gfsim/cpp/src', 'gfsim/cpp/examples/ripes5')
             for p in (root / directory).rglob('*') if p.suffix in ('.hpp', '.cpp')]
    paths += [p for p in (root / 'pycircuit').iterdir() if p.suffix in ('.py', '.hpp')]
    paths += [root / 'pycircuit/examples/ripes5' / name for name in ('model.py', 'logic.py')]
    return dict(root=str(root), files={str(p.relative_to(root)): sha(p) for p in sorted(paths)},
                cmake_cache_sha256=sha(build / 'CMakeCache.txt'),
                runtime_flags=(build / 'gfsim/CMakeFiles/gfsim.dir/flags.make').read_text())


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--before-build', type=Path, required=True)
    p.add_argument('--after-build', type=Path, required=True)
    p.add_argument('--native', type=Path, required=True)
    p.add_argument('--evidence', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--cpu', type=int)
    p.add_argument('--repeats', type=int, default=15)
    p.add_argument('--verify-only', action='store_true')
    p.add_argument('--reuse-gates', action='store_true')
    a = p.parse_args()
    if a.repeats < 4:
        p.error('at least four rotated samples required')
    builds = dict(before=a.before_build.resolve(), after=a.after_build.resolve())
    configs = {name: configuration(build) for name, build in builds.items()}
    if configs['before'] != configs['after']:
        raise ValueError(f'compiler/flags differ: {configs}')
    binaries = {f'{version}_{kind}': build / runner for version, build in builds.items()
                for kind, runner in (('generated', 'acpy-ripes5-compiled'),
                                     ('handwritten', 'gfsim/gfsim-ripes5'))}
    binaries['native'] = a.native.resolve()
    fingerprints = {label: sha(binary) for label, binary in binaries.items()}
    reference = v.verify_runner(a.native)
    available = sorted(os.sched_getaffinity(0))
    cpu = available[0] if a.cpu is None else a.cpu
    if cpu not in available:
        p.error('CPU outside affinity set')
    report = dict(timestamp_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
                  host=platform.platform(), build_configuration=configs,
                  sources={name: source_identity(build) for name, build in builds.items()},
                  compiler_version=subprocess.check_output([configs['after']['CMAKE_CXX_COMPILER'], '--version'], text=True),
                  binaries={k: dict(path=str(v), sha256=fingerprints[k]) for k, v in binaries.items()},
                  reference=reference, cpu=cpu, repeats=a.repeats, warmups=1,
                  measurement_sources={name: sha(Path(__file__).with_name(name)) for name in
                                       ('bench_signals.py', 'benchmark_programs.py', 'benchmark_support.py')},
                  scope='Times cover N step() calls after 1024 warmup ticks. Counters include initialization and warmup. '
                        'Cache enabled, forward Module order; alternating serial runs on one CPU. '
                        'Each long trace matches pinned Ripes, each sample checks final state. No cross-version scheduling-counter equality assumed.',
                  programs=[])
    # Verify traces first; keep this separate from timed, pinned serial sampling.
    cases = suite()
    for case in cases:
        directory = a.evidence / case['name']
        identity = directory / 'binaries.json'
        if a.reuse_gates:
            if json.loads(identity.read_text()) != fingerprints:
                raise ValueError('stale gate binaries')
            if json.loads((directory / 'input.json').read_text()) != case:
                raise ValueError('stale gate input')
            gate = json.loads((directory / 'verification.json').read_text())
        else:
            gate = stream_verify(case, binaries, directory, 1024)
            identity.write_text(json.dumps(fingerprints, indent=2) + '\n')
        check_result(case, gate['final'])
        if gate['cycles'] - 1024 < 100000:
            raise AssertionError('long benchmark must measure at least 100,000 ticks')
        report['programs'].append(dict(name=case['name'], input=case, verification=gate))
    if not a.verify_only:
        os.sched_setaffinity(0, {cpu})
        jobs = [key for key in binaries if key != 'native']
        for case, program in zip(cases, report['programs']):
            gate = program['verification']
            def sample(label):
                return fixed_sample(binaries[label], case, a.evidence / case['name'] / 'input.json',
                                    1024, gate['cycles'] - 1024, gate['boundary'], gate['final'])
            program['warmups'] = {label: sample(label) for label in jobs}
            program['samples'] = {label: [] for label in jobs}
            for i in range(a.repeats):
                for label in jobs[i % len(jobs):] + jobs[:i % len(jobs)]:
                    program['samples'][label].append(sample(label))
            program['median_ns_per_cycle'] = {label: statistics.median(s['ns_per_cycle'] for s in samples)
                                              for label, samples in program['samples'].items()}
            for samples in program['samples'].values():
                for key in ('signal_work', 'signal_evaluations', 'module_work', 'rule_work'):
                    if any(s[key] != samples[0][key] for s in samples):
                        raise AssertionError(f'non-deterministic counter {key}')
            print(case['name'], program['median_ns_per_cycle'], flush=True)
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
