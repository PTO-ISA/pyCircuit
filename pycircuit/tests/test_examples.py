"""Compile real review models and assert their transaction/connection contracts.

These tests inspect saved MLIR and static relations; they do not simulate CPUs.
"""
import itertools
import random
import re
from pathlib import Path
import shutil
import tempfile
import unittest

from pycircuit import analyze, compile_source
from pycircuit.analysis import both, either, neg, satisfiable, simplify_path
from pycircuit.ir import load

ROOT = Path(__file__).resolve().parents[2]
EXAMPLES = ROOT / 'pycircuit/examples'
MODELS = {
    'signal': ('signal_link.py', 'SignalLink'),
    'ehr': ('ehr_slots.py', 'EHRSlots'),
    'mixed': ('mixed_ports.py', 'MixedPorts'),
    'shared': ('shared_state.py', 'SharedState'),
    'dual': ('dual_channel.py', 'DualChannel'),
    'pipeline': ('riscv_pipeline/model.py', 'Pipeline'),
    'ooo': ('ooo_scheduler/model.py', 'OutOfOrder'),
    'cell_feedback': ('dependency_cycles.py', 'CellFeedback'),
    'exclusive': ('dependency_cycles.py', 'ExclusiveRing'),
    'ring': ('dependency_cycles.py', 'QueueRing'),
}


def decode_formula(value):
    if isinstance(value, bool):
        return value
    kind, parts = next(iter(value.items()))
    if kind == 'atom':
        return ('atom', parts)
    if kind == 'not':
        return neg(decode_formula(parts))
    return (both if kind == 'and' else either)(*(decode_formula(v) for v in parts))


def relations(graph):
    return {(e['source'], e['target'], e['kind'], e['resource']) for e in graph['edges']}


def effects(graph):
    return {(n['id'], a['operation'], a['resource'], a['role'], a['view'])
            for n in graph['nodes'] for a in n['accesses']}


class ExampleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        base = ROOT / 'pycircuit/builds/benchmarks/rule-graph/tests'
        base.mkdir(parents=True, exist_ok=True)
        cls.temp = tempfile.TemporaryDirectory(prefix='examples-', dir=base)
        cls.addClassCleanup(cls.temp.cleanup)
        cls.path = Path(cls.temp.name)
        cls.graphs = {}
        # Compile a disposable copy so subsequent reload really has no sources.
        copied = cls.path / 'sources'
        shutil.copytree(EXAMPLES, copied, ignore=shutil.ignore_patterns('__pycache__'))
        for key, (filename, top) in MODELS.items():
            model = compile_source(copied / filename, top)
            cls.graphs[key] = {
                opt: analyze(model, cls.path / key / str(opt), optimize=opt)
                for opt in (False, True)
            }
        shutil.rmtree(copied)

    def graph(self, key):
        return self.graphs[key][True]

    def accesses(self, key, rule, operation, resource=None):
        node = next(n for n in self.graph(key)['nodes'] if n['id'] == rule)
        return [a for a in node['accesses'] if a['operation'] == operation
                and (resource is None or a['resource'] == resource)
                and (operation != 'write' or a.get('aspect', 'value') == 'value')]

    def test_mixed_ports_atomic_effects_and_optional_outputs(self):
        graph = self.graph('mixed')
        self.assertEqual({n['id'] for n in graph['nodes']}, {'router.route', 'observer.sample'})
        self.assertEqual(len(self.accesses('mixed', 'router.route', 'pop', 'source')), 1)
        ledger = self.accesses('mixed', 'router.route', 'write', 'ledger.observed')[0]
        cell = self.accesses('mixed', 'router.route', 'write', 'router.last_accepted')[0]
        push = self.accesses('mixed', 'router.route', 'push', 'router.accepted')[0]
        self.assertEqual(cell['condition'], push['condition'])
        # Filtering omits output effects while consuming and recording the input.
        self.assertTrue(satisfiable(both(decode_formula(ledger['condition']),
                                         neg(decode_formula(push['condition'])))))
        sample = next(n for n in graph['nodes'] if n['id'] == 'observer.sample')
        self.assertFalse(sample['requirements'])
        self.assertIn(('router.route', 'observer.sample', 'method.order', 'router.result'), relations(graph))
        self.assertIn('!acir.signal', (self.path / 'mixed/True/model.acir.mlir').read_text())

    def test_signal_link_methods_rule_and_control(self):
        graph = self.graph('signal')
        self.assertEqual({n['id'] for n in graph['nodes']}, {'top.configure', 'producer.advance', 'reader.observe'})
        self.assertTrue(self.accesses('signal', 'top.configure', 'write', 'producer.offset'))
        self.assertTrue(self.accesses('signal', 'producer.advance', 'write', 'producer.result'))
        self.assertFalse(self.accesses('signal', 'reader.observe', 'pop'))
        self.assertIn(('producer.advance', 'reader.observe', 'method.order', 'producer.result'), relations(graph))
        self.assertFalse(next(n for n in graph['nodes'] if n['id'] == 'reader.observe')['requirements'])

    def test_shared_instances_consumption_roles_and_mutual_exclusion(self):
        for rule, resource in [('left.transfer', 'first'), ('right.transfer', 'second'),
                               ('independent.transfer', 'third')]:
            pops = self.accesses('shared', rule, 'pop')
            self.assertEqual([a['resource'] for a in pops], [resource])
            self.assertGreaterEqual(len(pops[0]['locations']), 2)
        left = self.accesses('shared', 'left.transfer', 'write')[0]
        right = self.accesses('shared', 'right.transfer', 'write')[0]
        third = self.accesses('shared', 'independent.transfer', 'write')[0]
        self.assertEqual(left['resource'], right['resource'])
        self.assertNotEqual(left['resource'], third['resource'])
        self.assertFalse(satisfiable(both(decode_formula(left['condition']), decode_formula(right['condition']))))

    def test_pipeline_stage_connections_and_cross_module_effects(self):
        graph = self.graph('pipeline')
        self.assertEqual({n['id'] for n in graph['nodes']}, {
            'fetch.fetch', 'decode_stage.read_operands', 'execute.execute',
            'memory_stage.access', 'writeback.commit',
        })
        for consumer, producer, resource in [
            ('decode_stage.read_operands', 'fetch.fetch', 'fetch.out'),
            ('execute.execute', 'decode_stage.read_operands', 'decode_stage.out'),
            ('memory_stage.access', 'execute.execute', 'execute.out'),
            ('writeback.commit', 'memory_stage.access', 'retirement'),
        ]:
            self.assertEqual(len(self.accesses('pipeline', consumer, 'pop', resource)), 1)
            self.assertIn((consumer, producer, 'method.order', resource), relations(graph))
        self.assertTrue(self.accesses('pipeline', 'decode_stage.read_operands', 'write', 'rf.pending'))
        self.assertTrue(self.accesses('pipeline', 'writeback.commit', 'write', 'rf.pending'))
        self.assertTrue(self.accesses('pipeline', 'writeback.commit', 'write', 'rf.registers'))
        self.assertTrue(self.accesses('pipeline', 'memory_stage.access', 'write', 'memory.words'))
        self.assertTrue(self.accesses('pipeline', 'memory_stage.access', 'read', 'memory.words'))
        observations = self.accesses('pipeline', 'decode_stage.read_operands', 'read', 'fetch.out')
        self.assertEqual({a['role'] for a in observations}, {'observe', 'consume'})
        self.assertTrue(self.accesses('pipeline', 'decode_stage.read_operands', 'query.empty', 'retirement'))
        self.assertFalse(self.accesses('pipeline', 'decode_stage.read_operands', 'query.empty', 'fetch.out'))
        decode_calls = [r for r in graph['preparations'] if r['boundary'] == 'call' and r['function'] == 'decode_stage_Work']
        self.assertTrue(decode_calls)
        self.assertTrue(all(r['users'] for r in decode_calls))
        self.assertTrue(all(any(req['ready'] == '!empty(fetch.out)' and req['condition'] is True
                                for req in r['requirements']) for r in decode_calls))
        # Only the Module calls decode; the Rule consumes source without decoding
        # it again. The shared helper definition is retained once in actual IR.
        mlir_path = self.path / 'pipeline/True/model.acir.mlir'
        self.assertEqual(len(re.findall(r'definition = "[^"]*decode"', mlir_path.read_text())), 1)
        self.assertFalse(any(e['kind'] == 'queue.data' for e in graph['edges']))
        for rule in ['decode_stage.read_operands', 'execute.execute', 'memory_stage.access', 'writeback.commit']:
            for read in self.accesses('pipeline', rule, 'read'):
                if read['role'] == 'consume':
                    self.assertEqual(read['view'], 'old')
                    self.assertEqual(read['ready'], '!empty(' + read['resource'] + ')')
        mlir = (self.path / 'pipeline/True/model.acir.mlir').read_text()
        self.assertIn('arith.subi', mlir)
        self.assertIn('arith.addi', mlir)
        self.assertIn('acir.update ', mlir)

    def test_ooo_dispatch_wakeup_and_commit_are_outer_transactions(self):
        graph = self.graph('ooo')
        self.assertEqual({n['id'] for n in graph['nodes']}, {
            'dispatch.dispatch', 'scheduler.issue_add', 'scheduler.issue_mul',
            'adder.execute', 'multiplier.accept', 'multiplier.advance',
            'cdb.broadcast', 'commit.commit', 'audit.observe',
        })
        slot_names = {'rob.slot0.state', 'rob.slot1.state', 'rob.slot2.state', 'rob.slot3.state'}
        for rule in ['dispatch.dispatch', 'cdb.broadcast', 'commit.commit']:
            writes = {a['resource'] for a in self.accesses('ooo', rule, 'write')}
            self.assertTrue(slot_names <= writes)
        for rule in ['dispatch.dispatch', 'cdb.broadcast', 'scheduler.issue_add', 'scheduler.issue_mul']:
            writes = {a['resource'] for a in self.accesses('ooo', rule, 'write')}
            self.assertTrue({'left.slot', 'right.slot'} <= writes)
        dispatch_writes = {a['resource'] for a in self.accesses('ooo', 'dispatch.dispatch', 'write')}
        commit_writes = {a['resource'] for a in self.accesses('ooo', 'commit.commit', 'write')}
        self.assertIn('rf.renamed', dispatch_writes)
        self.assertTrue({'rf.registers', 'rf.committed', 'rob.head'} <= commit_writes)
        self.assertNotIn('rf.registers', dispatch_writes)
        self.assertNotIn('rf.renamed', commit_writes)
        audit = next(n for n in graph['nodes'] if n['id'] == 'audit.observe')
        self.assertFalse(audit['requirements'])
        self.assertIn(('cdb.broadcast', 'audit.observe', 'method.order', 'cdb.event'), relations(graph))

    def test_ooo_dispatch_relies_on_implicit_ready_and_nested_guards(self):
        for graph in self.graphs['ooo'].values():
            node = next(n for n in graph['nodes'] if n['id'] == 'dispatch.dispatch')
            self.assertEqual(decode_formula(node['selection']),
                             both(('atom', 'empty(adder.out)'), ('atom', 'empty(multiplier.out)')))
            source = [a for a in node['accesses'] if a['resource'] == 'source' and a['operation'] in ('read', 'pop')]
            self.assertEqual({a['operation'] for a in source}, {'read', 'pop'})
            self.assertTrue(all(a['ready'] == '!empty(source)' for a in source))
            self.assertTrue(any(r['ready'] == '!empty(source)' for r in node['requirements']))
            methods = node['methods']
            self.assertFalse(any(m['interface'] == 'has_room' for m in methods))
            allocate = next(m for m in methods if m['component'] == 'rob' and m['interface'] == 'allocate')
            self.assertTrue(any(g['function'] == allocate['function'] and g['name'] == 'room' for g in node['guards']))
            places = [m for m in methods if m['interface'] == 'place']
            self.assertEqual({m['component'] for m in places}, {'left', 'right'})
            for place in places:
                self.assertTrue(any(g['function'] == place['function'] and g['name'] == 'free' for g in node['guards']))
            self.assertIs(satisfiable(both(*(decode_formula(m['condition']) for m in places))), False)

    def test_redundant_occupancy_and_issue_guards_are_supplied_by_accesses(self):
        for graph in self.graphs['mixed'].values():
            route = next(n for n in graph['nodes'] if n['id'] == 'router.route')
            self.assertIs(route['selection'], True)
            self.assertFalse(any(not g['implicit'] for g in route['guards']))
            self.assertTrue(any(r['ready'] == '!empty(source)' for r in route['requirements']))
        for graph in self.graphs['ooo'].values():
            broadcast = next(n for n in graph['nodes'] if n['id'] == 'cdb.broadcast')
            self.assertIs(broadcast['selection'], True)
            self.assertFalse(broadcast['guards'])
            for queue in ('adder.out', 'multiplier.out'):
                self.assertTrue(any(r['ready'] == '!empty(' + queue + ')' for r in broadcast['requirements']))
            for rule in ('scheduler.issue_add', 'scheduler.issue_mul'):
                issue = next(n for n in graph['nodes'] if n['id'] == rule)
                self.assertIs(issue['selection'], True)
                takes = [m for m in issue['methods'] if m['interface'] == 'take']
                self.assertEqual({m['component'] for m in takes}, {'left', 'right'})
                self.assertIs(satisfiable(both(*(decode_formula(m['condition']) for m in takes))), False)
                for take in takes:
                    self.assertTrue(any(g['name'] == 'issuable' and g['function'] == take['function']
                                        for g in issue['guards']))

    def test_ooo_branch_consumption_and_latency_output_omission(self):
        pops = self.accesses('ooo', 'cdb.broadcast', 'pop')
        self.assertEqual({a['resource'] for a in pops}, {'adder.out', 'multiplier.out'})
        self.assertFalse(satisfiable(both(*(decode_formula(a['condition']) for a in pops))))
        for record in pops:
            self.assertEqual(record['ready'], '!empty(' + record['resource'] + ')')
        advance = 'multiplier.advance'
        push = self.accesses('ooo', advance, 'push', 'multiplier.out')[0]
        countdown = self.accesses('ooo', advance, 'write', 'multiplier.remaining')[0]
        self.assertFalse(satisfiable(both(decode_formula(push['condition']),
                                          decode_formula(countdown['condition']))))
        self.assertTrue(self.accesses('ooo', 'multiplier.accept', 'pop', 'scheduler.mul_requests'))
        self.assertFalse(self.accesses('ooo', advance, 'pop'))
        self.assertFalse(any(e['kind'] == 'queue.data' for e in self.graph('ooo')['edges']))

    def test_old_cell_feedback_exclusive_ring_and_confirmed_queue_cycle(self):
        self.assertFalse(self.graph('cell_feedback')['edges'])
        for graph in [self.graph('cell_feedback'), self.graph('exclusive')]:
            self.assertFalse(graph['diagnostics'])
        diagnostics = self.graph('ring')['diagnostics']
        self.assertTrue(diagnostics)
        self.assertTrue(all(d['kind'] == 'confirmed-cycle' for d in diagnostics))
        self.assertEqual(set(diagnostics[0]['rules']), {'top.forward', 'top.backward'})

    def test_all_examples_optimize_preserves_contract_and_reload_needs_no_python(self):
        self.assertFalse((self.path / 'sources').exists())
        for key in MODELS:
            with self.subTest(model=key):
                raw, optimized = self.graphs[key][False], self.graphs[key][True]
                self.assertEqual(effects(raw), effects(optimized))
                self.assertEqual(relations(raw), relations(optimized))
                for optimize in (False, True):
                    saved = load(self.path / key / str(optimize) / 'model.acir.mlir')
                    self.assertNotIn('acir.analysis', saved)
                    self.assertNotIn('acir.contracts', saved)
                    reloaded = analyze(saved, self.path / key / ('reload-' + str(optimize)),
                                       optimize=optimize)
                    self.assertEqual(self.graphs[key][optimize], reloaded)
                if key != 'ring':
                    self.assertFalse(optimized['diagnostics'])
        # Full OoO summaries should remain practical to inspect after CFG merges.
        self.assertLess((self.path / 'ooo/True/rule-graph.json').stat().st_size, 2_000_000)


