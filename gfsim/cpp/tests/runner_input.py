"""Transport rejection and mandatory native identity gates."""
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

RUNNER = Path(sys.argv.pop(1)).resolve()
HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('ripes_verify', HERE.parent / 'examples/ripes5/verify.py')
v = importlib.util.module_from_spec(spec)
spec.loader.exec_module(v)


class InputTests(unittest.TestCase):
    def rejected(self, payload):
        result = subprocess.run([str(RUNNER)], input=payload, capture_output=True, text=True, timeout=10)
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue(result.stderr)
        self.assertFalse(result.stdout)

    def test_bad_transport(self):
        valid = v.numeric_input(v.suite()[0])
        tokens = valid.split()
        self.rejected('')
        self.rejected(' '.join(tokens[:-1]))
        self.rejected(valid + ' 1')
        for index, bad in ((0, '2'), (1, '0'), (2, '-1'), (4, '2'), (6, str(2**64)),
                           (7, str(2**21)), (8, str(2**32)), (8, 'hello')):
            changed = tokens.copy()
            changed[index] = bad
            self.rejected(' '.join(changed))

    def test_native_required(self):
        with tempfile.TemporaryDirectory() as directory:
            missing = Path(directory) / 'native'
            with self.assertRaises(FileNotFoundError):
                v.verify_runner(missing)
            missing.write_text('#!' + sys.executable + '\nprint(' + repr(json.dumps(dict(ripes='wrong', vsrtl='wrong'))) + ')\n')
            missing.chmod(0o755)
            with self.assertRaisesRegex(RuntimeError, 'version mismatch'):
                v.verify_runner(missing)


if __name__ == '__main__':
    unittest.main()
