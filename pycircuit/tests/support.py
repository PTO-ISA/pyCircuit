"""Shared compile/execute checks; this module defines no test cases."""
import json
from pathlib import Path
import tempfile
from pycircuit import analyze, compile_source
from pycircuit.ir import load

ROOT = Path(__file__).resolve().parents[2]
EXAMPLES = ROOT / 'pycircuit/examples'


def read_trace(path):
    return json.loads(path.read_text())['trace']


def check_reloads(trace, out, optimize=True):
    for stage in ('source', 'transaction', 'rtl'):
        destination = out / ('reload-' + stage)
        analyze(load(out / f'model.{stage}.mlir'), destination,
                cycles=len(trace), optimize=optimize)
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
                        config=config, optimize=optimize)
        trace = read_trace(out / 'transaction-trace.json')
        self.assertEqual(trace, read_trace(out / 'rtl-trace.json'))
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
