"""Field wiring and bounded conditional computation, independent of a CPU."""
import unittest
import json
import re
import subprocess
from pycircuit.tests.support import ExecutionChecks, check_cpp


def conditional_cycle_source(depth=4):
    source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        denominator = ac.cell[ac.u32](initial=2)
        zero = ac.cell[ac.u32](initial=0)
        enable = ac.cell[bool](initial=True)
        choice = ac.cell[bool](initial=True)
        guard = ac.cell[bool](initial=False)
        out = ac.cell[bool](initial=False)
        other = ac.cell[bool](initial=False)
    @ac.rule
    def first(value: bool) -> None:
        out = value
    @ac.rule
    def second(p: bool, q: bool) -> None:
        other = p or q
    @ac.rule
    def tick() -> None:
        choice = not choice
    def __call__() -> None:
        p = (20 // denominator) != 0
        q = p and enable
        r = (p and q) if choice else (q and p)
'''
    source += '        r = (r and q) if choice else (q and r)\n'*depth
    return source + '''        poison = (1 // zero) != 0
        first(poison if guard else r)
        second(p, q)
        tick()
'''


def shared_control_source(depth=4):
    branches = ''
    for i in range(depth):
        branches += f'        if (control.target & {1 << (i % 8)}) != 0:\n'
        branches += f'            value = value + {i+1}\n        else:\n            value = value ^ {i+3}\n'
    return '''from pycircuit import ac
class Control:
    stall: bool
    ecall_stall: bool
    stop: bool
    target: ac.u32
@ac.module
class Source:
    def __init__() -> None:
        phase = ac.cell[ac.u32](initial=0)
        signal = ac.signal[ac.u32]()
    @ac.rule
    def publish() -> None:
        if (phase & 1) == 0:
            signal = phase + 1
    @ac.rule
    def inspect() -> ac.u32:
        return signal
    @ac.rule
    def tick() -> None:
        phase = phase + 1
@ac.module
class Gate:
    def __init__() -> None:
        pass
    @ac.rule
    def advance(control: Control) -> ac.cell[ac.u32]:
        if control.stop:
            return 0
        if control.stall or control.ecall_stall:
            return None
        value = control.target
''' + branches + '''        return value
    def __call__(control: Control) -> ac.cell[ac.u32]:
        return advance(control)
@ac.module
class Top:
    def __init__() -> None:
        source = Source()
        gate = Gate()
        output = ac.cell[ac.u32](initial=0)
        saved = ac.cell[Control](initial=Control(False, False, False, 0))
    @ac.rule
    def observe(control: Control) -> None:
        saved = control
    def __call__() -> None:
        source.publish()
        word = source.inspect()
        control = Control((word & 2) != 0, (word & 4) != 0, (word & 8) != 0, 100 // (word | 1))
        output = gate(control)
        observe(control)
        source.tick()
'''


class FieldProjectionTests(ExecutionChecks, unittest.TestCase):
    def assert_cpp_assertion(self, out):
        from pycircuit.tests.support import cpp_compiler
        executable = out/'cpp/failure'
        built = subprocess.run(cpp_compiler()+['-std=c++20', '-O1',
            '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
            str(out/'cpp/sim.cpp'), '-o', str(executable)], capture_output=True, text=True)
        self.assertEqual(built.returncode, 0, built.stderr)
        result = subprocess.run([str(executable), '--cycles', '1'], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('assertion failed', result.stderr)
        self.assertNotIn('AddressSanitizer', result.stderr)
        self.assertNotIn('runtime error:', result.stderr)

    def test_standalone_projection_preserves_assertion_in_record_data(self):
        from pycircuit import analyze, compile_source
        from pycircuit.ir import CompileError, load
        source = '''from pycircuit import ac
class Bundle:
    flag: bool
    unused: bool
@ac.module
class Top:
    def __init__() -> None:
        out = ac.cell[bool](initial=False)
    @ac.rule
    def step() -> None:
        bundle = Bundle(True, False)
        out = bundle.flag
    def __call__() -> None:
        step()
'''
        path = self.directory/'source.py'; path.write_text(source)
        base = self.directory/'base'
        analyze(compile_source(path, 'Top'), base, optimize=False)
        ir = load(base/'model.transaction.mlir')
        match = re.search(r'(?m)^(\s*)(%\w+) = acir.aggregate\(%true, %false\)(.*)$', ir)
        self.assertIsNotNone(match)
        record_type = re.search(r'!acir.struct<"[^"]+">', match[3])[0]
        ir = ir[:match.start()] + match[1]+'%required_assertion = acir.assert %false\n' + match[0].replace('%true, %false', '%true, %required_assertion') + ir[match.end():]
        ir = re.sub(r'(?m)^(\s*)acir.effect (\S+)\(%true\)',
            lambda m: m[1]+'%required_field = acir.extract '+match[2]+' {field = "flag"} : ('+record_type+') -> i1\n'+m[1]+'acir.effect '+m[2]+'(%required_field)', ir, count=1)
        for optimize in (False, True):
            out = self.directory/('assertion-'+str(optimize))
            with self.assertRaisesRegex(CompileError, 'assertion'):
                analyze(ir, out, optimize=optimize, emit_cpp=True, cycles=1)
            self.assert_cpp_assertion(out)
    def test_projection_cannot_hide_effectful_producer(self):
        source = '''from pycircuit import ac
class Bundle:
    value: ac.u32
@ac.module
class Top:
    def __init__() -> None:
        count = ac.cell[ac.u32](initial=0)
        seen = ac.cell[ac.u32](initial=0)
    @ac.rule
    def produce() -> Bundle:
        count = count + 1
        return Bundle(7)
    @ac.rule
    def consume(value: ac.u32) -> None:
        seen = value
    def __call__() -> None:
        bundle = produce()
        consume(bundle.value)
'''
        from pycircuit.ir import CompileError
        for optimize in (False, True):
            with self.assertRaisesRegex(CompileError, 'ordinary return value crosses'):
                self.run_source(source, optimize=optimize)

    def test_conditional_cycle_uses_bounded_lazy_graph(self):
        sizes = []
        for depth in (4, 12):
            trace, _, out = self.run_source(conditional_cycle_source(depth), cycles=4, optimize=False)
            self.assertTrue(all(row['state']['top.out'] for row in trace))
            stats = json.loads((out/'cpp/lowering.json').read_text())
            self.assertGreater(stats['conditional_cycles'] + stats['demand_limits'], 0)
            if depth == 4:
                self.assertGreater(stats['conditional_cycles'], 0)
            self.assertGreater(stats['local_memoized_blocks'], 0)
            sizes.append(len((out/'cpp/model.hpp').read_text()))
            if depth == 4:
                self.assert_reloads(trace, out,)
                check_cpp(trace, out, sanitize=True, reset=True)
        self.assertLess(sizes[1], sizes[0]*4)

    def test_shared_control_and_bounded_branch_growth(self):
        sizes = []
        for depth in (4, 16):
            trace, _, out = self.run_source(shared_control_source(depth), cycles=24)
            previous = 0
            for phase, row in enumerate(trace):
                if phase % 2 == 0:
                    word = phase + 1
                    if word & 8:
                        previous = 0
                    elif not (word & 6):
                        previous = 100 // word
                        for i in range(depth):
                            previous = previous + i+1 if (100 // word) & (1 << (i % 8)) else previous ^ (i+3)
                self.assertEqual(row['state']['top.output'], previous)
            sizes.append(len((out/'cpp/model.hpp').read_text()))
            if depth == 4:
                self.assert_reloads(trace, out)
                check_cpp(trace, out, sanitize=True, reset=True)
        self.assertLess(sizes[1], sizes[0]*5)

    def test_field_argument_does_not_inherit_unused_signal_readiness(self):
        source = '''from pycircuit import ac
class Bundle:
    enable: bool
    unused: ac.u32
@ac.module
class Top:
    def __init__() -> None:
        missing = ac.signal[ac.u32]()
        count = ac.cell[ac.u32](initial=0)
    @ac.rule
    def inspect() -> ac.u32:
        return missing
    @ac.rule
    def consume(enable: bool) -> None:
        if enable:
            count = count + 1
    def __call__() -> None:
        bundle = Bundle(True, inspect())
        consume(bundle.enable)
'''
        for branching in (False, True):
            case = source.replace('        return missing\n',
                '        if missing == 0:\n            return 7\n        return 9\n') if branching else source
            for optimize in (False, True):
                trace, _, out = self.run_source(case, cycles=4, optimize=optimize)
                self.assertEqual(trace[-1]['state']['top.count'], 4)
                self.assert_reloads(trace, out)
        guarded = source.replace('    @ac.rule\n    def inspect()',
            '    @ac.rule\n    def ready() -> bool:\n        return False\n'
            '    @ac.rule(guard=ready)\n    def inspect()')
        for optimize in (False, True):
            trace, _, _ = self.run_source(guarded, cycles=4, optimize=optimize)
            self.assertEqual(trace[-1]['state']['top.count'], 0)

    def test_field_projection_preserves_called_rule_assertion(self):
        source = '''from pycircuit import ac
class Bundle:
    enable: bool
    unused: ac.u32
@ac.module
class Top:
    def __init__() -> None:
        flag = ac.cell[bool](initial=False)
        count = ac.cell[ac.u32](initial=0)
    @ac.rule
    def make() -> Bundle:
        assert flag
        return Bundle(True, 7)
    @ac.rule
    def consume(enable: bool) -> None:
        if enable:
            count = count + 1
    def __call__() -> None:
        bundle = make()
        consume(bundle.enable)
'''
        from pycircuit import analyze, compile_source
        from pycircuit.ir import CompileError
        path = self.directory/'source.py'; path.write_text(source)
        for optimize in (False, True):
            with self.assertRaisesRegex(CompileError, 'assertion'):
                analyze(compile_source(path, 'Top'), self.directory/str(optimize),
                        cycles=1, emit_cpp=True, optimize=optimize)
            self.assert_cpp_assertion(self.directory/str(optimize))

    def test_nested_projection_selection_and_static_update(self):
        source = '''from pycircuit import ac
class Inner:
    enable: bool
    unused: ac.u32
class Outer:
    inner: Inner
    other: ac.u32
@ac.module
class Top:
    def __init__() -> None:
        missing = ac.signal[ac.u32]()
        choice = ac.cell[bool](initial=False)
        count = ac.cell[ac.u32](initial=0)
    @ac.rule
    def inspect() -> ac.u32:
        return missing
    @ac.rule
    def consume(enable: bool) -> None:
        if enable:
            count = count + 1
    @ac.rule
    def tick() -> None:
        choice = not choice
    def __call__() -> None:
        bundle = Outer(Inner(True, inspect()), 7)
        if choice:
            bundle = Outer(Inner(False, inspect()), 9)
        bundle.inner.enable = not choice
        consume(bundle.inner.enable)
        tick()
'''
        for optimize in (False, True):
            trace, _, out = self.run_source(source, cycles=6, optimize=optimize)
            self.assertEqual([r['state']['top.count'] for r in trace], [1,1,2,2,3,3])
            self.assert_reloads(trace, out)

    def test_unused_field_cannot_discard_an_explicit_assertion(self):
        source = '''from pycircuit import ac
class Bundle:
    enable: bool
    unused: ac.u32
@ac.module
class Top:
    def __init__() -> None:
        flag = ac.cell[bool](initial=False)
        count = ac.cell[ac.u32](initial=0)
    @ac.rule
    def checked() -> ac.u32:
        assert flag
        return 7
    @ac.rule
    def consume(enable: bool) -> None:
        if enable:
            count = count + 1
    def __call__() -> None:
        bundle = Bundle(True, checked())
        consume(bundle.enable)
'''
        from pycircuit import analyze, compile_source
        from pycircuit.ir import CompileError
        path = self.directory/'source.py'; path.write_text(source)
        for optimize in (False, True):
            with self.assertRaisesRegex(CompileError, 'assertion'):
                analyze(compile_source(path, 'Top'), self.directory/str(optimize),
                        cycles=1, emit_cpp=True, optimize=optimize)
            self.assert_cpp_assertion(self.directory/str(optimize))


if __name__ == '__main__':
    unittest.main()
