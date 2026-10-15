"""Recapture enhanced native observations against already verified model traces."""
import hashlib
import json
from pathlib import Path
from .paths import FIXTURES
from .program import ProgramImage
from .e2e import DEFAULT_OUTPUT, NATIVE, compare_native, native_run, check_oracle
from pycircuit.tests.support import read_trace


def main():
    records=[]
    known=json.loads((FIXTURES / 'reference-differences.json').read_text())
    paths=list(DEFAULT_OUTPUT.glob('*/verification.json'))+list((DEFAULT_OUTPUT/'legacy-live').glob('*/verification.json'))
    for path in sorted(paths):
        summary=json.loads(path.read_text())
        if not summary.get('oracle_equal'):continue
        directory=path.parent
        image=ProgramImage.load(directory/'input.json')
        trace=read_trace(directory/'transaction-trace.json')
        native=native_run(image,directory)
        name=summary['name']
        differences=compare_native(trace,native,image,collect=bool(known.get(name)))
        if name in known:
            assert differences==known[name]['expected_differences'],(name,differences)
            (directory/'reference-defect.json').write_text(json.dumps(known[name]|{'observed_differences':differences},indent=2)+'\n')
        check_oracle(trace,image,json.loads((directory/'oracle.json').read_text()))
        summary.update(native_equal=not differences,reference_classification='confirmed-reference-defect' if differences else 'equal',
                       enhanced_native_observations=True,reference_binary_sha256=hashlib.sha256(NATIVE.read_bytes()).hexdigest())
        path.write_text(json.dumps(summary,indent=2)+'\n')
        records.append(summary)
        print(name+': refreshed native, load/enable/clear/sequential-PC checks pass',flush=True)
    (DEFAULT_OUTPUT/'summary.json').write_text(json.dumps(records,indent=2)+'\n')

if __name__=='__main__':main()
