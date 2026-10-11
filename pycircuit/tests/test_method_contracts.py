"""Bound component contracts through full Module/rule/method SSA models."""
import itertools
import unittest
from pycircuit import analyze
from pycircuit.analysis import atom, both, either, neg, satisfiable
from pycircuit.ir import CompileError, load
import test_rule_graph as support


class MethodContractTests(unittest.TestCase):
    setUp = support.RuleGraphTests.setUp
    model = support.RuleGraphTests.model
    node = support.RuleGraphTests.node
    accesses = support.RuleGraphTests.accesses

    def variants(self, source):
        raw = self.model(source)
        saved = load(self.path / 'out/model.acir.mlir')
        optimized = analyze(saved, self.path / 'optimized', optimize=True)
        for graph, directory in [(raw, 'out'), (optimized, 'optimized')]:
            rebuilt = analyze(load(self.path / directory / 'model.acir.mlir'), self.path / ('reload-' + directory), optimize=False)
            self.assertEqual(graph, rebuilt)
            yield graph

    def test_pipeline_fifo_old_read_and_capacity_ready(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.queue[ac.u32](capacity=1, initial=9)
                @ac.rule
                def exchange(x: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
                    x.value = 3
                    return x.value + 1
                @ac.rule
                def queries():
                    a = q.empty()
                    b = q.full()
                    c = q.size()
                q = exchange(q)
                queries()
        '''):
            self.assertEqual({(e['source'], e['target']) for e in graph['edges']}, {('top.exchange', 'top.queries')})
            self.assertFalse(graph['diagnostics'])
            for operation in ('read', 'revise', 'pop'):
                self.assertEqual(self.accesses(graph, 'top.exchange', operation)[0]['ready'], '!empty(q)')
                self.assertEqual(self.accesses(graph, 'top.exchange', operation)[0]['view'], 'old')
            self.assertEqual(self.accesses(graph, 'top.exchange', 'push')[0]['ready'], '!ehr.full(q,1)')
            for op in ('query.empty', 'query.full', 'query.size'):
                self.assertEqual(self.accesses(graph, 'top.queries', op)[0]['ready'], 'true')

    def test_rule_inherits_queue_ready_and_only_selected_method_guards(self):
        def decode(value):
            if isinstance(value, bool): return value
            kind, parts = next(iter(value.items()))
            if kind == 'atom': return atom(parts)
            if kind == 'not': return neg(decode(parts))
            return (both if kind == 'and' else either)(*(decode(v) for v in parts))

        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def ROB(room: bool):
                state = ac.cell[ac.u32](initial=0)
                @ac.method(guard=room)
                def allocate(value: ac.u32) -> ac.u32:
                    state.value = value
                    return value
            @ac.module
            def Station(free: bool):
                slot = ac.cell[ac.u32](initial=0)
                @ac.method(guard=free)
                def place(value: ac.u32):
                    slot.value = value
            @ac.module
            def Top(room: bool, left_free: bool, right_free: bool, choose_left: bool):
                source = ac.queue[ac.u32]()
                rob = ROB(room)
                left = Station(left_free)
                right = Station(right_free)
                renamed = ac.cell[ac.u32](initial=0)
                @ac.rule
                def dispatch(message: ac.queue[ac.u32]):
                    value = message.value
                    tag = rob.allocate(value)
                    renamed.value = tag
                    if choose_left:
                        left.place(tag)
                    else:
                        right.place(tag)
                dispatch(source)
        '''):
            node = self.node(graph, 'top.dispatch')
            self.assertIs(node['selection'], True)
            read, = [a for a in self.accesses(graph, 'top.dispatch', 'read') if a['resource'] == 'source']
            pop, = self.accesses(graph, 'top.dispatch', 'pop')
            self.assertEqual(read['ready'], '!empty(source)')
            self.assertEqual(pop['ready'], '!empty(source)')
            self.assertEqual({r['ready'] for r in node['requirements']}, {'!empty(source)'})
            completion = either(*(decode(c) for c in node['completion_conditions']))
            names = ('room', 'left_free', 'right_free', 'choose_left')
            writes = self.accesses(graph, 'top.dispatch', 'write')
            self.assertEqual({a['resource'] for a in writes if a.get('aspect', 'value') == 'value'},
                             {'rob.state', 'left.slot', 'right.slot', 'renamed'})
            for bits in itertools.product((False, True), repeat=4):
                room, left_free, right_free, choose_left = bits
                assignment = both(*(atom('config:' + name) if bit else neg(atom('config:' + name))
                                    for name, bit in zip(names, bits)))
                permitted = room and (left_free if choose_left else right_free)
                self.assertIs(satisfiable(both(assignment, completion)), permitted)
                # A late place guard cancels the earlier pop, allocation and
                # rename together; the unused station cannot veto the Rule.
                for access in [pop, *writes]:
                    expected = permitted and (access['resource'] != 'left.slot' or choose_left) and (access['resource'] != 'right.slot' or not choose_left)
                    self.assertIs(satisfiable(both(assignment, decode(access['condition']))), expected)

    def test_selected_input_and_output_ready_need_no_explicit_occupancy_guard(self):
        def decode(value):
            if isinstance(value, bool): return value
            kind, parts = next(iter(value.items()))
            if kind == 'atom': return atom(parts)
            if kind == 'not': return neg(decode(parts))
            return (both if kind == 'and' else either)(*(decode(v) for v in parts))

        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top():
                a = ac.queue[ac.u32]()
                b = ac.queue[ac.u32]()
                out = ac.queue[ac.u32](capacity=2)
                observed = ac.cell[ac.u32](initial=0)
                @ac.rule
                def merge(first: ac.queue[ac.u32], second: ac.queue[ac.u32], choose_first: bool) -> tuple[ac.queue[ac.u32], ac.signal[ac.u32]]:
                    if choose_first:
                        value = first.value
                    else:
                        value = second.value
                    return value, value
                @ac.rule
                def audit(value: ac.var[ac.u32]):
                    observed.value = value
                event = ac.signal[ac.u32](initial=0)
                out, event = merge(a, b, not a.empty())
                audit(event)
                return out, observed
        '''):
            node = self.node(graph, 'top.merge')
            self.assertIs(node['selection'], True)
            self.assertFalse(any(not g['implicit'] for g in node['guards']))
            self.assertFalse(self.node(graph, 'top.audit')['requirements'])
            conditions = either(*(decode(c) for c in node['completion_conditions']))
            ready = {'!empty(a)': neg(atom('empty(a)')), '!empty(b)': neg(atom('empty(b)')),
                     '!ehr.full(out,1)': neg(atom('ehr.full(out,1)'))}
            for requirement in node['requirements']:
                conditions = both(conditions, either(neg(decode(requirement['condition'])), ready[requirement['ready']]))
            pops = self.accesses(graph, 'top.merge', 'pop')
            push, = self.accesses(graph, 'top.merge', 'push')
            self.assertEqual(push['ready'], '!ehr.full(out,1)')
            # The selected input must contain old data and the output must have
            # space. An empty unselected input does not block.
            # The audit reads the retained Signal even if merge cannot commit.
            names = ('empty(a)', 'empty(b)', 'ehr.full(out,1)')
            for empty_a, empty_b, full_out in itertools.product((False, True), repeat=3):
                bits = (empty_a, empty_b, full_out)
                assignment = both(*(atom(name) if bit else neg(atom(name)) for name, bit in zip(names, bits)))
                permitted = (not empty_a or not empty_b) and not full_out
                self.assertIs(satisfiable(both(assignment, conditions)), permitted)
                for pop in pops:
                    selected = (pop['resource'] == 'a') != empty_a
                    self.assertIs(satisfiable(both(assignment, conditions, decode(pop['condition']))), permitted and selected)

    def test_empty_fifo_no_data_bypass_and_closure_without_pop(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.queue[ac.u32]()
                @ac.rule
                def put() -> ac.queue[ac.u32]:
                    return 7
                @ac.rule
                def observe():
                    x = q.value
                q = put()
                observe()
        '''):
            self.assertEqual([(e['source'], e['target'], e['interfaces']) for e in graph['edges']], [('top.observe', 'top.put', ['read', 'push'])])
            edge = graph['edges'][0]
            self.assertIs(edge['condition'], True)
            self.assertEqual({p['source'] for p in edge['provenance']}, {'default', 'derived'})
            self.assertGreater(len(edge['provenance']), 1)
            read = self.accesses(graph, 'top.observe', 'read')[0]
            self.assertEqual((read['view'], read['ready']), ('old', '!empty(q)'))
            self.assertFalse(self.accesses(graph, 'top.observe', 'pop'))

    def test_after_guard_and_user_closure_without_intermediate_call(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Pool():
                state = ac.cell[ac.u32](initial=0)
                def ready(enabled: bool) -> bool:
                    return enabled
                @ac.method
                def release():
                    state.value = 0
                @ac.method(After=['release'])
                def middle():
                    pass
                @ac.method(After=('middle',), guard=ready)
                def allocate(enabled: bool):
                    state.value = 1
            @ac.module
            def Caller(pool, enabled: bool):
                @ac.rule
                def acquire():
                    pool.allocate(enabled)
                acquire()
            @ac.module
            def Top(enabled: bool):
                pool = Pool()
                caller = Caller(pool, enabled)
                @ac.rule
                def free():
                    pool.release()
                free()
        '''):
            edge = next(e for e in graph['edges'] if e['source'] == 'top.free' and e['target'] == 'caller.acquire')
            self.assertEqual(edge['interfaces'], ['release', 'allocate'])
            self.assertEqual({p['source'] for p in edge['provenance']}, {'user'})
            self.assertEqual(edge['condition'], {'atom': 'config:enabled'})
            self.assertTrue(self.node(graph, 'caller.acquire')['guards'])
            self.assertTrue(all(p['loc']['line'] > 0 for p in edge['provenance']))
            self.assertTrue(self.node(graph, 'caller.acquire')['methods'][0]['identity'].endswith(':allocate'))

    def test_nested_methods_infer_outer_component_contract(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Inner():
                @ac.method
                def release():
                    pass
                @ac.method(After='release')
                def allocate():
                    pass
            @ac.module
            def Wrapper():
                inner = Inner()
                @ac.method
                def free():
                    inner.release()
                @ac.method
                def take():
                    inner.allocate()
            @ac.module
            def Top():
                wrapper = Wrapper()
                @ac.rule
                def one():
                    wrapper.free()
                @ac.rule
                def two():
                    wrapper.take()
                one()
                two()
        '''):
            edge = next(e for e in graph['edges'] if e['contract'] == 'Module:wrapper')
            self.assertEqual(edge['interfaces'], ['free', 'take'])
            self.assertEqual(edge['provenance'][0]['source'], 'derived')
            def sources(items):
                return {p['source'] for p in items} | set().union(*(sources(p.get('provenance', [])) for p in items))
            self.assertIn('user', sources(edge['provenance']))
            contract = next(c for c in graph['contracts'] if c['id'] == 'Module:wrapper')
            self.assertTrue(any(r['source'] == 'derived' for r in contract['relations']))

    def test_instances_and_mutually_exclusive_calls(self):
        source = '''
            from pycircuit import ac
            @ac.module
            def Pool():
                @ac.method
                def release():
                    pass
                @ac.method(After='release')
                def allocate():
                    pass
            @ac.module
            def Top(flag: bool):
                first = Pool()
                second = Pool()
                @ac.rule
                def one():
                    first.release()
                @ac.rule
                def two():
                    second.allocate()
                one()
                two()
        '''
        for graph in self.variants(source): self.assertFalse(graph['edges'])
        source = source.replace('second.allocate()', 'first.allocate()').replace('                one()\n                two()', '                if flag:\n                    one()\n                else:\n                    two()')
        for graph in self.variants(source): self.assertFalse(graph['edges'])

    def test_user_order_merges_with_internal_default_and_reports_cycle(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Store():
                q = ac.queue[ac.u32](initial=1)
                @ac.method(After='revise')
                def observe() -> ac.u32:
                    return q.value
                @ac.method
                def revise():
                    q.value = 2
            @ac.module
            def Top():
                store = Store()
                @ac.rule
                def one():
                    x = store.observe()
                @ac.rule
                def two():
                    store.revise()
                one()
                two()
        '''):
            self.assertTrue(graph['diagnostics'])
            self.assertEqual({e['source'] for e in graph['edges']}, {'top.one', 'top.two'})
            self.assertTrue(all(d['kind'] == 'confirmed-cycle' for d in graph['diagnostics']))
            sources = {p['source'] for e in graph['edges'] for p in e['provenance']}
            self.assertEqual(sources, {'user', 'default', 'derived'})

    def test_helper_lexical_capture_ignores_shadowing_caller(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.queue[ac.u32](initial=1)
                other = ac.queue[ac.u32](initial=2)
                def read() -> ac.u32:
                    return q.value
                @ac.rule
                def use(q: ac.queue[ac.u32]):
                    x = read()
                use(other)
        '''):
            self.assertEqual([a['resource'] for a in self.accesses(graph, 'top.use', 'read')], ['q'])
            self.assertFalse(self.accesses(graph, 'top.use', 'pop'))

    def test_helper_loop_result_carries_queue_and_signal_dependencies(self):
        for graph in self.variants('''
            from pycircuit import ac
            def sum(q: ac.queue[ac.u32], n: ac.u32, seed: ac.u32) -> ac.u32:
                result = seed
                for i in range(n):
                    result = result + q.value
                return result
            @ac.module
            def Top(n: ac.u32):
                q = ac.queue[ac.u32]()
                @ac.rule
                def put() -> ac.queue[ac.u32]:
                    return 1
                @ac.rule
                def seed() -> ac.signal[ac.u32]:
                    return 2
                @ac.rule
                def use(x: ac.u32):
                    pass
                q = put()
                x = ac.signal[ac.u32](initial=0)
                x = seed()
                total = sum(q, n, x)
                use(total)
        '''):
            self.assertIn(('top.use', 'top.put'), {(e['source'], e['target']) for e in graph['edges']})
            self.assertIn(('top.seed', 'top.use'), {(e['source'], e['target']) for e in graph['edges']})
            self.assertTrue(self.accesses(graph, 'top.use', 'read'))

    def test_select_keeps_identity_selection_and_readiness(self):
        source = '''
            from pycircuit import ac
            @ac.module
            def Top(flag: bool):
                left = ac.queue[ac.u32](initial=1)
                right = ac.queue[ac.u32](initial=2)
                @ac.rule
                def put() -> ac.queue[ac.u32]:
                    return 3
                @ac.rule
                def use(a: ac.queue[ac.u32], b: ac.queue[ac.u32], flag: bool):
                    selected = a if flag else b
                    x = selected.value
                left = put()
                use(left, right, flag)
        '''
        graphs = list(self.variants(source))
        self.assertIn('arith.select', load(self.path / 'optimized/model.acir.mlir'))
        for graph in graphs:
            reads = {a['resource']: a['condition'] for a in self.accesses(graph, 'top.use', 'read')}
            self.assertEqual(reads, {'left': {'atom': 'config:flag'}, 'right': {'not': {'atom': 'config:flag'}}})
            self.assertTrue(any(e['source'] == 'top.use' and e['target'] == 'top.put' for e in graph['edges']))

    def test_hierarchy_names_do_not_collide_with_underscores(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Leaf():
                state = ac.cell[ac.u32](initial=0)
                @ac.rule
                def update():
                    state.value = 1
                update()
            @ac.module
            def Parent():
                b = Leaf()
            @ac.module
            def Top():
                a = Parent()
                a_b = Leaf()
        '''):
            self.assertEqual({n['id'] for n in graph['nodes']}, {'a.b.update', 'a_b.update'})
            self.assertEqual({r['name'] for r in graph['resources']}, {'a.b.state', 'a_b.state'})
            self.assertEqual(len(graph['instances']), 4)

    def test_unused_internal_method_participates_in_merged_nested_closure(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Inner():
                q = ac.queue[ac.u32](initial=1)
                @ac.method
                def read() -> ac.u32:
                    return q.value
                @ac.method
                def revise():
                    q.value = 2
                @ac.method(After='revise')
                def finish():
                    pass
            @ac.module
            def Wrapper():
                inner = Inner()
                @ac.method
                def observe() -> ac.u32:
                    return inner.read()
                @ac.method
                def finish():
                    inner.finish()
            @ac.module
            def Top():
                wrapper = Wrapper()
                @ac.rule
                def one():
                    x = wrapper.observe()
                @ac.rule
                def two():
                    wrapper.finish()
                one()
                two()
        '''):
            orders = {(e['source'], e['target'], e['contract']) for e in graph['edges']}
            self.assertIn(('top.one', 'top.two', 'Module:wrapper'), orders)
            self.assertFalse(graph['diagnostics'])
            inner = next(c for c in graph['contracts'] if c['id'] == 'Module:wrapper.inner')
            self.assertIn(('read', 'finish'), {(r['before'], r['after']) for r in inner['closure']})

    def test_scalar_index_binding_does_not_invent_cross_element_order(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Store():
                entries = ac.array(ac.queue[ac.u32], shape=(2,), initial=1)
                @ac.method
                def read(index: ac.u32) -> ac.u32:
                    return entries[index].value
                @ac.method
                def revise(index: ac.u32):
                    entries[index].value = 2
            @ac.module
            def Top():
                store = Store()
                @ac.rule
                def one():
                    x = store.read(0)
                @ac.rule
                def two():
                    store.revise(1)
                one()
                two()
        '''):
            self.assertFalse(graph['edges'])

    def test_helpers_capture_the_bound_instance_after_multiple_constructions(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Store(initial: ac.u32):
                state = ac.cell[ac.u32](initial=initial)
                def ready() -> bool:
                    return state.value != 0
                def read() -> ac.u32:
                    return state.value
                @ac.method(guard=ready)
                def get() -> ac.u32:
                    return read()
            @ac.module
            def Top():
                left = Store(1)
                right = Store(2)
                @ac.rule
                def one():
                    x = left.get()
                @ac.rule
                def two():
                    x = right.get()
                one()
                two()
        '''):
            self.assertEqual({a['resource'] for a in self.accesses(graph, 'top.one', 'read')}, {'left.state'})
            self.assertEqual({a['resource'] for a in self.accesses(graph, 'top.two', 'read')}, {'right.state'})

    def test_after_validation_and_v3_recompile_diagnostic(self):
        template = '''
            from pycircuit import ac
            @ac.module
            def Top():
                @ac.method(After=AFTER)
                def m():
                    pass
                @ac.rule
                def r():
                    m()
                r()
        '''
        for invalid in ('3', '("missing",)', '("m", 4)'):
            with self.subTest(invalid=invalid), self.assertRaisesRegex(CompileError, 'After'):
                self.model(template.replace('AFTER', invalid))
        self.model(template.replace('AFTER', '()'))
        saved = load(self.path / 'out/model.acir.mlir')
        with self.assertRaisesRegex(CompileError, 'recompile'):
            analyze(saved.replace('format_version = 6', 'format_version = 3'), self.path / 'v3')


if __name__ == '__main__': unittest.main()
