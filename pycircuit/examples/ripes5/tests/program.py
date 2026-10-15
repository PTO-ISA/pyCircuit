"""Validated, deterministic byte images shared by ACPy and the native adapter."""
import hashlib
import json
from dataclasses import dataclass, field
from pathlib import Path
import subprocess
import struct

from .paths import ROOT, MODEL_DIR
LLVM = Path('/home/lc/opt/llvm-22.1.8/bin')


@dataclass
class ProgramImage:
    text: bytes
    ram: bytes = b''
    registers: list[int] = field(default_factory=lambda: [0, 0, 0x2000] + [0] * 29)
    isa: str = 'IMC'
    entry: int = 0
    end_pc: int = 0xffffffff
    max_cycles: int = 5000

    def __post_init__(self):
        if self.isa not in ('I', 'IM', 'IMC'):
            raise ValueError('ISA must be I, IM or IMC')
        if not 0 < len(self.text) <= 4096 or len(self.ram) > 4096:
            raise ValueError('text and RAM must fit their 4 KiB windows')
        if len(self.registers) != 32 or self.registers[0] != 0 or any(not 0 <= x <= 0xffffffff for x in self.registers):
            raise ValueError('expected 32 u32 registers with x0=0')
        alignment = 2 if 'C' in self.isa else 4
        if not 0 <= self.entry < len(self.text) or self.entry % alignment:
            raise ValueError('entry is outside text or misaligned')
        if self.end_pc != 0xffffffff and (not 0 <= self.end_pc < len(self.text) or self.end_pc % alignment):
            raise ValueError('end marker is outside text or misaligned')
        if self.max_cycles < 1:
            raise ValueError('cycle limit must be positive')

    @staticmethod
    def words(data, size):
        data = data.ljust(size, b'\0')
        return [int.from_bytes(data[i:i+4], 'little') for i in range(0, size, 4)]

    def payload(self):
        return dict(text=list(self.text), data=self.words(self.ram, 4096),
                    registers=self.registers, isa=self.isa, entry=self.entry,
                    data_base=4096, end_pc=self.end_pc, max_cycles=self.max_cycles)

    def checksum(self):
        return hashlib.sha256(json.dumps(self.payload(), sort_keys=True, separators=(',', ':')).encode()).hexdigest()

    def save(self, path):
        payload = self.payload()
        payload['sha256'] = self.checksum()
        Path(path).write_text(json.dumps(payload, indent=2) + '\n')

    @classmethod
    def load(cls, path):
        data = json.loads(Path(path).read_text())
        if data.get('data_base', 4096) != 4096:
            raise ValueError('RAM base must be 0x1000')
        ram = b''.join(x.to_bytes(4, 'little') for x in data['data'])
        image = cls(bytes(data['text']), ram, data['registers'], data['isa'], data['entry'], data['end_pc'], data['max_cycles'])
        if data.get('sha256', image.checksum()) != image.checksum():
            raise ValueError('program checksum mismatch')
        return image


def load_elf(path, isa='IMC', *, max_cycles=5000):
    """Load ELF32 RISC-V PT_LOAD segments into the fixed, flat memory map.

    Section names do not determine placement. File bytes retain their addresses,
    and the remaining p_memsz bytes (including BSS) are zero initialized.
    """
    elf = Path(path).read_bytes()
    if len(elf) < 52 or elf[:7] != b'\x7fELF\x01\x01\x01':
        raise ValueError('expected a little-endian ELF32 file')
    (_, kind, machine, version, entry, phoff, _, _, ehsize, stride,
     count, _, _, _) = struct.unpack_from('<16sHHIIIIIHHHHHH', elf)
    if (kind, machine, version, ehsize) != (2, 243, 1, 52):
        raise ValueError('expected an executable ELF32 RISC-V image')
    if not count or stride < 32 or phoff < 52 or phoff + count * stride > len(elf):
        raise ValueError('invalid ELF program header table')
    memory = bytearray(8192)
    ranges, executable = [], []
    text_end, ram_end = 0, 4096
    for index in range(count):
        ptype, offset, address, physical, filesz, memsz, flags, _ = struct.unpack_from('<8I', elf, phoff + index * stride)
        if ptype != 1:
            continue
        if filesz > memsz or offset > len(elf) or filesz > len(elf) - offset:
            raise ValueError('invalid ELF load segment file range')
        if physical != address:
            raise ValueError('ELF virtual/physical addresses differ in flat memory')
        if not memsz:
            continue
        end = address + memsz
        if not (address < 4096 and end <= 4096 or 4096 <= address and end <= 8192):
            raise ValueError(f'ELF load segment outside fixed windows: {address:#x}+{memsz}')
        if any(address < hi and lo < end for lo, hi in ranges):
            raise ValueError('overlapping ELF load segments')
        ranges.append((address, end))
        memory[address:address + filesz] = elf[offset:offset + filesz]
        if address < 4096:
            text_end = max(text_end, end)
        else:
            ram_end = max(ram_end, end)
        if flags & 1:
            if end > 4096:
                raise ValueError('executable ELF segment outside text window')
            executable.append((address, address + filesz))
    if not any(lo <= entry < hi for lo, hi in executable):
        raise ValueError('ELF entry is outside file-backed executable segments')
    return ProgramImage(bytes(memory[:text_end]), bytes(memory[4096:ram_end]),
                        isa=isa, entry=entry, max_cycles=max_cycles)


