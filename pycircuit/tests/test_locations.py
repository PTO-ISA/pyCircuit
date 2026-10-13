"""Source provenance is optional diagnostic data, independent of SSA identity."""
import json
import os
import re
import subprocess
import unittest

from pycircuit import analyze
from pycircuit.ir import load
from pycircuit.mlir_text import quote
import test_rule_graph as support


def without_positions(value):
    if isinstance(value, dict):
        return {k: without_positions(v) for k, v in value.items()
                if k not in ('loc', 'location', 'locations')}
    if isinstance(value, list):
        return [without_positions(v) for v in value]
    return value


SOURCE = '''
    from pycircuit import ac
    @ac.module
    def Top(flag: bool):
        q = ac.queue[ac.u32]()
        def peek(source: ac.ref[ac.u32]) -> bool:
            return source.value > 2
        @ac.rule
        def put() -> ac.queue[ac.u32]:
            return 1
        @ac.rule
        def use(source: ac.queue[ac.u32]):
            first = source.value
            second = source.value
            assert first < second or flag
        q = put()
        if peek(q):
            use(q)
'''


class LocationTests(unittest.TestCase):
    setUp = support.RuleGraphTests.setUp
    model = support.RuleGraphTests.model

    def test_source_move_and_readable_projection_preserve_analysis(self):
        for optimize in (False, True):
            with self.subTest(optimize=optimize):
                graph = self.model(SOURCE, optimize=optimize)
                output = self.path / ('opt' if optimize else 'out')
                saved = load(output / 'model.acir.mlir')
                readable = load(output / 'model.readable.mlir')
                self.assertNotIn('.analysis-', saved)
                self.assertNotIn('acir.type:', saved)
                self.assertNotIn('loc(unknown)', saved)
                self.assertNotIn(' loc(', readable)
                self.assertNotIn('#loc', readable)
                self.assertIn(str(self.path / 'source.py'), saved)
                self.assertEqual(graph, analyze(saved, self.path / ('reload-' + str(optimize)), optimize=False))
                moved = saved.replace(str(self.path / 'source.py'), 'relocated.py')
                self.assertNotEqual(saved, moved)
                changed = analyze(moved, self.path / ('moved-' + str(optimize)), optimize=False)
                projected = analyze(readable, self.path / ('readable-' + str(optimize)), optimize=False)
                self.assertEqual(without_positions(graph), without_positions(changed))
                self.assertEqual(without_positions(graph), without_positions(projected))

    def test_legacy_generated_locations_and_user_metadata(self):
        self.model(SOURCE)
        saved = load(self.path / 'out/model.acir.mlir')
        label = 'keep loc(unknown), "loc(unknown)" and \\loc(unknown)'
        legacy = saved.replace('{acir.id =', '{label = ' + quote(label) + ', extra_loc = loc(unknown), acir.id =', 1)
        legacy = legacy.replace('%arg0: i1', '%arg0: i1 loc("acir.type:bool")')
        legacy = legacy.replace('    return\n', '    return loc("/old/.analysis-obsolete/input.mlir":7:3)\n')
        self.assertNotEqual(legacy, saved)
        analyze(legacy, self.path / 'legacy', optimize=False)
        cleaned = load(self.path / 'legacy/model.acir.mlir')
        self.assertNotIn('/old/.analysis-obsolete/', cleaned)
        self.assertNotIn('acir.type:', cleaned)
        # UnknownLoc remains a valid ordinary attribute; its alias must survive.
        source = self.path / 'check.mlir'
        source.write_text(cleaned)
        result = subprocess.run([os.environ['ACPY_MLIR_COMPILER'], str(source), str(self.path / 'native')],
                                capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)
        program = json.loads((self.path / 'native/program.json').read_text())
        attributes = [op['attrs'] for fn in program['functions'] for block in fn['blocks']
                      for op in block['ops'] if 'label' in op['attrs']]
        self.assertEqual(len(attributes), 1)
        self.assertEqual(attributes[0]['label'], label)
        self.assertEqual(attributes[0]['extra_loc'], 'loc(unknown)')

    def test_anonymous_and_duplicate_ids_keep_distinct_read_sites(self):
        self.model(SOURCE)
        readable = load(self.path / 'out/model.readable.mlir')
        for duplicate in (False, True):
            with self.subTest(duplicate=duplicate):
                def change(line):
                    if 'acir.queue.peek ' not in line:
                        return line
                    return re.sub(r'acir.id = "[^"]+", ',
                                  'acir.id = "same", ' if duplicate else '', line)
                changed = '\n'.join(change(line) for line in readable.splitlines())
                self.assertNotEqual(readable, changed)
                graph = analyze(changed, self.path / ('sites-' + str(duplicate)), optimize=False)
                node = next(n for n in graph['nodes'] if n['id'] == 'top.use')
                reads = [a for a in node['accesses'] if a['operation'] == 'read' and a['function'] == 'top.use']
                self.assertEqual(len(reads), 2)
                self.assertEqual(len({a['id'] for a in reads}), 2)
                self.assertEqual(len([a for a in node['accesses'] if a['operation'] == 'pop']), 1)
