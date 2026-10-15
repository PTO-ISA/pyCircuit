"""Live native RV5S vs compiled ACPy, with independent sequential expectations."""
import argparse
import hashlib
import json
import os
from pathlib import Path
from .paths import FIXTURES
import subprocess
from pycircuit import analyze, compile_source
from pycircuit.tests.support import check_cpp, check_reloads, read_trace
from .program import ROOT, ProgramImage, make_source
from .oracle import disassemble, execute

DEFAULT_OUTPUT = ROOT / 'pycircuit/builds/benchmarks/rv32imc'
NATIVE = ROOT / 'pycircuit/builds/ripes-native/ripes5-reference'


def native_capture(image, directory, runner=NATIVE):
    image.save(directory / 'input.json')
    env = dict(os.environ)
    env['LD_LIBRARY_PATH'] = str(ROOT / 'pycircuit/builds/ripes-qt/lib') + ':/home/lc/opt/gcc14/lib:' + env.get('LD_LIBRARY_PATH', '')
    command = [str(runner), str(directory / 'input.json')]
    result = subprocess.run(command, capture_output=True, text=True, env=env)
    (directory / 'native.jsonl').write_text(result.stdout)
    (directory / 'native.stderr').write_text(result.stderr)
    return result


def native_run(image, directory, runner=NATIVE):
    result = native_capture(image, directory, runner)
    if result.returncode:
        raise RuntimeError(f'native failed ({result.returncode}): {result.stderr}; input: {directory / "input.json"}')
    return [json.loads(line) for line in result.stdout.splitlines()]


def project(row, code_size):
    state = row['state']
    packets = [state['top.cpu.' + name] for name in ('if_id', 'id_ex', 'ex_mem', 'mem_wb')]
    pc = state['top.cpu.fetch.pc']
    stages = [pc if pc < code_size else None] + [p[1] if p[0] and p[1] < code_size else None for p in packets]
    event = state['top.cpu.writeback.retirement']
    retire = dict(pc=event[1], word=event[2], write=[event[3], event[5]] if event[4] else None) if event[0] else None
    event = state['top.cpu.memory.store_event']
    store = event[1:3] if event[0] else None
    return dict(pc=pc, stages=stages, stalled=[False, False, packets[1][6], packets[2][10], packets[3][6]],
                control=state['top.cpu.last_control'], registers=[state[f'top.cpu.rf.registers.{i}'] for i in range(32)],
                data=state['top.cpu.memory.words'], retire=retire, store=store,
                retired=state['top.cpu.writeback.retired'], system=state['top.cpu.system_event'] if state['top.cpu.system_event'][0] else None)


def compare_native(trace, native, image, *, collect=False):
    differences = []
    if len(trace) + 1 != len(native):
        raise AssertionError(('trace length', len(trace), len(native)))
    for i, row in enumerate(trace):
        actual = project(row, len(image.text))
        expected, before = native[i+1], native[i]
        ctrl = before['control']
        target = ctrl['target']
        edge = [ctrl['stall'], target is not None, target if target is not None else 0,
                ctrl['forward_a'], ctrl['forward_b'], ctrl['exmem_clear']]
        pairs = dict(pc=expected['fetch_pc'], stages=[pc if valid and pc < len(image.text) else None for pc,valid in zip(expected['raw']['pcs'],expected['raw']['valid'])],
                     stalled=expected['stalled'], registers=expected['registers'], data=expected['data'],
                     retire=expected['retire'], store=expected['store'], retired=expected['retired'], system=expected.get('system'))
        # Store observations carry the actual bus width in each model; mask unused high bits.
        if pairs['store']:
            size = before['raw']['mem_size']
            event = row['state']['top.cpu.memory.store_event']
            actual['store_size'] = event[3] if event[0] else None
            pairs['store_size'] = size
            # Do not mutate the saved native trace during comparison.
            pairs['store'] = [pairs['store'][0], pairs['store'][1] & ((1 << (size * 8)) - 1)]
        if 'mem_read' in before['raw']:
            event = row['state']['top.cpu.memory.load_event']
            pairs['load'] = [before['raw']['mem_address'], before['raw']['mem_value'] & ((1 << (before['raw']['mem_size']*8)) - 1), before['raw']['mem_size']] if before['raw']['mem_read'] else None
            actual['load'] = event[1:] if event[0] else None
            control = row['state']['top.cpu.last_control']
            actual['enables'] = [not (control[0] or control[5]), not control[5]]
            pairs['enables'] = [ctrl['pc_enable'], ctrl['idex_enable']]
            actual['flushes'] = [control[1] or control[6], control[0] or control[1] or control[6], control[5]]
            pairs['flushes'] = [ctrl['flush_ifid'], ctrl['flush_idex'], ctrl['exmem_clear']]
            packets = [row['state']['top.cpu.' + name] for name in ('if_id','id_ex','ex_mem','mem_wb')]
            actual['sequential_pcs'] = [packet[index] if packet[0] else None for packet,index in zip(packets,(4,7,15,10))]
            pairs['sequential_pcs'] = [pc if valid else None for pc,valid in zip(expected['raw']['sequential_pcs'],expected['raw']['valid'][1:])]
            actual['ifid_expanded'] = packets[0][3] if packets[0][0] else None
            pairs['ifid_expanded'] = expected['raw']['ifid_expanded'] if expected['raw']['valid'][1] else None
        pairs['control'] = edge
        actual['control'] = actual['control'][:6]
        for key, value in pairs.items():
            if actual[key] != value:
                difference = dict(cycle=i+1, field=key, actual=actual[key], native=value)
                if collect:
                    differences.append(difference)
                else:
                    raise AssertionError(difference | dict(before_control=before['control'], before_pcs=before['raw']['pcs'], model_control=actual['control']))
    return differences


