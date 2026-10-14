"""Retain example stage artifacts and reproducible cycle-verification evidence."""
import json
import subprocess
from pycircuit import analyze, compile_source
from pycircuit.tests.support import ROOT, check_reloads, read_trace

BUILDS = ROOT / 'pycircuit/builds'
EVIDENCE = BUILDS / 'benchmarks/compiler-acceptance'
MODELS = [
    ('array_arbiter', 'examples/array_arbiter.py', 'ArrayArbiter'),
    ('ehr_shared', 'examples/ehr_shared.py', 'EHRShared'),
    ('riscv_pipeline', 'examples/riscv_pipeline/model.py', 'Pipeline'),
    ('ooo_scheduler', 'examples/ooo_scheduler/model.py', 'OutOfOrder'),
]


def clear_review_artifacts(directory):
    # These are generated compiler outputs from earlier formats, preserved in
    # Git history and the pre-refactor baseline snapshot.
    directory.mkdir(parents=True, exist_ok=True)
    for path in directory.iterdir():
        if path.is_file() and path.suffix in ('.mlir', '.json', '.dot', '.svg', '.png'):
            path.unlink()


def main():
    EVIDENCE.mkdir(parents=True, exist_ok=True)
    summary = []
    for name, source, top in MODELS:
        model = compile_source(ROOT / 'pycircuit' / source, top)
        expected = None
        for optimize in (False, True):
            out = EVIDENCE / name / ('opt' if optimize else 'raw')
            graph = analyze(model, out, optimize=optimize, cycles=32)
            txn = read_trace(out / 'transaction-trace.json')
            assert txn == read_trace(out / 'rtl-trace.json'), name
            if expected is None:
                expected = txn
            else:
                assert expected == txn, name + ': optimization'
            check_reloads(txn, out, optimize)
        review = BUILDS / name
        clear_review_artifacts(review)
        analyze(model, review)
        subprocess.run(['python3', str(ROOT / 'pycircuit/draw_rules.py'), str(review / 'rule-graph.json'),
                        '-o', str(review / 'rule-graph.dot')], check=True)
        summary.append({'model': name, 'cycles': 32, 'transactions': len(graph['nodes']),
                        'two_stages_equal': True, 'optimization_equal': True,
                        'all_stages_reload': True, 'final_state': expected[-1]['state']})
        print(name + ': transaction/RTL, optimization and reload match', flush=True)
    (EVIDENCE / 'summary.json').write_text(json.dumps(summary, indent=2, ensure_ascii=False) + '\n')


if __name__ == '__main__':
    main()
