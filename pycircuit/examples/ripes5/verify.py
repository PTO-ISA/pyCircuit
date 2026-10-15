"""Compile instruction images and compare complete cycles with captured RV5S runs.

The checked-in fixture is a compact, lossless projection of native Ripes traces.
Changes to registers/data reconstruct the full architectural snapshots. Control
signals in an after-edge model row are compared with the preceding native row:
last_control records the combinational decisions used at that edge.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

from pycircuit import analyze, compile_source
from pycircuit.tests.support import check_cpp, read_trace

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
DEFAULT_OUTPUT = ROOT / 'pycircuit/builds/benchmarks/ripes5'


def load_reference():
    return json.loads((HERE / 'reference.json').read_text())


def make_source(directory, case):
    directory.mkdir(parents=True, exist_ok=True)
    module = directory / 'model.py'
    if not module.exists():
        module.symlink_to(HERE / 'model.py')
    image = case['input']

    def array(values, size):
        if len(values) > size:
            raise ValueError(f'input exceeds fixed array capacity {size}')
        return repr(values + [0] * (size - len(values)))

    text = f'''from pycircuit import ac
from model import Ripes5, Program, Registers, DataWords

@ac.module
class Top:
    def __init__() -> None:
        program: Program = {array(image['words'], 256)}
        registers: Registers = {array(image['registers'], 32)}
        data: DataWords = {array(image['data'], 64)}
        cpu = Ripes5(program, registers, data, {image['data_base']}, {len(image['words']) * 4})

    def __call__() -> None:
        cpu()
'''
    source = directory / 'source.py'
    source.write_text(text)
    return source


def compare_reference(trace, case):
    image, expected = case['input'], case['trace']
    assert len(trace) == len(expected), case['name']
    registers, data = list(image['registers']), list(image['data'])
    for actual, reference in zip(trace, expected):
        state = actual['state']
        context = (case['name'], reference['cycle'])
        for index, value in reference['register_changes']:
            registers[index] = value
        for index, value in reference['memory_changes']:
            data[index] = value
        assert [state[f'top.cpu.rf.registers.{i}'] for i in range(32)] == registers, (context, 'registers')
        assert [state[f'top.cpu.memory.words.{i}'] for i in range(len(data))] == data, (context, 'memory')
        assert all(state[f'top.cpu.memory.words.{i}'] == 0 for i in range(len(data), 64)), (context, 'memory padding')
        pc = state['top.cpu.fetch.pc']
        packets = [state['top.cpu.if_id'], state['top.cpu.id_ex'],
                   state['top.cpu.ex_mem'], state['top.cpu.mem_wb']]
        code_size = len(image['words']) * 4
        stages = [pc if pc < code_size else None]
        stages += [packet[1] if packet[0] and packet[1] < code_size else None for packet in packets]
        assert pc == reference['pc'], (context, 'pc', pc, reference['pc'])
        assert stages == reference['stages'], (context, 'stages', stages, reference['stages'])
        assert [False, False, packets[1][6], packets[2][10], packets[3][6]] == reference['stalled'], (context, 'stalled markers')
        assert state['top.cpu.last_control'] == reference['edge_control'], (context, 'edge control', state['top.cpu.last_control'], reference['edge_control'])
        event = state['top.cpu.writeback.retirement']
        retire = None
        if event[0]:
            retire = dict(pc=event[1], word=event[2], write=[event[3], event[5]] if event[4] else None)
        assert retire == reference['retire'], (context, 'retire', retire, reference['retire'])
        event = state['top.cpu.memory.store_event']
        store = [event[1], event[2]] if event[0] else None
        assert store == reference['store'], (context, 'store')
        assert state['top.cpu.writeback.retired'] == reference['retired'], (context, 'retired count')


def run_case(case, directory, optimize=True, *, reload=False):
    source = make_source(directory, case)
    model = compile_source(source, 'Top')
    graph = analyze(model, directory, cycles=len(case['trace']), emit_cpp=True, optimize=optimize)
    trace = read_trace(directory / 'transaction-trace.json')
    assert trace == read_trace(directory / 'rtl-trace.json'), case['name']
    metrics = check_cpp(trace, directory)
    compare_reference(trace, case)
    if reload:
        from pycircuit.tests.support import check_reloads
        check_reloads(trace, directory, optimize)
    summary = dict(case=case['name'], cycles=len(trace), transactions=len(graph['nodes']),
                   native_ripes_capture_equal=True, cpp_and_ir_equal=True, cpp_metrics=metrics,
                   source_sha256=hashlib.sha256(source.read_bytes()).hexdigest())
    (directory / 'verification.json').write_text(json.dumps(summary, indent=2) + '\n')
    return trace, summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument('--case', action='append', help='case names; default: all captured programs')
    parser.add_argument('--no-opt', action='store_true')
    parser.add_argument('--reload', action='store_true')
    args = parser.parse_args()
    reference = load_reference()
    cases = reference['cases']
    if args.case:
        unknown = set(args.case) - {case['name'] for case in cases}
        if unknown:
            parser.error(f'unknown cases: {sorted(unknown)}')
        cases = [case for case in cases if case['name'] in args.case]
    summaries = []
    for case in cases:
        _, summary = run_case(case, args.output / case['name'], not args.no_opt, reload=args.reload)
        summaries.append(summary)
        print(f"{case['name']}: {summary['cycles']} cycles match native Ripes capture and C++/IR", flush=True)
    (args.output / 'summary.json').write_text(json.dumps(dict(reference=reference['provenance'], results=summaries), indent=2) + '\n')
    review = ROOT / 'pycircuit/builds/ripes5'
    analyze(compile_source(HERE / 'demo.py', 'Demo'), review, emit_cpp=True)
    subprocess.run(['python', str(ROOT / 'pycircuit/draw_rules.py'), str(review / 'rule-graph.json'),
                    '-o', str(review / 'rule-graph.dot')], check=True)


if __name__ == '__main__':
    main()
