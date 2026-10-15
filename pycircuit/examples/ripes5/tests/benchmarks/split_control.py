"""Compare packed Control with scalar interfaces, without production edits."""
import argparse
import ast
import copy
import difflib
import hashlib
import json
import os
from pathlib import Path
import subprocess

from pycircuit import analyze, compile_source
from pycircuit.ir import save
from pycircuit.tests.support import check_cpp, read_trace
from ..paths import MODEL_DIR, ROOT
from ..program import ProgramImage, make_source
from ..e2e import native_capture, compare_native, NATIVE
from .inspect_c import metrics, cpp_functions


FIELDS = ('stall', 'redirect', 'target', 'forward_a', 'forward_b', 'ecall_stall', 'stop')
NEEDED = dict(Fetch=('stall', 'redirect', 'target', 'ecall_stall', 'stop'),
              Decode=('stall', 'redirect', 'ecall_stall', 'stop'),
              Execute=('ecall_stall',), Ripes5=FIELDS)
CALLEES = dict(fetch='Fetch', decode_stage='Decode', execute='Execute', observe='Ripes5')


def split_source(source, needed_only, field_arguments=False):
    tree = ast.parse(source)
    top = next(n for n in tree.body if isinstance(n, ast.ClassDef) and n.name == 'Ripes5')
    call = next(n for n in top.body if isinstance(n, ast.FunctionDef) and n.name == '__call__')
    assignment = next(n for n in call.body if isinstance(n, ast.Assign) and
                      any(isinstance(t, ast.Name) and t.id == 'control' for t in n.targets))
    assert isinstance(assignment.value, ast.Call) and assignment.value.func.id == 'Control'
    values = dict(zip(FIELDS, assignment.value.args))
    assert len(values) == len(FIELDS)

    class Split(ast.NodeTransformer):
        owner = None

        def fields(self, owner):
            return NEEDED[owner] if needed_only else FIELDS

        def visit_ClassDef(self, node):
            previous = self.owner
            self.owner = node.name
            node = self.generic_visit(node)
            self.owner = previous
            return node

        def visit_FunctionDef(self, node):
            args = []
            for arg in node.args.args:
                if arg.arg == 'control':
                    for name in self.fields(self.owner):
                        annotation = 'ac.u32' if name in ('target', 'forward_a', 'forward_b') else 'bool'
                        args.append(ast.arg(arg=name, annotation=ast.parse(annotation, mode='eval').body))
                else:
                    args.append(arg)
            node.args.args = args
            return self.generic_visit(node)

        def visit_Assign(self, node):
            if node is assignment:
                return node if field_arguments else None
            return self.generic_visit(node)

        def visit_Attribute(self, node):
            if isinstance(node.value, ast.Name) and node.value.id == 'control':
                if self.owner == 'Ripes5' and field_arguments:
                    return node
                assert node.attr in self.fields(self.owner)
                return ast.copy_location(ast.Name(id=node.attr, ctx=node.ctx), node)
            return self.generic_visit(node)

        def visit_Call(self, node):
            args = []
            for arg in node.args:
                if isinstance(arg, ast.Name) and arg.id == 'control':
                    assert isinstance(node.func, ast.Name)
                    owner = CALLEES[node.func.id] if self.owner == 'Ripes5' else self.owner
                    for name in self.fields(owner):
                        if self.owner != 'Ripes5':
                            args.append(ast.Name(id=name, ctx=ast.Load()))
                        elif field_arguments:
                            args.append(ast.Attribute(value=ast.Name(id='control', ctx=ast.Load()), attr=name, ctx=ast.Load()))
                        else:
                            args.append(copy.deepcopy(values[name]))
                else:
                    args.append(arg)
            node.args = args
            return self.generic_visit(node)

        def visit_Name(self, node):
            # Keep the trace's last_control state identical; rebuild the record
            # only inside observe, never as an input to another Rule/Module.
            if node.id == 'control':
                assert self.owner == 'Ripes5' and isinstance(node.ctx, ast.Load)
                return ast.Call(func=ast.Name(id='Control', ctx=ast.Load()),
                                args=[ast.Name(id=name, ctx=ast.Load()) for name in FIELDS], keywords=[])
            return node

    tree = ast.fix_missing_locations(Split().visit(tree))
    if not field_arguments:
        assert not any(isinstance(n, ast.Name) and n.id == 'control' for n in ast.walk(tree))
    assert not any(isinstance(n, ast.arg) and n.arg == 'control' for n in ast.walk(tree))
    return ast.unparse(tree)+'\n'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check_reference(out, summary):
    results = []
    for mode in ('c_off', 'c_on'):
        image = ProgramImage.load(out/'packed'/mode/'input.json')
        bounded = ProgramImage(image.text, image.ram, image.registers, image.isa,
                               image.entry, image.end_pc, summary['cycles'])
        directory = out/'reference'/mode; directory.mkdir(parents=True, exist_ok=True)
        captured = native_capture(bounded, directory)
        # This input loops forever. The normal reference protocol requires a
        # retired end marker; accept only its exact window-limit outcome and
        # require every sample, rather than claiming program completion.
        assert captured.returncode == 1 and captured.stderr.strip() == 'end marker did not retire within max_cycles', captured.stderr
        native = [json.loads(line) for line in captured.stdout.splitlines()]
        assert len(native) == summary['cycles']+1
        for case in (r for r in summary['cases'] if r['mode'] == mode):
            compare_native(read_trace(out/case['variant']/mode/'transaction-trace.json'), native, bounded)
            case['native_reference_equal'] = True
            results.append(dict(variant=case['variant'], mode=mode, cycles=summary['cycles'], passed=True))
    (out/'reference-summary.json').write_text(json.dumps(dict(native_binary_sha256=sha(NATIVE),
        termination='expected cycle limit for nonterminating loop; not program completion', results=results), indent=2)+'\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'pycircuit/builds/benchmarks/ripes5-standard/control-split-20261015')
    parser.add_argument('--cycles', type=int, default=64)
    args = parser.parse_args()
    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    original = (MODEL_DIR/'model.py').read_text()
    variants = dict(packed=original, scalar_all=split_source(original, False),
                    scalar_needed=split_source(original, True),
                    field_arguments=split_source(original, True, True))
    for name, source in variants.items():
        directory = out/'models'/name; directory.mkdir(parents=True, exist_ok=True)
        (directory/'model.py').write_text(source)
        (directory/'isa.py').write_bytes((MODEL_DIR/'isa.py').read_bytes())
        (directory/'change.diff').write_text(''.join(difflib.unified_diff(original.splitlines(True), source.splitlines(True), fromfile='packed/model.py', tofile=name+'/model.py')))
    image = ProgramImage.load(ROOT/'pycircuit/builds/benchmarks/rv32imc-performance/im/input.json')
    inputs = {name: ProgramImage(image.text, image.ram, image.registers[:], isa, image.entry,
                                image.end_pc, image.max_cycles) for name, isa in [('c_off', 'IM'), ('c_on', 'IMC')]}
    sources = [MODEL_DIR/'model.py', MODEL_DIR/'isa.py', ROOT/'pycircuit/mlir/CppLowering.cpp', ROOT/'pycircuit/mlir/CppEmitter.cpp']
    summary = dict(cycles=args.cycles, same_program_bytes=True, production_source_sha256={str(p.relative_to(ROOT)):sha(p) for p in sources}, cases=[])
    traces = {}
    diagnostic = ROOT/'pycircuit/builds/benchmarks/ripes5-standard/c-cause-20261015/acir-compile-diagnostic'
    for variant in variants:
        for mode, program in inputs.items():
            directory = out/variant/mode
            wrapper = make_source(directory, program)
            program.save(directory/'input.json')
            for filename in ('model.py', 'isa.py'):
                dest = directory/filename
                if dest.is_symlink(): dest.unlink()
                dest.write_bytes((out/'models'/variant/filename).read_bytes())
            model = compile_source(wrapper, 'Top')
            save(model, directory/'model.frontend.mlir')
            analyze(model, directory, cycles=args.cycles, emit_cpp=True)
            trace = read_trace(directory/'transaction-trace.json')
            assert trace == read_trace(directory/'rtl-trace.json')
            check_cpp(trace, directory)
            expected = traces.get(('packed', mode), trace)
            if trace != expected:
                for cycle, (actual, reference) in enumerate(zip(trace, expected), 1):
                    if actual != reference:
                        (directory/'first-mismatch.json').write_text(json.dumps(dict(cycle=cycle, actual=actual, packed=reference), indent=2)+'\n')
                        raise AssertionError(f'{variant}/{mode}: cycle {cycle} differs from packed')
            traces[variant, mode] = trace
            assert trace == traces[variant, 'c_off']
            record = dict(variant=variant, mode=mode, input_sha256=program.checksum(),
                          stages={s:metrics(directory/f'model.{s}.mlir') for s in ('frontend', 'source', 'transaction', 'rtl')},
                          cpp=metrics(directory/'cpp/model.hpp'), methods=cpp_functions(directory/'cpp/model.hpp'),
                          cpp_and_both_ir_equal=True, trace_equal_to_packed=True)
            statistics = directory/'cpp/lowering.json'
            if statistics.exists():
                record['lowering'] = json.loads(statistics.read_text())
                record['conditional_cycles'] = record['lowering']['conditional_cycles']
            elif diagnostic.exists():
                provenance = json.loads((diagnostic.parent/'manifest.json').read_text())
                assert provenance['production_source_sha256'] == sha(ROOT/'pycircuit/mlir/CppLowering.cpp'), 'historical diagnostic compiler does not match current lowerer'
                with (directory/'diagnostic.log').open('w') as log:
                    subprocess.run([str(diagnostic), str(directory/'model.transaction.mlir'), str(directory/'diagnostic'), '--stop-after', 'transaction', '--emit-cpp'],
                                   env=dict(os.environ, LD_LIBRARY_PATH='/home/lc/opt/gcc14/lib'), stdout=log, stderr=log, check=True)
                assert (directory/'diagnostic/cpp/model.hpp').read_bytes() == (directory/'cpp/model.hpp').read_bytes()
                record['conditional_cycles'] = (directory/'diagnostic.log').read_text().count('CYCLE root ')
            summary['cases'].append(record)
            (out/'summary.json').write_text(json.dumps(summary, indent=2)+'\n')
            fetch = next(m for m in record['methods'] if '.Fetch.try_advance' in m['name'])
            print(f'{variant}/{mode}: C++ {record["cpp"]["lines"]} lines; Fetch {fetch["lines"]}; equal {args.cycles} cycles; cycles-in-demand-graph {record.get("conditional_cycles")}', flush=True)
    for p in sources:
        assert sha(p) == summary['production_source_sha256'][str(p.relative_to(ROOT))]
    summary['production_sources_unchanged'] = True
    summary['all_traces_equal'] = all(t == next(iter(traces.values())) for t in traces.values())
    for mode in inputs:
        baseline = next(c for c in summary['cases'] if c['variant'] == 'scalar_needed' and c['mode'] == mode)
        for case in (c for c in summary['cases'] if c['mode'] == mode and c['variant'] in ('packed', 'field_arguments')):
            assert case['cpp']['bytes'] <= 2*baseline['cpp']['bytes'], case['variant']+' code grew beyond scalar control'
    check_reference(out, summary)
    summary['experiment_script_sha256'] = sha(Path(__file__))
    (out/'summary.json').write_text(json.dumps(summary, indent=2)+'\n')


if __name__ == '__main__':
    main()
