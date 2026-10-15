"""Explicit program, Ripes and compiler checks over the same ELF byte image.

    No backend-specific test source is generated here except the existing ACPy
    initial-state adapter. The program manifest and ELF can outlive that adapter.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess

from pycircuit import analyze, compile_source
from pycircuit.tests.support import cpp_compiler, check_reloads, read_trace
from .cases import build_case
from .e2e import native_capture, compare_native, check_oracle, NATIVE
from .oracle import execute, disassemble
from .paths import FIXTURES
from .program import make_source


def write_json(path, value):
    Path(path).write_text(json.dumps(value, indent=2, ensure_ascii=False, default=str) + '\n')


def prepare_program(case, directory):
    image = build_case(case, directory)
    oracle = execute(image, disassemble(directory / 'program.elf'))
    if oracle['reason'] != case['expected_reason']:
        raise AssertionError(dict(field='termination', actual=oracle['reason'], expected=case['expected_reason']))
    for field, key in [('status', 'expected_status'), ('output', 'expected_output')]:
        if case.get(key) is not None and oracle[field] != case[key]:
            raise AssertionError(dict(field=field, actual=oracle[field], expected=case[key]))
    write_json(directory / 'oracle.json', oracle)
    return image, oracle


def run_cpp(directory, cycles, sanitize=False):
    """Run the generated simulator; compilation and trace I/O are not benchmarks."""
    cpp = directory / 'cpp'
    binary = cpp / ('sim-sanitized' if sanitize else 'sim')
    flags = ['-std=c++20', '-O1']
    if sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-g']
    commands = [cpp_compiler() + flags + [str(cpp / 'sim.cpp'), '-o', str(binary)],
                [str(binary), '--cycles', str(cycles), '--trace', str(cpp / 'trace.json')]]
    write_json(directory / 'cpp-commands.json', commands)
    env = dict(os.environ)
    if sanitize:
        env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    for name, command in zip(('compile', 'run'), commands):
        result = subprocess.run(command, capture_output=True, text=True, env=env)
        (cpp / (name + '.stdout')).write_text(result.stdout)
        (cpp / (name + '.stderr')).write_text(result.stderr)
        if result.returncode:
            raise RuntimeError(f'C++ {name} failed ({result.returncode}): {result.stderr[:2000]}')
    return read_trace(cpp / 'trace.json')


def completed_trace(trace, oracle):
    """A program completes when its last expected instruction retires."""
    count = len(oracle['retired'])
    for index, row in enumerate(trace):
        if row['state']['top.cpu.writeback.retired'] == count:
            return trace[:index + 1]
    raise AssertionError(dict(field='completion', expected_retired=count,
                              actual_retired=trace[-1]['state']['top.cpu.writeback.retired']))


def check_program(trace, image, oracle):
    committed = completed_trace(trace, oracle)
    # A PC marker is an external stop condition. For terminating instructions,
    # also check the drain tail so late younger effects cannot be hidden.
    check_oracle(committed if oracle['reason'] is None else trace, image, oracle)
    if oracle['reason'] in ('break', 'illegal'):
        events = [row['state']['top.cpu.system_event'] for row in trace
                  if row['state']['top.cpu.system_event'][0]]
        kind = 2 if oracle['reason'] == 'break' else 3
        expected = oracle['events'] + [[True, oracle['retired'][-1]['pc'], kind, 0, 0]]
        if events != expected:
            raise AssertionError(dict(field='trap events', actual=events, expected=expected))
    return committed


def check_reference(case, trace, native_result, image, oracle, directory):
    """Match only documented reference outcomes; new differences always fail."""
    special = case.get('reference')
    native = [json.loads(line) for line in native_result.stdout.splitlines()]
    if special:
        if 'expected_error' in special:
            if native_result.returncode == 0 or special['expected_error'] not in native_result.stderr:
                raise AssertionError('documented native error changed')
        else:
            if native_result.returncode or not native:
                raise AssertionError('documented native execution changed')
            expected = special['expected_ram_word']
            if native[-1]['data'][expected['index']] != expected['value']:
                raise AssertionError('documented unsupported-trap outcome changed')
        write_json(directory / 'reference-difference.json', special)
        return 'unsupported' if special['classification'] == 'reference-unsupported' else 'reference-defect'
    if native_result.returncode:
        raise RuntimeError(f'native failed ({native_result.returncode}): {native_result.stderr[:2000]}')
    known = json.loads((FIXTURES / 'reference-differences.json').read_text()).get(case['name'])
    differences = compare_native(trace, native, image, collect=bool(known))
    if known:
        if differences != known['expected_differences']:
            raise AssertionError(dict(field='documented reference differences', actual=differences,
                                      expected=known['expected_differences']))
        write_json(directory / 'reference-difference.json', known | {'observed_differences': differences})
    if oracle['reason'] == 'exit':
        for field, expected in [('exit_status', oracle['status']), ('output', oracle['output'])]:
            if native[-1][field] != expected:
                raise AssertionError(dict(field='native ' + field, actual=native[-1][field], expected=expected))
    return 'reference-defect' if known else 'passed'


def run_case(case, directory, *, check='ripes', sanitize=False, build_only=False):
    directory = Path(directory).resolve()
    directory.mkdir(parents=True, exist_ok=True)
    checks = {name: 'not-run' for name in ('input', 'program', 'ripes', 'compiler')}
    result = dict(schema_version=1, name=case['name'], isa=case['isa'], check=check,
                  status='failed', checks=checks, known_issue=case.get('known_issue'),
                  regression_issue=case.get('regression_issue'),
                  rtl_simulation=False)
    stage = 'input'
    try:
        image, oracle = prepare_program(case, directory)
        result.update(input_sha256=image.checksum(), elf_sha256=hashlib.sha256((directory / 'program.elf').read_bytes()).hexdigest())
        checks['input'] = 'passed'
        if build_only:
            result['status'] = 'built'
            return result
        cycles = min(image.max_cycles, 5 * len(oracle['retired']) + 12)
        native_result = None
        if check in ('ripes', 'all'):
            stage = 'ripes-execution'
            native_result = native_capture(image, directory)
            result['reference_binary_sha256'] = hashlib.sha256(NATIVE.read_bytes()).hexdigest()
            if not case.get('reference'):
                if native_result.returncode:
                    raise RuntimeError(f'native failed: {native_result.stderr[:2000]}')
                cycles = len(native_result.stdout.splitlines()) - 1
                if not 0 < cycles <= image.max_cycles:
                    raise ValueError('native cycle count outside program bound')
        compiler_checks = check in ('compiler', 'all')
        stage = 'model-compilation'
        analyze(compile_source(make_source(directory, image), 'Top'), directory,
                stop_after='rtl' if compiler_checks else 'transaction',
                cycles=cycles if compiler_checks else 0, emit_cpp=True)
        stage = 'model-execution'
        trace = run_cpp(directory, cycles, sanitize)
        stage = 'program'
        committed = check_program(trace, image, oracle)
        checks['program'] = 'passed'
        result.update(cycles=len(committed), executed_cycles=len(trace), status='passed',
                      exit_status=oracle['status'], output=oracle['output'], coverage=oracle['coverage'])
        if native_result is not None:
            stage = 'ripes-comparison'
            checks['ripes'] = check_reference(case, trace, native_result, image, oracle, directory)
            result['status'] = checks['ripes']
        if compiler_checks:
            stage = 'compiler'
            for filename in ('transaction-trace.json', 'rtl-trace.json'):
                if read_trace(directory / filename) != trace:
                    raise AssertionError(filename + ' differs from generated C++')
            check_reloads(trace, directory)
            checks['compiler'] = 'passed'
        return result
    except Exception as error:
        result['status'] = 'failed'
        failed_check = 'ripes' if stage.startswith('ripes') else 'compiler' if stage == 'compiler' else 'input' if stage == 'input' else 'program'
        checks[failed_check] = 'failed'
        (directory / 'failure.log').write_text(str(error) + '\n')
        failure = dict(program=case['name'], stage=stage, error_type=type(error).__name__,
                       detail=error.args[0] if isinstance(error, AssertionError) and error.args else str(error)[:2000],
                       reproduce=f'python -m pycircuit.examples.ripes5.tests run --case {case["name"]} --check {check}' + (' --sanitize' if sanitize else ''))
        if 'elf' in case:
            import shlex
            failure['reproduce'] = ('python -m pycircuit.examples.ripes5.tests run --elf ' +
                                    shlex.quote(case['elf']) + ' --isa ' + case['isa'] + ' --check ' + check +
                                    ' --max-cycles ' + str(image.max_cycles if 'image' in locals() else case.get('max_cycles', 5000)))
            if case.get('expected_status') is not None:
                failure['reproduce'] += ' --expect-status ' + str(case['expected_status'])
            if case.get('expected_output') is not None:
                failure['reproduce'] += ' --expect-output ' + shlex.quote(case['expected_output'])
            if sanitize:
                failure['reproduce'] += ' --sanitize'
        write_json(directory / 'first-mismatch.json', failure)
        result['failure'] = failure
        return result
    finally:
        write_json(directory / 'result.json', result)
