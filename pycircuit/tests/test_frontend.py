import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from pycircuit import compile_source
from pycircuit.ir import CompileError
from pycircuit.mlir_text import serialize


class SourceTests(unittest.TestCase):
    def compile(self, source):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'model.py'
            path.write_text(source)
            return serialize(compile_source(path, 'Top'))

    def test_source_rule_body_and_scoped_assignment(self):
        ir = self.compile('''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        count = ac.cell[ac.u32](initial=1)
    @ac.rule(SB="other")
    def advance(count: ac.u32) -> None:
        local = count + 1
        other(local)
    @ac.rule
    def other(value: ac.u32) -> None:
        count = value
    def __call__() -> None:
        value = count + 1
        if value < 4:
            advance(value)
''')
        for operation in ('acir.rule ', 'acir.rule.call', 'acir.body ', 'scf.if', 'acir.contract', 'acir.cell.write'):
            self.assertIn(operation, ir)
        self.assertNotIn('acir.method', ir)
        self.assertNotIn('acir.work', ir)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'source.mlir'
            path.write_text(ir)
            result = subprocess.run([os.environ['ACPY_MLIR_OPT'], str(path)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_old_api_and_missing_annotations_are_rejected(self):
        for source in (
            'from pycircuit import ac\n@ac.module\ndef Top() -> None:\n    pass\n',
            'from pycircuit import ac\n@ac.module\nclass Top:\n    def __init__():\n        pass\n    def __call__() -> None:\n        pass\n',
            'from pycircuit import ac\n@ac.module\nclass Top:\n    def __init__() -> None:\n        pass\n    @ac.method\n    def old() -> None:\n        pass\n    def __call__() -> None:\n        old()\n',
        ):
            with self.assertRaises(CompileError):
                self.compile(source)

    def test_signal_initial_state_runtime_construction_and_bad_contract_rejected(self):
        template = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        BODY
    @ac.rule RELATION
    def change() -> None:
        pass
    def __call__() -> None:
        change()
'''
        for body, relation in (
            ('s = ac.signal[ac.u32](initial=0)', ''),
            ('s = ac.cell[ac.u32](initial=0)\n        runtime = s + 1', ''),
            ('s = ac.cell[ac.u32](initial=0)', '(SB="missing")'),
        ):
            with self.assertRaises(CompileError):
                self.compile(template.replace('BODY', body).replace(' RELATION', relation))

    def test_passive_module_has_no_connection_or_outputs(self):
        template = '''from pycircuit import ac
@ac.module
class Storage:
    def __init__() -> None:
        state = ac.cell[ac.u32](initial=1)
    @ac.rule
    def read() -> ac.u32:
        return state
@ac.module
class Top:
    def __init__() -> None:
        storage = Storage()
        seen = ac.cell[ac.u32](initial=0)
    @ac.rule
    def capture() -> None:
        seen = storage.read()
    def __call__() -> None:
        BODY
'''
        ir = self.compile(template.replace('BODY', 'capture()'))
        self.assertEqual(ir.count('acir.body '), 1)
        self.assertIn('acir.rule.call', ir)
        for expression, diagnostic in (
            ('storage()', 'Module has no __call__; invoke its public Rules directly'),
            ('value = storage.outputs', 'Module .outputs is removed'),
        ):
            with self.assertRaisesRegex(CompileError, diagnostic):
                self.compile(template.replace('BODY', expression))
        without_init = template.replace('    def __init__() -> None:\n        state = ac.cell[ac.u32](initial=1)\n', '')
        with self.assertRaisesRegex(CompileError, 'Module requires __init__'):
            self.compile(without_init.replace('BODY', 'capture()'))

    def test_legacy_module_outputs_and_implicit_destinations_are_rejected(self):
        template = '''from pycircuit import ac
@ac.module
class Producer:
    def __init__() -> None:
        internal = ac.queue[ac.u32]()
    @ac.rule
    def produce() -> ac.queue[ac.u32]:
        return 7
    def __call__() -> ac.queue[ac.u32]:
        RETURN
@ac.module
class Top:
    def __init__() -> None:
        destination = ac.queue[ac.u32]()
        producer = Producer()
    def __call__() -> None:
        BODY
'''
        for returned, body, diagnostic in (
            ('return produce()', 'temporary = producer()', 'previously declared component'),
            ('return produce()', 'producer()', 'previously declared component'),
            ('return produce()', 'destination = producer()\n        saved = producer.outputs', 'Module .outputs is removed'),
            ('return internal', 'destination = producer()', 'no longer export internal components'),
            ('implicit = produce()', 'destination = producer()', 'Rule component output must bind'),
            ('return (produce(), produce())', 'destination = producer()', 'one Rule output per destination'),
        ):
            with self.assertRaisesRegex(CompileError, diagnostic):
                self.compile(template.replace('RETURN', returned).replace('BODY', body))
        wrong_type = template.replace('destination = ac.queue[ac.u32]()', 'destination = ac.cell[ac.u32](initial=0)')
        with self.assertRaisesRegex(CompileError, 'previously declared component of type queue'):
            self.compile(wrong_type.replace('RETURN', 'return produce()').replace('BODY', 'destination = producer()'))
