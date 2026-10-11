"""Verify registered state operations and optimization effects independently."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class MLIRTests(unittest.TestCase):
    def setUp(self):
        base = ROOT / 'pycircuit/builds/benchmarks/rule-graph/tests'
        base.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix='ods-', dir=base)
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name)
        self.opt = os.environ.get('ACPY_MLIR_OPT') or str(Path(os.environ['ACPY_MLIR_COMPILER']).with_name('acir-opt'))

    def optimize(self, text, *flags):
        source = self.path / 'input.mlir'; source.write_text(text)
        return subprocess.run([self.opt, str(source), *flags], capture_output=True, text=True, timeout=30)

    def test_all_assembly_formats_preserve_generic_ir(self):
        source = '''module {
  "acir.resource"() {name = "q", kind = "queue", type = "u32", contract = "Queue", visibility = "old", capacity_fn = "capacity"} : () -> ()
  func.func @formats(%q: !acir.queue<i32>, %qa: !acir.queue<!acir.array<i32, 2>>, %v: i32, %i: i32, %j: i64, %flag: i1, %a: !acir.array<i32, 2>, %nested: !acir.array<!acir.array<i32, 2>, 2>, %s: !acir.struct<"Pair">) {
    %ref = "acir.get"() {name = "q", acir.id = "resource"} : () -> !acir.queue<i32>
    %cfg = "acir.config"() {name = "capacity"} : () -> i32
    %read = "acir.read"(%q) {view = "old", role = "observe", acir.id = "read"} : (!acir.queue<i32>) -> i32 loc("formats.py":12:4)
    %query = "acir.query"(%q) {kind = "empty"} : (!acir.queue<i32>) -> i1
    "acir.pop"(%q) : (!acir.queue<i32>) -> ()
    "acir.push"(%q, %v) : (!acir.queue<i32>, i32) -> ()
    "acir.write"(%q, %flag) {aspect = "full", view = "ehr", port = 0 : i64, ports = [{resource = "q", aspect = "full", port = 0 : i64}]} : (!acir.queue<i32>, i1) -> ()
    "acir.guard"(%flag) {name = "enabled", acir.id = "guard"} : (i1) -> ()
    %export = "acir.export"(%v) {acir.id = "shared"} : (i32) -> i32
    %external = "acir.external"() {source = "other_Work", identity = "shared"} : () -> i32
    "acir.revise"(%q, %v) {path = []} : (!acir.queue<i32>, i32) -> ()
    "acir.revise"(%qa, %v, %i) {path = [unit]} : (!acir.queue<!acir.array<i32, 2>>, i32, i32) -> ()
    "acir.invoke"() {name = "no_args"} : () -> ()
    "acir.invoke"(%q, %v) {name = "with_args", acir.id = "call"} : (!acir.queue<i32>, i32) -> ()
    %zero = "acir.aggregate"() : () -> !acir.struct<"Pair">
    %array = "acir.aggregate"(%v, %i) : (i32, i32) -> !acir.array<i32, 2>
    %field = "acir.extract"(%s) {field = "left"} : (!acir.struct<"Pair">) -> i32
    %index = "acir.extract"(%a, %i) {indexed = true} : (!acir.array<i32, 2>, i32) -> i32
    %indices = "acir.extract"(%nested, %i, %j) {indexed = true} : (!acir.array<!acir.array<i32, 2>, 2>, i32, i64) -> i32
    %update = "acir.update"(%s, %v) {path = ["left"]} : (!acir.struct<"Pair">, i32) -> !acir.struct<"Pair">
    %updated = "acir.update"(%a, %v, %i) {path = [unit]} : (!acir.array<i32, 2>, i32, i32) -> !acir.array<i32, 2>
    %length = "acir.length"(%a) : (!acir.array<i32, 2>) -> i32
    %cast = "acir.cast"(%v) : (i32) -> i64
    %unary = "acir.unary"(%v) {operator_name = "invert"} : (i32) -> i32
    %compare = "acir.compare"(%v, %i) {operator_name = "eq"} : (i32, i32) -> i1
    %range = "acir.range"(%i) : (i32) -> !acir.vector<i32>
    "acir.check"(%flag) : (i1) -> ()
    "acir.unreachable"() : () -> ()
    return
  }
}'''
        compact = self.optimize(source, '--mlir-print-debuginfo')
        self.assertEqual(compact.returncode, 0, compact.stderr)
        for operation in ('resource', 'get', 'config', 'read', 'query', 'pop', 'push',
                          'write', 'guard', 'export', 'external', 'revise', 'invoke',
                          'aggregate', 'extract', 'update', 'length', 'cast', 'unary',
                          'compare', 'range', 'check', 'unreachable'):
            with self.subTest(operation=operation):
                self.assertIn('acir.' + operation, compact.stdout)
                self.assertNotIn('"acir.' + operation + '"', compact.stdout)
        self.assertIn('acir.invoke "no_args"()', compact.stdout)
        self.assertIn('acir.invoke "with_args"(', compact.stdout)
        self.assertIn('acir.pop %arg0 : !acir.queue<i32>', compact.stdout)
        query = next(line for line in compact.stdout.splitlines() if 'acir.query ' in line)
        self.assertIn(': !acir.queue<i32> -> i1', query)
        pushed = next(line for line in compact.stdout.splitlines() if 'acir.push ' in line)
        self.assertIn(': !acir.queue<i32>, i32', pushed)
        vector = next(line for line in compact.stdout.splitlines() if 'acir.range ' in line)
        self.assertIn('-> !acir.vector<i32>', vector)
        self.assertIn('field = "left"', compact.stdout)
        field = next(line for line in compact.stdout.splitlines() if 'field = "left"' in line)
        self.assertNotIn('[]', field)
        self.assertIn('acir.id = "shared"', compact.stdout)
        self.assertIn('ports = [{', compact.stdout)
        self.assertIn('"formats.py":12:4', compact.stdout)
        flags = ('--mlir-print-op-generic', '--mlir-print-debuginfo')
        original = self.optimize(source, *flags)
        roundtrip = self.optimize(compact.stdout, *flags)
        self.assertEqual(original.returncode, 0, original.stderr)
        self.assertEqual(roundtrip.returncode, 0, roundtrip.stderr)
        self.assertEqual(original.stdout, roundtrip.stdout)

    def test_reads_and_queries_keep_conservative_effects(self):
        result = self.optimize('''module {
  func.func @observe(%q: !acir.queue<i32>, %cell: !acir.cell<i32>, %condition: i1) {
    cf.cond_br %condition, ^yes, ^done
  ^yes:
    %a = "acir.read"(%q) : (!acir.queue<i32>) -> i32
    %b = "acir.read"(%q) : (!acir.queue<i32>) -> i32
    %c = "acir.query"(%q) {kind = "empty"} : (!acir.queue<i32>) -> i1
    %d = "acir.query"(%q) {kind = "empty"} : (!acir.queue<i32>) -> i1
    %e = "acir.read"(%cell) : (!acir.cell<i32>) -> i32
    %f = "acir.read"(%cell) : (!acir.cell<i32>) -> i32
    cf.br ^done
  ^done:
    return
  }
}''', '--canonicalize', '--cse')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count('acir.read '), 4)
        self.assertEqual(result.stdout.count('acir.query '), 2)
        self.assertLess(result.stdout.index('cf.cond_br'), result.stdout.index('acir.read '))

    def test_ehr_reads_ports_and_capacity_writes_survive_optimization(self):
        result = self.optimize('''module {
          func.func @views(%q: !acir.queue<i32>, %c: !acir.cell<i32>, %v: i1) {
            %a = "acir.read"(%c) {view = "ehr", port = 0 : i64} : (!acir.cell<i32>) -> i32
            %b = "acir.read"(%c) {view = "ehr", port = 1 : i64} : (!acir.cell<i32>) -> i32
            "acir.write"(%q, %v) {view = "ehr", aspect = "full", port = 0 : i64} : (!acir.queue<i32>, i1) -> ()
            %f = "acir.read"(%q) {view = "ehr", aspect = "full", port = 1 : i64} : (!acir.queue<i32>) -> i1
            return
          }
        }''', '--canonicalize', '--cse')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count('acir.read '), 3)
        self.assertEqual(result.stdout.count('acir.write '), 1)
        self.assertIn('port = 0', result.stdout)
        self.assertIn('port = 1', result.stdout)

    def test_ods_rejects_invalid_payload_query_and_result(self):
        for operation, diagnostic in [
            ('"acir.push"(%q, %v) : (!acir.queue<i32>, i64) -> ()', 'pushed value must match'),
            ('%x = "acir.query"(%q) {kind = "full"} : (!acir.queue<i32>) -> i1', 'EHR capacity read'),
            ('%x = "acir.read"(%q) {view = "ehr"} : (!acir.queue<i32>) -> i32', 'old state view'),
            ('%x = "acir.query"(%q) {kind = "size"} : (!acir.queue<i32>) -> i1', 'incorrect result width'),
            ('"acir.revise"(%q, %v) {path = [unit]} : (!acir.queue<i32>, i64) -> ()', 'path/index count mismatch'),
            ('%x = "acir.read"(%c) {view = "current"} : (!acir.cell<i32>) -> i32', 'current view requires a Signal'),
            ('"acir.write"(%c, %v) : (!acir.cell<i32>, i64) -> ()', 'written value must match'),
            ('%x = "acir.invoke"() {name = "r"} : () -> i32', 'requires zero results'),
            ('%x = "acir.result"(%t) {index = 1 : i64, producer = "r"} : (tuple<i32>) -> i32', 'unregistered operation'),
            ('%x = "acir.read"(%q) {aspect = "full", view = "ehr"} : (!acir.queue<i32>) -> i32', 'capacity view'),
            ('"acir.write"(%q, %v) : (!acir.queue<i32>, i64) -> ()', 'capacity write'),
            ('%x = "acir.read"(%q) {view = "old_or_bypass"} : (!acir.queue<i32>) -> i32', 'old state view'),
        ]:
            result = self.optimize('module { func.func @invalid(%q: !acir.queue<i32>, %c: !acir.cell<i32>, %v: i64, %t: tuple<i32>) {\n' + operation + '\n return\n} }')
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(diagnostic, result.stderr)

    def test_signal_current_reads_and_sets_survive_optimization(self):
        result = self.optimize('''module {
          func.func @signal(%s: !acir.signal<i32>, %v: i32, %condition: i1) {
            cf.cond_br %condition, ^yes, ^done
          ^yes:
            "acir.write"(%s, %v) {view = "current", interface = "set"} : (!acir.signal<i32>, i32) -> ()
            %a = "acir.read"(%s) {view = "current", interface = "get"} : (!acir.signal<i32>) -> i32
            %b = "acir.read"(%s) {view = "current", interface = "get"} : (!acir.signal<i32>) -> i32
            cf.br ^done
          ^done:
            return
          }
        }''', '--canonicalize', '--cse')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.count('acir.read '), 2)
        self.assertEqual(result.stdout.count('acir.write '), 1)
        self.assertLess(result.stdout.index('cf.cond_br'), result.stdout.index('acir.write '))
        self.assertNotIn('port =', result.stdout)

    def test_signal_verifier_rejects_ehr_views_ports_and_bad_payloads(self):
        for operation, diagnostic in [
            ('%x = "acir.read"(%s) {view = "ehr", interface = "get"} : (!acir.signal<i32>) -> i32', 'Signal requires current'),
            ('%x = "acir.read"(%s) {view = "current", interface = "get", port = 0 : i64} : (!acir.signal<i32>) -> i32', 'no EHR ports'),
            ('%x = "acir.read"(%s) {view = "current", interface = "set"} : (!acir.signal<i32>) -> i32', 'get interface'),
            ('%x = "acir.read"(%s) {view = "current", interface = "get"} : (!acir.signal<i32>) -> i64', 'result must match'),
            ('"acir.write"(%s, %v) {view = "current", interface = "set"} : (!acir.signal<i32>, i64) -> ()', 'written value must match'),
            ('"acir.write"(%s, %v) {view = "ehr", interface = "set"} : (!acir.signal<i32>, i64) -> ()', 'Signal requires current'),
            ('"acir.resource"() {name = "s", kind = "signal", type = "u32", visibility = "current"} : () -> ()', 'initialization'),
            ('"acir.resource"() {name = "s", kind = "signal", type = "u32", visibility = "current", initial_fn = "init", capacity_fn = "cap"} : () -> ()', 'no capacity'),
        ]:
            with self.subTest(operation=operation):
                result = self.optimize('module { func.func @invalid(%s: !acir.signal<i32>, %v: i64) {\n' + operation + '\n return\n} }')
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(diagnostic, result.stderr)
