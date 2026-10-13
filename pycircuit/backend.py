"""Reload MLIR, rebuild conditional rule relations, and save reviewable artifacts."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from .analysis import analyze_program, dot
from .ir import CompileError, save


def compiler():
    tool = os.environ.get('ACPY_MLIR_COMPILER') or shutil.which('acir-compile')
    if not tool:
        raise CompileError('acir-compile not found; build pycircuit and set ACPY_MLIR_COMPILER')
    return tool


def analyze(model, directory, optimize=True):
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.analysis-', dir=directory) as temporary:
        source = Path(temporary) / 'input.mlir'
        save(model, source)
        command = [compiler(), str(source), temporary]
        if optimize: command.append('--opt')
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            raise CompileError(result.stderr or result.stdout or 'MLIR verification failed')
        program = json.loads((Path(temporary) / 'program.json').read_text())
        graph = analyze_program(program)
        save((Path(temporary) / 'model.acir.mlir').read_text(), directory / 'model.acir.mlir')
        save((Path(temporary) / 'model.readable.mlir').read_text(), directory / 'model.readable.mlir')
        (directory / 'rule-graph.json').write_text(json.dumps(graph, indent=2, ensure_ascii=False) + '\n')
        (directory / 'rule-graph.dot').write_text(dot(graph))
    return graph
