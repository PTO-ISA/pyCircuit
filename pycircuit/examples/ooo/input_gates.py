"""Reject malformed numeric host inputs before constructing a CPU."""
import subprocess
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from pycircuit.examples.ooo.programs import suite
from pycircuit.examples.ooo.verify import numeric_input


def main():
    values = numeric_input(next(c for c in suite() if c['name'] == 'latency')).split()
    invalid = ['', ' '.join(values[:-1]), ' '.join(values + ['0'])]
    for index, value in [(0, '1'), (1, '0'), (1, '-1'), (2, '3'), (3, '2'),
                         (4, '10001'), (5, '1'), (6, '0'), (7, '1048577'), (8, '4294967296'),
                         (8 + int(values[6]), '1')]:
        tokens = list(values)
        tokens[index] = value
        invalid.append(' '.join(tokens))
    for text in invalid:
        result = subprocess.run([sys.argv[1]], input=text, text=True, capture_output=True, timeout=10)
        assert result.returncode == 1 and result.stderr and not result.stdout, result
    print(f'{len(invalid)} malformed inputs rejected')


if __name__ == '__main__':
    main()
