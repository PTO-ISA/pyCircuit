"""Full-CPU architectural boundaries independent of the optional native runner."""
import unittest
import subprocess
from pycircuit import analyze, compile_source
from pycircuit.ir import CompileError
from pycircuit.examples.ripes5.tests.program import ProgramImage, compile_program, make_source
from pycircuit.tests.support import ExecutionChecks, read_trace, check_cpp


class RV32IMCTests(ExecutionChecks, unittest.TestCase):
    def execute(self, image, cycles=24, sanitize=False):
        out=self.directory/'cpu'
        analyze(compile_source(make_source(out,image),'Top'),out,cycles=cycles,emit_cpp=True)
        trace=read_trace(out/'transaction-trace.json')
        self.assertEqual(trace,read_trace(out/'rtl-trace.json'))
        check_cpp(trace,out,sanitize=sanitize)
        return trace

    def test_mixed_lengths_memory_signed_division_and_ecall_drain(self):
        image=compile_program('''.text
.globl main
main:
c.nop
.option push
.option norvc
li t0,0x80000000
li t1,-1
mulh t2,t0,t1
mulhsu t3,t0,t1
mulhu t4,t0,t1
div s0,t0,t1
rem s1,t0,t1
divu s2,t0,zero
remu s3,t0,zero
li t5,0x1000
sw t0,3(t5)
lh s4,5(t5)
lbu s5,6(t5)
li a0,42
li a7,93
ecall
sw zero,4(t5)
.option pop
''',self.directory/'image')
        trace=self.execute(image,32,sanitize=True)
        state=trace[-1]['state']
        expected={7:0,28:0x80000000,29:0x7fffffff,8:0x80000000,9:0,18:0xffffffff,19:0x80000000,20:0xffff8000,21:128}
        for index,value in expected.items(): self.assertEqual(state[f'top.cpu.rf.registers.{index}'],value)
        self.assertEqual(state['top.cpu.memory.words'][:2],[0,0x00800000])
        self.assertTrue(state['top.cpu.halted'])
        self.assertEqual(state['top.cpu.exit_status'],42)
        events=[row['state']['top.cpu.system_event'] for row in trace if row['state']['top.cpu.system_event'][0]]
        self.assertEqual(len(events),1)
        self.assertEqual(events[0][2:],[1,93,42])
        self.assertGreater(sum(row['state']['top.cpu.last_control'][5] for row in trace),0)

    def test_wrong_path_illegal_reserved_branch_and_c_hints(self):
        image=compile_program('''.text
.globl main
main:
li s0,0x1000
li t0,7
sw t0,0(s0)
lw t1,0(s0)
beq t0,t1,taken
.word 0x00002063
sw zero,0(s0)
taken:
.2byte 0x0001,0x0005,0x4001,0x6005,0x0002,0x8006,0x9006
c.ebreak
sw zero,0(s0)
''',self.directory/'image')
        trace=self.execute(image,32)
        state=trace[-1]['state']
        self.assertEqual(state['top.cpu.rf.registers.0'],0)
        self.assertEqual(state['top.cpu.memory.words'][0],7)
        events=[row['state']['top.cpu.system_event'] for row in trace if row['state']['top.cpu.system_event'][0]]
        self.assertEqual(len(events),1)
        self.assertEqual(events[0][2],2)
        self.assertTrue(state['top.cpu.halted'])

    def test_reachable_illegal_and_disabled_extensions_trap(self):
        # Reserved branch encoding used to assert in combinational decode/EX.
        for word,isa in [(0x00002063,'I'),(0x02000033,'I'),(0,'IMC')]:
            with self.subTest(word=word,isa=isa):
                image=ProgramImage(word.to_bytes(4,'little')+b'\x13\0\0\0'*4,isa=isa)
                trace=self.execute(image,cycles=8)
                events=[row['state']['top.cpu.system_event'] for row in trace if row['state']['top.cpu.system_event'][0]]
                self.assertEqual(len(events),1)
                self.assertEqual(events[0][2],3)

    def test_memory_window_is_checked_without_truncation(self):
        for text,ram,entry in [(bytes(4097),b'',0),(b'\x13\0\0\0',bytes(4097),0),(b'\x13\0\0\0',b'',2)]:
            with self.assertRaises(ValueError):ProgramImage(text,ram,isa='I',entry=entry)
        for instruction in ('lw t1,0(t0)','sw t1,0(t0)'):
            image=compile_program('.text\n.globl main\nmain:\nli t0,8191\n'+instruction+'\n',self.directory/'image','I')
            with self.assertRaisesRegex(CompileError,'assertion failed'):
                self.execute(image,cycles=8)
        image=ProgramImage(bytes(4094)+b'\x03\0',isa='IMC',entry=4094)
        trace=self.execute(image,cycles=2)
        self.assertTrue(trace[0]['state']['top.cpu.if_id'][-1])
        self.assertTrue(trace[1]['state']['top.cpu.id_ex'][-1])
        failed=subprocess.run([str(self.directory/'cpu/cpp/sim'),'--cycles','3'],capture_output=True,text=True)
        self.assertNotEqual(failed.returncode,0)
        self.assertIn('assert',failed.stderr.lower())
        with self.assertRaisesRegex(CompileError,'assertion failed'):
            self.execute(image,cycles=3)

    def test_sequential_fetch_outside_rom_still_fails_on_executed_path(self):
        for image in (ProgramImage(bytes(4092)+b'\x13\0\0\0',isa='I',entry=4092),
                      ProgramImage(bytes(4094)+b'\x01\0',isa='IMC',entry=4094)):
            with self.subTest(isa=image.isa):
                trace=self.execute(image,cycles=3)
                self.assertTrue(trace[1]['state']['top.cpu.if_id'][-1])
                failed=subprocess.run([str(self.directory/'cpu/cpp/sim'),'--cycles','4'],capture_output=True,text=True)
                self.assertNotEqual(failed.returncode,0)
                self.assertIn('assert',failed.stderr.lower())
                with self.assertRaisesRegex(CompileError,'assertion failed'):
                    self.execute(image,cycles=4)
