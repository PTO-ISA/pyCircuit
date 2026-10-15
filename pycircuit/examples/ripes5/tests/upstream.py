"""Pinned upstream inventory and explicit Ripes-to-LLVM syntax adaptation."""
import hashlib
import re
from .paths import ROOT

UPSTREAM = ROOT / "reference/ripes-reference/test"
INTEGER_TESTS = set('add addi and andi or ori xor xori sub sll slli srl srli sra srai slt slti sltu sltiu lui memory simple'.split())
M_TESTS = set('mul mulh mulhsu mulhu div divu rem remu'.split())

def upstream_source(path):
    source = path.read_text()
    notes = []
    if path.parent.name == 'riscv-tests-c':
        # Ripes' assembler uses full LUI values and nonstandard memory operands.
        source = re.sub(r'c\.lui\s+(\w+),\s*(-?\d+)', lambda m:f'c.lui {m[1]}, {(int(m[2]) >> 12) & 0xfffff}', source)
        source = re.sub(r'c\.addi4spn\s+(\w+),\s*(\d+)', r'c.addi4spn \1, sp, \2', source)
        source = re.sub(r'c\.addi16sp\s+(-?\d+)', r'c.addi16sp sp, \1', source)
        source = re.sub(r'c\.(lw|sw)\s+(\w+),\s*(\w+)\s*,\s*(\d+)', r'c.\1 \2, \4(\3)', source)
        source = re.sub(r'c\.(lwsp|swsp)\s+(\w+),\s*(\d+)', r'c.\1 \2, \3(sp)', source)
        source = re.sub(r'addi\s+(a1|sp),\1,\s*d1', lambda m:'la '+m[1]+',d1', source)
        source = source.replace('255','254').replace('-254','-256')
        # Original mutable d1..d4 constants are in .text. Move only that block
        # to RAM to meet this environment's explicit read-only text policy.
        source = source.replace('#data', '.data\n#data').replace('test_2:', '.text\ntest_2:', 1)
        lines = []
        for line in source.splitlines():
            code = line.split('#')[0].strip()
            if code and not code.startswith(('.', 'c.')) and not code.endswith(':'):
                lines += ['.option push', '.option norvc', line, '.option pop']
            else:
                lines.append(line)
        source = '.option norelax\n' + '\n'.join(lines)
        notes = ['Keep ordinary instructions 32 bits; only explicit c.* instructions compress, matching Ripes assembly.', 'Normalize Ripes assembly operand syntax to LLVM syntax.',
                 'Move mutable d1..d4 block to .data; replace its address materialization with la.',
                 'Spell odd branch offsets 255/-255 as the encoded even offsets 254/-256.']
    return source, notes


def inventory():
    records = []
    for folder in ('riscv-tests','riscv-tests-c','riscv-tests-64','riscv-tests-c-64'):
        for path in sorted((UPSTREAM / folder).glob('*')):
            if not path.is_file() or path.suffix.lower() not in ('.s','.c'): continue
            name = path.stem
            include = folder == 'riscv-tests' and name in INTEGER_TESTS | M_TESTS or folder == 'riscv-tests-c' and name == 'rvc'
            if include: reason = 'RV32 integer self-check; pass exit status 42'
            elif '64' in folder: reason = 'RV64 ISA outside RV32IMC'
            elif name == 'ecall_file': reason = 'file I/O outside deterministic environment'
            elif name == 'brk_syscall': reason = 'heap allocation syscall outside deterministic environment'
            elif path.suffix == '.c': reason = 'floating-point stochastic C workload; deterministic integer algorithms supplied separately'
            else: reason = 'F/D floating-point instructions outside RV32IMC'
            records.append(dict(path=str(path.relative_to(ROOT)), name='upstream_'+name, included=include, reason=reason,
                                sha256=hashlib.sha256(path.read_bytes()).hexdigest(), license='BSD-3-Clause: test/riscv-tests/LICENSE (RARS/riscv-tests provenance)' if folder=='riscv-tests' else 'Ripes MIT: repository LICENSE'))
    return records
