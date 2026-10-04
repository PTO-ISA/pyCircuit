"""Compare every clock boundary with pinned skyzh, then check the ISA separately."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import subprocess
from .assemble import assemble
from .oracle import interpret, load_image

HERE = Path(__file__).resolve().parent
KNOWN = {'auipc': 'R02: AUIPC never dispatches',
         'integer': 'R04: SRAI tests the wrong immediate bit',
         'memory': 'R03/R05: overlapping memory dependencies and unsigned native char'}


def run(binary, image, output, reverse=False, fixed=None):
    command = [str(binary.resolve()), str(image.resolve()), '1000000']
    if reverse:
        command.append('--reverse')
    if fixed:
        command.extend(['--fixed', str(fixed)])
    with output.open('w') as stream:
        result = subprocess.run(command, stdout=stream, stderr=subprocess.PIPE, text=True, timeout=180)
    if result.returncode:
        raise AssertionError(f'{command}: {result.stderr} (trace: {output})')


def compare(actual, reference):
    fields = None
    digest = hashlib.sha256()
    with actual.open() as a, reference.open() as b:
        for index, pair in enumerate(itertools.zip_longest(a, b)):
            left, right = pair
            if left is None or right is None:
                raise AssertionError(f'trace length mismatch at record {index}: {actual}')
            x, y = json.loads(left), json.loads(right)
            if x.get('final') and y.get('final'):
                for key in y.keys() - {'run_ns', 'construct_ns'}:
                    if x.get(key) != y[key]:
                        raise AssertionError(f'{actual}: final {key}: {x.get(key)} != {y[key]}')
            elif x != y:
                differences = [(name, xv, yv) for name, xv, yv in zip(fields or [], x.get('state', []), y.get('state', [])) if xv != yv]
                failure = dict(tick=index, actual=str(actual), reference=str(reference),
                               fields=differences, actual_commit=x.get('commit'), reference_commit=y.get('commit'))
                actual.with_suffix('.difference.json').write_text(json.dumps(failure, indent=2)+'\n')
                raise AssertionError(f'clock mismatch: {failure}')
            if not index:
                fields = y['fields']
            if not x.get('final'):
                digest.update(left.encode())
    return digest.hexdigest()


def architectural_commits(raw):
    """Join skyzh's JALR link/jump micro-ops into one architectural retirement."""
    result, pending = [], None
    for item in raw:
        rid, pc, word, op, dest, value, prediction, imm = item
        write = [dest, value] if op not in (0x23, 0x63, 0x67) and dest else None
        store = None
        target = (pc + 4) & 0xffffffff
        if op == 0x23:
            width = 1 << ((word >> 12) & 7)
            store = [dest, width, value & ((1 << (width * 8)) - 1)]
        elif op == 0x63:
            f = (word >> 12) & 7
            taken = value == 0 if f in (0, 5, 7) else value != 0 if f == 1 else value == 1
            target = (dest + (imm if taken else 4)) & 0xffffffff
        elif op == 0x6f:
            if word & 127 == 0x67:
                assert pending is None
                pending = dict(pc=pc, word=word, write=write, store=None)
                continue
            target = (pc + imm) & 0xffffffff
        elif op == 0x67:
            assert pending is not None and pending['pc'] == pc and pending['word'] == word
            result.append(dict(pending, next_pc=value))
            pending = None
            continue
        result.append(dict(pc=pc, word=word, write=write, store=store, next_pc=target))
    assert pending is None
    return result


