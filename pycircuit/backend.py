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
        functions = {}
        for record in graph['ehr_bindings']:
            if not record['identity']: continue
            entries = functions.setdefault(record['function'], {}).setdefault(record['identity'], [])
            binding = dict(resource=record['resource'], aspect=record['aspect'], port=record['port'])
            if binding not in entries: entries.append(binding)
        bindings = Path(temporary) / 'bindings.json'
        bindings.write_text(json.dumps(dict(views=graph['ehr'], functions=functions)))
        annotated = Path(temporary) / 'annotated'
        result = subprocess.run([compiler(), str(Path(temporary) / 'model.acir.mlir'), str(annotated), '--bindings', str(bindings)], capture_output=True, text=True)
        if result.returncode: raise CompileError(result.stderr or 'EHR annotation failed')
        text = (annotated / 'model.acir.mlir').read_text()
        save(text, directory / 'model.acir.mlir')
        save((annotated / 'model.readable.mlir').read_text(), directory / 'model.readable.mlir')
        (directory / 'rule-graph.json').write_text(json.dumps(graph, indent=2, ensure_ascii=False) + '\n')
        (directory / 'rule-graph.dot').write_text(dot(graph))
    return graph
