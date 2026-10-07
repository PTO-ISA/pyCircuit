"""Independent native Match transport guards; no design execution or fabricated headers."""
import argparse
import ast
import copy
import dataclasses
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

BASE = '''import pycircuit as ac
@ac.struct
class Result:
    value: ac.u3
@ac.rule
def evaluate(selector, flag) -> Result:
    value: ac.u3 = 0
    match selector:
        case 0 | 2:
            value = 3
        case _:
            value = 5
    match flag:
        case True:
            value = 1
        case _:
            pass
    return Result(value=value)
@ac.module
def Top(selector: ac.u2, flag: bool) -> Result:
    return evaluate(selector, flag)
'''

# Minimal mutations stay inside actual AST field types accepted by the official
# transport serializer, so generic record/header/span validation stays valid.
CASES = [
    ('match-subject-scalar', 'match', 'subject', 'bad', "captured Match field 'subject' must be an AST node"),
    ('match-cases-scalar', 'match', 'cases', 'bad', "captured Match field 'cases' must be an ArrayAttr"),
    ('match-cases-wrong-node', 'match', 'cases', 'wrong-node-array', "captured Match field 'cases' must contain match_case"),
    ('arm-pattern-unit', 'arm', 'pattern', None, "captured match_case field 'pattern' must be an AST node"),
    ('arm-guard-scalar', 'arm', 'guard', 'bad', "captured match_case field 'guard' must be an AST node or UnitAttr"),
    ('arm-body-scalar', 'arm', 'body', 'bad', "captured match_case field 'body' must be an ArrayAttr"),
    ('arm-body-scalar-element', 'arm', 'body', ['bad'], "captured match_case field 'body' must contain AST nodes"),
    ('value-key-scalar', 'value', 'value', 'bad', "captured MatchValue field 'value' must be an AST node"),
    ('singleton-key-scalar', 'singleton', 'value', 'bad', "captured MatchSingleton shape is malformed"),
    ('catch-all-name-bool', 'catchall', 'name', True, "captured MatchAs field 'name' must be a StringAttr or UnitAttr"),
    ('or-patterns-empty', 'or', 'patterns', [], 'match OR pattern requires at least two alternatives'),
    ('or-patterns-single', 'or', 'patterns', 'first-atom', 'match OR pattern requires at least two alternatives'),
]

def targets(syntax):
    matches = [n for n in ast.walk(syntax) if isinstance(n, ast.Match)]
    first = matches[0]
    return {'match': first, 'arm': first.cases[0], 'or': first.cases[0].pattern,
            'value': first.cases[0].pattern.patterns[0],
            'catchall': first.cases[1].pattern, 'singleton': matches[1].cases[0].pattern}

def mutate(captured, target, field, replacement):
    changed = dataclasses.replace(captured, syntax=copy.deepcopy(captured.syntax))
    node = targets(changed.syntax)[target]
    if replacement == 'wrong-node-array':
        replacement = [copy.deepcopy(targets(changed.syntax)['match'].cases[1].body[0])]
        # The original assignment is already a completely valid AST node;
        # Match.cases alone gives the wrong element-kind diagnosis.
    elif replacement == 'first-atom':
        replacement = [copy.deepcopy(node.patterns[0])]
    setattr(node, field, replacement)
    return changed