def inspect(path):
    raw, coverage = [], dict(max_rob=0, max_completed=0, allocations=0, dual_dispatch=0,
                            flushes=0, rename_commit_collision=0, allocation_bypass=0,
                            flush_inflight=0, load_phases=[])
    previous = None
    phases = set()
    with path.open() as stream:
        for line in stream:
            row = json.loads(line)
            if row.get('final'):
                final = row
                continue
            if row['tick'] == 0:
                names = row['fields']
            state = dict(zip(names, row['state']))
            coverage['max_rob'] = max(coverage['max_rob'], sum(state[f'rob{i}.0'] for i in range(1, 9)))
            phases.update(state[f'lsu{i}.0'] for i in range(7, 10))
            if row['commit']:
                raw.append(row['commit'])
            if previous is not None:
                completed = {i for i in range(1, 9) if previous[f'rob{i}.0'] and not previous[f'rob{i}.4'] and state[f'rob{i}.4']}
                coverage['max_completed'] = max(coverage['max_completed'], len(completed))
                flush = row['commit'] and (row['commit'][3] == 0x67 or row['commit'][3] == 0x63 and state['head'] == state['tail'] == 1)
                if flush:
                    coverage['flushes'] += 1
                    coverage['flush_inflight'] += any(previous[f'rs{i}.0'] for i in range(10))
                else:
                    allocated = (state['tail'] - previous['tail']) % 8
                    coverage['allocations'] += allocated
                    coverage['dual_dispatch'] += allocated == 2
                    if allocated:
                        word = state[f'rob{previous["tail"]}.1']
                        rs1, rs2 = word >> 15 & 31, word >> 20 & 31
                        producers = {previous[f'rat{r}.1'] for r in (rs1, rs2) if previous[f'rat{r}.0']}
                        if producers & completed:
                            coverage['allocation_bypass'] += 1
                        if row['commit']:
                            dest = row['commit'][4]
                            if dest < 32 and state[f'rat{dest}.0'] and state[f'rat{dest}.1'] == previous['tail']:
                                coverage['rename_commit_collision'] += 1
            previous = state
    coverage['load_phases'] = sorted(phases)
    return final, raw, coverage


def verify(binaries, reference, output, selected=None):
    output.mkdir(parents=True, exist_ok=True)
    reports = []
    for source in sorted((HERE / 'programs').glob('*.s')):
        case = source.stem
        if selected and selected != case:
            continue
        image = assemble(source, output / (case + '.hex'))
        native = output / f'{case}-reference.jsonl'
        fixed = 40 if case == 'auipc' else None
        run(reference, image, native, fixed=fixed)
        expected = interpret(load_image(image), trace=True)
        final, raw, coverage = inspect(native)
        commits = architectural_commits(raw)
        isa = final['stopped'] and final['registers'] == expected['registers'] and final['memory_changes'] == expected['memory_changes']
        isa = isa and len(commits) == len(expected['commits']) and all(all(a[k] == b[k] for k in a) for a, b in zip(commits, expected['commits']))
        if case in KNOWN:
            assert not isa, f'{case}: expected native defect was not exercised'
        else:
            assert isa, f'{case}: native ISA mismatch'
        traces = {}
        for name, binary in binaries.items():
            for reverse in (False, True):
                path = output / f'{case}-{name}-{int(reverse)}.jsonl'
                run(binary, image, path, reverse=reverse, fixed=fixed)
                traces[f'{name}-{int(reverse)}'] = compare(path, native)
        reports.append(dict(program=case, cycles=final['cycles'], instructions=len(commits),
                            isa_passed=bool(isa), known_defect=KNOWN.get(case), coverage=coverage, trace_sha256=traces))
        print(f'{case}: {final["cycles"]} clocks identical, {len(traces)} variants, ISA={isa}', flush=True)
    if not selected:
        assert max(r['coverage']['max_rob'] for r in reports) == 7
        assert max(r['coverage']['max_completed'] for r in reports) >= 2
        for key in ('dual_dispatch', 'flush_inflight', 'rename_commit_collision', 'allocation_bypass'):
            assert any(r['coverage'][key] for r in reports), f'missing coverage: {key}'
        assert any(r['coverage']['allocations'] > 100 for r in reports)
        assert any(r['coverage']['load_phases'] == [0, 1, 2] for r in reports)
    result = dict(passed=True, comparison='every clock boundary and retirement; independent native core', reports=reports)
    (output / 'results.json').write_text(json.dumps(result, indent=2)+'\n')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiled', type=Path, required=True)
    parser.add_argument('--emitted', type=Path)
    parser.add_argument('--no-opt', type=Path)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--case')
    args = parser.parse_args()
    binaries = {'compiled': args.compiled}
    if args.emitted: binaries['emitted'] = args.emitted
    if args.no_opt: binaries['no-opt'] = args.no_opt
    verify(binaries, args.reference, args.output, args.case)
