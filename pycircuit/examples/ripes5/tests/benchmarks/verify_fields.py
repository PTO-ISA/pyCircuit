"""Run compiler field-wiring acceptance using the standard CPU case runner."""
import argparse
from collections import Counter
from concurrent.futures import ProcessPoolExecutor, as_completed
import hashlib
import json
from pathlib import Path

from ..cases import load_cases
from ..paths import ROOT, MODEL_DIR
from ..runner import run_case, write_json
from .split_control import split_source


def run(task):
    variant_name, case, directory, variant = task
    directory.mkdir(parents=True, exist_ok=True)
    if variant_name == 'field_arguments':
        (directory/'model.py').write_text(variant)
        (directory/'isa.py').write_bytes((MODEL_DIR/'isa.py').read_bytes())
    # Serialization has process-local naming tables. Each case needs its own
    # process, just like the standard CLI, rather than concurrent threads.
    result = run_case(case, directory, check='all', sanitize=case['group']=='boundary')
    return dict(variant=variant_name, **result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'pycircuit/builds/benchmarks/field-analysis-fix/acceptance')
    parser.add_argument('--workers', type=int, default=8)
    args = parser.parse_args()
    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    sources = [Path(__file__), Path(__file__).with_name('split_control.py'), ROOT/'pycircuit/frontend.py', ROOT/'pycircuit/mlir_text.py', MODEL_DIR/'model.py', MODEL_DIR/'isa.py', *sorted((ROOT/'pycircuit/mlir').glob('*.cpp')), *sorted((ROOT/'pycircuit/mlir').glob('*.h'))]
    hashes = {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
    cases = load_cases()
    tasks = [('packed', c) for c in cases]
    tasks += [('field_arguments', c) for c in cases if c.get('smoke') or c['name'] in ('upstream_rvc', 'c_matrix_imc')]
    variant = split_source((MODEL_DIR/'model.py').read_text(), True, True)
    (out/'field-arguments-model.py').write_text(variant)
    write_json(out/'provenance.json', dict(source_sha256=hashes, workers=args.workers,
        variants={'packed':'production model', 'field_arguments':'Control retained, only needed fields passed as scalar arguments'},
        cases=[dict(variant=v, name=c['name']) for v,c in tasks]))

    results = []
    with ProcessPoolExecutor(max_workers=args.workers) as pool:
        futures = [pool.submit(run, (v,c,out/v/c['name'],variant)) for v,c in tasks]
        for future in as_completed(futures):
            result = future.result(); results.append(result)
            write_json(out/'summary.json', dict(results=sorted(results, key=lambda r:(r['variant'],r['name'])),
                counts=dict(Counter(r['status'] for r in results)), planned=len(tasks)))
            print(f'{len(results)}/{len(tasks)} {result["variant"]}/{result["name"]}: {result["status"]}', flush=True)
    assert all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==h for p,h in hashes.items()), 'sources changed during acceptance'
    assert not any(r['status']=='failed' for r in results), 'see summary/first-mismatch.json'


if __name__ == '__main__':
    main()
