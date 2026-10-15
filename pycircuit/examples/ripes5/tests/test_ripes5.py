"""Instruction-image tests against native Ripes RV5S cycle captures."""
import unittest

from pycircuit.examples.ripes5.tests.verify import compare_reference, load_reference, run_case
from pycircuit.ir import CompileError
from pycircuit.tests.support import ExecutionChecks


class Ripes5Tests(ExecutionChecks, unittest.TestCase):
    def test_readable_demo_matches_native_capture(self):
        case = next(case for case in load_reference()['cases'] if case['name'] == 'load_branch_wrong_path')
        trace, _ = self.run_example('ripes5/model/demo.py', 'Demo', cycles=len(case['trace']))
        compare_reference(trace, case)
        self.assertEqual(len(trace[0]['fire']), 8)
        for stage_register in ('if_id', 'id_ex', 'ex_mem', 'mem_wb'):
            self.assertIn('top.cpu.' + stage_register, trace[0]['state'])
        for internal in ('fetch.if_id', 'decode_stage.id_ex', 'execute.ex_mem', 'memory.mem_wb'):
            self.assertNotIn('top.cpu.' + internal, trace[0]['state'])

    def test_instruction_programs_match_native_ripes_cycles(self):
        for case in load_reference()['cases']:
            with self.subTest(program=case['name']):
                run_case(case, self.directory / case['name'])

    def test_branch_load_use_raw_and_reload(self):
        case = next(case for case in load_reference()['cases'] if case['name'] == 'load_branch_wrong_path')
        optimized, _ = run_case(case, self.directory / 'opt', reload=True)
        raw, _ = run_case(case, self.directory / 'raw', optimize=False)
        self.assertEqual(raw, optimized)

    def test_nested_early_return_and_field_helper_name_collision(self):
        source = '''from pycircuit import ac
class Field:
    value: ac.u32 = 0
class Packet:
    choose: ac.u32 = 0
    Field: Field
def choose(value: ac.u32) -> ac.u32:
    if value != 0:
        if value == 1:
            return 7
        if value == 2:
            return 9
    return 3
@ac.module
class Top:
    def __init__() -> None:
        cycle = ac.cell[ac.u32](initial=0)
        result = ac.cell[Packet](initial=Packet())
    @ac.rule
    def step() -> None:
        packet = Packet()
        packet.choose = choose(cycle)
        packet.Field.value = choose(cycle + 1)
        result = packet
        cycle = cycle + 1
    def __call__() -> None:
        step()
'''
        for optimize in (False, True):
            trace, _, _ = self.run_source(source, cycles=4, optimize=optimize)
            self.assertEqual([row['state']['top.result'] for row in trace], [[3, [7]], [7, [9]], [9, [3]], [3, [3]]])

    def test_shared_checked_control_with_nested_rule_branches(self):
        source = '''from pycircuit import ac
class Control:
    stall: bool = False
    redirect: bool = False
def checked(valid: bool, stall: bool, redirect: bool) -> Control:
    assert valid
    return Control(stall, redirect)
@ac.module
class Top:
    def __init__() -> None:
        valid = ac.cell[bool](initial=VALID)
        count = ac.cell[ac.u32](initial=0)
        result = ac.cell[ac.u32](initial=0)
        event = ac.cell[ac.u32](initial=0)
    @ac.rule
    def choose(control: Control) -> None:
        if not control.stall:
            if control.redirect:
                result = 1
            else:
                result = 2
    @ac.rule
    def observe(control: Control) -> None:
        event = 1 if control.stall or control.redirect else 0
        count = count + 1
    def __call__() -> None:
        control = checked(valid, count == 1, count == 2)
        choose(control)
        observe(control)
'''
        for optimize in (False, True):
            trace, _, _ = self.run_source(source.replace('VALID', 'True'), cycles=4, optimize=optimize)
            self.assertEqual([row['state']['top.result'] for row in trace], [2, 2, 1, 2])
            self.assertEqual([row['state']['top.event'] for row in trace], [0, 1, 1, 0])
            with self.assertRaisesRegex(CompileError, 'assertion failed'):
                self.run_source(source.replace('VALID', 'False'), optimize=optimize)


if __name__ == '__main__':
    unittest.main()
