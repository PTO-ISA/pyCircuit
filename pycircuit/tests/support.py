"""Shared compile/execute checks; this module defines no test cases."""
import json
import re
import os
import shlex
import subprocess
import time
from pathlib import Path
import tempfile
from pycircuit import analyze, compile_source
from pycircuit.ir import load

ROOT = Path(__file__).resolve().parents[2]
EXAMPLES = ROOT / 'pycircuit/examples'


def read_trace(path):
    return json.loads(path.read_text())['trace']


def cpp_compiler():
    default = '/home/lc/opt/pycircuit-dev/bin/c++'
    return shlex.split(os.environ.get('CXX', default if Path(default).exists() else 'c++'))


def check_cpp(trace, out, *, sanitize=False, reset=False, benchmark_cycles=None):
    if benchmark_cycles is None:
        benchmark_cycles = len(trace)
    cpp = out / 'cpp'
    executable = cpp / ('sim-sanitized' if sanitize else 'sim')
    flags = ['-std=c++20', '-O1']
    if sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                  '-fno-omit-frame-pointer', '-g']
    source = cpp / 'sim.cpp'
    if reset:
        source = cpp / 'reset.cpp'
        source.write_text('#include "model.hpp"\n#include <iostream>\n'
                          '#include <memory>\nint main() {\n'
                          'auto m=std::make_unique<acir_generated::Model>();\n'
                          f'for(unsigned i=0;i<{len(trace)};++i) m->tick();\n'
                          'm->reset(); std::cout<<"{\\\"trace\\\":[";\n'
                          f'for(unsigned i=0;i<{len(trace)};++i) {{ m->tick(); '
                          'if(i)std::cout<<","; m->trace(std::cout,i); }\n'
                          'std::cout<<"]}"; }\n')
    started = time.perf_counter()
    built = subprocess.run(cpp_compiler() + flags + [str(source), '-o', str(executable)],
                           capture_output=True, text=True)
    if built.returncode:
        raise AssertionError(built.stderr)
    compile_seconds = time.perf_counter() - started
    result = cpp / ('sanitized-trace.json' if sanitize else 'trace.json')
    if reset:
        with result.open('w') as output:
            subprocess.run([str(executable)], check=True, stdout=output)
    else:
        subprocess.run([str(executable), '--cycles', str(len(trace)), '--trace', str(result)], check=True)
    assert read_trace(result) == trace, f'C++ differs: {out}'
    run_seconds = None
    if not reset:
        started = time.perf_counter()
        subprocess.run([str(executable), '--cycles', str(benchmark_cycles)], check=True)
        run_seconds = time.perf_counter() - started
    header = (cpp / 'model.hpp').read_text()
    return {'cache_count': len(re.findall(r'^.*\b(?:c\d+|\w+_cache)(?:\{\})?;\s*$', header, re.MULTILINE)),
            'code_lines': sum(len((cpp / f).read_text().splitlines()) for f in ('model.hpp', 'runtime.hpp', 'sim.cpp')),
            'code_bytes': sum((cpp / f).stat().st_size for f in ('model.hpp', 'runtime.hpp', 'sim.cpp')),
            'compile_seconds': compile_seconds, 'untraced_cycles': benchmark_cycles if not reset else None,
            'untraced_seconds': run_seconds, 'sanitized': sanitize, 'reset': reset}


def check_reloads(trace, out, optimize=True):
    for stage in ('source', 'transaction', 'rtl'):
        destination = out / ('reload-' + stage)
        analyze(load(out / f'model.{stage}.mlir'), destination,
                cycles=len(trace), optimize=optimize, emit_cpp=stage != "rtl")
        if stage != "rtl":
            check_cpp(trace, destination)
        for filename in ('transaction-trace.json', 'rtl-trace.json'):
            path = destination / filename
            if path.exists():
                assert read_trace(path) == trace, f'{stage}: {filename}'


class ExecutionChecks:
    def setUp(self):
        temporary = ROOT / 'pycircuit/builds/benchmarks/test-runs'
        temporary.mkdir(parents=True, exist_ok=True)
        self.tmp = tempfile.TemporaryDirectory(dir=temporary)
        self.directory = Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()

    def run_model(self, path, top, out, cycles, config, optimize):
        graph = analyze(compile_source(path, top), out, cycles=cycles,
                        config=config, optimize=optimize, emit_cpp=True)
        trace = read_trace(out / 'transaction-trace.json')
        self.assertEqual(trace, read_trace(out / 'rtl-trace.json'))
        check_cpp(trace, out)
        self.assertFalse((out / 'program.json').exists())
        ir = load(out / 'model.rtl.mlir')
        for forbidden in ('acir.rule.call', 'acir.array.', 'acir.update', 'acir.read ',
                          'acir.queue.', 'acir.signal.', 'acir.ehr.', 'scf.if', 'cf.cond_br'):
            self.assertNotIn(forbidden, ir)
        return trace, graph, out

    def run_source(self, source, cycles=4, config=None, optimize=True):
        path = self.directory / 'source.py'
        path.write_text(source)
        return self.run_model(path, 'Top', self.directory / ('opt' if optimize else 'raw'),
                              cycles, config, optimize)

    def run_example(self, source, top, cycles=32, config=None, optimize=True):
        out = self.directory / (top + str(optimize) + str(config))
        trace, _, out = self.run_model(EXAMPLES / source, top, out, cycles, config, optimize)
        return trace, out

    def assert_reloads(self, trace, out):
        check_reloads(trace, out)
