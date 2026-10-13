"""Signal identities, current views and atomic publication rebuilt from ACIR."""
import unittest
from pycircuit import analyze
from pycircuit.ir import CompileError, load
import test_rule_graph as support


class SignalTests(unittest.TestCase):
    setUp = support.RuleGraphTests.setUp
    model = support.RuleGraphTests.model
    node = support.RuleGraphTests.node
    accesses = support.RuleGraphTests.accesses

    def variants(self, source):
        original = self.model(source)
        saved = load(self.path / 'out/model.acir.mlir')
        (self.path / 'source.py').unlink()
        for optimize in (False, True):
            graph = analyze(saved, self.path / str(optimize), optimize=optimize)
            self.assertEqual(graph, analyze(load(self.path / str(optimize) / 'model.acir.mlir'), self.path / ('reload-' + str(optimize)), optimize=False))
            self.assertEqual({(e['source'], e['target'], e['resource']) for e in original['edges']},
                             {(e['source'], e['target'], e['resource']) for e in graph['edges']})
            self.assertEqual(graph['format_version'], 7)
            self.assertFalse(graph['diagnostics'])
            self.assertTrue(all(e['kind'] == 'method.order' for e in graph['edges']))
            signals = {r['name'] for r in graph['resources'] if r['kind'] == 'signal'}
            self.assertTrue(all(r['contract'] == 'Signal' and 'initial_fn' in r and 'capacity_fn' not in r for r in graph['resources'] if r['name'] in signals))
            self.assertFalse(any(v['resource'] in signals for v in graph['ehr']))
            for node in graph['nodes']:
                self.assertNotIn('required_validity', node)
                for a in node['accesses']:
                    if a['resource'] in signals:
                        self.assertNotIn('port', a)
                        self.assertEqual(a['ready'], 'true')
                        self.assertIn(a['operation'], ('read', 'write'))
                        self.assertEqual(a['interface'], 'get' if a['operation'] == 'read' else 'set')
                        self.assertEqual(a['view'], 'current')
            yield graph

    def test_assignment_keeps_both_identities_and_reads_into_ssa(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top():
                source = ac.signal[ac.u32](initial=3)
                target = ac.signal[ac.u32](initial=0)
                @ac.rule
                def copy(out: ac.signal[ac.u32], incoming: ac.signal[ac.u32]):
                    out = incoming
                    saved = out
                    out = saved + 1
                copy(target, source)
                return source, target
        '''):
            self.assertEqual({a['resource'] for a in self.accesses(graph, 'top.copy', 'write')}, {'target'})
            reads = self.accesses(graph, 'top.copy', 'read')
            self.assertEqual({a['resource'] for a in reads}, {'source', 'target'})
            target = next(a for a in reads if a['resource'] == 'target')
            self.assertEqual(len(target['bypass']['sources']), 1)
            self.assertEqual(target['bypass']['sources'][0]['visibility'], 'temporary')
            top = next(i for i in graph['instances'] if i['name'] == 'top')
            self.assertEqual([v['resources'] for v in top['outputs']], [['source'], ['target']])
            self.assertFalse(self.node(graph, 'top.copy')['requirements'])
            self.assertFalse(graph['edges'])

    def test_writer_readiness_never_becomes_reader_admission(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Reader(source: ac.signal[ac.u32]):
                value = source + 1
                @ac.rule
                def use(x: ac.u32):
                    pass
                use(value)
            @ac.module
            def Top():
                q = ac.queue[ac.u32]()
                result = ac.signal[ac.u32](initial=5)
                @ac.rule
                def produce(message: ac.queue[ac.u32]) -> ac.signal[ac.u32]:
                    return message.value
                result = produce(q)
                reader = Reader(result)
                return result
        '''):
            writer = self.node(graph, 'top.produce')
            reader = self.node(graph, 'reader.use')
            self.assertTrue(any(r['ready'] == '!empty(q)' for r in writer['requirements']))
            self.assertFalse(reader['requirements'])
            self.assertTrue(any(e['source'] == writer['id'] and e['target'] == reader['id'] for e in graph['edges']))
            read, = self.accesses(graph, reader['id'], 'read')
            bypass = read['bypass']
            self.assertEqual(bypass['fallback'], 'retained')
            source, = bypass['sources']
            self.assertEqual(source['visibility'], 'granted_transaction')
            self.assertIn('granted(top.produce)', repr(source['eligibility']))
            self.assertTrue(reader['preparations'])
            self.assertFalse(any(a['operation'] == 'write' for a in graph['computations']))
            self.assertEqual(len([a for a in self.accesses(graph, writer['id'], 'write') if a['resource'] == 'result']), 1)

    def test_late_failure_cancels_publication_but_keeps_decision_order(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top():
                result = ac.signal[ac.u32](initial=5)
                @ac.rule
                def produce():
                    result = 9
                    temporary = result
                    assert temporary == 9
                    assert False
                @ac.rule
                def use(source: ac.signal[ac.u32]):
                    saved = source
                produce()
                use(result)
        '''):
            self.assertEqual(self.node(graph, 'top.produce')['completion_conditions'], [False])
            local, = self.accesses(graph, 'top.produce', 'read')
            self.assertEqual(local['bypass']['sources'][0]['value_constant'], 9)
            self.assertEqual(local['bypass']['sources'][0]['visibility'], 'temporary')
            remote, = self.accesses(graph, 'top.use', 'read')
            self.assertFalse(remote['bypass']['sources'])
            self.assertEqual(remote['bypass']['fallback'], 'retained')
            self.assertFalse(self.node(graph, 'top.use')['requirements'])
            edge, = graph['edges']
            self.assertEqual((edge['source'], edge['target'], edge['interfaces'], edge['condition']),
                             ('top.produce', 'top.use', ['set', 'get'], True))

    def test_only_previous_local_sets_are_visible_across_helpers(self):
        for graph in self.variants('''
            from pycircuit import ac
            def set_value(out: ac.signal[ac.u32], value: ac.u32):
                out = value
            def peek(source: ac.signal[ac.u32]) -> ac.u32:
                return source
            @ac.module
            def Top():
                result = ac.signal[ac.u32](initial=0)
                @ac.rule
                def update():
                    before = peek(result)
                    set_value(result, 1)
                    middle = peek(result)
                    set_value(result, 2)
                    after = peek(result)
                update()
        '''):
            reads = sorted(self.accesses(graph, 'top.update', 'read'), key=lambda a: a['sequence'])
            self.assertEqual([len(a['bypass']['sources']) for a in reads], [0, 1, 2])
            self.assertEqual([s['value_constant'] for s in reads[-1]['bypass']['sources']], [1, 2])
            self.assertFalse(graph['edges'])

    def test_module_method_autonomous_rule_and_control_share_signal(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Producer(run: bool):
                result = ac.signal[ac.u32](initial=0)
                @ac.method
                def set_value(value: ac.u32):
                    result = value
                @ac.method
                def peek() -> ac.u32:
                    return result
                @ac.rule
                def advance():
                    result = result + 1
                if run:
                    advance()
                return result
            @ac.module
            def Reader(source: ac.signal[ac.u32]):
                value = source
                enabled = value != 0
                @ac.rule(guard=enabled)
                def use(x: ac.u32):
                    pass
                use(value + 1)
            @ac.module
            def Top(run: bool):
                producer = Producer(run)
                @ac.rule
                def write():
                    producer.set_value(7)
                write()
                shared = producer.peek()
                @ac.rule
                def extra(x: ac.u32):
                    pass
                extra(shared)
                reader = Reader(producer.outputs)
        '''):
            for target in ('reader.use', 'top.extra'):
                self.assertFalse(self.node(graph, target)['requirements'])
                self.assertTrue(any(e['source'] == 'top.write' and e['target'] == target for e in graph['edges']))
            self.assertTrue(any(a['resource'] == 'producer.result' and a['operation'] == 'read' for a in graph['computations']))
            self.assertTrue(any(p['state_dependencies'] and p['users'] for p in graph['preparations']))

    def test_module_query_keeps_method_guard_and_only_gates_its_user(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Store(flag: bool):
                state = ac.signal[ac.u32](initial=0)
                @ac.method
                def set_value(value: ac.u32):
                    state = value
                @ac.method(guard=flag)
                def peek() -> ac.u32:
                    return state
            @ac.module
            def Top(flag: bool):
                store = Store(flag)
                dependent = ac.cell[ac.u32](initial=0)
                independent = ac.cell[ac.u32](initial=0)
                @ac.rule
                def produce():
                    store.set_value(7)
                @ac.rule
                def use(value: ac.u32):
                    dependent.value = value
                @ac.rule
                def other():
                    independent.value = 1
                produce()
                shared = store.peek()
                use(shared + 1)
                other()
        '''):
            consumer = self.node(graph, 'top.use')
            self.assertFalse(consumer['requirements'])
            read, = self.accesses(graph, 'top.use', 'read')
            self.assertEqual(read['condition'], {'atom': 'config:flag'})
            self.assertTrue(any(e['source'] == 'top.produce' and e['target'] == 'top.use' for e in graph['edges']))
            self.assertEqual(self.accesses(graph, 'top.use', 'write')[0]['condition'], {'atom': 'config:flag'})
            self.assertFalse(self.accesses(graph, 'top.other', 'read'))
            self.assertFalse(self.node(graph, 'top.other')['preparations'])
            self.assertIs(self.accesses(graph, 'top.other', 'write')[0]['condition'], True)

    def test_struct_payload_and_instance_lvalue(self):
        for graph in self.variants('''
            from pycircuit import ac
            class Payload:
                count: ac.u32 = 0
            @ac.module
            def Store():
                result = ac.signal[Payload](initial=Payload())
                return result
            @ac.module
            def Top():
                store = Store()
                @ac.rule
                def update():
                    store.result = Payload(7)
                    count = store.result.count
                update()
        '''):
            self.assertEqual(self.accesses(graph, 'top.update', 'write')[0]['resource'], 'store.result')
            self.assertEqual(self.accesses(graph, 'top.update', 'read')[0]['bypass']['sources'][0]['visibility'], 'temporary')

    def test_conditional_reference_arguments_and_loop_writes(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top(flag: bool, n: ac.u32):
                first = ac.signal[ac.u32](initial=1)
                second = ac.signal[ac.u32](initial=2)
                result = ac.signal[ac.u32](initial=0)
                @ac.rule
                def copy(out: ac.signal[ac.u32], source: ac.signal[ac.u32]):
                    for i in range(n):
                        out = source + i
                    saved = out
                copy(result, first if flag else second)
                return first, second, result
        '''):
            self.assertEqual({a['resource'] for a in self.accesses(graph, 'top.copy', 'write')}, {'result'})
            reads = self.accesses(graph, 'top.copy', 'read')
            self.assertEqual({a['resource'] for a in reads}, {'first', 'second', 'result'})
            first = next(a for a in reads if a['resource'] == 'first')
            second = next(a for a in reads if a['resource'] == 'second')
            self.assertNotEqual(first['condition'], second['condition'])
            self.assertFalse(graph['edges'])

    def test_actual_ir_mutation_rebuilds_signal_relations(self):
        self.model('''
            from pycircuit import ac
            @ac.module
            def Top():
                s = ac.signal[ac.u32](initial=0)
                @ac.rule
                def set_value():
                    s = 7
                @ac.rule
                def use():
                    saved = s
                set_value()
                use()
        ''')
        saved = load(self.path / 'out/model.acir.mlir')
        changed = '\n'.join(line.replace('acir.signal.get ', 'acir.length ').replace(' -> i32', '') if 'acir.signal.get ' in line else line for line in saved.splitlines())
        self.assertNotEqual(changed, saved)
        graph = analyze(changed, self.path / 'mutated', optimize=False)
        self.assertFalse(graph['edges'])
        self.assertFalse(self.accesses(graph, 'top.use', 'read'))
        self.assertNotIn('"acir.result"', saved)
        self.assertNotIn('var.validity', saved)

    def test_invalid_constructors_outputs_and_writes(self):
        cases = [
            ('s = ac.signal[ac.u32](initial=None)', 'initial must be a payload value'),
            ('s = ac.signal[ac.u32]()', 'Signal requires initial'),
            ('s = ac.signal[ac.u32](initial=0, capacity=1)', 'Signal requires initial'),
            ('s = ac.signal[ac.u32](initial=[1, 2])', 'type mismatch'),
            ('s = ac.signal[ac.u32](initial=0)\ns = 7', 'Signal writes require a Rule transaction'),
            ('@ac.rule\ndef output() -> ac.signal[ac.u32]:\n    return 1\ns = output()', 'explicitly initialized Signal'),
            ('@ac.rule\ndef output() -> ac.u32:\n    return 1\ns = output()', 'ordinary var outputs are removed'),
            ('@ac.rule\ndef output() -> ac.var[ac.u32]:\n    return 1\ns = output()', 'ordinary var outputs are removed'),
            ('s = ac.signal[ac.u32](initial=0)\n@ac.method\ndef set_value():\n    s = 7\nset_value()', 'state effect outside a transaction'),
            ('s = ac.signal[ac.u32](initial=0)\ndef ready() -> bool:\n    s = 7\n    return True\n@ac.rule(guard=ready)\ndef use():\n    pass\nuse()', 'guard must only observe state'),
        ]
        for body, error in cases:
            with self.subTest(error=error), self.assertRaisesRegex(CompileError, error):
                self.model('from pycircuit import ac\n@ac.module\ndef Top():\n' + ''.join('    ' + line + '\n' for line in body.splitlines()))


if __name__ == '__main__': unittest.main()
