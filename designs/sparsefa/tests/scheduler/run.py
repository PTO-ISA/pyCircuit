"""Compile and execute generated C++ Systems and emit the same final IR as Verilog.

Uses the toolchain built in this checkout. Set CC/CXX/PATH/LD_LIBRARY_PATH for
that build's runtime environment. Evidence is ignored and never a test input.
"""
import argparse
import hashlib
import json
import os
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]


def validate_transcript(path, cycles, event):
    rows = [json.loads(line) for line in path.read_text().splitlines()
            if line.startswith('{')]
    terminal = [row for row in rows if row.get('kind') == 'result']
    if (len(terminal) != 1 or terminal[0].get('status') != 'TERMINATED'
            or 'error' not in terminal[0] or terminal[0]['error'] is not None):
        raise ValueError('missing or unsuccessful terminal result')
    if str(terminal[0].get('epoch_time')) != str(2 * cycles):
        raise ValueError('incomplete generated run')
    if not any(row.get('kind') == 'log' and row.get('spec', {}).get('event') == event
               for row in rows):
        raise ValueError('missing bounded-progress oracle event: ' + event)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--toolchain', type=Path, default=ROOT / '.pycircuit_out/toolchain/install')
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--case', choices=('product', 'tensor', 'layout', 'recovery', 'all'), default='all')
    args = parser.parse_args()
    output = ROOT / '.pycircuit_out/fa2-scheduler-gate'
    evidence = ROOT / 'docs/gates/logs/fa2-p0-completion/scheduler-gate' / time.strftime('%Y%m%dT%H%M%S')
    output.mkdir(parents=True, exist_ok=True)
    evidence.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env['PYC_TOOLCHAIN_ROOT'] = str(args.toolchain)
    env['PYTHONPATH'] = str(args.toolchain / 'share/pycircuit/python')
    env['LD_LIBRARY_PATH'] = str(args.toolchain / 'lib') + ':' + env.get('LD_LIBRARY_PATH', '')
    compiler = str(args.toolchain / 'bin/pycircuit')
    records = []

    def run(name, command):
        start = time.monotonic()
        with (evidence / (name + '.log')).open('w') as log:
            result = subprocess.run([str(x) for x in command], cwd=ROOT, env=env,
                                    stdout=log, stderr=subprocess.STDOUT)
        records.append({"stage": name, "command": [str(x) for x in command],
                        "exit_status": result.returncode, "elapsed_seconds": time.monotonic() - start})
        (evidence / 'commands.json').write_text(json.dumps(records, indent=2) + '\n')
        if result.returncode:
            raise SystemExit(f'{name} failed ({result.returncode}); see {evidence / (name + ".log")}')
        return evidence / (name + '.log')

    sources = ROOT / 'designs/sparsefa/src'
    units = []
    identities = {}
    for stem in ('fa2_interfaces', 'fa2_numeric', 'fa2_engine', 'fa2_scheduler'):
        source = sources / (stem + '.py')
        identities[str(source.relative_to(ROOT))] = hashlib.sha256(source.read_bytes()).hexdigest()
        unit = output / (stem + '-unit')
        includes = [item for previous in units for item in ('-I', previous)]
        run('compile-' + stem, [compiler, 'compile', '-c', source, '--source-root', sources,
                               '--package-prefix', 'fa2', *includes, '-o', unit, '--replace'])
        units.append(unit)
    (evidence / 'source-hashes.json').write_text(json.dumps(identities, indent=2) + '\n')
    if args.case in ('product', 'all'):
        final = output / 'product-final'
        run('product-link', [compiler, 'link', *units, '--top',
                             'fa2.fa2_scheduler.FA2TensorScheduler', '-o', final, '--replace'])
        for target in ('cpp', 'verilog'):
            run('product-' + target + '-emit', [compiler, 'emit', final,
                                               '--target', target,
                                               '-o', output / ('product-' + target), '--replace'])
    cases = [('tensor', 'bench.py', 'TensorSchedulerSystem', 1200),
             ('layout', 'layout_bench.py', 'TensorLayoutSystem', 2),
             ('recovery', 'recovery_bench.py', 'TensorRecoverySystem', 500)]
    for case, filename, top, cycles in cases:
        if args.case not in ('all', case):
            continue
        source = Path(__file__).parent / filename
        identities[str(source.relative_to(ROOT))] = hashlib.sha256(source.read_bytes()).hexdigest()
        (evidence / 'source-hashes.json').write_text(json.dumps(identities, indent=2) + '\n')
        unit = output / (case + '-unit')
        includes = [item for previous in units for item in ('-I', previous)]
        run(case + '-compile', [compiler, 'compile', '-c', source, '--source-root', source.parent,
                               '--package-prefix', 'scheduler', *includes, '-o', unit, '--replace'])
        final = output / (case + '-final')
        run(case + '-link', [compiler, 'link', *units, unit, '--top',
                            'scheduler.' + source.stem + '.' + top, '-o', final, '--replace'])
        for target in ('cpp', 'verilog'):
            emitted = output / (case + '-' + target)
            build = output / (case + '-' + target + '-build')
            label = case + '-' + target
            run(label + '-emit', [compiler, 'emit', final, '--target', target, '-o', emitted, '--replace'])
            if target == 'verilog':
                records.append({'stage': label + '-execute', 'status': 'NOT RUN', 'reason': 'User explicitly excluded Verilog simulation'})
                (evidence / 'commands.json').write_text(json.dumps(records, indent=2) + '\n')
                continue
            run(label + '-configure', ['cmake', '-S', emitted, '-B', build,
                                       '-DCMAKE_PREFIX_PATH=' + str(args.toolchain)])
            run(label + '-build', ['cmake', '--build', build, '--target', 'pycircuit_sim', '-j', args.jobs])
            for workers in (1, 2):
                stage = label + '-execute-workers' + str(workers)
                log = run(stage, [build / 'bin/pycircuit_sim', '--cycles', cycles, '--workers', workers])
                event = {'tensor': 'tensor_scheduler_pass', 'layout': 'tensor_layout_pass',
                         'recovery': 'tensor_recovery_pass'}[case]
                failure = None
                try:
                    validate_transcript(log, cycles, event)
                except (ValueError, TypeError, AttributeError) as error:
                    failure = str(error)
                records.append({'stage': stage + '-oracle', 'status': 'FAIL' if failure else 'PASS',
                                'expected_epoch_time': 2 * cycles, 'required_event': event,
                                'failure': failure})
                (evidence / 'commands.json').write_text(json.dumps(records, indent=2) + '\n')
                if failure:
                    raise SystemExit(f'{stage}: {failure}; see {log}')
    artifacts = {}
    for artifact in output.rglob('*'):
        if artifact.is_file() and (artifact.name in ('unit.json', 'generated.json', 'pycircuit_sim') or artifact.suffix == '.mlir'):
            artifacts[str(artifact.relative_to(ROOT))] = hashlib.sha256(artifact.read_bytes()).hexdigest()
    (evidence / 'artifact-hashes.json').write_text(json.dumps(artifacts, indent=2) + '\n')
    print(f'Scheduler generated-System checks passed; evidence: {evidence}')  # noqa: T201


if __name__ == '__main__':
    main()
