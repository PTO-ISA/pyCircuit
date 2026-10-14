"""Array values, selection domains and physical resource identities."""
import unittest
import test_pipeline_v8 as pipeline
from pycircuit.ir import CompileError

class ArrayTests(unittest.TestCase):
    setUp = pipeline.PipelineTests.setUp
    tearDown = pipeline.PipelineTests.tearDown
    run_source = pipeline.PipelineTests.run_source
    assert_reloads = pipeline.PipelineTests.assert_reloads

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