def main():
    parser = argparse.ArgumentParser()
    for name in ('repo', 'source-compiler', 'optimizer', 'scratch'):
        parser.add_argument('--' + name, required=True)
    args = parser.parse_args()
    repo, root = Path(args.repo).resolve(), Path(args.scratch).resolve()
    root.mkdir(parents=True, exist_ok=True)
    source = root / 'source'
    source.mkdir(exist_ok=True)
    path = source / 'control.py'
    path.write_text(BASE)
    sys.path.insert(0, str(repo / 'python/pycircuit/src'))
    from pycircuit._source_capture import _capture_source_file
    from pycircuit._source_transport import _emit_source_transport
    captured = _capture_source_file(path, source_root=source)
    commands, outcomes = [], []
    def digest(path):
        return hashlib.sha256(Path(path).read_bytes()).hexdigest()
    env = dict(os.environ, PYTHONPATH=str(repo / 'python/pycircuit/src'), PYTHONDONTWRITEBYTECODE='1')
    def run(command, expected):
        result = subprocess.run(list(map(str, command)), cwd=repo, env=env, text=True, capture_output=True, timeout=45)
        row = {'command': list(map(str, command)), 'exit_status': result.returncode, 'stdout': result.stdout, 'stderr': result.stderr}
        commands.append(row)
        (root / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
        assert result.returncode == expected, row
        assert 'Traceback' not in result.stderr and 'Assertion failed' not in result.stderr, row
        return result
    def compile_capture(transport, out, expected):
        out.mkdir(exist_ok=True)
        return run([args.source_compiler, '--capture', transport, '--package', 'guards', '--path', 'control.py',
                    '--body-out', out / 'control.ac', '--interface-out', out / 'control.interface.ac',
                    '--deps-out', out / 'consumed.json'], expected)
    preimage = root / 'control.capture.mlir'
    preimage.write_text(_emit_source_transport(captured))
    compile_capture(preimage, root / 'control-unit', 0)
    # Source-unit owns source lowering. Independently exercise its emitted body
    # through the registered interface extraction verification entrance.
    run([args.optimizer, root / 'control-unit/control.ac', '--ac-extract-source-interface', '-o', root / 'control.extracted.ac'], 0)
    for name, target, field, replacement, diagnostic in CASES:
        out = root / name
        out.mkdir(exist_ok=True)
        changed = mutate(captured, target, field, replacement)
        transport = out / 'input.capture.mlir'
        transport.write_text(_emit_source_transport(changed))
        assert transport.read_bytes() != preimage.read_bytes()
        before = {p.name: p.read_bytes() for p in out.iterdir() if p.is_file()}
        result = compile_capture(transport, out / 'fresh', 1)
        assert diagnostic in result.stderr, (name, result.stderr)
        assert not list((out / 'fresh').iterdir()), name
        protected = out / 'protected'
        protected.mkdir(exist_ok=True)
        for p in (root / 'control-unit').iterdir():
            (protected / p.name).write_bytes(p.read_bytes())
        protected_before = {p.name: p.read_bytes() for p in protected.iterdir()}
        replaced = compile_capture(transport, protected, 1)
        assert diagnostic in replaced.stderr, (name, replaced.stderr)
        assert {p.name: p.read_bytes() for p in protected.iterdir()} == protected_before, name
        if target == 'or':
            assert 'loc("control.py":9:14): error:' in result.stderr, result.stderr
        assert {p.name: p.read_bytes() for p in out.iterdir() if p.is_file()} == before
        outcomes.append({'name': name, 'diagnostic': diagnostic, 'target': target, 'field': field,
                         'input_sha256': digest(transport), 'fresh_absent': True, 'replacement_unchanged': True,
                         'owning_pattern_site': 'control.py:9:14' if target == 'or' else None,
                         'preserved_output_sha256': {p.name: digest(p) for p in protected.iterdir()}})
    report = {'role': 'independent malformed-capture tests', 'source_bytes_sha256': digest(path),
              'preimage_capture_sha256': digest(preimage), 'base_compile_exit_status': 0,
              'base_lowered_body_interface_extraction_exit_status': 0, 'compiler_sha256': digest(args.source_compiler),
              'optimizer_sha256': digest(args.optimizer), 'cases': outcomes,
              'source_shape_diagnostic_scope': 'capture operation; pattern arity diagnostics retain owning original source site'}
    (root / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: valid native compile/lower + interface extraction controls; {len(outcomes)} malformed guards with fresh/replacement protection')  # noqa: T201 - standalone gate summary

if __name__ == '__main__':
    main()
