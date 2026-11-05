"""Run the branch/guard four-state oracle through both emitted targets (C++ only execution)."""
import hashlib
import json
import os
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
SOURCE = Path(__file__).parent
TOOLCHAIN = ROOT / '.pycircuit_out/toolchain/install'
RUN_ID = time.strftime('%Y%m%dT%H%M%S') + f'-{time.time_ns() % 1000000000:09d}'
OUTPUT = ROOT / '.pycircuit_out/fa2-guard-order' / RUN_ID
EVIDENCE = ROOT / 'docs/gates/logs/fa2-p0-completion/guard-order' / RUN_ID


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    EVIDENCE.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, PYC_TOOLCHAIN_ROOT=str(TOOLCHAIN),
               PYTHONPATH=str(TOOLCHAIN / 'share/pycircuit/python'))
    env['LD_LIBRARY_PATH'] = str(TOOLCHAIN / 'lib') + ':' + env.get('LD_LIBRARY_PATH', '')
    compiler = TOOLCHAIN / 'bin/pycircuit'
    records = []
    tools = [TOOLCHAIN / 'bin' / name for name in
             ('pycircuit', 'pycircuit-source-unit', 'pycircuit-link', 'pycircuit-emit')]

    def hashes(paths):
        return {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in paths}

    before_tools = hashes(tools)
    (EVIDENCE / 'tool-hashes-before.json').write_text(json.dumps(before_tools, indent=2) + '\n')
    (EVIDENCE / 'result.json').write_text(json.dumps({'status': 'RUNNING'}) + '\n')

    def fail(reason):
        (EVIDENCE / 'result.json').write_text(json.dumps({'status': 'FAIL', 'reason': reason}) + '\n')
        raise SystemExit(reason)

    def run(label, command):
        result = subprocess.run(list(map(str, command)), cwd=ROOT, env=env,
                                text=True, capture_output=True)
        (EVIDENCE / (label + '.log')).write_text(result.stdout + result.stderr)
        records.append({'stage': label, 'command': list(map(str, command)),
                        'exit_status': result.returncode})
        (EVIDENCE / 'commands.json').write_text(json.dumps(records, indent=2) + '\n')
        if result.returncode:
            fail(f'{label} failed; see {EVIDENCE / (label + ".log")}')
        return result.stdout

    unit = OUTPUT / 'unit'
    run('compile', [compiler, 'compile', '-c', SOURCE / 'guard_order.py',
                    '--source-root', SOURCE, '--package-prefix', 'guard_order',
                    '-o', unit, '--replace'])
    expected = ''.join(f'{outer} {inner} {int(outer >= 2 or (outer == 1 and inner >= 2))}\n'
                       for outer in range(4) for inner in range(4))
    for name in ('Original', 'Nested'):
        final = OUTPUT / (name + '-final')
        run(name + '-link', [compiler, 'link', unit, '--top',
                            'guard_order.guard_order.' + name, '-o', final, '--replace'])
        for target in ('cpp', 'verilog'):
            run(name + '-' + target, [compiler, 'emit', final, '--target', target,
                                      '-o', OUTPUT / (name + '-' + target), '--replace'])
        cpp = OUTPUT / (name + '-cpp')
        receipt = json.loads((cpp / 'generated.json').read_text())
        sources = [cpp / row['path'] for row in receipt['files'] if row['path'].endswith('.cpp')]
        binary = OUTPUT / (name + '-native')
        run(name + '-build', [os.environ.get('CXX', 'c++'), '-std=c++20', '-pthread',
                             '-I' + str(TOOLCHAIN / 'include'), '-I' + str(cpp),
                             SOURCE / 'guard_order.cpp', *sources,
                             TOOLCHAIN / 'lib/libpyc6_runtime.a', '-o', binary])
        for workers in (1, 2):
            output = run(name + '-workers' + str(workers), [binary, workers])
            if output != expected:
                fail(f'{name} workers={workers}: independent truth table mismatch')
    inputs = [SOURCE / 'guard_order.py', SOURCE / 'guard_order.cpp', Path(__file__),
              ROOT / 'compiler/lib/Transforms/LowerTransactions.cpp',
              ROOT / 'compiler/lib/Dialect/ACIR/ACIRTransactionOps.cpp',
              ROOT / 'compiler/lib/Dialect/ACIR/HardwareSourceCheckPreflight.cpp']
    artifacts = [path for path in OUTPUT.rglob('*') if path.is_file()]
    (EVIDENCE / 'hashes.json').write_text(json.dumps(
        {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
         for path in inputs + artifacts}, indent=2) + '\n')
    after_tools = hashes(tools)
    (EVIDENCE / 'tool-hashes-after.json').write_text(json.dumps(after_tools, indent=2) + '\n')
    if before_tools != after_tools:
        fail('Installed compiler tools changed during validation')
    (EVIDENCE / 'result.json').write_text(json.dumps({
        'status': 'PASS', 'variants': ['Original', 'Nested'], 'workers': [1, 2],
        'rows_per_run': 16, 'checks': ['independent reached-guard truth table',
                                       'zero commit and cycle advance on failure',
                                       'held/written payload and unrelated heartbeat'],
        'verilog_simulation': 'NOT RUN: excluded by user',
    }, indent=2) + '\n')
    print(f'Guard ordering passed: {EVIDENCE}')  # noqa: T201


if __name__ == '__main__':
    main()
