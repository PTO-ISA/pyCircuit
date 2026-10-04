"""Use LLVM's RV32I assembler; no model-specific instruction encoder."""
import argparse
from pathlib import Path
import shutil
import subprocess
from tempfile import TemporaryDirectory


def assemble(source, output):
    clang, objcopy = shutil.which('clang'), shutil.which('llvm-objcopy')
    if not clang or not objcopy:
        raise RuntimeError('clang and llvm-objcopy must be on PATH')
    with TemporaryDirectory(prefix='skyzh-assembly-') as directory:
        elf, binary = Path(directory) / 'program.elf', Path(directory) / 'program.bin'
        subprocess.run([clang, '--target=riscv32-unknown-elf', '-march=rv32i', '-mabi=ilp32',
                        '-nostdlib', '-fuse-ld=lld', '-Wl,-Ttext=0', '-Wl,--no-relax',
                        '-Wl,--image-base=0', '-Wl,-e,_start', str(source), '-o', str(elf)], check=True)
        subprocess.run([objcopy, '-O', 'binary', '--only-section=.text', str(elf), str(binary)], check=True)
        data = binary.read_bytes()
    if len(data) % 4:
        raise ValueError('RV32I text must contain complete 32-bit instructions')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(''.join(f'{int.from_bytes(data[i:i+4], "little"):08x}\n' for i in range(0, len(data), 4)))
    return output


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    assemble(args.source, args.output)
