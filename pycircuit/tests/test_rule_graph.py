"""Complete source models, actual MLIR reload, conditional graph contracts."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import textwrap
import unittest
from pycircuit import analyze, compile_source
from pycircuit.ir import CompileError, load, save
from pycircuit.mlir_text import attr

ROOT = Path(__file__).resolve().parents[2]


class RuleGraphTests(unittest.TestCase):
    def setUp(self):
        base = ROOT / 'pycircuit/builds/benchmarks/rule-graph/tests'
        base.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=base)
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name)

    def model(self, text, top='Top', optimize=False):
        source = self.path / 'source.py'
        source.write_text(textwrap.dedent(text))
        model = compile_source(source, top)
        return analyze(model, self.path / ('opt' if optimize else 'out'), optimize=optimize)

    def node(self, graph, name):
        return next(n for n in graph['nodes'] if n['id'] == name)

    def accesses(self, graph, name, operation):
        return [a for a in self.node(graph, name)['accesses'] if a['operation'] == operation]

    def test_shared_instances_import_aliases_and_namespaces(self):
        (self.path / 'state.py').write_text(textwrap.dedent('''
            from pycircuit import ac as hw
            @hw.module
            def Store():
                count = hw.cell[hw.u32](initial=0)
                @hw.method
                def add(value: hw.u32):
                    count.value = count.value + value
                return count
            shared = Store()
        '''))
        (self.path / 'other.py').write_text('from state import Store\nshared = Store()\n')
        graph = self.model('''
            from pycircuit import ac
            import state as s
            from state import shared as same
            import other
            @ac.module
            def Client(store):
                @ac.rule
                def change():
                    store.add(1)
                change()
            @ac.module
            def Top():
                left = Client(s.shared)
                right = Client(same)
                separate = s.Store()
                third = Client(separate)
                fourth = Client(other.shared)
        ''')
        resources = [self.accesses(graph, n, 'write')[0]['resource'] for n in ('left.change', 'right.change', 'third.change', 'fourth.change')]
        self.assertEqual(resources[0], resources[1])
        self.assertEqual(len(set(resources)), 3)
        self.assertEqual(len(graph['instances']), 8)

    def test_atomic_cross_module_method_guard_and_consumption(self):
        graph = self.model('''
            from pycircuit import ac
            @ac.module
            def State():
                count = ac.cell[ac.u32](initial=0)
                def ready(x: ac.queue[ac.u32]) -> bool:
                    return not x.empty() and count.value < 10
                @ac.method(guard=ready)
                def add(x: ac.queue[ac.u32]) -> ac.u32:
                    count.value = count.value + x.value
                    return x.value
                return count
            @ac.module
            def Top():
                state = State()
                q = ac.queue[ac.u32](initial=3)
                out = ac.queue[ac.u32]()
                @ac.rule
                def run(x: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
                    return state.add(x)
                out = run(q)
        ''')
        self.assertEqual([n['id'] for n in graph['nodes']], ['top.run'])
        for operation, resource in [('pop', 'q'), ('push', 'out'), ('write', 'state.count')]:
            records = [a for a in self.accesses(graph, 'top.run', operation) if a.get('aspect', 'value') == 'value']
            self.assertEqual(len(records), 1)
            self.assertEqual(records[0]['resource'], resource)
            self.assertNotEqual(records[0]['condition'], True)
        self.assertIn('!empty(q)', [a['ready'] for a in self.accesses(graph, 'top.run', 'pop')])
        self.assertFalse(graph['diagnostics'])

    def test_method_observation_and_helper_role_propagation(self):
        graph = self.model('''
            from pycircuit import ac
            @ac.module
            def Store():
                @ac.method
                def peek(q: ac.queue[ac.u32]) -> ac.u32:
                    return q.value
            def read(q: ac.queue[ac.u32]) -> ac.u32:
                return q.value
            @ac.module
            def Top():
                store = Store()
                q = ac.queue[ac.u32](initial=3)
                @ac.rule
                def observe():
                    x = store.peek(q)
                @ac.rule
                def consume(a: ac.queue[ac.u32], b: ac.queue[ac.u32]):
                    x = read(a) + store.peek(b) + a.value
                observe()
                consume(q, q)
        ''')
        self.assertFalse(self.accesses(graph, 'top.observe', 'pop'))
        self.assertEqual(len(self.accesses(graph, 'top.consume', 'pop')), 1)
        self.assertGreaterEqual(len(self.accesses(graph, 'top.consume', 'pop')[0]['locations']), 2)

    def test_branch_consumption_and_mutually_exclusive_rule_calls(self):
        graph = self.model('''
            from pycircuit import ac
            @ac.module
            def Top(choose: bool):
                left = ac.queue[ac.u32](initial=1)
                right = ac.queue[ac.u32](initial=2)
                @ac.rule
                def take(a: ac.queue[ac.u32], b: ac.queue[ac.u32], select: bool):
                    if select:
                        value = a.value + a.value
                    else:
                        value = b.value
                if choose:
                    take(left, right, choose)
                else:
                    take(left, right, choose)
        ''')
        self.assertEqual(len(graph['nodes']), 1)
        pops = self.accesses(graph, 'top.take', 'pop')
        self.assertEqual({a['resource'] for a in pops}, {'left', 'right'})
        self.assertTrue(all(a['condition'] is not True for a in pops))

    def test_pipeline_queue_method_order_and_old_observation(self):
        graph = self.model('''
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.queue[ac.u32]()
                @ac.rule
                def produce() -> ac.queue[ac.u32]:
                    return 7
                @ac.rule
                def consume(x: ac.queue[ac.u32]):
                    y = x.value
                @ac.rule
                def observe():
                    y = q.value
                q = produce()
                consume(q)
                observe()
        ''')
        relations = {(e['source'], e['target'], e['kind']) for e in graph['edges']}
        self.assertIn(('top.consume', 'top.produce', 'method.order'), relations)
        self.assertIn(('top.observe', 'top.produce', 'method.order'), relations)
        self.assertNotIn(('top.produce', 'top.consume', 'method.order'), relations)
        for e in graph['edges']:
            self.assertIs(e['condition'], True)
            self.assertEqual(e['kind'], 'method.order')
            self.assertEqual(e['relation'], 'SB')
            self.assertTrue(all(loc['line'] > 0 for loc in e['locations']))

    def test_mixed_outputs_omitted_effects_and_signal_order(self):
        graph = self.model('''
            from pycircuit import ac
            @ac.module
            def Top(emit: bool):
                q = ac.queue[ac.u32]()
                c = ac.cell[ac.u32](initial=0)
                @ac.rule
                def make(enabled: bool) -> tuple[ac.queue[ac.u32], ac.cell[ac.u32], ac.signal[ac.u32]]:
                    if enabled:
                        return 1, 2, 3
                    return None, None, 4
                @ac.rule
                def use(value: ac.var[ac.u32]):
                    c.value = value
                v = ac.signal[ac.u32](initial=0)
                q, c, v = make(emit)
                use(v)
        ''')
        self.assertEqual(len(self.accesses(graph, 'top.make', 'push')), 1)
        self.assertEqual(len(self.accesses(graph, 'top.make', 'write')), 3)
        self.assertNotEqual(self.accesses(graph, 'top.make', 'push')[0]['condition'], True)
        self.assertFalse(self.node(graph, 'top.use')['requirements'])
        self.assertIn(('top.make', 'top.use', 'method.order'), {(e['source'],e['target'],e['kind']) for e in graph['edges']})

    def test_shared_computation_cell_feedback_and_outputs(self):
        graph = self.model('''
            from pycircuit import ac
            @ac.module
            def Store():
                count = ac.cell[ac.u32](initial=0)
                return count
            @ac.module
            def Top(enabled: bool):
                store = Store()
                @ac.rule
                def update(value: ac.u32):
                    store.count.value = value
                @ac.rule
                def other(value: ac.u32):
                    store.outputs.value = value
                if enabled:
                    offset = store.count.value + 1
                    update(offset)
                    other(offset)
        ''')
        reads = [a for a in graph['computations'] if a['operation'] == 'read']
        self.assertEqual(len(reads), 1)
        self.assertEqual(reads[0]['view'], 'old')
        self.assertEqual(len(graph['edges']), 0)
        text = (self.path / 'out/model.acir.mlir').read_text()
        self.assertIn('scf.if', text)
        self.assertEqual(text.count('acir.cell.read '), 1)

    def test_preparation_queue_dependency_in_conditions_and_arguments(self):
        graph = self.model('''
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.queue[ac.u32]()
                @ac.rule
                def make() -> ac.queue[ac.u32]:
                    return 7
                @ac.rule
                def use(value: ac.u32):
                    pass
                q = make()
                x = q.value
                if x != 0:
                    use(x)
        ''')
        self.assertTrue(any(e['source'] == 'top.use' and e['target'] == 'top.make' and e['kind'] == 'method.order' for e in graph['edges']))
        self.assertTrue(self.node(graph,'top.use')['requirements'])

    def test_confirmed_cycle_and_unknown_order(self):
        text = '''
            from pycircuit import ac
            @ac.module
            def Top(a: bool, b: bool):
                left = ac.queue[ac.u32]()
                right = ac.queue[ac.u32]()
                @ac.rule
                def one() -> ac.queue[ac.u32]:
                    return right.value
                @ac.rule
                def two() -> ac.queue[ac.u32]:
                    return left.value
                left = one()
                right = two()
        '''
        graph = self.model(text)
        self.assertEqual(graph['diagnostics'][0]['kind'], 'confirmed-cycle')
        graph = self.model(text.replace('left = one()', 'if a:\n                    left = one()').replace('right = two()', 'if not a:\n                    right = two()'))
        self.assertFalse(graph['diagnostics'])
        graph = self.model(text.replace('left = one()', 'if left.value > 0:\n                    left = one()'))
        self.assertTrue(any(d['kind'] == 'unproven-order' for d in graph['diagnostics']))

    def test_fixed_array_constant_and_dynamic_identities(self):
        graph = self.model('''
            from pycircuit import ac
            @ac.module
            def Top(index: ac.u32):
                entries = ac.array(ac.queue[ac.u32], shape=(3,), initial=0)
                @ac.rule
                def fixed(q: ac.queue[ac.u32]):
                    x = q.value
                @ac.rule
                def dynamic(q: ac.queue[ac.u32]):
                    x = q.value
                fixed(entries[1])
                dynamic(entries[index])
        ''')
        self.assertEqual({a['resource'] for a in self.accesses(graph, 'top.fixed', 'pop')}, {'entries[1]'})
        self.assertEqual({a['resource'] for a in self.accesses(graph, 'top.dynamic', 'pop')}, {'entries[0]', 'entries[1]', 'entries[2]'})

    def test_source_free_reload_and_optimization(self):
        text = '''
            from pycircuit import ac
            @ac.module
            def Top(flag: bool):
                q = ac.queue[ac.u32]()
                @ac.rule
                def put() -> ac.queue[ac.u32]:
                    return 1
                @ac.rule
                def see():
                    if flag:
                        value = q.value
                q = put()
                see()
        '''
        graph = self.model(text)
        saved = load(self.path / 'out/model.acir.mlir')
        (self.path / 'source.py').unlink()
        rebuilt = analyze(saved, self.path / 'reload', optimize=False)
        self.assertEqual(graph, rebuilt)
        optimized = analyze(saved, self.path / 'optimized', optimize=True)
        self.assertEqual([(e['source'],e['target'],e['condition']) for e in graph['edges']], [(e['source'],e['target'],e['condition']) for e in optimized['edges']])
        self.assertNotIn('acir.analysis', saved)
        self.assertNotIn('acir.contracts', saved)
        self.assertIn('acir.model', saved)
        self.assertNotIn('acir.ehr', saved)
        self.assertNotIn('port =', saved)
        with self.assertRaisesRegex(CompileError, 'format_version 7.*recompile'):
            analyze(saved.replace('format_version = 7', 'format_version = 6'), self.path / 'legacy', optimize=False)
        changed = '\n'.join(line.replace('acir.queue.peek ', 'acir.length ').replace(' -> i32', '') if 'acir.queue.peek ' in line and '-> i32' in line else line for line in saved.splitlines())
        self.assertNotEqual(changed, saved)
        mutated = analyze(changed, self.path / 'mutated', optimize=False)
        self.assertFalse(mutated['edges'])

    def test_unsupported_calls_and_port_errors(self):
        models = [
            ('c = ac.cell[ac.u32]()', '', 'Cell requires initial'),
            ('', '@ac.rule\n    def r() -> ac.cell[ac.u32]:\n        return 1\n    missing = r()', 'explicitly initialized Cell'),
            ('', '@ac.rule\n    def r() -> ac.var[ac.u32]:\n        return None\n    x = r()', 'ordinary var outputs are removed'),
            ('', 'def bad() -> bool:\n        c.value = 1\n        return True\n    @ac.rule(guard=bad)\n    def r():\n        pass\n    r()', 'state updates require a transaction'),
            ('', '@ac.rule\n    def r():\n        pass\n    r()\n    r()', 'selected more than once'),
        ]
        for declaration, behavior, message in models:
            with self.subTest(message=message):
                source = 'from pycircuit import ac\n@ac.module\ndef Top():\n    c=ac.cell[ac.u32](initial=0)\n    ' + declaration + '\n    ' + behavior + '\n'
                with self.assertRaisesRegex(CompileError, message): self.model(source)

    def test_recursive_helper_rejected(self):
        with self.assertRaisesRegex(CompileError, 'recursive'):
            self.model('''
                from pycircuit import ac
                def loop(x: ac.u32) -> ac.u32:
                    return loop(x)
                @ac.module
                def Top():
                    @ac.rule
                    def r():
                        x = loop(1)
                    r()
            ''')

    def test_cross_instance_signal_outputs_and_guard(self):
        graph = self.model("""
            from pycircuit import ac
            @ac.module
            def Source():
                number = ac.signal[ac.u32](initial=0)
                enabled = ac.signal[bool](initial=False)
                @ac.rule
                def make() -> tuple[ac.signal[ac.u32], ac.signal[bool]]:
                    return 7, True
                number, enabled = make()
                return number, enabled
            @ac.module
            def Consumer(value: ac.signal[ac.u32], enabled: ac.signal[bool]):
                c = ac.cell[ac.u32](initial=0)
                @ac.rule
                def use(x: ac.u32):
                    c.value = x
                if enabled:
                    use(value)
            @ac.module
            def Top():
                source = Source()
                sink = Consumer(source.outputs[0], source.outputs[1])
        """)
        self.assertFalse(self.node(graph, 'sink.use')['requirements'])
        self.assertTrue(any(e['source'] == 'source.make' and e['target'] == 'sink.use' for e in graph['edges']))
        self.assertIn('!acir.signal', (self.path/'out/model.acir.mlir').read_text())
        self.assertEqual(graph, analyze(load(self.path/'out/model.acir.mlir'), self.path/'reload', optimize=False))

    def test_array_list_alias_constant_identity(self):
        graph = self.model("""
            from pycircuit import ac
            @ac.module
            def Top():
                left = ac.queue[ac.u32](initial=1)
                right = ac.queue[ac.u32](initial=2)
                entries = [left, right]
                @ac.rule
                def take(q: ac.qarray[ac.u32]):
                    value = q[1].value
                take(entries)
        """)
        self.assertEqual([a['resource'] for a in self.accesses(graph, 'top.take', 'pop')], ['right'])

    def test_preparation_loop_keeps_read_dependencies(self):
        graph = self.model("""
            from pycircuit import ac
            @ac.module
            def Top(n: ac.u32):
                q = ac.queue[ac.u32]()
                @ac.rule
                def produce() -> ac.queue[ac.u32]:
                    return 1
                @ac.rule
                def use(x: ac.u32):
                    pass
                q = produce()
                total = ac.u32(0)
                for i in range(n):
                    total = total + q.value
                use(total)
        """)
        self.assertTrue(any(e['source'] == 'top.use' and e['target'] == 'top.produce' for e in graph['edges']))
        with self.assertRaisesRegex(CompileError, 'runtime repeated rule'):
            self.model("""
                from pycircuit import ac
                @ac.module
                def Top(n: ac.u32):
                    @ac.rule
                    def use():
                        pass
                    for i in range(n):
                        use()
            """)

    def test_rule_guard_observes_consuming_inputs(self):
        graph = self.model("""
            from pycircuit import ac
            def ready(q: ac.queue[ac.u32]) -> bool:
                return q.value != 0
            @ac.module
            def Top():
                q = ac.queue[ac.u32]()
                @ac.rule(guard=ready)
                def use(q: ac.queue[ac.u32]):
                    pass
                use(q)
        """)
        self.assertFalse(self.accesses(graph, 'top.use', 'pop'))
        self.assertEqual(self.accesses(graph, 'top.use', 'read')[0]['role'], 'observe')

    def test_atomic_self_observation_and_consumption_have_no_self_loop(self):
        graph = self.model("""
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.queue[ac.u32]()
                @ac.rule
                def update() -> ac.queue[ac.u32]:
                    return q.value + 1
                q = update()
        """)
        self.assertFalse(graph['edges'])
        self.assertFalse(graph['diagnostics'])
        graph = self.model("""
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.queue[ac.u32]()
                @ac.rule
                def update(q: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
                    return q.value + 1
                q = update(q)
        """)
        self.assertFalse(graph['diagnostics'])

    def test_legacy_unannotated_output_and_forward_reference(self):
        graph = self.model("""
            from pycircuit import ac
            @ac.module
            def Top():
                @ac.rule
                def observe():
                    value = output.value
                @ac.rule
                def produce():
                    return ac.u16(5)
                observe()
                output = produce()
        """)
        self.assertEqual(self.accesses(graph, 'top.produce', 'push')[0]['resource'], 'top.produce.out0')
        self.assertTrue(any(e['source'] == 'top.observe' and e['target'] == 'top.produce' for e in graph['edges']))

    def test_conditional_resource_alias_consumes_only_selected_identity(self):
        graph = self.model("""
            from pycircuit import ac
            @ac.module
            def Top(choose: bool):
                a = ac.queue[ac.u32](initial=1)
                b = ac.queue[ac.u32](initial=2)
                @ac.rule
                def take(left: ac.queue[ac.u32], right: ac.queue[ac.u32], flag: bool):
                    selected = left if flag else right
                    value = selected.value
                take(a, b, choose)
        """)
        pops = {a['resource']: a['condition'] for a in self.accesses(graph, 'top.take', 'pop')}
        self.assertEqual(pops, {'a': {'atom': 'config:choose'}, 'b': {'not': {'atom': 'config:choose'}}})

    def test_constant_width_guards_and_expression_array_extent(self):
        graph = self.model("""
            from pycircuit import ac
            @ac.module
            def Top():
                entries = ac.array(ac.queue[ac.u32], shape=(2 + 2,), initial=0)
                @ac.rule
                def update(q: ac.queue[ac.u32]):
                    value = q.value
                if ac.u32(0xffffffff) < ac.u32(5):
                    update(entries[0])
        """)
        self.assertFalse(graph['nodes'])
        graph = self.model("""
            from pycircuit import ac
            @ac.module
            def Top(i: ac.u32):
                entries = ac.array(ac.queue[ac.u32], shape=(2 + 2,), initial=0)
                @ac.rule
                def update(q: ac.queue[ac.u32]):
                    value = q.value
                update(entries[i])
        """)
        self.assertEqual(len(self.accesses(graph, 'top.update', 'pop')), 4)

    def test_cycle_diagnostics_roundtrip_and_cli_failure(self):
        graph = self.model("""
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.queue[ac.u32]()
                other = ac.queue[ac.u32]()
                @ac.rule
                def r() -> ac.queue[ac.u32]:
                    return other.value
                q = r()
                @ac.rule
                def back() -> ac.queue[ac.u32]:
                    return q.value
                other = back()
        """)
        saved = load(self.path/'out/model.acir.mlir')
        self.assertEqual(graph, analyze(saved, self.path/'reload', optimize=False))
        result = subprocess.run([os.sys.executable, '-m', 'pycircuit', 'analyze', str(self.path/'out/model.acir.mlir'), '--output', str(self.path/'cli'), '--no-opt'], capture_output=True, text=True, cwd=ROOT)
        self.assertEqual(result.returncode, 1)
        self.assertIn('同周期依赖环', result.stderr)
        self.assertIn('top.r', result.stderr)
        self.assertTrue((self.path/'cli/rule-graph.json').exists())

    def test_helper_preparation_preserves_single_shared_evaluation(self):
        graph = self.model("""
            from pycircuit import ac
            def value(q: ac.queue[ac.u32]) -> ac.u32:
                return q.value
            @ac.module
            def Top():
                q = ac.queue[ac.u32]()
                @ac.rule
                def produce() -> ac.queue[ac.u32]:
                    return 7
                @ac.rule
                def left(x: ac.u32):
                    pass
                @ac.rule
                def right(x: ac.u32):
                    pass
                q = produce()
                shared = value(q)
                left(shared)
                right(shared)
        """)
        self.assertEqual(len([r for r in graph['computations'] if r['operation'] == 'read']), 1)
        self.assertTrue(any(e['source'] == 'top.left' and e['target'] == 'top.produce' for e in graph['edges']))
        self.assertTrue(any(e['source'] == 'top.right' and e['target'] == 'top.produce' for e in graph['edges']))
        self.assertFalse(any(e['source'] == 'top.produce' for e in graph['edges']))
        text = (self.path/'out/model.acir.mlir').read_text()
        work = text[text.index('acir.work '):]
        self.assertLess(work.index('acir.rule.call '), work.index('call @'))

    def test_cli_reload(self):
        self.model('''
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.cell[ac.u32](initial=0)
                @ac.rule
                def r():
                    q.value = 1
                r()
        ''')
        (self.path/'source.py').unlink()
        result = subprocess.run([os.sys.executable, '-m', 'pycircuit', 'analyze', str(self.path/'out/model.acir.mlir'), '--output', str(self.path/'cli')], capture_output=True, text=True, cwd=ROOT)
        self.assertEqual(result.returncode, 0, result.stderr)
        for name in ('model.acir.mlir', 'rule-graph.json', 'rule-graph.dot'): self.assertTrue((self.path/'cli'/name).exists())


if __name__ == '__main__': unittest.main()
