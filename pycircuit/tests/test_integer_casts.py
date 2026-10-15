"""Explicit integer bit-pattern conversion across all supported widths."""
import unittest
from pycircuit.tests.support import ExecutionChecks, check_cpp, check_reloads


class IntegerCastTests(ExecutionChecks, unittest.TestCase):
    def test_signed_extension_truncation_and_reinterpretation(self):
        types = [sign + str(width) for sign in ('u', 'i') for width in (8,16,32,64)]
        source = 'from pycircuit import ac\n@ac.module\nclass Top:\n    def __init__() -> None:\n'
        expected = {}
        for before in types:
            width = int(before[1:])
            value = -(1 << (width-1)) if before[0] == 'i' else (1 << (width-1)) + 3
            source += f'        s_{before} = ac.cell[ac.{before}](initial={value})\n'
            for after in types:
                source += f'        r_{before}_{after} = ac.cell[ac.{after}](initial=0)\n'
                expected[f'top.r_{before}_{after}'] = value & ((1 << int(after[1:])) - 1)
        source += '    @ac.rule\n    def convert() -> None:\n'
        for before in types:
            for after in types:
                source += f'        r_{before}_{after} = ac.{after}(s_{before})\n'
        source += '    def __call__() -> None:\n        convert()\n'
        for optimize in (False,True):
            trace,_,out = self.run_source(source,cycles=2,optimize=optimize)
            for key,value in expected.items(): self.assertEqual(trace[-1]['state'][key],value,key)
            if optimize:
                check_reloads(trace,out)
                check_cpp(trace,out,sanitize=True)
