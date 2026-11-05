"""Compile and execute independently authored numeric Systems from this checkout."""
import json
import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
INSTALL = Path(os.environ.get('PYC_TOOLCHAIN_ROOT', ROOT / '.pycircuit_out/toolchain/install')).resolve()
OUT = ROOT / '.pycircuit_out/fa2-numeric-current'
LOG = ROOT / 'docs/gates/logs/fa2-p0-completion'
ENV = dict(os.environ, LD_LIBRARY_PATH=f'/home/xiekunpeng/micromamba/envs/acir/lib:{INSTALL}/lib', PYTHONPATH=str(INSTALL / 'share/pycircuit/python'))
DRIVER = str(INSTALL / 'bin/pycircuit')
COMPILER = '/home/xiekunpeng/micromamba/envs/acir/bin/x86_64-conda-linux-gnu-'
OUT.mkdir(parents=True, exist_ok=True)
LOG.mkdir(parents=True, exist_ok=True)

def run(name, args):
    with (LOG / f'numeric-{name}.log').open('w') as stream:
        process = subprocess.run([str(x) for x in args], env=ENV, stdout=stream, stderr=subprocess.STDOUT)
    with (LOG / 'numeric-commands.jsonl').open('a') as stream:
        stream.write(json.dumps({"command": [str(x) for x in args], "exit": process.returncode})+'\n')
    if process.returncode:
        raise SystemExit(f'{name}: exit {process.returncode}; see {LOG}')

run('compile', [DRIVER, 'compile', '-c', ROOT/'designs/sparsefa/src/fa2_numeric.py', '--source-root', ROOT/'designs/sparsefa/src', '--package-prefix', 'fa2', '-o', OUT/'unit', '--replace'])
for name, top, cycles in [('bench', 'NumericSystem', 32), ('exp_sweep', 'ExpSweep', 4096), ('edges', 'Edges', 1)]:
    run(name+'-compile', [DRIVER, 'compile', '-c', Path(__file__).with_name(name+'.py'), '--source-root', Path(__file__).parent, '--package-prefix', 'numeric', '-I', OUT/'unit', '-o', OUT/(name+'-unit'), '--replace'])
    final = OUT/(name+'.ac')
    run(name+'-link', [DRIVER, 'link', OUT/'unit', OUT/(name+'-unit'), '--top', f'numeric.{name}.{top}', '-o', final, '--replace'])
    run(name+'-emit', [DRIVER, 'emit', final, '--target', 'cpp', '-o', OUT/(name+'-cpp'), '--replace'])
    run(name+'-configure', ['cmake', '-S', OUT/(name+'-cpp'), '-B', OUT/(name+'-build'), f'-DCMAKE_PREFIX_PATH={INSTALL}', '-DCMAKE_CXX_COMPILER='+COMPILER+'g++', '-DCMAKE_C_COMPILER='+COMPILER+'gcc'])
    run(name+'-build', ['cmake', '--build', OUT/(name+'-build'), '-j2'])
    for workers in (1, 2):
        run(name+f'-workers{workers}', [OUT/(name+'-build/bin/pycircuit_sim'), '--cycles', cycles, '--workers', workers])
    if name == 'exp_sweep':
        from check_sweep import check
        for workers in (1, 2):
            result = check(LOG / f'numeric-exp_sweep-workers{workers}.log')
            (LOG / f'numeric-exp_sweep-workers{workers}-oracle.json').write_text(json.dumps(result, indent=2)+'\n')
    run(name+'-rtl-emit', [DRIVER, 'emit', final, '--target', 'verilog', '-o', OUT/(name+'-rtl'), '--replace'])
print('Numeric Systems passed C++ workers 1/2; Verilog emitted; simulation excluded by user.')  # noqa: T201 -- command-line validation summary.
