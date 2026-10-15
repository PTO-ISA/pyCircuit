"""Count frontend source lines and the RV5S RISC-V header dependency closure."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

from ..paths import ROOT, MODEL_DIR


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'pycircuit/builds/benchmarks/field-analysis-fix/source-size')
    args = parser.parse_args()
    reference = ROOT/'reference/ripes-reference'
    scope = (reference/'src/processors/RISC-V').resolve()
    pending = [scope/'rv5s/rv5s.h']
    native = set()
    while pending:
        path = pending.pop()
        if path in native:
            continue
        native.add(path)
        for include in re.findall(r'^\s*#include\s+"([^"]+)"', path.read_text(), re.M):
            for base in (path.parent, reference/'src'):
                candidate = (base/include).resolve()
                if candidate.is_file() and candidate.is_relative_to(scope):
                    pending.append(candidate)
                    break

    def count(paths):
        files = []
        for path in sorted(paths):
            data = path.read_bytes()
            lines = data.decode().splitlines()
            files.append(dict(path=str(path.relative_to(ROOT)), sha256=hashlib.sha256(data).hexdigest(),
                physical_lines=len(lines), nonempty_lines=sum(bool(line.strip()) for line in lines), bytes=len(data)))
        return dict(files=files, totals={key:sum(f[key] for f in files)
                    for key in ('physical_lines', 'nonempty_lines', 'bytes')})

    ours = count([MODEL_DIR/'model.py', MODEL_DIR/'isa.py'])
    theirs = count(native)
    result = dict(method='Physical and nonempty lines; both include comments. Files counted once; no preprocessing or logical SLOC estimation.',
        scope='ACPy model.py + isa.py versus quoted-include closure of rv5s/rv5s.h restricted to src/processors/RISC-V; excludes both frameworks, test runners, demos and GUI layout files. Native core headers retain RV64 branches and observation interfaces.',
        reference_commit=subprocess.check_output(['git','-C',str(reference),'rev-parse','HEAD'],text=True).strip(),
        script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        frontend=ours, native_cpp=theirs,
        reduction={key:dict(lines_saved=theirs['totals'][key]-ours['totals'][key],
                       percent=100*(1-ours['totals'][key]/theirs['totals'][key]))
                   for key in ('physical_lines','nonempty_lines')})
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output/'summary.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(frontend=ours['totals'], native_cpp=theirs['totals'],
                         native_files=len(native), reduction=result['reduction']),indent=2))


if __name__ == '__main__':
    main()
