"""Generated / ACIR-reloaded / handwritten C++ / Python / pinned native Ripes."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
CPP_EXAMPLE = ROOT / 'gfsim/cpp/examples/ripes5'
spec = importlib.util.spec_from_file_location('handwritten_verify', CPP_EXAMPLE / 'verify.py')
v = importlib.util.module_from_spec(spec)
spec.loader.exec_module(v)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def fingerprint():
    paths = sorted(p for p in (ROOT / 'pycircuit').rglob('*') if p.suffix in ('.py', '.hpp', '.cpp')
                   and 'output' not in p.parts and '__pycache__' not in p.parts)
    return {str(p.relative_to(ROOT)): sha(p) for p in paths}


def verify(generated, emitted, handwritten, runner, output, case_name=None, acir=None):
    identity = v.verify_runner(runner)  # Missing/wrong native versions are fatal.
    cases = [c for c in v.suite() if case_name is None or c['name'] == case_name]
    if not cases:
        raise ValueError(f'unknown program: {case_name}')
    reports = []
    for case in cases:
        directory = output / case['name']
        native = v.run_reference(case, runner, directory)
        configs = []
        for reverse in (False, True):
            tag = f'reverse{int(reverse)}'
            (directory / f'{tag}.input.txt').write_text(v.numeric_input(case, reverse))
            traces = {'python': v.run_python(case, reverse)}
            for name, binary in (('handwritten', handwritten), ('generated', generated), ('emitted', emitted)):
                traces[name] = v.run_cpp(case, binary, reverse)
            hashes = {}
            for name, trace in traces.items():
                path = directory / f'{tag}.{name}.jsonl'
                v.write_jsonl(path, trace)
                v.compare(case, native, trace, directory / f'{tag}.native-{name}-mismatch.json')
                hashes[name] = sha(path)
            # Explicitly compare both generated paths to the same handwritten model.
            for name in ('generated', 'emitted'):
                v.compare(case, traces['handwritten'], traces[name], directory / f'{tag}.cpp-{name}-mismatch.json')
            configs.append(dict(reverse=reverse, rows=len(native), traces_sha256=hashes))
        reports.append(dict(name=case['name'], cycles=native[-1]['cycle'], configurations=configs))
        print(f'{case["name"]}: five implementations matched, two configurations', flush=True)
    report = dict(full_acceptance=case_name is None, configurations=sum(len(r['configurations']) for r in reports),
                  reference=identity, binaries_sha256={name: sha(binary) for name, binary in
                    (('generated', generated), ('emitted', emitted), ('handwritten', handwritten), ('native', runner))},
                  compiler_source_sha256=fingerprint(), existing_source_sha256=v.fingerprint(),
                  acir_sha256=sha(acir) if acir else None, results=reports)
    (output / 'comparison.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--generated-runner', type=Path, required=True)
    p.add_argument('--emitted-runner', type=Path, required=True)
    p.add_argument('--cpp-runner', type=Path, required=True)
    p.add_argument('--runner', type=Path, default=v.DEFAULT_RUNNER)
    p.add_argument('--output', type=Path, default=HERE / 'output')
    p.add_argument('--case')
    p.add_argument('--acir', type=Path)
    a = p.parse_args()
    verify(a.generated_runner, a.emitted_runner, a.cpp_runner, a.runner, a.output, a.case, a.acir)


if __name__ == '__main__':
    main()
