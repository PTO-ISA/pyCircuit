"""Bounded-memory trace gates and common fixed-window result checks."""
from collections import deque
from contextlib import ExitStack
import hashlib
import json
import subprocess
import tempfile
import threading
import time
from ..tests.verify import v
from examples.ripes5.tests.run import differences, native_env


def normalized(row):
    return {k: value for k, value in row.items() if k != 'raw'}


def state(row):
    return {k: value for k, value in row.items() if k not in ('raw', 'retire', 'store')}


def prepare(case, directory):
    directory.mkdir(parents=True, exist_ok=True)
    path = (directory / 'input.json').resolve()
    path.write_text(json.dumps(case, indent=2) + '\n')
    (directory / 'input.txt').write_text(v.numeric_input(case))
    return path


def stream_verify(case, binaries, directory, warmup, timeout=300):
    """Compare four live pipes; retain seven rows on failure and hashes on success.

    Pipe backpressure bounds producer memory. A watchdog kills stalled producers,
    including one blocked mid-line. Stderr goes to disk to avoid a second pipe
    deadlock. No complete trace is materialized or saved.
    """
    path = prepare(case, directory)
    jobs = [(label, [str(binary.resolve())] + ([str(path)] if label == 'native' else []))
            for label, binary in binaries.items()]
    jobs.append(('native_observed', [str(binaries['native'].resolve()), str(path), '--observe']))
    processes = []
    expired = threading.Event()

    def stop():
        expired.set()
        for proc in processes:
            if proc.poll() is None:
                proc.kill()

    hashes = {label: hashlib.sha256() for label, _ in jobs}
    raw_hashes = {label: hashlib.sha256() for label, _ in jobs}
    history, failure, remaining = deque(maxlen=3), None, 0
    initial = final = boundary = None
    rows = 0
    with ExitStack() as stack:
        errors = {}
        timer = threading.Timer(timeout, stop)
        try:
            for label, command in jobs:
                source = stack.enter_context((directory / 'input.txt').open())
                errors[label] = stack.enter_context(tempfile.TemporaryFile(mode='w+t'))
                processes.append(subprocess.Popen(command, stdin=source, stdout=subprocess.PIPE,
                                                   stderr=errors[label], env=native_env(), text=True))
            timer.start()
            while True:
                lines = [proc.stdout.readline() for proc in processes]
                if not any(lines):
                    break
                current = {}
                for (label, _), line in zip(jobs, lines):
                    row = json.loads(line) if line else None
                    raw_hashes[label].update(line.encode())
                    current[label] = normalized(row) if row is not None else None
                    hashes[label].update((json.dumps(current[label], sort_keys=True) + '\n').encode())
                reference = current['native']
                if failure is None:
                    diffs = {label: differences(reference, row) for label, row in current.items()
                             if row != reference}
                    # Observation toggles must preserve even native-only diagnostics.
                    native_index = next(i for i, (label, _) in enumerate(jobs) if label == 'native')
                    if json.loads(lines[native_index] or 'null') != json.loads(lines[-1] or 'null'):
                        diffs['native_raw'] = ['observation toggle changed raw diagnostics']
                    if diffs:
                        failure = dict(name=case['name'], first_row=rows, differences=diffs,
                                       context=list(history) + [current])
                        remaining = 3
                    else:
                        history.append(current)
                else:
                    failure['context'].append(current)
                    remaining -= 1
                if failure is not None and remaining == 0:
                    break
                if rows == 0:
                    initial = reference
                if rows == warmup:
                    boundary = reference
                final = reference
                rows += 1
                if rows % 50000 == 0:
                    print(f'  {case["name"]}: compared {rows} cycles', flush=True)
            if failure:
                mismatch = directory / 'first-mismatch.json'
                mismatch.write_text(json.dumps(failure, indent=2) + '\n')
                raise AssertionError(f'trace mismatch: {mismatch}')
            for (label, _), proc in zip(jobs, processes):
                code = proc.wait(timeout=10)
                errors[label].seek(0)
                error = errors[label].read()
                if code or expired.is_set():
                    raise RuntimeError(f'{label}: returncode={code}, timeout={expired.is_set()}: {error}')
        finally:
            timer.cancel()
            for proc in processes:
                if proc.poll() is None:
                    proc.kill()
                proc.wait()
                proc.stdout.close()
    if final is None or (final['retire'] or {}).get('pc') != case['end_pc']:
        raise AssertionError('trace did not terminate at marker retirement')
    if boundary is None or final['cycle'] <= warmup:
        raise ValueError('warmup must precede marker retirement')
    if initial['cycle'] != 0 or boundary['cycle'] != warmup or rows != final['cycle'] + 1:
        raise AssertionError('trace cycle numbering changed')
    result = dict(passed=True, rows=rows, cycles=final['cycle'], initial=initial,
                  boundary=boundary, final=final, observation_toggle_identical=True,
                  trace_sha256={k: h.hexdigest() for k, h in hashes.items()},
                  raw_trace_sha256={k: h.hexdigest() for k, h in raw_hashes.items()})
    (directory / 'verification.json').write_text(json.dumps(result, indent=2) + '\n')
    return result


def fixed_sample(binary, case, input_path, warmup, measured, boundary, final, *, native=False):
    command = [str(binary.resolve())] + ([str(input_path)] if native else [])
    command += ['--benchmark-fixed', str(warmup), str(measured)]
    payload = None if native else v.numeric_input(case)
    environment = native_env()
    start = time.perf_counter_ns()
    result = subprocess.run(command, input=payload,
                            capture_output=True, text=True, check=True, timeout=120, env=environment)
    wall = time.perf_counter_ns() - start
    values = json.loads(result.stdout)
    expected = dict(cycles=warmup + measured, warmup_cycles=warmup, measured_cycles=measured,
                    retired_before=boundary['retired'], retired=final['retired'],
                    retired_delta=final['retired'] - boundary['retired'])
    if any(values.get(k) != value for k, value in expected.items()):
        raise AssertionError(f'fixed window counter mismatch: {values}')
    if state(values['final_state']) != state(final):
        raise AssertionError(f'fixed window final state mismatch: {differences(state(final), state(values["final_state"]))}')
    if native and any(values[k] for k in ('port_notifications', 'clock_notifications', 'reverse_history')):
        raise AssertionError('native observation/history must be disabled')
    if values['run_ns'] <= 0:
        raise AssertionError('non-positive measurement')
    # Full state is checked for every sample, then retain a canonical digest.
    final_state = values.pop('final_state')
    values['final_state_sha256'] = hashlib.sha256(json.dumps(state(final_state), sort_keys=True).encode()).hexdigest()
    return dict(values, process_ns=wall, ns_per_cycle=values['run_ns'] / measured,
                instructions_per_second=values['retired_delta'] * 1e9 / values['run_ns'])
