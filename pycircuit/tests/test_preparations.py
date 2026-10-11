"""Demand, identity and visibility of preparations rebuilt from actual ACIR."""
import unittest
from pathlib import Path
from pycircuit import analyze, compile_source
from pycircuit.analysis import atom, both, either, neg, satisfiable
from pycircuit.ir import CompileError, load
import test_rule_graph as support


class PreparationTests(unittest.TestCase):
    setUp = support.RuleGraphTests.setUp
    model = support.RuleGraphTests.model
    node = support.RuleGraphTests.node
    accesses = support.RuleGraphTests.accesses

    def variants(self, source):
        graph = self.model(source)
        saved = load(self.path / 'out/model.acir.mlir')
        (self.path / 'source.py').unlink()
        for optimize in (False, True):
            current = analyze(saved, self.path / str(optimize), optimize=optimize)
            if not optimize: self.assertEqual(graph, current)
            self.assertEqual(current, analyze(load(self.path / str(optimize) / 'model.acir.mlir'), self.path / ('reload-' + str(optimize)), optimize=False))
            self.assertFalse(current['diagnostics'])
            self.assertTrue(all(a['operation'] not in ('write', 'pop', 'push', 'revise') for a in current['computations']))
            yield current

    def test_dual_channel_shared_definition_and_original_access(self):
        example = Path(__file__).resolve().parents[1] / 'examples/dual_channel.py'
        model = compile_source(example, 'DualChannel')
        for optimize in (False, True):
            graph = analyze(model, self.path / str(optimize), optimize=optimize)
            reads = [a for a in graph['computations'] if a['operation'] == 'read']
            self.assertEqual(len(reads), 1)
            self.assertEqual(reads[0]['ready'], '!empty(source)')
            shared = [r for r in graph['preparations'] if r['operation'] == 'addi' and {u['rule'] for u in r['users']} == {'top.left', 'top.right'}]
            self.assertEqual(len(shared), 1)
            methods = [m for m in graph['preparation_methods'] if m['interface'] == 'peek']
            self.assertEqual(len(methods), 1)
            for rule in ('top.left', 'top.right'):
                access = self.accesses(graph, rule, 'read')[0]
                self.assertEqual(len(self.accesses(graph, rule, 'read')), 1)
                self.assertEqual(access['source_access'], reads[0]['id'])
                self.assertEqual(access['call_chain'], reads[0]['call_chain'])
                self.assertEqual(access['source_methods'], reads[0]['methods'])
                self.assertTrue(self.node(graph, rule)['preparations'])
                self.assertTrue(any(r['ready'] == '!empty(source)' for r in self.node(graph, rule)['requirements']))
                self.assertFalse(any(not g['implicit'] for g in self.node(graph, rule)['guards']))
                self.assertFalse(self.accesses(graph, rule, 'pop'))

    def test_preparation_read_ready_reaches_checked_helper_and_only_its_user(self):
        for graph in self.variants('''
            from pycircuit import ac
            def prepare(x: ac.u32) -> ac.u32:
                assert x < 16
                return x + 1
            @ac.module
            def Top():
                q = ac.queue[ac.u32]()
                dependent_state = ac.cell[ac.u32](initial=0)
                independent_state = ac.cell[ac.u32](initial=0)
                value = prepare(q.value)
                @ac.rule
                def dependent(x: ac.u32):
                    dependent_state.value = x
                @ac.rule
                def independent():
                    independent_state.value = 7
                dependent(value)
                independent()
        '''):
            read, = [a for a in graph['computations'] if a['operation'] == 'read']
            self.assertIs(read['condition'], True)
            self.assertEqual(read['ready'], '!empty(q)')
            helper, = [p for p in graph['preparations'] if p['boundary'] == 'call' and p['function'] == 'top_Work']
            self.assertTrue(any(r['access'] == read['id'] and r['ready'] == '!empty(q)' and r['condition'] is True
                                for r in helper['requirements']))
            self.assertTrue(any(r['ready'] == '!empty(q)' for r in self.node(graph, 'top.dependent')['requirements']))
            self.assertIsNot(self.accesses(graph, 'top.dependent', 'write')[0]['condition'], True)
            self.assertFalse(self.node(graph, 'top.independent')['requirements'])
            self.assertIs(self.accesses(graph, 'top.independent', 'write')[0]['condition'], True)

    def test_shared_guard_and_payload_keep_one_access_association(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.queue[ac.u32]()
                c = ac.cell[ac.u32](initial=0)
                shared = q.value
                enabled = shared != 0
                @ac.rule(guard=enabled)
                def use(x: ac.u32):
                    c.value = x
                use(shared)
        '''):
            source, = [a for a in graph['computations'] if a['operation'] == 'read']
            access, = self.accesses(graph, 'top.use', 'read')
            self.assertEqual(access['source_access'], source['id'])
            self.assertEqual(access['condition'], True)
            self.assertEqual(set(access['vias']), {'selection or argument computation', 'explicit guard', 'effect payload'})

    def test_variable_guard_definition_scope_and_separate_path(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top(select: bool, initial: bool):
                c = ac.cell[ac.u32](initial=0)
                enabled = c.value != 0 and initial
                @ac.rule(guard=enabled)
                def use(enabled: bool):
                    c.value = 1
                enabled = False
                if select:
                    use(True)
        '''):
            node = self.node(graph, 'top.use')
            self.assertEqual(node['selection'], {'atom': 'config:select'})
            guard = next(g for g in node['guards'] if g['source'] == 'variable')
            self.assertNotEqual(guard['condition'], False)
            self.assertTrue(self.accesses(graph, 'top.use', 'write'))
            self.assertTrue(any(a['resource'] == 'c' for a in self.accesses(graph, 'top.use', 'read')))

    def test_guarded_nested_method_and_unused_read_do_not_gate_other_rule(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Store(enabled: bool):
                q = ac.queue[ac.u32](initial=7)
                def read() -> ac.u32:
                    return q.value
                @ac.method(guard=enabled)
                def inner() -> ac.u32:
                    return read()
                @ac.method
                def outer() -> ac.u32:
                    return inner() + 1
            @ac.module
            def Top(flag: bool):
                store = Store(flag)
                unused = ac.queue[ac.u32]()
                c = ac.cell[ac.u32](initial=0)
                observation = unused.value
                shared = store.outer()
                @ac.rule
                def dependent(x: ac.u32):
                    c.value = x
                @ac.rule
                def independent():
                    c.value = 1
                dependent(shared)
                independent()
        '''):
            independent = self.node(graph, 'top.independent')
            self.assertFalse(independent['requirements'])
            self.assertFalse(independent['preparations'])
            self.assertEqual(self.accesses(graph, 'top.independent', 'write')[0]['condition'], True)
            dependent = self.node(graph, 'top.dependent')
            self.assertTrue(dependent['requirements'])
            self.assertNotIn('unused', {a['resource'] for a in dependent['accesses']})
            self.assertEqual({m['interface'] for m in dependent['methods']}, {'inner', 'outer'})
            self.assertEqual(self.accesses(graph, 'top.dependent', 'write')[0]['condition'], {'atom': 'config:flag'})

    def test_separate_calls_keep_distinct_preparations(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Store():
                c = ac.cell[ac.u32](initial=0)
                @ac.method
                def peek() -> ac.u32:
                    return c.value
            @ac.module
            def Top():
                store = Store()
                @ac.rule
                def left(x: ac.u32):
                    pass
                @ac.rule
                def right(x: ac.u32):
                    pass
                a = store.peek()
                b = store.peek()
                left(a)
                right(b)
        '''):
            self.assertEqual(len([a for a in graph['computations'] if a['operation'] == 'read']), 2)
            calls = [r for r in graph['preparations'] if r['boundary'] == 'call' and r['users']]
            self.assertEqual(len(calls), 2)
            self.assertEqual({tuple(u['rule'] for u in r['users']) for r in calls}, {('top.left',), ('top.right',)})

    def test_signal_guard_orders_without_writer_validity(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top():
                enabled = ac.signal[bool](initial=False)
                @ac.rule
                def produce() -> ac.signal[bool]:
                    return True
                enabled = produce()
                @ac.rule(guard=enabled)
                def use():
                    pass
                use()
        '''):
            self.assertFalse(self.node(graph, 'top.use')['requirements'])
            self.assertTrue(any(e['kind'] == 'method.order' and e['source'] == 'top.produce' and e['target'] == 'top.use' for e in graph['edges']))
            self.assertTrue(any(r['state_dependencies'] and r['users'] for r in graph['preparations']))

    def test_short_circuit_and_loop_boundaries(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top(flag: bool, n: ac.u32):
                q = ac.queue[ac.u32]()
                enabled = flag and not q.empty() and q.value != 0
                @ac.rule(guard=enabled)
                def use(x: ac.u32):
                    pass
                total = ac.u32(0)
                for i in range(n):
                    total = total + q.size()
                use(total)
        '''):
            read = next(a for a in graph['computations'] if a['operation'] == 'read')
            self.assertNotEqual(read['condition'], True)
            self.assertTrue(any(r['boundary'] == 'loop' and r['users'] for r in graph['preparations']))
            self.assertTrue(self.node(graph, 'top.use')['preparations'])

    def test_ehr_late_preparation_retains_port_and_method_order(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Store():
                value = ac.ehr[ac.u32](initial=0)
                @ac.method
                def write():
                    value.value = 7
                @ac.method(After='write')
                def peek() -> ac.u32:
                    return value.value
            @ac.module
            def Top():
                store = Store()
                @ac.rule
                def producer():
                    store.write()
                @ac.rule
                def consumer(x: ac.u32):
                    pass
                producer()
                shared = store.peek()
                consumer(shared)
        '''):
            source = next(a for a in graph['computations'] if a['operation'] == 'read')
            read = self.accesses(graph, 'top.consumer', 'read')[0]
            self.assertEqual(source['port'], 1)
            self.assertEqual(read['port'], source['port'])
            self.assertEqual(read['source_access'], source['id'])
            self.assertEqual(read['bypass']['sources'][0]['transaction'], 'top.producer')
            self.assertEqual(read['bypass']['sources'][0]['visibility'], 'granted_transaction')
            self.assertTrue(any(e['source'] == 'top.producer' and e['target'] == 'top.consumer' for e in graph['edges']))

    def test_module_effects_and_bad_variable_guards_rejected(self):
        for text, error in [
            ('enabled = ac.u32(1)\n@ac.rule(guard=enabled)\ndef use():\n    pass\nuse()', 'variable guard must be bool'),
            ('@ac.rule(guard=missing)\ndef use():\n    pass\nuse()', 'unknown name'),
            ('@ac.method\ndef update():\n    c.value = 1\nupdate()', 'state effect outside a transaction'),
            ('def helper():\n    c.value = 1\n@ac.method\ndef update():\n    helper()\nupdate()', 'state effect outside a transaction'),
        ]:
            with self.subTest(error=error), self.assertRaisesRegex(CompileError, error):
                self.model('from pycircuit import ac\n@ac.module\ndef Top():\n    c = ac.cell[ac.u32](initial=0)\n' + ''.join('    ' + line + '\n' for line in text.splitlines()))

    def test_reconverged_branch_and_unused_loop_do_not_add_readiness(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top(n: ac.u32):
                q = ac.queue[ac.u32]()
                c = ac.cell[ac.u32](initial=0)
                if q.value != 0:
                    unused = q.value + 1
                else:
                    unused = ac.u32(0)
                total = ac.u32(0)
                for i in range(n):
                    total = total + q.value
                @ac.rule
                def independent():
                    c.value = 7
                independent()
        '''):
            node = self.node(graph, 'top.independent')
            self.assertFalse(node['requirements'])
            self.assertFalse(self.accesses(graph, 'top.independent', 'read'))

    def test_complex_short_circuit_reconvergence_keeps_next_rule_unconditional(self):
        def decode(value):
            if isinstance(value, bool): return value
            kind, parts = next(iter(value.items()))
            if kind == 'atom': return atom(parts)
            if kind == 'not': return neg(decode(parts))
            return (both if kind == 'and' else either)(*(decode(v) for v in parts))

        expected = either(both(*(atom('config:' + name) for name in 'abc')),
                          both(*(atom('config:' + name) for name in 'def')))
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top(a: bool, b: bool, c: bool, d: bool, e: bool, f: bool):
                first = ac.cell[ac.u32](initial=0)
                second = ac.cell[ac.u32](initial=0)
                @ac.rule
                def conditional():
                    first.value = 1
                @ac.rule
                def independent():
                    second.value = 2
                if (a and b and c) or (d and e and f):
                    conditional()
                independent()
        '''):
            selected = decode(self.node(graph, 'top.conditional')['selection'])
            self.assertIs(satisfiable(both(selected, neg(expected))), False)
            self.assertIs(satisfiable(both(expected, neg(selected))), False)
            independent = self.node(graph, 'top.independent')
            self.assertIs(independent['selection'], True)
            self.assertFalse(independent['requirements'])

    def test_pure_preparation_method_contract_reaches_consuming_rule(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Store():
                @ac.method
                def first() -> ac.u32:
                    return 1
                @ac.method(After='first')
                def second() -> ac.u32:
                    return 2
            @ac.module
            def Top():
                store = Store()
                @ac.rule
                def producer():
                    x = store.first()
                @ac.rule
                def consumer(x: ac.u32):
                    pass
                producer()
                value = store.second()
                consumer(value)
        '''):
            self.assertTrue(any(e['source'] == 'top.producer' and e['target'] == 'top.consumer' and e['kind'] == 'method.order' for e in graph['edges']))

    def test_dynamic_method_guard_captured_across_instance_boundary(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Store():
                c = ac.cell[ac.u32](initial=0)
                enabled = c.value != 0
                @ac.method(guard=enabled)
                def peek() -> ac.u32:
                    return c.value
            @ac.module
            def Top():
                store = Store()
                @ac.rule
                def use():
                    value = store.peek()
                use()
        '''):
            self.assertTrue(self.accesses(graph, 'top.use', 'read'))
            self.assertTrue(any(g['source'] == 'variable' for g in self.node(graph, 'top.use')['guards']))

    def test_unused_closure_values_do_not_become_method_arguments(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.queue[ac.u32]()
                unused = q.value
                def helper() -> ac.u32:
                    return 7
                @ac.method
                def peek() -> ac.u32:
                    return helper()
                shared = peek()
                @ac.rule
                def use(x: ac.u32):
                    pass
                use(shared)
        '''):
            self.assertFalse(self.node(graph, 'top.use')['requirements'])
            self.assertFalse(self.accesses(graph, 'top.use', 'read'))


if __name__ == '__main__': unittest.main()
