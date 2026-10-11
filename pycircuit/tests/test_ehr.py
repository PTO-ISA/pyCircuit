"""EHR stages, atomic eligibility and Queue capacity, through saved MLIR."""
import unittest
from pycircuit import analyze
from pycircuit.ir import CompileError, load
from pycircuit.analysis import satisfiable
import test_rule_graph as support
from test_examples import decode_formula


class EHRTests(unittest.TestCase):
    setUp = support.RuleGraphTests.setUp
    model = support.RuleGraphTests.model
    node = support.RuleGraphTests.node
    accesses = support.RuleGraphTests.accesses

    def variants(self, source):
        raw = self.model(source)
        optimized = analyze(load(self.path / 'out/model.acir.mlir'), self.path / 'optimized')
        for graph, path in [(raw, 'out'), (optimized, 'optimized')]:
            self.assertEqual(graph, analyze(load(self.path / path / 'model.acir.mlir'), self.path / ('reload-' + path), optimize=False))
            yield graph
        def stages(graph):
            return {(n['id'], a['resource'], a['operation'], a.get('port')) for n in graph['nodes'] for a in n['accesses']}
        self.assertEqual(raw['ehr'], optimized['ehr'])
        self.assertEqual(stages(raw), stages(optimized))
        self.assertEqual({(e['source'], e['target'], e['resource']) for e in raw['edges']}, {(e['source'], e['target'], e['resource']) for e in optimized['edges']})

    def view(self, graph, resource, aspect='value'):
        return next(v for v in graph['ehr'] if v['resource'] == resource and v['aspect'] == aspect)

    def test_unordered_methods_share_old_port_and_unused_ehr_is_saved(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Store():
                state = ac.ehr[ac.u32](initial=9)
                unused = ac.ehr[bool](initial=False)
                @ac.method
                def write():
                    state.value = 7
                @ac.method
                def read() -> ac.u32:
                    return state.value
            @ac.module
            def Top():
                store = Store()
                @ac.rule
                def put():
                    store.write()
                @ac.rule
                def get():
                    x = store.read()
                put()
                get()
        '''):
            self.assertFalse(graph['edges'])
            self.assertEqual(self.view(graph, 'store.state')['port_count'], 1)
            self.assertEqual(self.view(graph, 'store.unused')['port_count'], 1)
            read = self.accesses(graph, 'top.get', 'read')[0]
            self.assertEqual((read['view'], read['port']), ('ehr', 0))
            self.assertEqual(read['bypass']['sources'], [])
            self.assertEqual(read['bypass']['fallback'], 'old')
            self.assertTrue(all(r['initial_fn'] for r in graph['resources']))

    def test_conditional_writes_helpers_guards_and_atomic_grant(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Store():
                state = ac.ehr[bool](initial=False)
                def helper() -> bool:
                    return state.value
                @ac.method
                def write(flag: bool):
                    if flag:
                        state.value = True
                @ac.method(After='write')
                def ready() -> bool:
                    return helper()
            @ac.module
            def Top(flag: bool, allow: bool):
                store = Store()
                def enabled() -> bool:
                    return allow
                def visible() -> bool:
                    return store.ready()
                @ac.rule(guard=enabled)
                def put():
                    store.write(flag)
                @ac.rule(guard=visible)
                def get():
                    pass
                put()
                get()
        '''):
            read = self.accesses(graph, 'top.get', 'read')[0]
            write = self.accesses(graph, 'top.put', 'write')[0]
            self.assertEqual((write['port'], read['port']), (0, 1))
            self.assertEqual(read['bypass']['sources'][0]['visibility'], 'granted_transaction')
            eligibility = str(read['bypass']['sources'][0]['eligibility'])
            for predicate in ('config:flag', 'config:allow', 'granted(top.put)'):
                self.assertIn(predicate, eligibility)
            self.assertIn(('top.put', 'top.get'), {(e['source'], e['target']) for e in graph['edges']})
            mlir = load(self.path / 'out/model.acir.mlir')
            self.assertIn('port = 1', mlir)
            self.assertIn('visibility = "ehr"', mlir)

    def test_same_rule_temporary_writes_and_same_port_invisibility(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Store():
                state = ac.ehr[ac.u32](initial=2)
                @ac.method
                def first():
                    state.value = 4
                    own = state.value
                    state.value = 5
                @ac.method(After='first')
                def second() -> ac.u32:
                    return state.value
            @ac.module
            def Top(ok: bool):
                store = Store()
                @ac.rule
                def combined():
                    store.first()
                    result = store.second()
                    assert ok
                combined()
        '''):
            self.assertFalse(graph['edges'])
            reads = self.accesses(graph, 'top.combined', 'read')
            own = next(r for r in reads if r['port'] == 0)
            later = next(r for r in reads if r['port'] == 1)
            self.assertEqual(own['bypass']['sources'], [])
            sources = later['bypass']['sources']
            self.assertEqual([s['value'] for s in sources], [{'atom': 'const:4'}, {'atom': 'const:5'}])
            self.assertTrue(all(s['visibility'] == 'temporary' and s['eligibility'] is True for s in sources))
            self.assertTrue(all('config:ok' in str(a['condition']) for a in self.accesses(graph, 'top.combined', 'write')))

    def test_pure_read_order_does_not_add_visibility_and_direct_reads_use_zero(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top():
                state = ac.ehr[ac.u32](initial=0)
                prepared = state.value
                @ac.method
                def a() -> ac.u32:
                    return state.value
                @ac.method(After='a')
                def b() -> ac.u32:
                    return state.value
                @ac.rule
                def use(x: ac.u32):
                    direct = state.value
                    y = a() + b()
                use(prepared)
        '''):
            self.assertEqual(self.view(graph, 'state')['port_count'], 1)
            self.assertTrue(all(a['port'] == 0 for a in self.accesses(graph, 'top.use', 'read')))
            self.assertEqual(graph['computations'][0]['port'], 0)

    def test_nested_methods_keep_own_stage_and_lift_parent_dependency(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Inner():
                state = ac.ehr[ac.u32](initial=0)
                @ac.method
                def write():
                    state.value = 1
                @ac.method(After='write')
                def read() -> ac.u32:
                    return state.value
            @ac.module
            def Wrapper():
                inner = Inner()
                @ac.method
                def release():
                    inner.write()
                @ac.method
                def acquire() -> ac.u32:
                    return inner.read()
            @ac.module
            def Top():
                wrapper = Wrapper()
                @ac.rule
                def a():
                    wrapper.release()
                @ac.rule
                def b():
                    x = wrapper.acquire()
                a()
                b()
        '''):
            read = self.accesses(graph, 'top.b', 'read')[0]
            self.assertEqual(read['phase'], 'Module:wrapper.inner:read')
            self.assertEqual(read['port'], 1)
            self.assertTrue(any(e['contract'] == 'Module:wrapper' for e in graph['edges']))

    def test_parent_order_lifts_transactions_without_overriding_nested_stages(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Inner():
                state = ac.ehr[ac.u32](initial=0)
                @ac.method
                def put():
                    state.value = 1
                @ac.method
                def get() -> ac.u32:
                    return state.value
            @ac.module
            def Wrapper():
                inner = Inner()
                @ac.method
                def put():
                    inner.put()
                @ac.method(After='put')
                def get() -> ac.u32:
                    return inner.get()
            @ac.module
            def Top():
                wrapper = Wrapper()
                @ac.rule
                def a():
                    wrapper.put()
                @ac.rule
                def b():
                    x = wrapper.get()
                a()
                b()
        '''):
            self.assertEqual(self.view(graph, 'wrapper.inner.state')['port_count'], 1)
            read = self.accesses(graph, 'top.b', 'read')[0]
            self.assertEqual(read['port'], 0)
            self.assertEqual(read['bypass']['sources'], [])
            self.assertTrue(any(e['contract'] == 'Module:wrapper' for e in graph['edges']))

    def test_concrete_ehr_parameters_instances_and_branch_independent_stages(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Access():
                @ac.method
                def put(state: ac.ehr[ac.u32]):
                    state.value = 1
                @ac.method(After='put')
                def get(state: ac.ehr[ac.u32]) -> ac.u32:
                    return state.value
            @ac.module
            def Top(flag: bool):
                access = Access()
                left = ac.ehr[ac.u32](initial=0)
                right = ac.ehr[ac.u32](initial=0)
                @ac.rule
                def a():
                    access.put(left)
                @ac.rule
                def b():
                    x = access.get(left)
                    y = access.get(right)
                if flag:
                    a()
                else:
                    b()
        '''):
            self.assertEqual(self.view(graph, 'left')['port_count'], 2)
            self.assertEqual(self.view(graph, 'right')['port_count'], 1)
            ports = {a['resource']: a['port'] for a in self.accesses(graph, 'top.b', 'read')}
            self.assertEqual(ports, {'left': 1, 'right': 0})
            self.assertFalse(graph['edges'])

    def test_inconsistent_partial_order_is_rejected_with_evidence(self):
        with self.assertRaisesRegex(CompileError, 'state.*same port.*orders=.*same-stage='):
            self.model('''
                from pycircuit import ac
                @ac.module
                def Top():
                    state = ac.ehr[ac.u32](initial=0)
                    @ac.method
                    def write():
                        state.value = 1
                    @ac.method(After='write')
                    def ordered() -> ac.u32:
                        return state.value
                    @ac.method
                    def unordered() -> ac.u32:
                        state.value = 2
                        return state.value
                    @ac.rule
                    def use():
                        write()
                        x = ordered() + unordered()
                    use()
            ''')

    def test_queue_full_and_push_guard_share_view_for_all_capacities(self):
        for capacity in (1, 3):
            for graph in self.variants(f'''
                from pycircuit import ac
                @ac.module
                def Top(pop_ok: bool):
                    q = ac.queue[ac.u32](capacity={capacity}, initial={[9] * capacity})
                    @ac.rule
                    def take(x: ac.queue[ac.u32]):
                        if pop_ok:
                            data = x.value
                    @ac.rule
                    def put() -> ac.queue[ac.u32]:
                        return 8
                    @ac.rule
                    def observe():
                        f = q.full()
                        e = q.empty()
                        s = q.size()
                    take(q)
                    q = put()
                    observe()
            '''):
                self.assertEqual(self.view(graph, 'q', 'full')['port_count'], 2)
                read = self.accesses(graph, 'top.observe', 'query.full')[0]
                guard = self.accesses(graph, 'top.put', 'query.full')[0]
                self.assertEqual((read['port'], guard['port']), (1, 1))
                self.assertEqual(read['bypass'], guard['bypass'])
                write = self.accesses(graph, 'top.take', 'write')[0]
                self.assertEqual((write['port'], write['value_constant']), (0, False))
                eligible = str(read['bypass']['sources'][0]['eligibility'])
                for predicate in ('config:pop_ok', 'empty(q)', 'granted(top.take)'):
                    self.assertIn(predicate, eligible)
                self.assertIn(('top.take', 'top.put'), {(e['source'], e['target']) for e in graph['edges']})
                self.assertTrue(all(self.accesses(graph, 'top.observe', op)[0]['view'] == 'old' for op in ('query.empty', 'query.size')))
                resource = graph['resources'][0]
                self.assertEqual(resource['views'][0]['commit'], 'queue.effects')
                self.assertEqual(len(graph['resources']), 1)

    def test_empty_queue_has_no_payload_bypass_and_full_falls_back_without_pop(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.queue[ac.u32](capacity=3)
                ordinary = ac.cell[ac.u32](initial=0)
                @ac.rule
                def put() -> ac.queue[ac.u32]:
                    ordinary.value = 1
                    return 8
                @ac.rule
                def look():
                    full = q.full()
                    data = q.value
                    old = ordinary.value
                q = put()
                look()
        '''):
            guard = self.accesses(graph, 'top.put', 'query.full')[0]
            self.assertEqual(guard['bypass']['sources'], [])
            for read in self.accesses(graph, 'top.look', 'read'):
                self.assertEqual(read['view'], 'old')
                self.assertNotIn('port', read)
            self.assertFalse(any(v['resource'] == 'ordinary' for v in graph['ehr']))

    def test_queue_atomic_exchange_capacity_eligibility_truth_table(self):
        # Evaluate the saved eligibility formulas as a contract oracle; this is
        # not a model executor and supplies readiness/grants explicitly.
        def truth(expr, values):
            if isinstance(expr, bool): return expr
            kind, parts = next(iter(expr.items()))
            if kind == 'atom': return values[parts]
            if kind == 'not': return not truth(parts, values)
            return (all if kind == 'and' else any)(truth(p, values) for p in parts)
        for graph in self.variants(''' 
            from pycircuit import ac
            @ac.module
            def Top(enabled: bool):
                q = ac.queue[ac.u32](capacity=3, initial=[1, 2, 3])
                @ac.rule
                def exchange(x: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
                    if enabled:
                        data = x.value
                    return 8
                q = exchange(q)
        '''):
            guard = self.accesses(graph, 'top.exchange', 'query.full')[0]
            source = guard['bypass']['sources'][0]
            self.assertEqual(source['visibility'], 'temporary')
            self.assertFalse(source['value_constant'])
            self.assertFalse(graph['edges'])
            for enabled, empty, old_full, expected in [(True, False, True, False), (False, False, True, True), (True, True, False, False)]:
                values = {'config:enabled': enabled, 'empty(q)': empty}
                effective = truth(source['eligibility'], values)
                full = source['value_constant'] if effective else old_full
                self.assertEqual(full, expected)
                self.assertEqual(not full, effective or not old_full)
            self.assertEqual(self.accesses(graph, 'top.exchange', 'read')[0]['view'], 'old')

    def test_shared_preparation_full_query_lifts_to_its_users(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.queue[ac.u32](initial=1)
                available = not q.full()
                @ac.rule
                def take(x: ac.queue[ac.u32]):
                    data = x.value
                @ac.rule
                def use(x: bool):
                    assert x
                take(q)
                use(available)
        '''):
            read = self.accesses(graph, 'top.use', 'query.full')[0]
            self.assertEqual(read['port'], 1)
            self.assertEqual(read['via'], 'selection or argument computation')
            self.assertEqual(read['bypass']['sources'][0]['transaction'], 'top.take')
            self.assertIn(('top.take', 'top.use'), {(e['source'], e['target']) for e in graph['edges']})

    def test_three_ports_last_prior_writer_and_discarded_derived_cache(self):
        graph = self.model('''
            from pycircuit import ac
            @ac.module
            def Top(flag: bool):
                state = ac.ehr[ac.u32](initial=0)
                @ac.method
                def first():
                    state.value = 1
                @ac.method(After='first')
                def middle(flag: bool):
                    if flag:
                        state.value = 2
                    own = state.value
                @ac.method(After='middle')
                def last() -> ac.u32:
                    return state.value
                @ac.rule
                def use():
                    first()
                    middle(flag)
                    x = last()
                use()
        ''')
        self.assertEqual(self.view(graph, 'state')['port_count'], 3)
        reads = self.accesses(graph, 'top.use', 'read')
        read = next(r for r in reads if r['port'] == 2)
        self.assertEqual([s['port'] for s in read['bypass']['sources']], [0, 1])
        same = next(r for r in reads if r['port'] == 1)
        self.assertEqual([s['port'] for s in same['bypass']['sources']], [0])
        saved = load(self.path / 'out/model.acir.mlir')
        changed = saved.replace('port = 2 : i64', 'port = 99 : i64')
        self.assertEqual(graph, analyze(changed, self.path / 'recomputed', optimize=False))
        for old in (3, 4, 5):
            with self.assertRaisesRegex(CompileError, 'format_version 6.*recompile'):
                analyze(saved.replace('format_version = 6', 'format_version = ' + str(old)), self.path / str(old))

    def test_denied_method_keeps_its_stage_but_never_exports_a_write(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top():
                state = ac.ehr[bool](initial=False)
                def denied() -> bool:
                    return False
                @ac.method(guard=denied)
                def put():
                    state.value = True
                @ac.method(After='put')
                def get() -> bool:
                    return state.value
                @ac.rule
                def a():
                    put()
                @ac.rule
                def b():
                    x = get()
                a()
                b()
        '''):
            self.assertEqual(self.view(graph, 'state')['port_count'], 2)
            read = self.accesses(graph, 'top.b', 'read')[0]
            self.assertEqual(read['port'], 1)
            self.assertEqual(read['bypass']['sources'], [])
            self.assertFalse(self.accesses(graph, 'top.a', 'write'))
            self.assertTrue(any(a['phase'].endswith(':put') and a['port'] == 0 for a in graph['ehr_bindings']))

    def test_graph_shows_resource_ports_and_provenance(self):
        from pycircuit.draw_rules import to_dot
        from pycircuit.analysis import dot
        graph = self.model('''
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.queue[ac.u32](initial=1)
                @ac.rule
                def pop(x: ac.queue[ac.u32]):
                    data = x.value
                @ac.rule
                def push() -> ac.queue[ac.u32]:
                    return 2
                pop(q)
                q = push()
        ''')
        for text in (dot(graph), to_dot(graph)):
            for expected in ('EHR:', 'q.full', 'p0', 'p1', 'derived'):
                self.assertIn(expected, text)
        queue = next(c for c in graph['contracts'] if c['id'] == 'Queue')
        self.assertFalse(any(r['before'] == 'pop' and r['after'] == 'push' for r in queue['relations']))
        closure = next(r for r in queue['closure'] if r['before'] == 'pop' and r['after'] == 'push')
        self.assertTrue(any('guard calls' in p['origin'] for p in closure['provenance']))

    def test_self_order_ehr_cycle_is_rejected(self):
        with self.assertRaisesRegex(CompileError, 'EHR stage cycle.*state.*write'):
            self.model('''
                from pycircuit import ac
                @ac.module
                def Top():
                    state = ac.ehr[ac.u32](initial=0)
                    @ac.method(After='write')
                    def write():
                        state.value = 1
                    @ac.rule
                    def use():
                        write()
                    use()
            ''')

    def test_ehr_late_full_cannot_use_old_occupancy_exclusion(self):
        for graph in self.variants('''
            from pycircuit import ac
            @ac.module
            def Top():
                q = ac.queue[ac.u32]()
                @ac.rule
                def use():
                    if q.empty() and q.full():
                        data = q.value
                use()
        '''):
            read = self.accesses(graph, 'top.use', 'read')[0]
            self.assertTrue(satisfiable(decode_formula(read['condition'])))
            self.assertIn('ehr.full', str(read['condition']))


if __name__ == '__main__': unittest.main()