def compile_program(source, directory, isa='IMC', *, language='asm', max_cycles=5000, pad_fetch=True):
    """Use LLVM's assembler/compiler and ELF sections, never a CPU-specific frontend."""
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / ('program.c' if language == 'c' else 'program.s')
    # Leave initialized instructions in the speculative fetch slots after exit.
    path.write_text(source + ('\n.text\n.word 0x13,0x13,0x13\n' if language == 'asm' and pad_fetch else ''))
    linker = directory / 'memory.ld'
    linker.write_text('ENTRY(main)\nSECTIONS { . = 0; .text : { *(.text.start) *(.text*) *(.rodata*) }\n'
                      '. = 0x1000; .data : { *(.sdata*) *(.data*) } .bss : { *(.sbss*) *(.bss*) *(COMMON) }\n'
                      '/DISCARD/ : { *(.comment) *(.riscv.attributes) *(.eh_frame*) } }\n')
    command = [str(LLVM / 'clang'), '--target=riscv32-unknown-elf', '-march=rv32' + isa.lower(), '-mabi=ilp32', '-mno-relax',
               '-fuse-ld=lld', '-nostdlib', '-ffreestanding', '-fno-builtin', '-fno-stack-protector',
               '-msmall-data-limit=0', '-Wl,--no-relax', '-Wl,-T,' + str(linker), '-O1', str(path)]
    if language == 'c':
        startup = directory / 'start.s'
        startup.write_text('.section .text.start\n.globl main\nmain:\ncall algorithm\nli a7,93\necall\n.insn 4,19\n.insn 4,19\n.insn 4,19\n.section .text.tail\n.insn 4,19\n.insn 4,19\n.insn 4,19\n')
        command += [str(startup)]
    elf = directory / 'program.elf'
    (directory / 'build-commands.json').write_text(json.dumps([command + ['-o', str(elf)]], indent=2) + '\n')
    subprocess.run(command + ['-o', str(elf)], check=True, capture_output=True, text=True)
    image = load_elf(elf, isa, max_cycles=max_cycles)
    (directory / 'text.bin').write_bytes(image.text)
    (directory / 'data.bin').write_bytes(image.ram)
    image.save(directory / 'input.json')
    with (directory / 'disassembly.txt').open('w') as out:
        subprocess.run([str(LLVM / 'llvm-objdump'), '-d', str(elf)], stdout=out, check=True)
    return image


def make_source(directory, image):
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    model = directory / 'model.py'
    if model.is_symlink() and model.resolve() != MODEL_DIR / 'model.py':
        model.unlink()
    if not model.exists():
        model.symlink_to(MODEL_DIR / 'model.py')
    source = directory / 'source.py'
    source.write_text(f'''from pycircuit import ac
from model import Ripes5, Program, Registers, DataWords
@ac.module
class Top:
    def __init__() -> None:
        program: Program = {image.words(image.text, 4096)!r}
        registers: Registers = {image.registers!r}
        data: DataWords = {image.words(image.ram, 4096)!r}
        cpu = Ripes5(program, registers, data, 4096, {len(image.text)}, {image.entry}, {'M' in image.isa}, {'C' in image.isa})
    def __call__() -> None:
        cpu()
''')
    return source
