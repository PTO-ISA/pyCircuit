"""ODS verification, observable reads, and the standalone registered pass pipeline."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from pycircuit import compile_source
from pycircuit.ir import save


class MLIRTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='acir-ods-')
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name)
        self.opt = os.environ.get('ACPY_MLIR_OPT') or str(Path(os.environ['ACPY_MLIR_COMPILER']).with_name('acir-opt'))

    def optimize(self, text, *flags):
        source = self.path / 'input.mlir'
        source.write_text(text)
        return subprocess.run([self.opt, str(source), *flags], capture_output=True, text=True, timeout=30)

    def test_reads_and_queries_are_observable(self):
        result = self.optimize('''module {
  func.func @observe(%q: !acir.queue<i32>, %condition: i1) {
    cf.cond_br %condition, ^yes, ^done
  ^yes:
    %a = "acir.read"(%q) : (!acir.queue<i32>) -> i32
    %b = "acir.read"(%q) : (!acir.queue<i32>) -> i32
    %c = "acir.query"(%q) {kind = "empty"} : (!acir.queue<i32>) -> i1
    %d = "acir.query"(%q) {kind = "empty"} : (!acir.queue<i32>) -> i1
    cf.br ^done
  ^done:
    return
  }
}''', '--canonicalize', '--cse')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count('"acir.read"'), 2)
        self.assertEqual(result.stdout.count('"acir.query"'), 2)
        self.assertLess(result.stdout.index('cf.cond_br'), result.stdout.index('"acir.read"'))

    def test_ods_rejects_invalid_payload_and_query(self):
        for operation, diagnostic in [
            ('"acir.push"(%q, %v) : (!acir.queue<i32>, i64) -> ()', 'pushed value must match'),
            ('%x = "acir.query"(%q) {kind = "size"} : (!acir.queue<i32>) -> i1', 'incorrect result width'),
            ('"acir.revise"(%q, %v) {path = [unit]} : (!acir.queue<i32>, i64) -> ()', 'path/index count mismatch'),
        ]:
            result = self.optimize('module { func.func @invalid(%q: !acir.queue<i32>, %v: i64) {\n' + operation + '\n return\n} }')
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(diagnostic, result.stderr)

    def test_standalone_pipeline(self):
        source = self.path / 'source.py'
        source.write_text('from pycircuit import ac\n@ac.module\ndef Saved():\n    q=ac.queue[ac.u32](initial=7)\n    @ac.rule\n    def change():\n        if q.value==7:\n            q.value=9\n    change()\n')
        mlir = self.path / 'model.acir.mlir'
        save(compile_source(source, 'Saved'), mlir)
        source.unlink()
        result = subprocess.run([self.opt, str(mlir), '--acir-analyze-resources', '--canonicalize', '--cse', '--acir-lower-gfsim', '--acir-convert-to-emitc'], capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('emitc.call_opaque', result.stdout)
        self.assertIn('abortRule', result.stdout)
        self.assertIn('completeRule', result.stdout)
        self.assertNotIn('"acir.', result.stdout)
