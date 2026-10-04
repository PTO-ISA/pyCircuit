"""Run complete programs; compare every commit to the independent RV32I oracle."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import subprocess
from .assemble import assemble
from .oracle import interpret, load_image

HERE = Path(__file__).resolve().parent


def check(rows, expected):
    commits = [r['retire'] for r in rows[:-1] if r['retire']]
    assert len(commits) == len(expected['commits']), (len(commits), len(expected['commits']))
    for index, (actual, reference) in enumerate(zip(commits, expected['commits'])):
        for field in ('pc', 'word', 'write', 'store', 'next_pc'):
            assert actual[field] == reference[field], (index, field, actual, reference)
        assert actual['fault'] == 0, (index, actual)
    final = rows[-1]
    assert final['final'] and final['stopped']
    assert final['registers'] == expected['registers']
    assert final['memory_changes'] == expected['memory_changes']
    assert final['instructions'] == expected['instructions']


def coverage(rows):
    seen_issue, seen_complete = {}, {}
    issue_ooo = completion_ooo = recovery_inflight = False
    issued = 0
    for r in rows[:-1]:
        for lane, seq, epoch in r['issued']:
            issue_ooo |= seq < seen_issue.get(epoch, 0)
            seen_issue[epoch] = max(seq, seen_issue.get(epoch, 0))
            issued += 1
        for seq, epoch in r['completed']:
            completion_ooo |= seq < seen_complete.get(epoch, 0)
            seen_complete[epoch] = max(seq, seen_complete.get(epoch, 0))
        recovery_inflight |= bool(r['retire'] and r['retire']['redirect'] and r['inflight'])
    return dict(max_rob=max(r['rob'] for r in rows[:-1]), issued=issued,
                issue_ooo=issue_ooo, completion_ooo=completion_ooo,
                recovery_inflight=recovery_inflight)


def verify(binaries, output, selected=None, jobs=4):
    output.mkdir(parents=True, exist_ok=True)
    reports = []
    for source in sorted((HERE / 'programs').glob('*.s')):
        if selected and source.stem != selected:
            continue
        image = assemble(source, output / (source.stem + '.hex'))
        expected = interpret(load_image(image), trace=True)
        for period, closed in ((0, 0), (13, 10)):
            baseline = None
            variants = [(name, binary, cache, reverse) for name, binary in binaries.items()
                        for cache in (True, False) for reverse in (False, True)]
            def execute(variant):
                name, binary, cache, reverse = variant
                cmd = [str(binary.resolve()), str(image), '100000', '--wb-period', str(period), '--wb-closed', str(closed)]
                if not cache: cmd.append('--cache-off')
                if reverse: cmd.append('--reverse')
                proc = subprocess.run(cmd, text=True, capture_output=True, timeout=120)
                return cmd, proc
            with ThreadPoolExecutor(max_workers=jobs) as pool:
                for variant, (cmd, proc) in zip(variants, pool.map(execute, variants)):
                    name, _, cache, reverse = variant
                    if proc.returncode:
                        (output / 'failure.stdout').write_text(proc.stdout)
                        raise AssertionError(f'{cmd}: {proc.stderr}')
                    rows = [json.loads(line) for line in proc.stdout.splitlines()]
                    check(rows, expected)
                    if baseline is None: baseline = rows
                    if rows != baseline:
                        mismatch = next((i for i, (a,b) in enumerate(zip(rows, baseline)) if a != b), min(len(rows),len(baseline)))
                        (output / 'first-difference.json').write_text(json.dumps(dict(index=mismatch, actual=rows[max(0,mismatch-1):mismatch+2], baseline=baseline[max(0,mismatch-1):mismatch+2]), indent=2))
                        raise AssertionError(f'tick mismatch: {source.stem} {name} {cache=} {reverse=} {mismatch=}')
            cov = coverage(baseline)
            reports.append(dict(program=source.stem, wb_period=period, wb_closed=closed,
                                cycles=baseline[-1]['cycles'], instructions=expected['instructions'],
                                variants=len(binaries)*4, trace_sha256=hashlib.sha256(json.dumps(baseline,sort_keys=True).encode()).hexdigest(), **cov))
            (output / f'{source.stem}-{period}.jsonl').write_text(''.join(json.dumps(r)+'\n' for r in baseline))
            print(f'{source.stem} wb={period}/{closed}: {expected["instructions"]} commits, {baseline[-1]["cycles"]} cycles, {len(binaries)*4} variants', flush=True)
    if not selected:
        assert max(r['max_rob'] for r in reports) == 12, 'full ROB not exercised'
        for field in ('issue_ooo', 'completion_ooo', 'recovery_inflight'):
            assert any(r[field] for r in reports), f'{field} not exercised'
        assert max(r['issued'] for r in reports) > 120, 'station/ROB reuse not exercised'
    result = dict(passed=True, oracle='independent RV32I interpreter', reports=reports,
                  binaries={k: dict(path=str(v),sha256=hashlib.sha256(v.read_bytes()).hexdigest()) for k,v in binaries.items()})
    (output / 'results.json').write_text(json.dumps(result, indent=2)+'\n')
    return result


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compiled', type=Path, required=True)
    p.add_argument('--emitted', type=Path)
    p.add_argument('--no-opt', type=Path)
    p.add_argument('--output', type=Path, default=HERE / 'output/mlir')
    p.add_argument('--case')
    p.add_argument('--jobs', type=int, default=4, help='independent simulator processes; each simulation stays single-threaded')
    a = p.parse_args()
    binaries = {'compiled': a.compiled}
    if a.emitted: binaries['emitted'] = a.emitted
    if a.no_opt: binaries['no-opt'] = a.no_opt
    if a.jobs < 1: p.error('--jobs must be positive')
    verify(binaries, a.output, a.case, a.jobs)
