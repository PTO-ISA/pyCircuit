"""Memory writes become visible only when a Store retires."""
from .types import ac


@ac.module
def Memory(memory, retirement):
    @ac.rule
    def write():
        commit = retirement.value
        if commit.store:
            address = commit.entry.dest
            width = 1 << commit.entry.ins.funct3
            assert address + width <= 0x400000 and address % width == 0
            # Combine all bytes of this Store in one word update.
            index = (address >> 2) & 63
            page = address >> 8
            old = memory[page].value.words[index]
            shift = (address & 3) * 8
            mask = ac.u32(0xffffffff) if width == 4 else ((1 << (width * 8)) - 1) << shift
            memory[page].value.words[index] = (old & ~mask) | ((commit.entry.value << shift) & mask)
    write()
