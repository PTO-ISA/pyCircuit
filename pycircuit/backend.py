"""Invoke the registered C++ ACIR/EmitC compiler using only saved MLIR."""
import os
from pathlib import Path
import shutil
import subprocess
from .ir import CompileError, save


def compiler():
    tool = os.environ.get('ACPY_MLIR_COMPILER') or shutil.which('acir-compile')
    if not tool:
        raise CompileError('acir-compile not found; build pycircuit/mlir and set ACPY_MLIR_COMPILER to the built executable')
    return tool


def emit(model, directory, optimize=True):
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    source = directory / 'model.acir.mlir'
    save(model, source)
    command = [compiler(), str(source), str(directory)]
    if not optimize:
        command.append('--no-opt')
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode:
        raise CompileError(result.stderr or result.stdout or 'ACIR lowering failed')
    (directory / 'ac_support.hpp').write_text(Path(__file__).with_name('support.hpp').read_text())
