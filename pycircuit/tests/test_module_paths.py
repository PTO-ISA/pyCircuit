"""Module calls are execution sites, not unconditional body declarations."""
import unittest
import test_pipeline_v8 as pipeline

class ModulePathTests(unittest.TestCase):
    setUp = pipeline.PipelineTests.setUp
    tearDown = pipeline.PipelineTests.tearDown
    run_source = pipeline.PipelineTests.run_source
    assert_reloads = pipeline.PipelineTests.assert_reloads
    # Reuse the execution harness, without inheriting its test cases below.
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

    def test_module_array_outputs_bind_static_parent_resources(self):
        trace,_,out=self.run_source('''from pycircuit import ac
@ac.module
class Stage:
    def __init__(value: ac.u32) -> None:
        initial = value
    @ac.rule
    def produce() -> ac.queue[ac.u32]:
        return initial
    def __call__() -> ac.queue[ac.u32]:
        return produce()
@ac.module
class Top:
    def __init__() -> None:
        stages: ac.array(Stage, shape=(2,)) = [Stage(i) for i in range(2)]
        outputs: ac.array(ac.queue[ac.u32], shape=(2,)) = [ac.queue[ac.u32]() for i in range(2)]
    def __call__() -> None:
        for i in range(2):
            outputs[i] = stages[i]()
''',1)
        self.assertEqual(trace[0]['state']['top.outputs.0'],[0])
        self.assertEqual(trace[0]['state']['top.outputs.1'],[1])
        self.assert_reloads(trace,out)
