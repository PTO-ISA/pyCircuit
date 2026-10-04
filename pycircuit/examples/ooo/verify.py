"""Commit oracle, microarchitecture invariants, and eight-way cycle parity."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT))
from pycircuit.examples.ooo.programs import suite
from pycircuit.examples.ooo.reference import interpret


def numeric_input(case, cache=True, reverse=False):
    values = [1, case['max_cycles'], case['base'], int(cache), int(reverse),
              case['wb_period'], case['wb_closed'], len(case['words']), len(case['data']),
              *case['words'], *case['registers'], *case['data']]
    return ' '.join(map(str, values)) + '\n'


def run(binary, case, cache=True, reverse=False, benchmark=False):
    cmd = [str(binary)] + (['--benchmark'] if benchmark else [])
    proc = subprocess.run(cmd, input=numeric_input(case, cache, reverse), capture_output=True,
                          text=True, timeout=60)
    rows = [json.loads(line) for line in proc.stdout.splitlines()]
    if proc.returncode:
        error = AssertionError(f'{binary}: {proc.stderr.strip()}')
        error.rows = rows
        raise error
    return rows


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def context(path, rows, cycle, message, expected=None):
    path.write_text(json.dumps(dict(error=message, cycle=cycle, expected=expected,
                                    context=rows[max(0, cycle - 3):cycle + 4]), indent=2) + '\n')


def architecture(case, rows, path):
    expected = interpret(case['words'], case['registers'], case['data'], case['base'])
    regs, data, index = case['registers'], case['data'], 0
    for row in rows:
        try:
            if row['retire']:
                assert index < len(expected), 'extra retirement'
                ref = expected[index]
                observed = {k: row['retire'][k] for k in ('pc', 'word', 'write', 'store', 'fault', 'halt')}
                assert observed == {k: ref[k] for k in observed}, f'commit {index}: {observed} != {ref}'
                regs, data = ref['registers'], ref['data']
                index += 1
            assert row['registers'] == regs, 'architectural registers changed outside correct commit'
            assert row['data'] == data, 'memory changed outside correct store commit'
        except AssertionError as error:
            context(path, rows, row['cycle'], str(error), expected[max(0, index - 2):index + 2])
            raise
    assert index == len(expected), 'missing retirement'
    assert rows[-1]['stopped'], 'model did not stop'
    return len(expected)


def tag(event):
    return tuple(event['tag'])


def microarchitecture(rows, path):
    issued, completed, retired, allocation = {}, {}, set(), {}
    completion_cycle = {}
    executed, memory_start, slots = {}, {}, {}
    coverage = {k: 0 for k in ('out_of_order_issue', 'out_of_order_complete', 'dual_issue',
                'full_window', 'memory_busy', 'result_backpressure', 'request_backpressure',
                'wakeup_under_backpressure', 'flush_inflight', 'slot_reuse', 'epoch_slot_reuse',
                'squashed_memory_fault', 'squashed_store', 'operand_survives_reuse',
                'load_waited_for_store', 'three_tick_memory', 'one_tick_integer')}
    coverage['max_occupancy'] = max(r['occupancy'] for r in rows)
    for previous, row in zip(rows, rows[1:]):
        cycle = row['cycle']
        try:
            assert 0 <= row['occupancy'] <= 8
            assert len(row['allocate']) <= 1
            active = [e for e in previous['window'] if e['active']]
            if row['occupancy'] == 8:
                coverage['full_window'] += 1
            if previous['memory_remaining']:
                coverage['memory_busy'] += 1
            coverage['load_waited_for_store'] += sum(e['word'] & 127 == 3 and tag(e) not in issued
                and any(o['word'] & 127 == 0x23 and tag(o) < tag(e) for o in active) for e in active)
            for e in row['allocate']:
                t = tag(e)
                assert t not in allocation, 'tag allocated twice'
                allocation[t] = e
                s = (t[1] - 1) % 8
                if s in slots:
                    coverage['slot_reuse'] += slots[s][0] == t[0]
                    coverage['epoch_slot_reuse'] += slots[s][0] != t[0]
                slots[s] = t
            if row['issue_int'] and row['issue_mem']:
                coverage['dual_issue'] += 1
            for lane, memory in (('int', False), ('mem', True)):
                assert len(row['issue_' + lane]) <= 1
                ready = [e for e in active if e['tag'] == e['operands_tag'] and e['left'][0] and e['right'][0]
                         and tag(e) not in issued and ((e['word'] & 127) in (3, 0x23)) == memory]
                ready = [e for e in ready if e['word'] & 127 != 3 or not any(
                    o['word'] & 127 == 0x23 and tag(o) < tag(e) for o in active)]
                for event in row['issue_' + lane]:
                    t = tag(event)
                    assert t not in issued, 'instruction issued twice'
                    assert ready and t == min(tag(e) for e in ready), f'{lane}: not oldest ready: {event}, ready={ready}'
                    coverage['out_of_order_issue'] += any(tag(e) < t and tag(e) not in issued for e in active)
                    operand_state = next(e for e in ready if tag(e) == t)
                    for side in ('left', 'right'):
                        producer = operand_state[side][2]
                        if producer[1] and previous['window'][(producer[1] - 1) % 8]['tag'] != producer:
                            coverage['operand_survives_reuse'] += 1
                    issued[t] = cycle
                    assert row['inflight'][1 if memory else 0] == event['tag'], 'issue marker without request push'
                for event in row['complete_' + lane]:
                    t = tag(event)
                    assert t in issued and t not in completed, 'completion without unique issue'
                    assert t[0] == previous['epoch'], 'old generation wrote ROB'
                    coverage['out_of_order_complete'] += any(tag(e) < t and tag(e) not in completed for e in active)
                    completed[t] = event
                    completion_cycle[t] = cycle
                    assert previous['inflight'][3 if memory else 2] == event['tag'], 'writeback without prior completion'
            if row['retire']:
                t = tag(row['retire'])
                assert t == (previous['epoch'], previous['head']), 'retirement not in order'
                assert t in completed and t not in retired
                assert completion_cycle[t] < cycle, 'commit before completed state was visible'
                retired.add(t)
            for old, new in zip(previous['window'], row['window']):
                if old['operands_tag'] == new['operands_tag']:
                    for side in ('left', 'right'):
                        if old[side][0]:
                            assert old[side] == new[side], 'captured operand lost or changed'
                        elif new[side][0] and any(previous['queues'][1:]):
                            coverage['wakeup_under_backpressure'] += 1
            for q in range(4):
                a, b = previous['inflight'][q], row['inflight'][q]
                if a and a == b:
                    if q >= 2:
                        coverage['result_backpressure'] += 1
                    else:
                        coverage['request_backpressure'] += 1
                        assert not row['issue_int' if q == 0 else 'issue_mem'], 'issue marked despite blocked push'
                if q >= 2 and b and a != b:
                    t = tuple(b)
                    assert t not in executed
                    executed[t] = cycle
                    if q == 2:
                        assert previous['inflight'][0] == b, 'integer completion without prior request'
                        coverage['one_tick_integer'] += 1
                    else:
                        assert previous['memory_tag'] == b and previous['memory_remaining'] == 1
                        assert cycle - memory_start[t] >= 2, 'memory completed in fewer than three ticks'
                        coverage['three_tick_memory'] += cycle - memory_start[t] == 2
            if row['memory_remaining'] == 2 and (row['memory_tag'] != previous['memory_tag'] or previous['memory_remaining'] == 0):
                t = tuple(row['memory_tag'])
                assert previous['memory_remaining'] == 0, 'overlapping memory executions'
                assert previous['inflight'][1] == row['memory_tag']
                memory_start[t] = cycle
            if row['flush']:
                epoch = row['flush']['tag'][0]
                inflight = [t for t in row['inflight'] if t]
                if row['memory_remaining']:
                    inflight.append(row['memory_tag'])
                coverage['flush_inflight'] += any(t[0] == epoch for t in inflight)
        except (AssertionError, KeyError) as error:
            context(path, rows, cycle, str(error))
            raise AssertionError(f'cycle {cycle}: {error}') from error
    coverage['squashed_store'] = sum(e['word'] & 127 == 0x23 and t not in retired for t, e in completed.items())
    coverage['squashed_memory_fault'] = sum(e['fault'] == 3 and t not in retired for t, e in completed.items())
    return coverage


def verify(compiled, emitted, output, case_name=None):
    output.mkdir(parents=True, exist_ok=True)
    reports, total = [], {}
    cases = [c for c in suite() if case_name is None or c['name'] == case_name]
    if not cases:
        raise ValueError('unknown case')
    for case in cases:
        directory = output / case['name']
        directory.mkdir(parents=True, exist_ok=True)
        baseline, configs = None, []
        for binary_name, binary in (('compiled', compiled), ('emitted', emitted)):
            for cache in (True, False):
                for reverse in (False, True):
                    name = f'{binary_name}-cache{int(cache)}-reverse{int(reverse)}'
                    (directory / (name + '.input.txt')).write_text(numeric_input(case, cache, reverse))
                    mismatch = directory / (name + '.mismatch.json')
                    try:
                        rows = run(binary, case, cache, reverse)
                    except AssertionError as error:
                        context(mismatch, error.rows, len(error.rows) - 1, str(error))
                        raise
                    trace_path = directory / (name + '.jsonl')
                    trace_path.write_text(''.join(json.dumps(r, separators=(',', ':')) + '\n' for r in rows))
                    retired_count = architecture(case, rows, mismatch)
                    coverage = microarchitecture(rows, mismatch)
                    if baseline is None:
                        baseline = rows
                        for k, v in coverage.items():
                            total[k] = max(total.get(k, 0), v) if k == 'max_occupancy' else total.get(k, 0) + v
                    elif baseline != rows:
                        index = next((i for i, (a, b) in enumerate(zip(baseline, rows)) if a != b), min(len(baseline), len(rows)))
                        context(mismatch, rows, index, 'cycle parity failed', baseline[max(0, index - 3):index + 4])
                        raise AssertionError(f'{case["name"]}/{name}: cycle {index} differs')
                    configs.append(dict(binary=binary_name, cache=cache, reverse=reverse, trace_sha256=sha(trace_path)))
        if case['name'] == 'latency':
            issue_cycles = {e['pc']: r['cycle'] for r in baseline for lane in ('issue_int', 'issue_mem') for e in r[lane]}
            complete_cycles = {e['pc']: r['cycle'] for r in baseline for lane in ('complete_int', 'complete_mem') for e in r[lane]}
            assert issue_cycles[12] < issue_cycles[8], 'independent integer did not bypass dependent instruction'
            assert complete_cycles[4] < complete_cycles[0], 'young integer did not finish before older load'
        report = dict(name=case['name'], cycles=baseline[-1]['cycle'], retired=retired_count,
                      ipc=retired_count / baseline[-1]['cycle'], coverage=coverage, configurations=configs)
        reports.append(report)
        print(f'{case["name"]}: {report["cycles"]} cycles, {retired_count} commits; eight traces match', flush=True)
    if case_name is None:
        missing = [k for k, count in total.items() if not count]
        assert not missing, f'uncovered behaviors: {missing}'
        assert total['max_occupancy'] == 8
        latency = next(r for r in reports if r['name'] == 'latency')['coverage']
        assert latency['out_of_order_issue'] and latency['out_of_order_complete']
        assert next(r for r in reports if r['name'] == 'dual_issue')['coverage']['dual_issue']
        assert next(r for r in reports if r['name'] == 'recovery')['coverage']['squashed_memory_fault']
    result = dict(full_acceptance=case_name is None, runs=len(reports) * 8, coverage=total, results=reports,
                  binaries={str(p): sha(p) for p in (compiled, emitted)},
                  sources={str(p.relative_to(ROOT)): sha(p) for p in sorted(HERE.glob('*.py')) if p.name != 'results.json'})
    (output / 'comparison.json').write_text(json.dumps(result, indent=2) + '\n')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiled', type=Path, required=True)
    parser.add_argument('--emitted', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=HERE / 'output/Release')
    parser.add_argument('--case')
    args = parser.parse_args()
    verify(args.compiled, args.emitted, args.output, args.case)


if __name__ == '__main__':
    main()
