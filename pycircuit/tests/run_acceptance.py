"""Retain example stage artifacts and reproducible cycle-verification evidence."""
import json
from pathlib import Path
import subprocess
from pycircuit import analyze, compile_source
from pycircuit.ir import CompileError, load

ROOT = Path(__file__).resolve().parents[2]
BUILDS = ROOT / 'pycircuit/builds'
EVIDENCE = BUILDS / 'benchmarks/compiler-acceptance'
MODELS = [
    ('array_arbiter', 'examples/array_arbiter.py', 'ArrayArbiter', {}),
    ('dual_channel', 'examples/dual_channel.py', 'DualChannel', {}),
    ('shared_state', 'examples/shared_state.py', 'SharedState', {'choose_left': 1}),
    ('mixed_ports', 'examples/mixed_ports.py', 'MixedPorts', {'enabled': 1}),
    ('signal_link', 'examples/signal_link.py', 'SignalLink', {'enabled': 1}),
    ('cell_feedback', 'examples/dependency_cycles.py', 'CellFeedback', {}),
    ('ehr_shared', 'examples/ehr_shared.py', 'EHRShared', {}),
    ('riscv_pipeline', 'examples/riscv_pipeline/model.py', 'Pipeline', {}),
    ('ooo_scheduler', 'examples/ooo_scheduler/model.py', 'OutOfOrder', {}),
]
NEGATIVE = [
    ('ehr_slots', 'examples/ehr_slots.py', 'EHRSlots', {'release_enabled': 1}, 'incompatible operations'),
    ('exclusive_ring', 'examples/dependency_cycles.py', 'ExclusiveRing', {'choose': 1}, 'branch-union'),
    ('queue_ring', 'examples/dependency_cycles.py', 'QueueRing', {}, 'branch-union'),
]


def trace(path):
    return json.loads(path.read_text())['trace']


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
    for name, source, top, config in MODELS:
        model = compile_source(ROOT / 'pycircuit' / source, top)
        expected = None
        for optimize in (False, True):
            out = EVIDENCE / name / ('opt' if optimize else 'raw')
            graph = analyze(model, out, optimize=optimize, config=config, cycles=32)
            txn = trace(out / 'transaction-trace.json')
            assert txn == trace(out / 'rtl-trace.json'), name
            if expected is None:
                expected = txn
            else:
                assert expected == txn, name + ': optimization'
            for stage in ('source', 'transaction', 'rtl'):
                reload = out / ('reload-' + stage)
                analyze(load(out / f'model.{stage}.mlir'), reload, optimize=optimize, cycles=32)
                for file in ('transaction-trace.json', 'rtl-trace.json'):
                    if (reload / file).exists():
                        assert txn == trace(reload / file), name + ': ' + stage
        review = BUILDS / name
        clear_review_artifacts(review)
        analyze(model, review, config=config)
        subprocess.run(['python3', str(ROOT / 'pycircuit/draw_rules.py'), str(review / 'rule-graph.json'),
                        '-o', str(review / 'rule-graph.dot')], check=True)
        summary.append({'model': name, 'cycles': 32, 'transactions': len(graph['nodes']),
                        'two_stages_equal': True, 'optimization_equal': True,
                        'all_stages_reload': True, 'final_state': expected[-1]['state']})
        print(name + ': transaction/RTL, optimization and reload match', flush=True)
    for name, source, top, config, diagnostic in NEGATIVE:
        model = compile_source(ROOT / 'pycircuit' / source, top)
        review = BUILDS / name
        clear_review_artifacts(review)
        analyze(model, review, config=config, stop_after='source')
        try:
            analyze(model, EVIDENCE / name, config=config)
        except CompileError as error:
            assert diagnostic in str(error), str(error)
            (EVIDENCE / (name + '-diagnostic.log')).write_text(str(error))
            summary.append({'model': name, 'expected_diagnostic': str(error)})
            print(name + ': expected diagnostic', flush=True)
        else:
            raise AssertionError(name + ': required diagnostic absent')
    (EVIDENCE / 'summary.json').write_text(json.dumps(summary, indent=2, ensure_ascii=False) + '\n')


if __name__ == '__main__':
    main()
