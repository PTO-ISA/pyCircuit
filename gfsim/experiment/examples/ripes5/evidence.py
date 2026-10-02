"""Preserve the pinned baseline and compare old/new/native traces without shifts."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tarfile

from .programs import suite
from .run import HERE, DEFAULT_RUNNER, native_env, verify_runner, run_reference, run_python, compare

BASELINE_COMMIT = '56fe061'
DEFAULT_EVIDENCE = HERE / 'review-output' / 'dependency-wakeup'
MODEL_FILES = ('stages.py', 'model.py', 'logic.py')


def fingerprint(root):
    files = ('engine.py', 'construction.py', 'examples/riscv/isa.py',
             'examples/riscv/records.py', 'examples/ripes5/programs.py')
    files += tuple('examples/ripes5/' + name for name in MODEL_FILES)
    return {name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in files}


def baseline_source(directory):
    return directory / 'before' / 'source' / 'gfsim' / 'experiment'


def preserve(directory):
    repo = HERE.parents[3]
    commit = subprocess.check_output(['git', 'rev-parse', BASELINE_COMMIT], cwd=repo, text=True).strip()
    archive = subprocess.check_output(['git', 'archive', commit, 'gfsim/experiment'], cwd=repo)
    destination = directory / 'before' / 'source'
    destination.mkdir(parents=True, exist_ok=True)
    # Only tracked files from the fixed local commit, never overwrite changed evidence.
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        for member in tar.getmembers():
            if not member.isfile():
                continue
            path = destination / member.name
            content = tar.extractfile(member).read()
            if path.exists() and path.read_bytes() != content:
                raise ValueError(f'preserved source changed: {path}')
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(content)
    root = baseline_source(directory)
    manifest = dict(commit=commit, sha256=fingerprint(root))
    (directory / 'before' / 'identity.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return manifest


def worker_command(case_path, cache, reverse):
    return [sys.executable, str(HERE / 'worker.py'), str(case_path)] + (
        [] if cache else ['--no-cache']) + (['--reverse'] if reverse else [])


def source_env(root):
    env = native_env()
    env['PYTHONPATH'] = str(root.resolve())
    return env


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=DEFAULT_EVIDENCE)
    parser.add_argument('--runner', type=Path, default=DEFAULT_RUNNER)
    args = parser.parse_args()
    directory = args.output.resolve()
    baseline = preserve(directory)
    identity = verify_runner(args.runner)
    results = []
    for case in suite():
        before = directory / 'before' / 'traces' / case['name']
        after = directory / 'after' / case['name']
        before.mkdir(parents=True, exist_ok=True)
        expected = run_reference(case, args.runner, after)
        configurations = []
        for cache in (True, False):
            for reverse in (False, True):
                tag = f'cache{int(cache)}-reverse{int(reverse)}'
                old_path = before / f'{tag}.jsonl'
                subprocess.run(worker_command(after / 'input.json', cache, reverse)
                               + ['--trace', str(old_path)], env=source_env(baseline_source(directory)),
                               check=True, capture_output=True, text=True)
                old = [json.loads(line) for line in old_path.read_text().splitlines()]
                compare(case, expected, old, before / f'{tag}.mismatch.json')
                new = run_python(case, cache, reverse,
                                 after / f'{tag}.html' if case['name'] == 'array_sum' else None)
                from .run import write_jsonl
                new_path = after / f'{tag}.jsonl'
                write_jsonl(new_path, new)
                compare(case, expected, new, after / f'{tag}.native-mismatch.json')
                compare(case, old, new, after / f'{tag}.baseline-mismatch.json')
                configurations.append(dict(cache=cache, reverse=reverse, rows=len(new),
                    before_sha256=hashlib.sha256(old_path.read_bytes()).hexdigest(),
                    after_sha256=hashlib.sha256(new_path.read_bytes()).hexdigest()))
        results.append(dict(name=case['name'], cycles=expected[-1]['cycle'],
                            retired=expected[-1]['retired'], configurations=configurations))
        print(f'{case["name"]}: old/new/native matched, four configurations', flush=True)
    current = HERE.parents[1]
    lines = {name: dict(before=len((baseline_source(directory) / 'examples/ripes5' / name).read_text().splitlines()),
                        after=len((HERE / name).read_text().splitlines())) for name in MODEL_FILES}
    lines['total'] = {key: sum(count[key] for count in lines.values()) for key in ('before', 'after')}
    report = dict(baseline=baseline, current_sha256=fingerprint(current), reference=identity,
                  binary_sha256=hashlib.sha256(args.runner.read_bytes()).hexdigest(),
                  line_count_scope='Physical lines, including comments/blanks; stages.py + model.py + logic.py. Shared ISA/records and engine unchanged; tools/tests excluded.',
                  lines=lines, results=results)
    (directory / 'comparison.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
