"""Compile and execute standalone MLIR stages; JSON is a report only."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from .ir import CompileError, save


def compiler():
    tool = os.environ.get('ACPY_MLIR_COMPILER') or shutil.which('acir-compile')
    if not tool:
        raise CompileError('acir-compile not found; build pycircuit and set ACPY_MLIR_COMPILER')
    return tool


def analyze(model, directory, optimize=True, *, stop_after='rtl', cycles=0, config=None):
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.compile-', dir=directory) as temporary:
        source = Path(temporary) / 'input.mlir'
        save(model, source)
        command = [compiler(), str(source), str(directory), '--stop-after', stop_after]
        if optimize:
            command.append('--opt')
        if cycles:
            command += ['--cycles', str(cycles)]
        for key, value in (config or {}).items():
            command += ['--config', f'{key}={int(value)}']
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            raise CompileError(result.stderr or result.stdout or 'MLIR compilation failed')
    graph = directory / 'rule-graph.json'
    return json.loads(graph.read_text()) if graph.exists() else {'nodes': [], 'edges': [], 'diagnostics': []}
