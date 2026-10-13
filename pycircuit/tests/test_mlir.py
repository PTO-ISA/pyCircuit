"""Native ACIR parsing, symbols, regions, effects and standalone lowering."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
import textwrap
import unittest
from pycircuit import compile_source, analyze
from pycircuit.ir import CompileError
from pycircuit.mlir_text import serialize

ROOT = Path(__file__).resolve().parents[2]
MODEL = '''
from pycircuit import ac
@ac.module
def Store():
    c = ac.cell[ac.u32](initial=0)
    s = ac.signal[ac.u32](initial=0)
    e = ac.ehr[ac.u32](initial=0)
    @ac.method
    def put(x: ac.u32) -> ac.u32:
        c.value = x
        e.value = x
        s = x
        return x
@ac.module
def Top(flag: bool, n: ac.u32):
    store = Store()
    q = ac.queue[ac.u32](initial=1)
    out = ac.queue[ac.u32]()
    def ready(x: ac.queue[ac.u32]) -> bool:
        return not x.empty()
    @ac.rule(guard=ready)
    def move(x: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        value = x.value
        if flag:
            store.put(value)
            return value
        return None
    @ac.rule
    def compute(limit: ac.u32):
        total = ac.u32(0)
        for i in range(limit):
            if flag:
                total = total + i
        store.put(total)
    out = move(q)
    compute(n)
'''

class MLIRTests(unittest.TestCase):
    def setUp(self):
        base = ROOT / 'pycircuit/builds/benchmarks/rule-graph/tests'; base.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix='ods-', dir=base); self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name)
        self.opt = os.environ.get('ACPY_MLIR_OPT') or str(Path(os.environ['ACPY_MLIR_COMPILER']).with_name('acir-opt'))

    def optimize(self, text, *flags):
        source = self.path / 'input.mlir'; source.write_text(text)
        return subprocess.run([self.opt, str(source), *flags], capture_output=True, text=True, timeout=30)

    def model(self, text=MODEL):
        source = self.path / 'source.py'; source.write_text(textwrap.dedent(text))
        return serialize(compile_source(source, 'Top'))

    def test_native_roundtrip_and_lowering(self):
        text = self.model()
        compact = self.optimize(text, '--mlir-print-debuginfo')
        self.assertEqual(compact.returncode, 0, compact.stderr)
        for op in ('module', 'instance', 'interface', 'rule', 'method', 'work', 'rule.call', 'method.call', 'guard.yield', 'return', 'work.end', 'queue', 'cell', 'signal', 'ehr', 'queue.peek', 'cell.write', 'signal.set', 'ehr.write'):
            self.assertIn('acir.' + op, compact.stdout)
        self.assertIn('scf.if', compact.stdout); self.assertIn('scf.while', compact.stdout)
        self.assertIn('cf.cond_br', compact.stdout) # Multiple returns retain CFG.
        self.assertIn('output_bindings = [@top::@out]', compact.stdout)
        for op in ('acir.queue.pop ', 'acir.queue.push ', 'acir.analysis.', 'port =', 'fallback_'):
            self.assertNotIn(op, compact.stdout)
        flags = ('--mlir-print-op-generic',)
        original = self.optimize(text, *flags); rebuilt = self.optimize(compact.stdout, *flags)
        self.assertEqual(rebuilt.returncode, 0, rebuilt.stderr); self.assertEqual(original.stdout, rebuilt.stdout)
        lowered = self.optimize(compact.stdout, '--acir-lower-analysis')
        self.assertEqual(lowered.returncode, 0, lowered.stderr)
        for op in ('acir.queue.pop ', 'acir.queue.push ', 'acir.analysis.capacity.read', 'acir.analysis.capacity.write', 'call @', 'cf.cond_br'):
            self.assertIn(op, lowered.stdout)
        self.assertNotIn('scf.', lowered.stdout); self.assertNotIn('acir.rule @', lowered.stdout)
        self.assertEqual(self.optimize(lowered.stdout).returncode, 0)

    def test_symbols_parameters_and_output_slots_rejected(self):
        text = self.model()
        cases = [
            (text.replace('@top::@out', '@top::@missing'), 'output binding'),
            (text.replace('@top::@q', '@top::@missing'), 'unknown resource'),
            (text.replace('"slots" = [0 : i64]', '"slots" = [4 : i64]'), 'return slot'),
            (text.replace('"slots" = [0 : i64]', '"slots" = []'), 'one output slot'),
            (text.replace('output_bindings = [@top::@out]', 'output_bindings = [@store::@store.c]'), 'component kind'),
            (text.replace('target = @top::@top.move', 'target = @top::@missing'), 'target symbol'),
            (text.replace('^entry(%v1: !acir.queue<i32>):', '^entry(%v1: !acir.cell<i32>):', 1), 'different type'),
        ]
        for changed, diagnostic in cases:
            with self.subTest(diagnostic=diagnostic):
                self.assertNotEqual(changed, text)
                result = self.optimize(changed)
                self.assertNotEqual(result.returncode, 0); self.assertIn(diagnostic, result.stderr)

    def test_component_effects_stay_on_their_paths(self):
        result = self.optimize('''module {
          func.func @observe(%q: !acir.queue<i32>, %c: !acir.cell<i32>, %s: !acir.signal<i32>, %e: !acir.ehr<i32>, %flag: i1) {
            scf.if %flag {
              %a = acir.queue.peek %q : !acir.queue<i32> -> i32
              %b = acir.queue.peek %q : !acir.queue<i32> -> i32
              %c1 = acir.cell.read %c : !acir.cell<i32> -> i32
              %s1 = acir.signal.get %s : !acir.signal<i32> -> i32
              %e1 = acir.ehr.read %e : !acir.ehr<i32> -> i32
              %empty = acir.queue.empty %q : !acir.queue<i32> -> i1
              %full = acir.queue.full %q : !acir.queue<i32> -> i1
              %size = acir.queue.size %q : !acir.queue<i32> -> i32
              acir.signal.set %s, %a : !acir.signal<i32>, i32
            }
            return
          }
        }''', '--canonicalize', '--cse')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count('acir.queue.peek '), 2)
        self.assertLess(result.stdout.index('scf.if'), result.stdout.index('acir.queue.peek'))
        for op in ('cell.read', 'signal.get', 'ehr.read', 'queue.empty', 'queue.full', 'queue.size', 'signal.set'):
            self.assertIn('acir.' + op, result.stdout)

    def test_payloads_and_removed_operations(self):
        for operation, diagnostic in [
            ('acir.queue.push %q, %v : !acir.queue<i32>, i64', 'payload'),
            ('%a = acir.queue.peek %q : !acir.queue<i32> -> i64', 'payload'),
            ('%a = acir.queue.size %q : !acir.queue<i32> -> i1', 'i32'),
            ('%a = acir.signal.get %s {port = 0 : i64} : !acir.signal<i32> -> i32', 'derived'),
            ('"acir.read"(%q) : (!acir.queue<i32>) -> i32', 'unregistered operation'),
            ('"acir.revise"(%q, %v) {path = []} : (!acir.queue<i32>, i64) -> ()', 'unregistered operation'),
        ]:
            with self.subTest(operation=operation):
                result = self.optimize('module { func.func @bad(%q: !acir.queue<i32>, %s: !acir.signal<i32>, %v: i64) {\n' + operation + '\nreturn\n} }')
                self.assertNotEqual(result.returncode, 0); self.assertIn(diagnostic, result.stderr)

    def test_v6_requires_recompile_and_queue_assignment_is_removed(self):
        text = self.model()
        with self.assertRaisesRegex(CompileError, 'format_version 7.*recompile'):
            analyze(text.replace('7 : i64', '6 : i64', 1), self.path / 'old')
        with self.assertRaisesRegex(CompileError, 'Queue revise is removed'):
            self.model('''from pycircuit import ac
@ac.module
def Top():
    q = ac.queue[ac.u32](initial=1)
    @ac.rule
    def change():
        q.value = 2
    change()
''')

    def test_lowering_coalesces_repeated_consumption_and_retains_sources(self):
        text = self.model('''
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.queue[ac.u32](initial=7)
                @ac.rule
                def exchange(x: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
                    first = x.value
                    second = x.value
                    return first + second
                q = exchange(q)
        ''')
        self.assertNotIn('acir.queue.pop', text)
        lowered = self.optimize(text, '--acir-lower-analysis')
        self.assertEqual(lowered.returncode, 0, lowered.stderr)
        self.assertEqual(lowered.stdout.count('acir.queue.pop '), 1)
        self.assertEqual(lowered.stdout.count('acir.analysis.capacity.write '), 1)
        graph = analyze(text, self.path / 'effects', optimize=False)
        pop = next(a for a in graph['nodes'][0]['accesses'] if a['operation'] == 'pop')
        self.assertEqual(len(pop['locations']), 2)

    def test_optional_tuple_keeps_method_evaluation_order(self):
        text = self.model('''
            from pycircuit import ac
            @ac.module
            def Top():
                s = ac.signal[bool](initial=False)
                q = ac.queue[ac.u32]()
                c = ac.cell[ac.u32](initial=0)
                @ac.method
                def set_ready() -> ac.u32:
                    s = True
                    return 9
                @ac.method
                def ready() -> bool:
                    return s
                @ac.rule
                def make() -> tuple[ac.cell[ac.u32], ac.queue[ac.u32]]:
                    return set_ready(), 3 if ready() else None
                c, q = make()
        ''')
        graph = analyze(text, self.path / 'tuple', optimize=False)
        node = graph['nodes'][0]
        read = next(a for a in node['accesses'] if a['resource'] == 's' and a['operation'] == 'read')
        self.assertEqual(read['bypass']['sources'][0]['visibility'], 'temporary')
        write = next(a for a in node['accesses'] if a['resource'] == 's' and a['operation'] == 'write')
        self.assertLess(write['sequence'], read['sequence'])
        cells = [a for a in node['accesses'] if a['resource'] == 'c' and a['operation'] == 'write']
        self.assertEqual({a['value_constant'] for a in cells}, {9})

    def test_native_ir_analyzes_without_frontend_function_summaries(self):
        text = '''module attributes {acir.model = {format_version = 7 : i64, top = "Top", exports = {}, config = [], types = {}, type_order = [], constants = {}}} {
          func.func @capacity() -> i32 {
            %one = arith.constant 1 : i32
            return %one : i32
          }
          func.func @initial() -> !acir.vector<i32> {
            %empty = acir.aggregate() : () -> !acir.vector<i32>
            return %empty : !acir.vector<i32>
          }
          acir.module @top {
            acir.queue @q : i32 {name = "q", type = "u32", capacity_fn = "capacity", initial_fn = "initial"}
            acir.rule @r : () -> () {output_types = [!acir.queue<i32>]} body {
              %seven = arith.constant 7 : i32
              acir.return %seven : i32 {slots = [0 : i64]}
            }
            acir.work {
              acir.rule.call @top::@r() {output_bindings = [@top::@q]} : () -> ()
              acir.work.end
            }
          }
        }'''
        graph = analyze(text, self.path / 'native', optimize=False)
        self.assertEqual([n['id'] for n in graph['nodes']], ['top.r'])
        push = next(a for a in graph['nodes'][0]['accesses'] if a['operation'] == 'push')
        self.assertEqual((push['resource'], push['value_constant']), ('q', 7))
        saved = (self.path / 'native/model.acir.mlir').read_text()
        self.assertNotIn('acir.function', saved)
        self.assertEqual(graph, analyze(saved, self.path / 'reload', optimize=False))

    def test_explicit_native_pop_releases_capacity(self):
        text = self.model("""
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.queue[ac.u32](initial=1)
                @ac.rule
                def take(source: ac.queue[ac.u32]):
                    value = source.value
                @ac.rule
                def put() -> ac.queue[ac.u32]:
                    return 2
                take(q)
                q = put()
        """)
        # Editing actual native IR replaces the consuming peek with explicit pop.
        text = re.sub(r'%v\d+ = "acir.queue.peek"\((%v\d+)\).*?: \(!acir.queue<i32>\) -> i32[^\n]*',
                      r'"acir.queue.pop"(\1) : (!acir.queue<i32>) -> ()', text)
        self.assertNotIn('"acir.queue.peek"', text)
        graph = analyze(text, self.path / 'explicit', optimize=False)
        self.assertIn(('top.take', 'top.put'), {(e['source'], e['target']) for e in graph['edges']})
        put = next(n for n in graph['nodes'] if n['id'] == 'top.put')
        capacity = next(a for a in put['accesses'] if a['operation'] == 'query.full')
        self.assertEqual(capacity['bypass']['sources'][0]['transaction'], 'top.take')

    def test_resources_stay_in_the_declaring_module_scope(self):
        text = self.model('''
            from pycircuit import ac
            @ac.module
            def Child(source: ac.queue[ac.u32]):
                @ac.rule
                def take(message: ac.queue[ac.u32]):
                    value = message.value
                take(source)
            @ac.module
            def Top():
                q = ac.queue[ac.u32](initial=1)
                child = Child(q)
        ''')
        self.assertLess(text.index('acir.module @top'), text.index('acir.queue @q'))
        self.assertEqual(text.count('acir.queue @q'), 1)
        self.assertIn('target = @top::@q', text)
        graph = analyze(text, self.path / 'owned', optimize=False)
        self.assertEqual([n['id'] for n in graph['nodes']], ['child.take'])
        self.assertEqual([a['resource'] for a in graph['nodes'][0]['accesses'] if a['operation'] == 'pop'], ['q'])
