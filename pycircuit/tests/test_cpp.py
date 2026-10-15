"""Static C++ backend contracts and independent edge cases."""
import unittest
from pycircuit.ir import CompileError
from pycircuit.tests.support import ExecutionChecks


class GraphRegressions(ExecutionChecks, unittest.TestCase):
    def test_effectful_return_cannot_control_module_branch(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        count = ac.cell[ac.u32](initial=0)
        result = ac.cell[ac.u32](initial=0)
    @ac.rule
    def produce() -> bool:
        count = count + 1
        return True
    @ac.rule
    def consume() -> None:
        result = 1
    def __call__() -> None:
        if produce():
            consume()
'''
        cases = [source]
        for condition in ('count == 0', 'count != 0'):
            cases.append(source.replace('if produce():',
                         f'value = False\n        if {condition}:\n'
                         '            value = produce()\n        if value:'))
        for case in cases:
            with self.assertRaisesRegex(CompileError, 'ordinary return value crosses'):
                self.run_source(case)

    def test_outer_sb_does_not_hide_inner_sbr(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        a = ac.cell[ac.u32](initial=0)
        b = ac.cell[ac.u32](initial=0)
    @ac.rule(SBR='inner_second')
    def inner_first() -> None:
        a = 1
    @ac.rule
    def inner_second() -> None:
        b = 2
    @ac.rule(SB='second')
    def first() -> None:
        inner_first()
    @ac.rule
    def second() -> None:
        inner_second()
    @ac.rule
    def outer() -> None:
        first()
        second()
    def __call__() -> None:
        outer()
'''
        with self.assertRaisesRegex(CompileError, 'forbids nested calls|incompatible operations'):
            self.run_source(source)

    def test_shared_constant_call_contract_orders_all_consumers(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        a = ac.cell[ac.u32](initial=0)
        b = ac.cell[ac.u32](initial=0)
        c = ac.cell[ac.u32](initial=0)
    @ac.rule(SB='shared')
    def early() -> None:
        a = 1
    @ac.rule(SB='late')
    def shared() -> ac.u32:
        return 7
    @ac.rule
    def late() -> None:
        a = 2
    @ac.rule
    def first(v: ac.u32) -> None:
        b = v
    @ac.rule
    def second(v: ac.u32) -> None:
        c = v
    def __call__() -> None:
        late()
        v = shared()
        first(v)
        second(v)
        early()
'''
        trace, graph, _ = self.run_source(source)
        self.assertEqual(graph['schedule'], [3, 1, 2, 0])
        self.assertEqual(trace[-1]['state'], {'top.a': 2, 'top.b': 7, 'top.c': 7})


class CppTests(ExecutionChecks, unittest.TestCase):
    def test_short_circuit_invalid_branch_and_reset(self):
        from pycircuit.tests.support import check_cpp
        source = '''from pycircuit import ac
def dangerous(v: ac.u32) -> ac.u32:
    assert v != 0
    return 10 // v
@ac.module
class Top:
    def __init__() -> None:
        choose = ac.cell[bool](initial=False)
        zero = ac.cell[ac.u32](initial=0)
        bad_index = ac.cell[ac.u32](initial=99)
        data = ac.cell[ac.array(ac.u32, shape=(2,))](initial=[4,5])
        a = ac.cell[ac.u32](initial=0)
        b = ac.cell[ac.u32](initial=0)
        c = ac.cell[bool](initial=False)
    @ac.rule
    def run() -> None:
        a = dangerous(zero) if choose else 7
        b = data[bad_index] if choose else 11
        c = (choose and (1 // zero == 0)) or (not choose)
    def __call__() -> None:
        run()
'''
        for optimize in (False, True):
            trace, _, out = self.run_source(source, cycles=32, optimize=optimize)
            self.assertEqual(trace[-1]['state']['top.a'], 7)
            self.assertEqual(trace[-1]['state']['top.b'], 11)
            self.assertTrue(trace[-1]['state']['top.c'])
            check_cpp(trace, out, sanitize=True, reset=True)

    def test_integer_widths_signed_operations_and_wrap(self):
        from pycircuit.tests.support import check_cpp
        for width in (8, 16, 32, 64):
            mask = (1 << width) - 1
            minimum = -(1 << (width - 1))
            source = f'''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        low = ac.cell[ac.i{width}](initial={minimum})
        minus = ac.cell[ac.i{width}](initial=-1)
        negative = ac.cell[ac.i{width}](initial=-7)
        three = ac.cell[ac.i{width}](initial=3)
        maximum = ac.cell[ac.u{width}](initial={mask})
        wrap = ac.cell[ac.u{width}](initial=0)
        product = ac.cell[ac.u{width}](initial=0)
        quotient = ac.cell[ac.i{width}](initial=0)
        floor = ac.cell[ac.i{width}](initial=0)
        remainder = ac.cell[ac.i{width}](initial=0)
        shift = ac.cell[ac.i{width}](initial=0)
        left = ac.cell[ac.u{width}](initial=0)
        comparison = ac.cell[bool](initial=False)
    @ac.rule
    def run() -> None:
        wrap = maximum + 1
        product = maximum * maximum
        quotient = low // minus
        floor = negative // three
        remainder = negative % three
        shift = negative >> 1
        left = maximum << {width-1}
        comparison = (low < minus) and (minus >= low) and (negative <= minus) and (three > negative)
    def __call__() -> None:
        run()
'''
            trace, _, out = self.run_source(source, cycles=3)
            state = trace[-1]['state']
            for name, value in {'wrap': 0, 'product': 1, 'quotient': -minimum,
                                'floor': (-3 & mask), 'remainder': (-1 & mask),
                                'shift': (-4 & mask), 'left': -minimum,
                                'comparison': True}.items():
                self.assertEqual(state['top.' + name], value, (width, name))
            check_cpp(trace, out, sanitize=True, reset=True)

    def test_input_stage_errors_and_unsupported_operation(self):
        from pycircuit import analyze, compile_source
        from pycircuit.ir import load
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        count = ac.cell[ac.u32](initial=0)
    @ac.rule
    def run() -> None:
        count = count + 1
    def __call__() -> None:
        run()
'''
        _, _, out = self.run_source(source, cycles=1)
        model = compile_source(self.directory / 'source.py', 'Top')
        with self.assertRaisesRegex(CompileError, 'stop-after source'):
            analyze(model, self.directory / 'invalid-source', stop_after='source', emit_cpp=True)
        with self.assertRaisesRegex(CompileError, 'not RTL'):
            analyze(load(out / 'model.rtl.mlir'), self.directory / 'invalid-rtl', emit_cpp=True)
        unsupported = load(out / 'model.transaction.mlir').replace('arith.addi', 'arith.maxui')
        with self.assertRaisesRegex(CompileError, 'unsupported operation arith.maxui'):
            analyze(unsupported, self.directory / 'unsupported', stop_after='transaction', emit_cpp=True)

    def test_standalone_mlir_i1_uses_bit_truncation(self):
        from pycircuit import analyze
        from pycircuit.tests.support import check_cpp, read_trace
        model = '''module attributes {acir.model = {format_version = 9 : i64,
          config = [], types = {}, type_order = [], constants = {}, top = "Top"},
          acir.stage = "transaction"} {
          %two = arith.constant 2 : i8
          %true = arith.constant true
          acir.circuit attributes {stage = "transaction", schedule = [0], edges = [], conflicts = []} {
            acir.state @top.result : i1 {kind = "cell", initial = true}
            %truncated = arith.trunci %two : i8 to i1
            %sum = arith.addi %true, %true : i1
            %result = arith.ori %truncated, %sum : i1
            %fire = acir.fire "top.run#0" %true {txn = 0 : i64, successors = []}
            acir.effect %fire(%result) {txn = 0 : i64, port = 0 : i64,
              resource = "top.result", method = "write"} : (i1, i1) -> ()
          }
        }'''
        out = self.directory / 'bits'
        analyze(model, out, cycles=3, optimize=False, emit_cpp=True)
        trace = read_trace(out / 'transaction-trace.json')
        self.assertEqual(trace, read_trace(out / 'rtl-trace.json'))
        self.assertFalse(trace[-1]['state']['top.result'])
        check_cpp(trace, out, sanitize=True, reset=True)

    def test_selected_array_update_retains_domain(self):
        source = '''from pycircuit import ac
Data = ac.array(ac.u32, shape=(2,))
@ac.module
class Top:
    def __init__() -> None:
        data = ac.cell[Data](initial=[1,2])
        result = ac.cell[ac.u32](initial=0)
        count = ac.cell[ac.u32](initial=0)
    @ac.rule
    def run() -> None:
        values = data
        view = values[values.match(lambda v: v > 1)]
        view[1] = 9
        result = view[1]
        count = count + 1
    def __call__() -> None:
        run()
'''
        for optimize in (False, True):
            trace, _, _ = self.run_source(source, cycles=3, optimize=optimize)
            self.assertEqual(trace[-1]['state']['top.result'], 9)
            self.assertEqual(trace[-1]['state']['top.data'], [1, 2])
            self.assertEqual(trace[-1]['state']['top.count'], 3)
            cancelled, _, _ = self.run_source(source.replace('result = view[1]',
                                                            'result = view[0]'),
                                               cycles=3, optimize=optimize)
            self.assertEqual(cancelled[-1]['state']['top.result'], 0)
            self.assertEqual(cancelled[-1]['state']['top.count'], 0)

class CppStructureTests(ExecutionChecks, unittest.TestCase):
    def test_source_names_scopes_branches_and_standalone_reload(self):
        from pycircuit import analyze
        from pycircuit.ir import load
        from pycircuit.tests.support import check_cpp
        source = '''from pycircuit import ac
class Instruction:
    rd: ac.u32
    template: ac.u32

def calculate(argument: ac.u32, choose: bool) -> ac.u32:
    candidate = argument + 1
    candidate_2 = argument + 3
    Context = argument + 4
    if choose:
        candidate = candidate + 2
        if argument > 3:
            candidate = candidate + 4
    else:
        candidate = candidate + 8
    return candidate + candidate_2 + Context
@ac.module
class Worker:
    def __init__() -> None:
        registers: ac.array(ac.cell[ac.u32], shape=(4,)) = [ac.cell[ac.u32](initial=0) for i in range(4)]
        cursor = ac.cell[ac.u32](initial=1)
        saved = ac.cell[Instruction](initial=Instruction(0, 0))
    @ac.rule
    def update() -> None:
        inst = Instruction(cursor, 3)
        saved = inst
        template = calculate(inst.template, True)
        ctx = calculate(inst.rd, False)
        computed_value = template + ctx
        registers[inst.rd] = computed_value
        cursor = (cursor + 1) & 3
@ac.module
class Top:
    def __init__() -> None:
        worker = Worker()
    def __call__() -> None:
        worker.update()
'''
        expected = None
        for optimize in (False, True):
            trace, _, out = self.run_source(source, cycles=8, optimize=optimize)
            if expected is None: expected = trace
            self.assertEqual(trace, expected)
            cpp = (out / 'cpp/model.hpp').read_text()
            for name in ('struct Worker', 'struct Instruction', ' rd{}', ' template_value{}',
                         'try_update', 'candidate', 'argument', 'registers.at('):
                self.assertIn(name, cpp)
            if not optimize:
                for name in ('inst', 'ctx_2'):
                    self.assertIn(name, cpp)
            self.assertRegex(cpp, r'std::array<Cell<uint32_t>,4> registers')
            self.assertNotRegex(cpp, r'\bv\d+\(\)|\bc\d+\{|valid\[|ACIR_SIM_NOINLINE')
            txn = load(out / 'model.transaction.mlir')
            self.assertIn('acir.binding:', txn)
            self.assertIn('callsite(', txn)
            self.assertIn('array = "top.worker.registers"', txn)
            self.assertIn('resources = ["top.worker.registers.0"', txn)
            # Re-emission uses only the saved transaction file and its locations.
            standalone = self.directory / ('standalone-' + str(optimize))
            analyze(txn, standalone, emit_cpp=True, cycles=8, optimize=False)
            check_cpp(trace, standalone, reset=True)
            self.assertEqual(cpp, (standalone / 'cpp/model.hpp').read_text())

    def test_shared_blocks_keep_unselected_division_lazy(self):
        source = '''from pycircuit import ac
def quotient(argument: ac.u32) -> ac.u32:
    assert argument != 0
    result = 20 // argument
    return result
@ac.module
class Top:
    def __init__() -> None:
        choose = ac.cell[bool](initial=False)
        zero = ac.cell[ac.u32](initial=0)
        a = ac.cell[ac.u32](initial=0)
        b = ac.cell[ac.u32](initial=0)
    @ac.rule
    def first(argument: ac.u32) -> None:
        a = argument
    @ac.rule
    def second(argument: ac.u32) -> None:
        b = argument + 1
    def __call__() -> None:
        shared_value = quotient(zero) if choose else 7
        first(shared_value)
        second(shared_value)
'''
        for optimize in (False, True):
            trace, _, out = self.run_source(source, cycles=4, optimize=optimize)
            self.assertEqual(trace[-1]['state']['top.a'], 7)
            self.assertEqual(trace[-1]['state']['top.b'], 8)
            cpp = (out / 'cpp/model.hpp').read_text()
            self.assertIn('compute_shared_value', cpp)
            self.assertIn('_cache', cpp)
            self.assertIn('if (', cpp)
            self.assertNotIn('valid[', cpp)
            self.assert_reloads(trace, out)

    def test_names_do_not_change_cse_or_execution(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        count = ac.cell[ac.u32](initial=0)
        a = ac.cell[ac.u32](initial=0)
        b = ac.cell[ac.u32](initial=0)
    @ac.rule
    def update() -> None:
        primary = count + 1
        alias = count + 1
        a = primary
        b = alias
        count = primary
    def __call__() -> None:
        update()
'''
        original, graph, out = self.run_source(source)
        renamed, new_graph, _ = self.run_source(source.replace('primary', 'template').replace('alias', 'ctx'))
        self.assertEqual(original, renamed)
        self.assertEqual(graph, new_graph)
        for tick in original:
            self.assertEqual(tick['state']['top.a'], tick['state']['top.b'])

    def test_parameter_name_in_guard_and_body(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        result = ac.cell[ac.u32](initial=0)
    @ac.rule(guard=argument > 0)
    def update(argument: ac.u32) -> None:
        result = argument
    def __call__() -> None:
        update(0)
        update(7)
'''
        trace, _, out = self.run_source(source, cycles=2, optimize=False)
        self.assertEqual(trace[-1]['state']['top.result'], 7)
        self.assertFalse(trace[0]['fire']['top.update#0'])
        self.assertTrue(trace[0]['fire']['top.update#1'])
        self.assertIn('argument', (out / 'cpp/model.hpp').read_text())
        self.assert_reloads(trace, out)
