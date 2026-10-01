"""Sequential ISA oracle. No CPU decoder, ALU, stages, Queue or scheduler calls."""

from .records import Completed, ModelError


class Interpreter:
    def __init__(self, words, data_words=256):
        self.words, self.registers, self.memory = words, [0] * 32, [0] * data_words
        self.pc, self.halted = 0, False

    def step(self):
        if self.halted:
            raise ModelError('reference already halted')
        pc = self.pc
        if pc % 4 or not 0 <= pc // 4 < len(self.words):
            raise ModelError(f'invalid reference PC {pc}')
        word = self.words[pc // 4]
        opcode, funct3, funct7 = word & 0x7f, (word >> 12) & 7, (word >> 25) & 127
        dest, source1, source2 = (word >> 7) & 31, (word >> 15) & 31, (word >> 20) & 31
        a, b = self.registers[source1], self.registers[source2]
        immediate = word >> 20
        if immediate >= 2048:
            immediate -= 4096
        value, store, address, data, halt = 0, False, 0, 0, False
        next_pc = (pc + 4) & 0xffffffff
        if word == 0x00100073:
            halt, dest = True, 0
        elif opcode == 0x33:
            if (funct3, funct7) == (0, 0): value = a + b
            elif (funct3, funct7) == (0, 32): value = a - b
            elif (funct3, funct7) == (7, 0): value = a & b
            elif (funct3, funct7) == (6, 0): value = a | b
            elif (funct3, funct7) == (4, 0): value = a ^ b
            elif (funct3, funct7) == (2, 0):
                value = int((a ^ 0x80000000) < (b ^ 0x80000000))
            else:
                raise ModelError(f'illegal reference instruction {word:08x}')
        elif opcode == 0x13 and funct3 == 0:
            value = a + immediate
        elif opcode == 0x37:
            value = word & 0xfffff000
        elif opcode in (0x03, 0x23) and funct3 == 2:
            offset = immediate
            if opcode == 0x23:
                offset = ((word >> 25) << 5) | ((word >> 7) & 31)
                if offset >= 2048:
                    offset -= 4096
            location = (a + offset) & 0xffffffff
            if location % 4 or location // 4 >= len(self.memory):
                raise ModelError(f'invalid reference data address {location}')
            if opcode == 0x03:
                value = self.memory[location // 4]
            else:
                store, address, data, dest = True, location, b, 0
                self.memory[location // 4] = b
        elif opcode == 0x63 and funct3 in (0, 1):
            offset = ((word >> 31) & 1) * 4096 + ((word >> 7) & 1) * 2048
            offset += ((word >> 25) & 63) * 32 + ((word >> 8) & 15) * 2
            if offset >= 4096:
                offset -= 8192
            if (a == b) == (funct3 == 0):
                next_pc = (pc + offset) & 0xffffffff
            dest = 0
        elif opcode == 0x6f:
            offset = ((word >> 31) & 1) * 1048576 + ((word >> 12) & 255) * 4096
            offset += ((word >> 20) & 1) * 2048 + ((word >> 21) & 1023) * 2
            if offset >= 1048576:
                offset -= 2097152
            value, next_pc = pc + 4, (pc + offset) & 0xffffffff
        elif opcode == 0x67 and funct3 == 0:
            value, next_pc = pc + 4, ((a + immediate) & 0xffffffff) & ~1
        else:
            raise ModelError(f'illegal reference instruction {word:08x}')
        if next_pc % 4:
            raise ModelError(f'unaligned reference target {next_pc}')
        value &= 0xffffffff
        if dest:
            self.registers[dest] = value
        self.pc, self.halted = next_pc, halt
        return Completed(pc, word, dest, value, store, address, data, halt)
