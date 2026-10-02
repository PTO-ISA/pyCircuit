"""Whole-program acceptance, including actual upstream traces when built."""
import copy
import json
import os
from pathlib import Path
import tempfile
import unittest
from .programs import suite, make_case
from .run import (DEFAULT_RUNNER, run_python, run_reference, compare, verify_runner)


class RipesFiveStageTests(unittest.TestCase):
    def test_complete_programs_and_configuration_independence(self):
        for case in suite():
            with self.subTest(case=case['name']):
                rows = run_python(case)
                for cache, reverse in ((True, True), (False, False), (False, True)):
                    self.assertEqual(rows, run_python(case, cache, reverse))
                self.assertEqual(rows[0]['cycle'], 0)
                self.assertEqual(rows[-1]['retire']['pc'], case['end_pc'])
                self.assertTrue(all(row['registers'][0] == 0 for row in rows))
                final = rows[-1]
                if case['name'] == 'alu':
                    self.assertEqual(final['cycle'], 15)
                    self.assertEqual(final['registers'][4:11], [4, 10, 5, 0xffffffff, 0xfffffffa, 1, 0])
                elif case['name'] == 'forward_priority':
                    self.assertEqual(final['registers'][1:5], [7, 14, 14, 21])
                    self.assertTrue(any(r['control']['forward_a'] == 1 for r in rows))
                elif case['name'] == 'wb_id_store':
                    self.assertEqual(final['data'][:2], [42, 83])
                elif case['name'] == 'array_sum':
                    self.assertEqual(final['data'][:9], [1, 2, 3, 4, 5, 6, 7, 8, 36])
                    self.assertEqual(sum(r['control']['stall'] for r in rows), 8)
                elif case['name'] == 'jumps_branches':
                    self.assertEqual(final['registers'][5:10], [24, 25, 26, 40, 66])
                    self.assertEqual(final['registers'][20:22], [0, 0])
                elif case['name'] == 'load_branch_wrong_path':
                    self.assertEqual(final['data'][:2], [17, 18])
                    self.assertEqual(final['registers'][20:22], [0, 0])
                    self.assertEqual([r['store'] for r in rows if r['store']], [[4096, 17], [4100, 18]])
                elif case['name'] == 'jalr_forwarded':
                    self.assertEqual(final['registers'][5:9], [12, 13, 36, 48])
                    self.assertEqual(final['data'][0], 40)
                    self.assertEqual(sum(r['control']['stall'] for r in rows), 1)
                elif case['name'] == 'no_false_load_use':
                    self.assertEqual(sum(r['control']['stall'] for r in rows), 0)
                    self.assertEqual(final['registers'][5:8], [9, 5, 0x528000])
                elif case['name'] == 'initial_and_x0_load':
                    self.assertEqual(sum(r['control']['stall'] for r in rows), 2)
                    self.assertEqual(final['registers'][7], 0x80000001)

    def test_native_ripes_all_configurations(self):
        runner = Path(os.environ.get('RIPES5_RUNNER', DEFAULT_RUNNER))
        if not runner.is_file():
            if os.environ.get('RIPES5_REQUIRE_NATIVE'):
                self.fail(f'native reference required but missing: {runner}')
            self.skipTest('build original Ripes; this skip does not certify alignment')
        verify_runner(runner)
        with tempfile.TemporaryDirectory() as tmp:
            for case in suite():
                directory = Path(tmp) / case['name']
                expected = run_reference(case, runner, directory)
                for cache in (True, False):
                    for reverse in (False, True):
                        with self.subTest(case=case['name'], cache=cache, reverse=reverse):
                            compare(case, expected, run_python(case, cache, reverse), directory / 'mismatch.json')

    def test_first_divergence_report_on_complete_program(self):
        case = make_case('report', 'addi x1,x0,1\nadd x2,x1,x1')
        expected = run_python(case)
        actual = copy.deepcopy(expected)
        actual[3]['control']['forward_a'] = 99
        with tempfile.TemporaryDirectory() as tmp:
            report = Path(tmp) / 'mismatch.json'
            with self.assertRaisesRegex(AssertionError, 'row 3'):
                compare(case, expected, actual, report)
            data = json.loads(report.read_text())
            self.assertEqual(data['first_row'], 3)
            self.assertEqual(data['differences'][0]['field'], '.control.forward_a')
            self.assertGreater(len(data['ripes']), 3)
            self.assertEqual(data['instructions'][0]['pc'], 0)
