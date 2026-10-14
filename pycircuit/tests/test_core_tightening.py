"""Diagnostics for the deliberately small ACPy expression core."""
from pathlib import Path
import tempfile
import unittest
from pycircuit import compile_source
from pycircuit.ir import CompileError

class TightCoreTests(unittest.TestCase):
    def reject(self, body, diagnostic, members='', methods=''):
        source = f'''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        q = ac.queue[ac.u32](initial=7)
        state = ac.cell[ac.u32](initial=0)
{members}
{methods}
    @ac.rule
    def action() -> None:
{body}
    def __call__() -> None:
        action()
'''
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / 'model.py'
            p.write_text(source)
            with self.assertRaisesRegex(CompileError, diagnostic):
                compile_source(p, 'Top')

    def test_strict_conditions_and_arguments(self):
        for body, diagnostic in [
            ('        if 2:\n            state = 7', 'type mismatch'),
            ('        state = not 2', 'type mismatch'),
            ('        state = q.peek(123)', 'accepts no arguments'),
            ('        state = q.peek(ignored=456)', 'accepts no arguments'),
            ('        state = ac.u32(7)', 'conversion calls'),
            ('        state = ac.var[ac.u32](7)', 'unsupported'),
            ('        a = b = 7', 'chained assignment'),
            ('        for i in range(state):\n            state = i', 'statically bounded'),
            ('        def nested() -> None:\n            state = 7', 'nested declarations'),
            ('        x: ac.u8 = 256', 'out of range'),
            ('        x: ac.u8 = 1\n        state = x', 'type mismatch'),
        ]:
            with self.subTest(body=body): self.reject(body, diagnostic)

    def test_helper_cannot_observe_or_modify_even_in_guard(self):
        for statement in ('return state', 'state = 1\n        return 1'):
            self.reject('        state = helper()', 'helper cannot access', methods=f'    def helper() -> ac.u32:\n        {statement}\n')
    def test_output_contract_is_not_inferred_from_assignment(self):
        source = '''from pycircuit import ac
@ac.module
class Top:
    def __init__() -> None:
        out = ac.queue[ac.u32]()
    @ac.rule
    def produce() -> ac.u32:
        return 7
    def __call__() -> None:
        out = produce()
'''
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / 'model.py'; p.write_text(source)
            with self.assertRaisesRegex(CompileError, 'output type must match'):
                compile_source(p, 'Top')