def model_output(trace, image):
    output, events = '', []
    for row in trace:
        state = row['state']
        event = state['top.cpu.system_event']
        if not event[0]:
            continue
        events.append(event)
        _, pc, kind, number, arg = event
        if kind != 1:
            continue
        if number == 1:
            output += str(arg if arg < 0x80000000 else arg - 0x100000000)
        elif number == 11:
            output += chr(arg & 255)
        elif number == 4:
            memory = image.text.ljust(4096, b'\0') + b''.join(x.to_bytes(4, 'little') for x in state['top.cpu.memory.words'])
            while True:
                if not 0 <= arg < len(memory):
                    raise ValueError('string read outside memory')
                if not memory[arg]: break
                output += chr(memory[arg]); arg += 1
        elif number not in (10, 93):
            raise ValueError(f'unsupported ecall {number} at {pc:#x}')
    return output, events


def check_oracle(trace, image, oracle):
    actual = [project(row, len(image.text))['retire'] for row in trace]
    actual = [event for event in actual if event]
    assert actual == oracle['retired'], ('retirement vs LLVM-driven oracle', actual, oracle['retired'])
    final = project(trace[-1], len(image.text))
    assert final['registers'] == oracle['registers'], 'oracle final registers'
    ram = b''.join(x.to_bytes(4, 'little') for x in final['data'])
    assert list(ram) == oracle['ram'], 'oracle final RAM'
    output, events = model_output(trace, image)
    if oracle['reason'] == 'exit':
        assert events == oracle['events'], ('system events', events, oracle['events'])
        assert trace[-1]['state']['top.cpu.exit_status'] == oracle['status'], 'exit status'
    assert output == oracle['output'], 'oracle output'


def run(name, image, directory, *, expected_status=None, expected_output=None, sanitize=False, reload=False):
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    oracle = execute(image, disassemble(directory / 'program.elf'))
    if expected_status is not None:
        assert oracle['status'] == expected_status, ('independent program result', name, oracle['status'], expected_status)
    if expected_output is not None:
        assert oracle['output'] == expected_output, ('independent output', name)
    (directory / 'oracle.json').write_text(json.dumps(oracle, indent=2) + '\n')
    native = native_run(image, directory)
    source = make_source(directory, image)
    graph = analyze(compile_source(source, 'Top'), directory, cycles=len(native)-1, emit_cpp=True)
    trace = read_trace(directory / 'transaction-trace.json')
    assert trace == read_trace(directory / 'rtl-trace.json'), name + ': IR interpretation mismatch'
    metrics = check_cpp(trace, directory, sanitize=sanitize)
    known = json.loads((FIXTURES / 'reference-differences.json').read_text()).get(name)
    differences = []
    try:
        check_oracle(trace, image, oracle)
        if known:
            differences = compare_native(trace, native, image, collect=True)
            assert differences == known['expected_differences'], ('recorded reference defect changed', differences)
            (directory / 'reference-defect.json').write_text(json.dumps(known | {'observed_differences': differences}, indent=2) + '\n')
        else:
            compare_native(trace, native, image)
    except AssertionError as error:
        failure = dict(program=name, input_sha256=image.checksum(), detail=error.args,
                       reproduce=f'python -m pycircuit.examples.ripes5.tests.suite --case {name}')
        (directory / 'first-mismatch.json').write_text(json.dumps(failure, indent=2, ensure_ascii=False) + '\n')
        raise
    if oracle['reason'] == 'exit':
        assert native[-1]['exit_status'] == oracle['status'], 'native exit status'
        assert native[-1]['output'] == oracle['output'], 'native output'
    if reload:
        check_reloads(trace, directory)
    summary = dict(name=name, cycles=len(trace), input_sha256=image.checksum(), isa=image.isa,
                   native_equal=not differences, reference_classification='confirmed-reference-defect' if differences else 'equal', transaction_ir_equal=True, rtl_ir_equal=True, cpp_equal=True,
                   oracle_equal=True, status=oracle['status'], output=oracle['output'],
                   coverage=oracle['coverage'], transactions=len(graph['nodes']), cpp_metrics=metrics,
                   reference_binary_sha256=hashlib.sha256(NATIVE.read_bytes()).hexdigest())
    (directory / 'verification.json').write_text(json.dumps(summary, indent=2) + '\n')
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--reload', action='store_true')
    args = parser.parse_args()
    image = ProgramImage.load(args.input)
    print(json.dumps(run(args.input.parent.name, image, args.input.parent, sanitize=args.sanitize, reload=args.reload), indent=2))

if __name__ == '__main__': main()
