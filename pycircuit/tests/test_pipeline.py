import json
import os
import subprocess
import unittest
from pycircuit import analyze
from pycircuit.ir import CompileError, load
from pycircuit.tests.support import ExecutionChecks, ROOT


class PipelineTests(ExecutionChecks, unittest.TestCase):
    def test_failed_read_never_selects_else_and_does_not_block_unrelated(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        q = ac.queue[ac.u32]()
        result = ac.cell[ac.u32](initial=0)
        independent = ac.cell[ac.u32](initial=0)
    @ac.rule
    def read() -> ac.u32:
        return q.peek()
    @ac.rule
    def set(value: ac.u32) -> None:
        result = value
    @ac.rule
    def clock() -> None:
        independent = independent + 1
    def __call__() -> None:
        value = read()
        if value == 1:
            set(10)
        else:
            set(20)
        clock()
'''
        trace, graph, out = self.run_source(source)
        self.assertEqual(len(graph['nodes']), 3)
        self.assertEqual(trace[-1]['state']['top.result'], 0)
        self.assertEqual(trace[-1]['state']['top.independent'], 4)
        self.assert_reloads(trace, out)

    def test_branch_requirements_nested_atomicity_and_repeated_pop(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        q = ac.queue[ac.u32](capacity=2, initial=[3,4])
        empty = ac.queue[ac.u32]()
        result = ac.cell[ac.u32](initial=0)
    @ac.rule
    def nested(source: ac.queue[ac.u32]) -> ac.u32:
        a = source
        b = source
        return a + b
    @ac.rule
    def transfer() -> None:
        value = nested(q)
        if value == 6:
            result = value
        else:
            result = value + empty
    def __call__() -> None:
        transfer()
'''
        trace, graph, out = self.run_source(source)
        self.assertEqual(len(graph['nodes']), 1)
        self.assertEqual(trace[0]['state']['top.q'], [4])
        self.assertEqual(trace[0]['state']['top.result'], 6)
        self.assertEqual(trace[-1]['state']['top.q'], [4])
        self.assertEqual(trace[-1]['state']['top.result'], 6)
        self.assert_reloads(trace, out)

    def test_signal_is_valid_only_when_whole_producer_fires(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        q = ac.queue[ac.u32](initial=5)
        blocked = ac.queue[ac.u32](initial=99)
        s = ac.signal[ac.u32]()
        received = ac.cell[ac.u32](initial=0)
        other = ac.cell[ac.u32](initial=0)
    @ac.rule
    def produce() -> None:
        s = q
        blocked = 10
    @ac.rule
    def accept(value: ac.u32) -> None:
        received = value
    @ac.rule
    def clock() -> None:
        other = other + 1
    def __call__() -> None:
        produce()
        value = s
        if value == 0:
            accept(10)
        else:
            accept(value)
        clock()
'''
        trace, _, out = self.run_source(source)
        self.assertEqual(trace[-1]['state']['top.q'], [5])
        self.assertEqual(trace[-1]['state']['top.received'], 0)
        self.assertEqual(trace[-1]['state']['top.other'], 4)
        self.assert_reloads(trace, out)

    def test_ehr_shared_read_fixed_view_and_next_state(self):
        trace, out = self.run_example('ehr_shared.py', 'EHRShared', cycles=4)
        self.assertEqual(trace[0]['state'], {'top.first_seen': 7, 'top.second_seen': 8, 'top.storage.state': 9})
        graph = json.loads((out / 'rule-graph.json').read_text())
        nodes = {n['txn']: n['id'].split('#')[0] for n in graph['nodes']}
        edges = {(nodes[e['from']], nodes[e['to']]) for e in graph['edges']}
        self.assertIn(('top.storage.early', 'top.first'), edges)
        self.assertIn(('top.storage.early', 'top.second'), edges)
        self.assert_reloads(trace, out)
        raw, _ = self.run_example('ehr_shared.py', 'EHRShared', cycles=4, optimize=False)
        self.assertEqual(raw, trace)

    def test_unordered_ehr_read_sees_old_value(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        e = ac.ehr[ac.u32](initial=2)
        seen = ac.cell[ac.u32](initial=0)
    @ac.rule
    def write() -> None:
        e = 10
    @ac.rule
    def read() -> None:
        seen = e
    def __call__() -> None:
        write()
        read()
'''
        trace, _, _ = self.run_source(source, 2)
        self.assertEqual(trace[0]['state']['top.seen'], 2)
        self.assertEqual(trace[1]['state']['top.seen'], 10)

    def test_effectful_return_cannot_cross_independent_transactions(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        a = ac.cell[ac.u32](initial=0)
        b = ac.cell[ac.u32](initial=0)
    @ac.rule
    def first() -> ac.u32:
        a = 1
        return 4
    @ac.rule
    def second(value: ac.u32) -> None:
        b = value
    def __call__() -> None:
        value = first()
        second(value + 1)
'''
        with self.assertRaisesRegex(CompileError, 'independent transactions'):
            self.run_source(source)

    def test_same_transaction_signal_set_get_rejected(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        signal = ac.signal[ac.u32]()
        result = ac.cell[ac.u32](initial=0)
    @ac.rule
    def bad() -> None:
        signal = 1
        result = signal
    def __call__() -> None:
        bad()
'''
        with self.assertRaisesRegex(CompileError, 'incompatible operations'):
            self.run_source(source)

    def test_branch_union_cycle_rejected(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        a = ac.queue[ac.u32](initial=1)
        b = ac.queue[ac.u32](initial=2)
    @ac.rule
    def forward() -> None:
        b = a
    @ac.rule
    def backward() -> None:
        a = b
    def __call__() -> None:
        forward()
        backward()
'''
        for calls in ('forward()\n        backward()', 'if a.empty():\n            forward()\n        else:\n            backward()'):
            with self.subTest(calls=calls), self.assertRaisesRegex(CompileError, 'branch-union.*cycle'):
                self.run_source(source.replace('forward()\n        backward()', calls))

    def test_cpu_matches_independent_isa_and_directed_retirement_cycles(self):
        # Independent RV32I interpreter: decodes instruction bits, not compiler
        # helpers or pipeline structs. No pipeline state is used as a reference.
        words = [0x00700093, 0x00500113, 0x002081b3, 0x00302023, 0x00002203, 0x402202b3]
        registers = [0] * 32
        memory = [0] * 16
        for word in words:
            opcode, rd, rs1, rs2 = word & 127, (word >> 7) & 31, (word >> 15) & 31, (word >> 20) & 31
            if opcode == 0x13:
                value = registers[rs1] + (word >> 20)
            elif opcode == 0x33:
                value = registers[rs1] - registers[rs2] if word >> 30 & 1 else registers[rs1] + registers[rs2]
            elif opcode == 0x23:
                address = registers[rs1] + ((word >> 25) << 5 | ((word >> 7) & 31))
                memory[address // 4] = registers[rs2]
                continue
            elif opcode == 0x03:
                value = memory[(registers[rs1] + (word >> 20)) // 4]
            else:
                raise AssertionError('unsupported reference instruction')
            if rd:
                registers[rd] = value & 0xffffffff
        trace, out = self.run_example('riscv_pipeline/model.py', 'Pipeline')
        self.assertEqual(trace[-1]['state']['top.rf.registers'], registers)
        self.assertEqual(trace[-1]['state']['top.memory.words'], memory)
        retire = [t['tick'] for t in trace if any(f for n, f in t['fire'].items() if 'writeback.commit' in n)]
        self.assertEqual(retire, [4, 5, 9, 13, 14, 18])
        self.assertEqual(trace[-1]['state']['top.writeback.retired'], 6)
        self.assert_reloads(trace, out)
        raw, _ = self.run_example('riscv_pipeline/model.py', 'Pipeline', optimize=False)
        self.assertEqual(raw, trace)

    def test_ooo_allocate_wait_issue_broadcast_and_ordered_retire(self):
        trace, out = self.run_example('ooo_scheduler/model.py', 'OutOfOrder')
        self.assertEqual(trace[-1]['state']['top.rf.registers'], [0, 42, 6, 7, 48, 13, 61, 0])
        self.assertEqual(trace[-1]['state']['top.commit.retired'], 4)
        tags = [t['state']['top.commit.last_tag'] for t in trace if any(f for n, f in t['fire'].items() if 'commit.commit' in n)]
        self.assertEqual(tags, [1, 2, 3, 4])
        self.assertTrue(any(t['state']['top.right.slot'][0] and not t['state']['top.right.slot'][4][0] for t in trace))
        for name in ('dispatch.dispatch', 'scheduler.issue_add', 'scheduler.issue_mul', 'cdb.broadcast'):
            self.assertTrue(any(f for t in trace for n, f in t['fire'].items() if name in n), name)
        self.assert_reloads(trace, out)
        raw, _ = self.run_example('ooo_scheduler/model.py', 'OutOfOrder', optimize=False)
        self.assertEqual(raw, trace)

    def test_mixed_optional_module_outputs(self):
        source = '''from pycircuit import ac
@ac.module
class Router:
    def __init__() -> None:
        count = ac.cell[ac.u32](initial=0)
    @ac.rule
    def route(source: ac.queue[ac.u32], take: bool) -> tuple[ac.queue[ac.u32], ac.cell[ac.u32], ac.signal[ac.u32]]:
        value = source
        count = count+1
        if take:
            return value+1, value, value
        return None, None, value
    def __call__(source: ac.queue[ac.u32], take: bool) -> tuple[ac.queue[ac.u32], ac.cell[ac.u32], ac.signal[ac.u32]]:
        return route(source,take)
@ac.module
class Top:
    def __init__(enabled: bool) -> None:
        source = ac.queue[ac.u32](initial=7)
        accepted = ac.queue[ac.u32]()
        last = ac.cell[ac.u32](initial=0)
        signal = ac.signal[ac.u32]()
        seen = ac.cell[ac.u32](initial=0)
        router = Router()
    @ac.rule
    def observe() -> None:
        seen = signal
    def __call__() -> None:
        accepted, last, signal = router(source,enabled)
        observe()
'''
        for enabled in (False, True):
            with self.subTest(enabled=enabled):
                trace, _, out = self.run_source(source, config={'enabled':enabled})
                self.assertEqual(trace[0]['state']['top.accepted'], [8] if enabled else [])
                self.assertEqual(trace[0]['state']['top.last'], 7 if enabled else 0)
                self.assertEqual(trace[-1]['state']['top.seen'], 7)
                self.assertEqual(trace[-1]['state']['top.router.count'], 1)
                self.assertEqual(trace[-1]['state']['top.source'], [])
                self.assert_reloads(trace,out)

    def test_declared_sb_order_without_shared_resources(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        a = ac.cell[ac.u32](initial=0)
        b = ac.cell[ac.u32](initial=0)
    @ac.rule(SB="second")
    def first() -> None:
        a = 1
    @ac.rule
    def second() -> None:
        b = 2
    def __call__() -> None:
        second()
        first()
'''
        trace, graph, _ = self.run_source(source)
        self.assertEqual(graph['schedule'], [1, 0])
        self.assertEqual(trace[0]['state'], {'top.a': 1, 'top.b': 2})
        for edge in graph['edges']:
            self.assertIn('source_branch', edge)
            self.assertIn('target_branches', edge)

    def test_release_then_allocate_uses_guard_and_write_same_ehr_port(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        occupied = ac.ehr[bool](initial=True)
        count = ac.cell[ac.u32](initial=0)
    @ac.rule(SB="allocate")
    def release() -> None:
        occupied = False
    @ac.rule
    def free() -> bool:
        return not occupied
    @ac.rule(guard=free)
    def allocate() -> None:
        occupied = True
        count = count + 1
    def __call__() -> None:
        allocate()
        release()
'''
        trace, graph, out = self.run_source(source)
        self.assertEqual(trace[-1]['state']['top.count'], 4)
        self.assertTrue(trace[-1]['state']['top.occupied'])
        self.assertEqual(graph['schedule'], [1, 0])
        self.assert_reloads(trace, out)
        invalid = source.replace('@ac.rule\n    def free()', '@ac.rule(SB="allocate")\n    def free()')
        with self.assertRaisesRegex(CompileError, 'incompatible operations'):
            self.run_source(invalid)

    def test_sbr_and_conflict_reject_same_atomic_path(self):
        for relation in ('SBR', 'C'):
            source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        a = ac.cell[ac.u32](initial=0)
        b = ac.cell[ac.u32](initial=0)
    @ac.rule(REL="second")
    def first() -> None:
        a = 1
    @ac.rule
    def second() -> None:
        b = 2
    @ac.rule
    def outer() -> None:
        first()
        second()
    def __call__() -> None:
        outer()
'''.replace('REL=', relation + '=')
            with self.assertRaisesRegex(CompileError, 'forbids nested calls|incompatible operations'):
                self.run_source(source)

    def test_unrelated_ehr_writer_cannot_gain_visibility_from_rank(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        state = ac.ehr[ac.u32](initial=0)
        seen = ac.cell[ac.u32](initial=0)
    @ac.rule(SB="read")
    def first() -> None:
        state = 1
    @ac.rule
    def unrelated() -> None:
        state = 2
    @ac.rule
    def read() -> None:
        seen = state
    def __call__() -> None:
        first()
        unrelated()
        read()
'''
        with self.assertRaisesRegex(CompileError, 'undeclared visibility'):
            self.run_source(source)

    def test_guarded_read_failure_and_private_rule(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        enabled = ac.cell[bool](initial=False)
        seen = ac.cell[ac.u32](initial=0)
        other = ac.cell[ac.u32](initial=0)
    @ac.rule
    def ready() -> bool:
        return enabled
    @ac.rule(guard=ready, public=False)
    def read() -> ac.u32:
        return 0
    @ac.rule
    def save(value: ac.u32) -> None:
        seen = value
    @ac.rule
    def clock() -> None:
        other = other + 1
    def __call__() -> None:
        if read() == 0:
            save(10)
        else:
            save(20)
        clock()
'''
        trace, _, _ = self.run_source(source)
        self.assertEqual(trace[-1]['state']['top.seen'], 0)
        self.assertEqual(trace[-1]['state']['top.other'], 4)

    def test_queue_array_selection_preserves_physical_identity(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        queues: ac.array(ac.queue[ac.u32], shape=(2,)) = [ac.queue[ac.u32](capacity=2, initial=7) for i in range(2)]
        index = ac.cell[ac.u32](initial=0)
        total = ac.cell[ac.u32](initial=0)
    @ac.rule
    def take() -> None:
        value = queues[index]
        again = queues[index]
        total = total + value + again
        index = index + 1
    def __call__() -> None:
        if index < 2:
            take()
'''
        trace, _, out = self.run_source(source, 3)
        self.assertEqual(trace[0]['state']['top.queues.0'], [])
        self.assertEqual(trace[0]['state']['top.queues.1'], [7])
        self.assertEqual(trace[-1]['state']['top.total'], 28)
        self.assert_reloads(trace, out)

    def test_rtl_verifier_rejects_stale_schedule(self):
        trace, _, out = self.run_source('''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        count = ac.cell[ac.u32](initial=0)
    @ac.rule
    def advance() -> None:
        count = count + 1
    def __call__() -> None:
        advance()
''')
        rtl = load(out / 'model.rtl.mlir')
        self.assertIn('schedule = [0]', rtl)
        malformed = rtl.replace('schedule = [0]', 'schedule = []')
        with self.assertRaisesRegex(CompileError, 'schedule must contain every fire'):
            analyze(malformed, out / 'invalid')

    def test_signal_has_no_rtl_state_and_none_output_keeps_other_effects(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        signal = ac.signal[ac.u32]()
        q = ac.queue[ac.u32](initial=99)
        count = ac.cell[ac.u32](initial=0)
        seen = ac.cell[ac.u32](initial=0)
    @ac.rule
    def produce() -> None:
        if count == 0:
            signal = 8
        count = count + 1
    @ac.rule
    def receive() -> None:
        seen = signal
    @ac.rule
    def omitted() -> ac.queue[ac.u32]:
        count = count + 1
        return None
    def __call__() -> None:
        produce()
        receive()
'''
        trace, _, out = self.run_source(source)
        self.assertEqual(trace[-1]['state']['top.count'], 4)
        self.assertEqual(trace[-1]['state']['top.seen'], 8)
        rtl = load(out / 'model.rtl.mlir')
        self.assertNotIn('kind = "signal"', rtl)
        source = source.replace('        produce()\n        receive()', '        q = omitted()')
        trace, _, _ = self.run_source(source)
        self.assertEqual(trace[-1]['state']['top.count'], 4)
        self.assertEqual(trace[-1]['state']['top.q'], [99])

    def test_native_passes_reload_and_cli_stages(self):
        trace, _, out = self.run_source('''from pycircuit import ac
@ac.module
class Top:
    def __init__(limit: ac.u32 = 3) -> None:
        count = ac.cell[ac.u32](initial=0)
    @ac.rule
    def advance() -> None:
        count = count + 1
    def __call__() -> None:
        if count < limit:
            advance()
''')
        self.assertEqual(trace[-1]['state']['top.count'], 3)
        native = subprocess.run([os.environ['ACPY_MLIR_OPT'], str(out / 'model.source.mlir'),
                                 '--acir-extract-transactions', '--acir-lower-rtl'], capture_output=True, text=True)
        self.assertEqual(native.returncode, 0, native.stderr)
        analyze(native.stdout, out / 'native', cycles=4)
        self.assertEqual(trace, json.loads((out / 'native/rtl-trace.json').read_text())['trace'])
        with self.assertRaisesRegex(CompileError, 'frozen'):
            analyze(load(out / 'model.transaction.mlir'), out / 'changed', config={'limit': 5})
        cli = subprocess.run(['python3', '-m', 'pycircuit', 'run', str(out / 'model.rtl.mlir'),
                              '--output', str(out / 'cli'), '--cycles', '4'], cwd=ROOT, capture_output=True, text=True)
        self.assertEqual(cli.returncode, 0, cli.stderr)
        self.assertEqual(trace, json.loads((out / 'cli/rtl-trace.json').read_text())['trace'])

    def test_annotated_local_shadows_member(self):
        trace, _, _ = self.run_source('''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        count = ac.cell[ac.u32](initial=1)
        seen = ac.cell[ac.u32](initial=0)
    @ac.rule
    def advance() -> None:
        count: ac.u32 = 10
        count = count + 1
        seen = count
    def __call__() -> None:
        advance()
''', 2)
        self.assertEqual(trace[-1]['state']['top.count'], 1)
        self.assertEqual(trace[-1]['state']['top.seen'], 11)

    def test_passive_modules_share_outer_atomicity_and_preserve_instances(self):
        source = '''from pycircuit import ac
@ac.module
class Counter:
    def __init__(step: ac.u32) -> None:
        count = ac.cell[ac.u32](initial=1)
    @ac.rule
    def ready() -> bool:
        return count < 7
    @ac.rule(guard=ready)
    def advance() -> None:
        count = count + step
    @ac.rule
    def read() -> ac.u32:
        return count
@ac.module
class Top:
    def __init__() -> None:
        a = Counter(2)
        b = Counter(5)
        unused = Counter(1)
        seen = ac.cell[ac.u32](initial=0)
    @ac.rule
    def advance() -> None:
        a.advance()
        b.advance()
        seen = a.read() + b.read()
    def __call__() -> None:
        advance()
'''
        trace, graph, out = self.run_source(source, 4)
        self.assertEqual(len(graph['nodes']), 1)
        self.assertEqual(trace[0]['state'], {'top.a.count': 3, 'top.b.count': 6,
                                             'top.unused.count': 1, 'top.seen': 2})
        self.assertEqual(trace[1]['state'], {'top.a.count': 5, 'top.b.count': 11,
                                             'top.unused.count': 1, 'top.seen': 9})
        self.assertEqual(trace[2]['state'], trace[1]['state'])
        self.assertEqual(list(trace[2]['fire'].values()), [False])
        self.assert_reloads(trace, out)

    def test_passive_top_does_not_automatically_schedule_rules(self):
        trace, graph, out = self.run_source('''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        count = ac.cell[ac.u32](initial=3)
    @ac.rule
    def advance() -> None:
        count = count + 1
''', 3)
        self.assertEqual(graph['nodes'], [])
        for tick in trace:
            self.assertEqual(tick['state'], {'top.count': 3})
            self.assertEqual(tick['fire'], {})
        self.assertNotIn('acir.body ', load(out / 'model.source.mlir'))
        self.assert_reloads(trace, out)

    def test_explicit_module_pipeline_destinations_and_forwarding(self):
        source = '''from pycircuit import ac
@ac.module
class Source:
    def __init__() -> None:
        produced = ac.cell[ac.u32](initial=0)
    @ac.rule
    def produce(input: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        value = input
        produced = produced + 1
        return value
    def __call__(input: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        if produced < 2:
            return produce(input)
@ac.module
class Stage:
    def __init__() -> None:
        pass
    @ac.rule
    def transfer(input: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return input + 10
    def __call__(input: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return transfer(input)
@ac.module
class Wrapper:
    def __init__() -> None:
        stage = Stage()
    def __call__(input: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return stage(input)
@ac.module
class Top:
    def __init__() -> None:
        input = ac.queue[ac.u32](capacity=2, initial=[1,2])
        if_id = ac.queue[ac.u32](initial=3)
        id_ex = ac.queue[ac.u32](initial=4)
        cycle = ac.cell[ac.u32](initial=0)
        total = ac.cell[ac.u32](initial=0)
        producer = Source()
        decoder = Wrapper()
    @ac.rule
    def drain() -> None:
        total = total + id_ex
    @ac.rule
    def clock() -> None:
        cycle = cycle + 1
    def __call__() -> None:
        if_id = producer(input)
        id_ex = decoder(if_id)
        if cycle != 1:
            drain()
        clock()
'''
        trace, graph, out = self.run_source(source, 6)
        self.assertEqual(trace[0]['state']['top.if_id'], [1])
        self.assertEqual(trace[0]['state']['top.id_ex'], [13])
        self.assertEqual(trace[0]['state']['top.total'], 4)
        self.assertEqual(trace[1]['state'], dict(trace[0]['state'], **{'top.cycle': 2}))
        self.assertEqual(trace[1]['state']['top.producer.produced'], 1)
        self.assertEqual(trace[-1]['state']['top.total'], 40)
        self.assertEqual(trace[-1]['state']['top.input'], [])
        self.assertEqual(graph['schedule'][:3], [2,1,0])
        self.assertEqual(trace[-1]['state']['top.producer.produced'], 2)
        self.assertEqual(set(trace[-1]['state']), {'top.input', 'top.if_id', 'top.id_ex',
                                                   'top.cycle', 'top.total', 'top.producer.produced'})
        self.assert_reloads(trace, out)
        raw, _, _ = self.run_source(source, 6, optimize=False)
        self.assertEqual(trace, raw)
        explicit_parameter = source.replace(
            '    def __call__(input: ac.queue[ac.u32]) -> ac.queue[ac.u32]:\n        return stage(input)',
            '    def __call__(input: ac.queue[ac.u32], out: ac.queue[ac.u32]) -> None:\n        out = stage(input)')
        explicit_parameter = explicit_parameter.replace('        id_ex = decoder(if_id)', '        decoder(if_id, id_ex)')
        explicit, _, explicit_out = self.run_source(explicit_parameter, 6)
        self.assertEqual(trace, explicit)
        self.assert_reloads(explicit, explicit_out)

    def test_nested_module_paths_and_failed_condition(self):
        source = '''from pycircuit import ac
@ac.module
class Stage:
    def __init__() -> None:
        count = ac.cell[ac.u32](initial=0)
    @ac.rule
    def produce() -> ac.queue[ac.u32]:
        count = count + 1
        return 7
    def __call__() -> ac.queue[ac.u32]:
        return produce()
@ac.module
class Middle:
    def __init__() -> None:
        stage = Stage()
    def __call__() -> ac.queue[ac.u32]:
        return stage()
@ac.module
class Top:
    def __init__() -> None:
        source = ac.queue[bool]()
        out = ac.queue[ac.u32](capacity=8)
        fallback = ac.cell[ac.u32](initial=0)
        cycle = ac.cell[ac.u32](initial=0)
        middle = Middle()
    @ac.rule
    def otherwise() -> None:
        fallback = fallback + 1
    @ac.rule
    def clock() -> None:
        cycle = cycle + 1
    def __call__() -> None:
        if CONDITION:
            out = middle()
        else:
            otherwise()
        clock()
'''
        for condition, count, fallback in [('False',0,3), ('source.peek()',0,0), ('cycle == 1',1,2)]:
            with self.subTest(condition=condition):
                traces=[]
                for opt in (False,True):
                    trace, _, out = self.run_source(source.replace('CONDITION',condition),3,optimize=opt)
                    self.assertEqual(trace[-1]['state']['top.middle.stage.count'],count)
                    self.assertEqual(trace[-1]['state']['top.out'],[7]*count)
                    self.assertEqual(trace[-1]['state']['top.fallback'],fallback)
                    self.assertEqual(trace[-1]['state']['top.cycle'],3)
                    self.assert_reloads(trace,out)
                    traces.append(trace)
                self.assertEqual(traces[0], traces[1])

    def test_mutual_static_bindings_precede_behavior_generation(self):
        trace,_,out=self.run_source('''from pycircuit import ac
@ac.module
class A:
    def __init__() -> None:
        state = ac.cell[ac.u32](initial=1)
    @ac.rule
    def read() -> ac.u32:
        return state
    @ac.rule
    def advance() -> None:
        state = b.read()+1
    def __call__(b: B) -> None:
        advance()
@ac.module
class B:
    def __init__() -> None:
        state = ac.cell[ac.u32](initial=2)
    @ac.rule
    def read() -> ac.u32:
        return state+a.read()
    @ac.rule
    def advance() -> None:
        state = a.read()
    def __call__(a: A) -> None:
        advance()
@ac.module
class Top:
    def __init__() -> None:
        a = A()
        b = B()
    def __call__() -> None:
        a(b)
        b(a)
''',1)
        self.assertEqual(trace[0]['state']['top.a.state'],4)
        self.assertEqual(trace[0]['state']['top.b.state'],1)
        self.assert_reloads(trace,out)
