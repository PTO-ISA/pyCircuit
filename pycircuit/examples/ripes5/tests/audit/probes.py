"""Recheck the four recorded audit findings without overwriting historical evidence."""
import argparse
import copy
from datetime import datetime, timezone
import json
from pathlib import Path

from pycircuit.tests.support import read_trace
from ..cases import load_cases
from ..e2e import compare_native
from ..paths import ROOT
from ..program import ProgramImage
from ..runner import run_case, write_json


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    output = (args.output or ROOT / 'pycircuit/builds/benchmarks/ripes5-standard' /
              ('audit-' + datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))).resolve()
    output.mkdir(parents=True, exist_ok=True)
    cases = {case['name']: case for case in load_cases()}
    findings = {}
    for name, issue in [('elf_extra_section', 'HARNESS-ELF-001'),
                        ('load_branch_wrong_path', 'HARNESS-STORE-001'),
                        ('audit_wrong_path_fetch', 'CPU-FETCH-001'),
                        ('audit_store_then_print', 'ENV-ECALL-001')]:
        result = run_case(cases[name], output / name, check='ripes')
        if issue == 'HARNESS-STORE-001' and result['status'] == 'passed':
            directory = output / name
            image = ProgramImage.load(directory / 'input.json')
            trace = read_trace(directory / 'cpp/trace.json')
            native = [json.loads(line) for line in (directory / 'native.jsonl').read_text().splitlines()]
            mutations = []
            for row in trace:
                event = row['state']['top.cpu.memory.store_event']
                if event[0]:
                    mutations.append(dict(cycle=row['tick'] + 1, original_size=event[3], mutated_size=1))
                    event[3] = 1
            detected = False
            try:
                compare_native(trace, copy.deepcopy(native), image)
            except AssertionError as error:
                detected = error.args[0].get('field') == 'store_size'
            findings[issue] = dict(status='fixed' if detected and mutations else 'unexpected',
                                    mutations=mutations, detected=detected)
        elif issue == 'HARNESS-ELF-001':
            findings[issue] = dict(status='fixed' if result['status'] == 'passed' and result['exit_status'] == 42 else 'unexpected')
        else:
            fixed = result['status'] == 'passed' and result.get('exit_status') == 42
            if issue == 'ENV-ECALL-001':
                fixed = fixed and result.get('output') == 'A'
            findings[issue] = dict(status='fixed' if fixed else 'unexpected',
                                    result_status=result['status'], failure=result.get('failure'))
        write_json(output / 'findings.json', findings)
        print(issue + ': ' + findings[issue]['status'], flush=True)
    # This exit code describes completion of the audit, not CPU acceptance.
    return int(any(row['status'] == 'unexpected' for row in findings.values()))


if __name__ == '__main__':
    raise SystemExit(main())
