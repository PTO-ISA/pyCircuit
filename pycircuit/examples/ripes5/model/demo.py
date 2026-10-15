"""A readable instruction image: load-use, forwarded branches, wrong-path stores.

Expected: x1=x2=x4=17, x3=x5=18, x20=x21=0, memory[0:2]=[17,18].
The final sentinel/jump/NOPs are the same suffix used by the native reference.
"""
from pycircuit import ac
from model import Ripes5, Program, Registers, DataWords


def program_image() -> Program:
    words: Program = [0 for i in range(1024)]
    words[0] = 0x00001537   # lui  x10,1
    words[1] = 0x01100093   # addi x1,x0,17
    words[2] = 0x00152023   # sw   x1,0(x10)
    words[3] = 0x00052103   # lw   x2,0(x10)
    words[4] = 0x00110663   # beq  x2,x1,taken
    words[5] = 0x00052023   # sw   x0,0(x10)       (flushed)
    words[6] = 0x06300A13   # addi x20,x0,99        (flushed)
    words[7] = 0x00110193   # taken: addi x3,x2,1
    words[8] = 0x00219663   # bne  x3,x2,next
    words[9] = 0x00052023   # sw   x0,0(x10)       (flushed)
    words[10] = 0x06300A93  # addi x21,x0,99        (flushed)
    words[11] = 0x00052203  # next: lw x4,0(x10)
    words[12] = 0x00121663  # bne  x4,x1,bad        (not taken)
    words[13] = 0x00120293  # addi x5,x4,1
    words[14] = 0x0080006F  # jal  x0,done
    words[15] = 0x00052023  # bad: sw x0,0(x10)     (flushed)
    words[16] = 0x00552223  # done: sw x5,4(x10)
    words[17] = 0x7FF00013  # end marker: addi x0,x0,2047
    words[18] = 0x0000006F  # jal x0,0
    words[19] = 0x00000013  # nop
    words[20] = 0x00000013  # nop
    return words


def zero_registers() -> Registers:
    return [0 for i in range(32)]


def zero_data() -> DataWords:
    return [0 for i in range(1024)]


@ac.module
class Demo:
    def __init__() -> None:
        cpu = Ripes5(program_image(), zero_registers(), zero_data(), data_base=4096, code_size=84)

    def __call__() -> None:
        cpu()
