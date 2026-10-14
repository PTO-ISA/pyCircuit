"""Array values, selection domains and physical resource identities."""
import unittest
import test_pipeline as pipeline
from pycircuit.ir import CompileError

class ArrayTests(unittest.TestCase):
    setUp = pipeline.PipelineTests.setUp
    tearDown = pipeline.PipelineTests.tearDown
    run_source = pipeline.PipelineTests.run_source
    assert_reloads = pipeline.PipelineTests.assert_reloads

    def test_ehr_array_shared_view_and_signal_selection(self):
        source = '''from pycircuit import ac
Data = ac.array(ac.u32, shape=(2,))
@ac.module
class Top:
    def __init__() -> None:
        states: ac.array(ac.ehr[ac.u32], shape=(2,)) = [ac.ehr[ac.u32](initial=1) for i in range(2)]
        signals: ac.array(ac.signal[ac.u32], shape=(2,)) = [ac.signal[ac.u32]() for i in range(2)]
        position = ac.cell[ac.u32](initial=1)
        first = ac.cell[ac.u32](initial=0)
        second = ac.cell[ac.u32](initial=0)
        seen = ac.cell[ac.u32](initial=0)
    @ac.rule(SB=('read','late'))
    def early() -> None:
        values: Data = [0,1]
        mask = values.match(lambda item: item == 1)
        states[mask] = 7
        signals[mask] = 11
    @ac.rule(SB='late')
    def read() -> ac.u32:
        return states[position]
    @ac.rule
    def late() -> None:
        states[position] = 9
    @ac.rule
    def save_first(value: ac.u32) -> None:
        first = value
    @ac.rule
    def save_second(value: ac.u32) -> None:
        second = value+1
    @ac.rule
    def observe() -> None:
        seen = signals[position]
    def __call__() -> None:
        late()
        shared = read()
        save_first(shared)
        save_second(shared)
        observe()
        early()
'''
        traces=[]
        for optimize in (False,True):
            trace,_,out=self.run_source(source,2,optimize=optimize)
            state=trace[-1]['state']
            self.assertEqual([state['top.states.0'],state['top.states.1'],state['top.first'],state['top.second'],state['top.seen']],[1,9,7,8,11])
            self.assert_reloads(trace,out)
            traces.append(trace)
        self.assertEqual(*traces)

    def test_shape_and_storage_diagnostics(self):
        from pycircuit import compile_source
        cases = [
            ('a = ac.cell[ac.array(ac.u32, shape=(2,2))](initial=[[1,2],[3]])','exactly match shape'),
            ('a = ac.cell[ac.array(ac.u32, shape=(0,))](initial=[])','positive'),
            ('a = ac.cell[ac.array(ac.array(ac.u32, shape=(2,)), shape=(2,))](initial=[])','array of array'),
            ('a = ac.queue[ac.array(ac.u32, shape=(2,))](initial=[1,2])','preloading'),
            ('a = ac.queue[ac.cell[ac.u32]]()','complete payload'),
        ]
        for member,diagnostic in cases:
            with self.subTest(member=member):
                path=self.directory/'invalid.py'
                path.write_text('from pycircuit import ac\n@ac.module\nclass Top:\n    def __init__() -> None:\n        '+member+'\n')
                with self.assertRaisesRegex(CompileError,diagnostic): compile_source(path,'Top')

    def test_reloaded_mlir_checks_shapes_and_update_paths(self):
        from pycircuit import analyze
        from pycircuit.ir import load
        _,_,out=self.run_source('''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        state = ac.cell[ac.array(ac.u32, shape=(2,))](initial=[1,2])
        position = ac.cell[ac.u32](initial=1)
    @ac.rule
    def update() -> None:
        value = state
        value[position] = 7
        state = value
    def __call__() -> None:
        update()
''',1,optimize=False)
        transaction=load(out/'model.transaction.mlir')
        self.assertIn('initial = [1 : i32, 2 : i32]',transaction)
        with self.assertRaisesRegex(CompileError,'initial state must match'):
            analyze(transaction.replace('initial = [1 : i32, 2 : i32]','initial = [1 : i32]'),out/'bad-state')
        source=load(out/'model.source.mlir')
        self.assertIn('path = [unit]',source)
        with self.assertRaisesRegex(CompileError,'update coordinate'):
            analyze(source.replace('path = [unit]','path = [unit, unit]'),out/'bad-update')

    def test_multidimensional_values_and_ssa_updates(self):
        source = '''from pycircuit import ac
Matrix = ac.array(ac.u32, shape=(2,3))
@ac.module
class Top:
    def __init__() -> None:
        state = ac.cell[Matrix](initial=[[1,2,3],[4,5,6]])
        old = ac.cell[ac.u32](initial=0)
    @ac.rule
    def change() -> None:
        a = state
        b = a
        b[0,1] = 9
        b[1] = [7,8,9]
        old = a[0,1]
        state = b
    def __call__() -> None:
        change()
'''
        trace, _, out = self.run_source(source,1)
        self.assertEqual(trace[0]['state']['top.state'], [[1,9,3],[7,8,9]])
        self.assertEqual(trace[0]['state']['top.old'],2)
        self.assert_reloads(trace,out)

    def test_selection_survives_tuple_return_and_array_of_structs(self):
        source='''from pycircuit import ac
Data = ac.array(ac.u32, shape=(2,))
class Packet:
    values: Data
def relay(data: Data) -> tuple[bool, Data]:
    return True, data
@ac.module
class Top:
    def __init__() -> None:
        result = ac.cell[ac.u32](initial=0)
        saved = ac.cell[Data](initial=[0,0])
    @ac.rule
    def action() -> None:
        data: Data = [3,4]
        found, selected = relay(data[data.match(lambda value: value == 4)])
        packets: ac.array(Packet, shape=(2,)) = [Packet(selected),Packet(data)]
        if found:
            result = packets[0].values[1]
    def __call__() -> None:
        action()
'''
        trace,_,out=self.run_source(source,1)
        self.assertEqual(trace[0]['state']['top.result'],4)
        self.assert_reloads(trace,out)
        trace,_,_=self.run_source(source.replace('packets[0].values[1]','packets[0].values[0]'),1)
        self.assertEqual(trace[0]['state']['top.result'],0)
        with self.assertRaisesRegex(CompileError,'cannot be stored'):
            self.run_source(source.replace('result = packets[0].values[1]','saved = packets[0].values'),1)

    def test_match_selected_helper_and_masked_component_write(self):
        source = '''from pycircuit import ac
Data = ac.array(ac.u32, shape=(3,))
def retain(a: Data) -> Data:
    return a
@ac.module
class Top:
    def __init__() -> None:
        cells: ac.array(ac.cell[ac.u32], shape=(3,)) = [ac.cell[ac.u32](initial=0) for i in range(3)]
        count = ac.cell[ac.u32](initial=0)
    @ac.rule
    def write() -> None:
        entries: Data = [1,2,3]
        selected = retain(entries[entries.match(lambda entry: entry > 1)])
        mask = selected.match(lambda entry: entry < 3)
        cells[mask] = 7
        count = count + 1
    def __call__() -> None:
        write()
'''
        traces=[]
        for opt in (False,True):
            trace, _, out = self.run_source(source,2,optimize=opt)
            self.assertEqual([trace[0]['state'][f'top.cells.{i}'] for i in range(3)], [0,7,0])
            self.assertEqual(trace[-1]['state']['top.count'],2)
            self.assert_reloads(trace,out)
            traces.append(trace)
        self.assertEqual(*traces)

    def test_dynamic_bounds_cancel_whole_transaction(self):
        source='''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        values = ac.cell[ac.array(ac.u32, shape=(2,2))](initial=[[1,2],[3,4]])
        index = ac.cell[ac.u32](initial=2)
        result = ac.cell[ac.u32](initial=0)
        count = ac.cell[ac.u32](initial=0)
    @ac.rule
    def access() -> None:
        result = values[0,index]
        count = count + 1
    def __call__() -> None:
        access()
'''
        trace,_,out=self.run_source(source,2)
        self.assertEqual(trace[-1]['state']['top.result'],0)
        self.assertEqual(trace[-1]['state']['top.count'],0)
        self.assert_reloads(trace,out)

    def test_queue_of_array_preserves_message_shape(self):
        trace,_,out=self.run_source("""from pycircuit import ac
Row = ac.array(ac.u32, shape=(2,))
@ac.module
class Top:
    def __init__() -> None:
        q = ac.queue[Row]()
        result = ac.cell[Row](initial=[0,0])
    @ac.rule
    def send() -> None:
        value: Row = [3,4]
        q = value
    @ac.rule
    def receive() -> None:
        result = q
    def __call__() -> None:
        send()
        receive()
""",2)
        self.assertEqual(trace[0]['state']['top.result'],[0,0])
        self.assertEqual(trace[1]['state']['top.result'],[3,4])
        self.assert_reloads(trace,out)

    def test_payload_arbitration_uses_found_and_ordinary_coordinate(self):
        source="""from pycircuit import ac
Data = ac.array(ac.u32, shape=(3,))
@ac.module
class Age:
    def __init__() -> None:
        pass
    @ac.rule
    def choose(candidates: Data) -> tuple[bool, ac.u32]:
        mask = candidates.match(lambda item: True)
        found = False
        position = 0
        for i in range(3):
            if mask[i] and not found:
                found = True
                position = i
        return found, position
@ac.module
class Top:
    def __init__() -> None:
        age = Age()
        result = ac.cell[ac.u32](initial=0)
    @ac.rule
    def use() -> None:
        values: Data = [1,2,3]
        found, position = age.choose(values[values.match(lambda item: item > 1)])
        if found:
            result = values[position]
    def __call__() -> None:
        use()
"""
        trace,_,out=self.run_source(source,1)
        self.assertEqual(trace[0]['state']['top.result'],2)
        self.assert_reloads(trace,out)

    def test_selection_survives_struct_and_helper(self):
        source="""from pycircuit import ac
Data = ac.array(ac.u32, shape=(2,))
class Packet:
    values: Data

def relay(packet: Packet) -> Packet:
    return packet
@ac.module
class Top:
    def __init__() -> None:
        result = ac.cell[ac.u32](initial=0)
        state = ac.cell[Data](initial=[0,0])
    @ac.rule
    def action() -> None:
        data: Data = [3,4]
        packet = relay(Packet(data[data.match(lambda value: value == 4)]))
        result = packet.values[1]
    def __call__() -> None:
        action()
"""
        trace,_,out=self.run_source(source,1)
        self.assertEqual(trace[0]['state']['top.result'],4)
        self.assert_reloads(trace,out)
        with self.assertRaisesRegex(CompileError,'cannot be stored'):
            self.run_source(source.replace('result = packet.values[1]','state = packet.values'),1)
        trace,_,_=self.run_source(source.replace('packet.values[1]','packet.values[0]'),1)
        self.assertEqual(trace[0]['state']['top.result'],0)
        updated=source.replace('packet = relay(Packet(data[data.match(lambda value: value == 4)]))',
            'packet = Packet(data)\n        packet.values = data[data.match(lambda value: value == 4)]\n        packet = relay(packet)')
        trace,_,out=self.run_source(updated,1)
        self.assertEqual(trace[0]['state']['top.result'],4)
        self.assert_reloads(trace,out)
        with self.assertRaisesRegex(CompileError,'cannot be stored'):
            self.run_source(updated.replace('result = packet.values[1]','state = packet.values'),1)

    def test_resource_alias_repeated_pop_and_unselected_empty_queue(self):
        trace,_,out=self.run_source("""from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        q = ac.queue[ac.u32](initial=9)
        empty = ac.queue[ac.u32]()
        slots: ac.array(ac.queue[ac.u32], shape=(3,)) = [empty,q,q]
        index = ac.cell[ac.u32](initial=1)
        result = ac.cell[ac.u32](initial=0)
    @ac.rule
    def receive() -> None:
        a = slots[index]
        b = slots[2]
        result = a+b
    def __call__() -> None:
        receive()
""",2)
        self.assertEqual(trace[0]['state']['top.result'],18)
        self.assertEqual(trace[-1]['state']['top.q'],[])
        self.assert_reloads(trace,out)

    def test_empty_mask_does_not_cancel_other_effects(self):
        trace,_,out=self.run_source("""from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        queues: ac.array(ac.queue[ac.u32], shape=(2,)) = [ac.queue[ac.u32](initial=7) for i in range(2)]
        count = ac.cell[ac.u32](initial=0)
    @ac.rule
    def action() -> None:
        data: ac.array(ac.u32, shape=(2,)) = [1,2]
        mask = data.match(lambda item: False)
        queues[mask] = 9
        count = count+1
    def __call__() -> None:
        action()
""",2)
        self.assertEqual(trace[-1]['state']['top.count'],2)
        self.assertEqual(trace[-1]['state']['top.queues.0'],[7])
        self.assert_reloads(trace,out)

    def test_module_array_dynamic_rule_and_static_connections(self):
        source='''from pycircuit import ac
@ac.module
class Slot:
    def __init__() -> None:
        value = ac.cell[ac.u32](initial=0)
    @ac.rule
    def put(value: ac.u32) -> None:
        write(value)
    @ac.rule
    def write(data: ac.u32) -> None:
        value = data
    @ac.rule
    def read() -> ac.u32:
        return value
@ac.module
class Top:
    def __init__() -> None:
        slots: ac.array(Slot, shape=(2,)) = [Slot() for i in range(2)]
        position = ac.cell[ac.u32](initial=1)
        result = ac.cell[ac.u32](initial=0)
    @ac.rule
    def access() -> None:
        slots[position].put(7)
        result = slots[position].read()
    def __call__() -> None:
        access()
'''
        trace,_,out=self.run_source(source,2)
        self.assertEqual(trace[-1]['state']['top.slots.0.value'],0)
        self.assertEqual(trace[-1]['state']['top.slots.1.value'],7)
        self.assertEqual(trace[-1]['state']['top.result'],7)
        self.assert_reloads(trace,out)

    def test_age_matrix_commits_only_when_issue_fires(self):
        traces=[]
        for opt in (False,True):
            from pycircuit import analyze, compile_source
            import json
            out=self.directory/('age-opt' if opt else 'age-raw')
            analyze(compile_source(pipeline.EXAMPLES/'array_arbiter.py','ArrayArbiter'),out,cycles=12,optimize=opt)
            trace=json.loads((out/'transaction-trace.json').read_text())['trace']
            self.assertEqual(trace,json.loads((out/'rtl-trace.json').read_text())['trace'])
            for before,blocked in [(2,3),(4,5)]:
                for key in ('top.age.used','top.age.older','top.issued'):
                    self.assertEqual(trace[before]['state'][key],trace[blocked]['state'][key])
            self.assertEqual([trace[t]['state']['top.last'] for t in (4,6,8,10)],[99,10,20,30])
            self.assertEqual([trace[t]['state']['top.issued'] for t in (3,4,5,6,8)],[0,1,1,2,3])
            self.assertEqual(trace[-1]['state']['top.age.used'],[False,False,False])
            self.assert_reloads(trace,out)
            traces.append(trace)
        self.assertEqual(*traces)

    def test_negative_coordinate_cancels_read_and_write(self):
        source="""from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        array = ac.cell[ac.array(ac.u32, shape=(2,))](initial=[1,2])
        index = ac.cell[ac.i8](initial=-1)
        count = ac.cell[ac.u32](initial=0)
    @ac.rule
    def access() -> None:
        array[index] = 9
        count = count+1
    def __call__() -> None:
        access()
"""
        trace,_,out=self.run_source(source,1)
        self.assertEqual(trace[0]['state']['top.array'],[1,2])
        self.assertEqual(trace[0]['state']['top.count'],0)
        self.assert_reloads(trace,out)
