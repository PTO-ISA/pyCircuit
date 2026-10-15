"""Program manifest and build adapter, independent of the simulation backend."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

from .paths import EXAMPLE, ROOT
from .program import ProgramImage, compile_program, load_elf, LLVM
from .upstream import UPSTREAM, upstream_source

MANIFEST = Path(__file__).with_name('cases.json')


def load_cases(path=MANIFEST):
    data = json.loads(Path(path).read_text())
    if data.get('schema_version') != 1:
        raise ValueError('unsupported case manifest version')
    cases = data['cases']
    seen = set()
    for case in cases:
        name = case['name']
        if not name or any(c not in 'abcdefghijklmnopqrstuvwxyz0123456789_' for c in name) or name in seen:
            raise ValueError(f'invalid or duplicate case name: {name}')
        seen.add(name)
        if ('source' in case) == ('upstream' in case):
            raise ValueError(f'{name}: expected exactly one source or upstream path')
        if case['isa'] not in ('I', 'IM', 'IMC') or case['language'] not in ('asm', 'c'):
            raise ValueError(f'{name}: unsupported ISA or language')
        if case['expected_reason'] not in (None, 'exit', 'break', 'illegal'):
            raise ValueError(f'{name}: unsupported termination reason')
    return cases


def source_for(case, directory=None):
    """Read local programs or adapt checksum-pinned upstream assembly."""
    if 'source' in case:
        path = (EXAMPLE / 'tests' / case['source']).resolve()
        if not path.is_relative_to(EXAMPLE / 'tests/programs'):
            raise ValueError('local program must be inside tests/programs')
        return path.read_text()
    path = (ROOT / case['upstream']).resolve()
    if not path.is_relative_to(ROOT / 'reference'):
        raise ValueError('upstream source must be inside reference/')
    if hashlib.sha256(path.read_bytes()).hexdigest() != case['source_sha256']:
        raise ValueError(f'upstream source checksum changed: {path}')
    source, notes = upstream_source(path)
    if notes != case['adaptations']:
        raise ValueError(f'upstream adaptation changed: {case["name"]}')
    if directory is not None:
        directory = Path(directory)
        directory.mkdir(parents=True, exist_ok=True)
        (directory / 'upstream.s').write_bytes(path.read_bytes())
        license = UPSTREAM / 'riscv-tests/LICENSE' if path.parent.name == 'riscv-tests' else UPSTREAM.parent / 'LICENSE'
        (directory / 'LICENSE').write_bytes(license.read_bytes())
    return source


def build_case(case, directory):
    directory = Path(directory)
    if 'elf' in case:
        directory.mkdir(parents=True, exist_ok=True)
        source_elf = Path(case['elf']).resolve()
        target = directory / 'program.elf'
        if source_elf != target.resolve():
            shutil.copyfile(source_elf, target)
        image = load_elf(target, case['isa'], max_cycles=case.get('max_cycles', 5000))
        with (directory / 'disassembly.txt').open('w') as output:
            subprocess.run([str(LLVM / 'llvm-objdump'), '-d', str(target)], stdout=output, check=True)
    else:
        image = compile_program(source_for(case, directory), directory, case['isa'],
                                language=case['language'], max_cycles=case.get('max_cycles', 5000),
                                pad_fetch=case.get('pad_fetch', True))
    registers = list(case.get('registers', image.registers))
    for index, value in case.get('register_overrides', {}).items():
        if not 0 <= int(index) < 32:
            raise ValueError('initial register index outside x0..x31')
        registers[int(index)] = value
    ram = b''.join(value.to_bytes(4, 'little') for value in case['ram']) if 'ram' in case else image.ram
    image = ProgramImage(image.text, ram, registers, image.isa,
                         case.get('entry', image.entry), case.get('end_pc', image.end_pc), image.max_cycles)
    image.save(directory / 'input.json')
    evidence = case | dict(input_sha256=image.checksum(),
                          elf_sha256=hashlib.sha256((directory / 'program.elf').read_bytes()).hexdigest())
    if 'elf' not in case:
        evidence['source_sha256'] = hashlib.sha256(source_for(case).encode()).hexdigest()
    (directory / 'case.json').write_text(json.dumps(evidence, indent=2) + '\n')
    return image
