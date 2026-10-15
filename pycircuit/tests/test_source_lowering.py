"""Source normalization remains independent of either execution backend."""
import os
import re
import subprocess
import unittest

from pycircuit import analyze, compile_source
from pycircuit.ir import CompileError, load
from pycircuit.tests.support import ExecutionChecks, check_cpp, cpp_compiler, read_trace


class SourceLoweringTests(ExecutionChecks, unittest.TestCase):
    def test_structured_source_standalone_native_rtl_pipeline(self):
        source = '''from pycircuit import ac
def initial(flag: bool) -> ac.u32:
    return 2 if flag else 3
def calculate(value: ac.u32, choose: bool) -> ac.u32:
    candidate = value + 1
    if choose:
        assert value < 20
        candidate = candidate + 2
    else:
        candidate = candidate + 4
    return candidate
@ac.module
class Top:
    def __init__(start: bool = False) -> None:
        count = ac.cell[ac.u32](initial=initial(start))
        choose = ac.cell[bool](initial=False)
    @ac.rule
    def step() -> None:
        count = calculate(count, choose)
        choose = not choose
    def __call__() -> None:
        if False:
            step()
        step()
'''
        previous = None
        for optimize in (False, True):
            trace, graph, out = self.run_source(source, cycles=4, optimize=optimize)
            self.assertEqual(len(graph['nodes']), 2)
            if previous is not None:
                self.assertEqual(previous, trace)
            previous = trace
            ir = load(out / 'model.source.mlir')
            self.assertIn('scf.if', ir)
            self.assertIn('func.call', ir)
            self.assertIn('acir.module @top', ir)
            for lowered in ('acir.circuit', 'acir.read(', 'acir.effect', 'acir.current'):
                self.assertNotIn(lowered, ir)
            native = subprocess.run(
                [os.environ['ACPY_MLIR_OPT'], str(out / 'model.source.mlir'),
                 '--acir-source-opt', '--acir-extract-transactions',
                 '--acir-cleanup', '--acir-lower-rtl', '--acir-cleanup'],
                capture_output=True, text=True)
            self.assertEqual(native.returncode, 0, native.stderr)
            destination = out / 'native-rtl'
            analyze(native.stdout, destination, cycles=4)
            self.assertEqual(read_trace(destination / 'rtl-trace.json'), trace)
            self.assertNotIn('scf.if', native.stdout)
            self.assert_reloads(trace, out)

    def test_zero_result_branch_keeps_helper_assertion(self):
        source = '''from pycircuit import ac
def checked(value: ac.u32, choose: bool) -> ac.u32:
    if choose:
        assert value != 0
    return 7
@ac.module
class Top:
    def __init__() -> None:
        zero = ac.cell[ac.u32](initial=0)
        choose = ac.cell[bool](initial=CHOOSE)
        result = ac.cell[ac.u32](initial=0)
    @ac.rule
    def put(value: ac.u32) -> None:
        result = value
    def __call__() -> None:
        value = checked(zero, choose)
        put(value)
'''
        for optimize in (False, True):
            trace, _, out = self.run_source(source.replace('CHOOSE', 'False'),
                                            optimize=optimize)
            self.assertEqual(trace[-1]['state']['top.result'], 7)
            check_cpp(trace, out, sanitize=True, reset=True)
            with self.assertRaisesRegex(CompileError, 'assertion failed'):
                self.run_source(source.replace('CHOOSE', 'True'), optimize=optimize)

    def test_shared_assertion_runs_even_when_later_capacity_blocks_fire(self):
        source = '''from pycircuit import ac
def checked(value: ac.u32) -> ac.u32:
    assert value != 0
    return 7
@ac.module
class Top:
    def __init__() -> None:
        zero = ac.cell[ac.u32](initial=0)
        output = ac.queue[ac.u32](capacity=1, initial=[9])
    @ac.rule
    def put(value: ac.u32) -> ac.queue[ac.u32]:
        return value
    def __call__() -> None:
        value = checked(zero)
        output = put(value)
'''
        path = self.directory / 'checked.py'
        path.write_text(source)
        model = compile_source(path, 'Top')
        for optimize in (False, True):
            out = self.directory / str(optimize)
            analyze(model, out, optimize=optimize, emit_cpp=True)
            if optimize:
                for stage in ('transaction', 'rtl'):
                    self.assertIn('checks(', load(out / f'model.{stage}.mlir'))
                invalid = load(out / 'model.transaction.mlir').replace(
                    '\n', '\n  %bad_check = arith.constant false\n', 1)
                invalid = re.sub(r'checks\([^)]*\)', 'checks(%bad_check)', invalid, count=1)
                with self.assertRaisesRegex(CompileError, 'return true or assert'):
                    analyze(invalid, out / 'invalid-check', optimize=False)
            cpp = out / 'cpp'
            executable = cpp / 'failure'
            subprocess.run(cpp_compiler() + ['-std=c++20', '-O1', str(cpp / 'sim.cpp'),
                                             '-o', str(executable)], check=True)
            run = subprocess.run([str(executable), '--cycles', '1'], capture_output=True, text=True)
            self.assertNotEqual(run.returncode, 0)
            self.assertIn('assertion failed', run.stderr)
            for stage in ('source', 'transaction', 'rtl'):
                with self.assertRaisesRegex(CompileError, 'assertion failed'):
                    analyze(load(out / f'model.{stage}.mlir'), out / stage,
                            cycles=1, optimize=optimize)