class ConditionRegressionTests(unittest.TestCase):
    def test_nested_branch_simplification_preserves_truth_table(self):
        a, b, c = [('atom', name) for name in ('a', 'b', 'c')]
        original = ('or', ('and', a, b), ('and', ('not', a), b, c))
        simplified = either(both(a, b), both(neg(a), b, c))

        def evaluate(expr, values):
            if isinstance(expr, bool):
                return expr
            if expr[0] == 'atom':
                return values[expr[1]]
            if expr[0] == 'not':
                return not evaluate(expr[1], values)
            return (all if expr[0] == 'and' else any)(evaluate(v, values) for v in expr[1:])

        for bits in itertools.product((False, True), repeat=3):
            values = dict(zip(('a', 'b', 'c'), bits))
            self.assertEqual(evaluate(original, values), evaluate(simplified, values))
            self.assertEqual(evaluate(neg(original), values), not evaluate(original, values))
        self.assertEqual(either(both(a, b), both(a, neg(b))), a)
        self.assertEqual(both(a, either(a, b)), a)
        self.assertEqual(either(a, both(neg(a), b)), either(a, b))

    def test_boolean_rewrites_against_exhaustive_assignments(self):
        rng = random.Random(42)
        atoms = [('atom', name) for name in ('a', 'b', 'c')]

        def expression(depth):
            if depth == 0:
                return rng.choice(atoms + [True, False])
            kind = rng.choice(('and', 'or', 'not'))
            return (kind, expression(depth - 1)) if kind == 'not' else (
                kind, expression(depth - 1), expression(depth - 1))

        def normalize(expr):
            if isinstance(expr, bool) or expr[0] == 'atom':
                return expr
            if expr[0] == 'not':
                return neg(normalize(expr[1]))
            return (both if expr[0] == 'and' else either)(*(normalize(v) for v in expr[1:]))

        def evaluate(expr, values):
            if isinstance(expr, bool):
                return expr
            if expr[0] == 'atom':
                return values[expr[1]]
            if expr[0] == 'not':
                return not evaluate(expr[1], values)
            return (all if expr[0] == 'and' else any)(evaluate(v, values) for v in expr[1:])

        for _ in range(100):
            original = expression(4)
            rewritten = normalize(original)
            compact = simplify_path(rewritten)
            for bits in itertools.product((False, True), repeat=3):
                values = dict(zip(('a', 'b', 'c'), bits))
                self.assertEqual(evaluate(original, values), evaluate(rewritten, values))
                self.assertEqual(evaluate(original, values), evaluate(compact, values))
