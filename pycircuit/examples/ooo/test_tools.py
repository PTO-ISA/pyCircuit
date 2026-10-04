"""Independent oracle spot checks and bounded host-input rejection."""
import unittest
from pycircuit.examples.ooo.programs import suite
from pycircuit.examples.ooo.reference import interpret
from gfsim.experiment.examples.ripes5.isa import assemble


class OracleTests(unittest.TestCase):
    def execute(self, source):
        return interpret(assemble(source), [0] * 31 + [4096], [11, 22])

    def test_signed_wrap_zero_and_link(self):
        rows = self.execute('''
            addi x1, x0, -1
            addi x2, x1, 2
            lui x3, 0x80000
            slt x4, x3, x1
            add x0, x1, x3
            jal x5, done
            sw x1, 0(x31)
            done: halt
        ''')
        self.assertEqual(rows[-1]['registers'][:6], [0, 0xffffffff, 1, 0x80000000, 1, 24])
        self.assertEqual(rows[-1]['data'], [11, 22])
        self.assertEqual([r['pc'] for r in rows], [0, 4, 8, 12, 16, 20, 28])

    def test_fault_has_no_write(self):
        rows = self.execute('addi x1, x0, 9\nlw x1, 1(x31)\nsw x1, 0(x31)')
        self.assertEqual(rows[-1]['fault'], 3)
        self.assertEqual(rows[-1]['registers'][1], 9)
        self.assertIsNone(rows[-1]['write'])
        self.assertEqual(rows[-1]['data'], [11, 22])

    def test_jalr_masks_bit_zero(self):
        rows = self.execute('addi x1, x0, 13\njalr x2, 0(x1)\n.word 0\nhalt')
        self.assertEqual([r['pc'] for r in rows], [0, 4, 12])
        self.assertEqual(rows[-1]['registers'][2], 8)

    def test_reproducible_programs_terminate(self):
        for case in suite():
            with self.subTest(case=case['name']):
                rows = interpret(case['words'], case['registers'], case['data'], case['base'])
                self.assertTrue(rows[-1]['halt'] or rows[-1]['fault'])


if __name__ == '__main__':
    unittest.main()
