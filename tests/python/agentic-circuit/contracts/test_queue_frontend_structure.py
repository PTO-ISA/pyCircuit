from __future__ import annotations

import ast
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
PACKAGE = ROOT / "python/agentic-circuit/src/agentic_circuit"
FACADE = PACKAGE / "_queue_frontend.py"
COMPILER = PACKAGE / "_queue_compiler"


class QueueFrontendStructureTest(unittest.TestCase):
    def test_facade_only_owns_source_orchestration(self) -> None:
        source = FACADE.read_text(encoding="utf-8")
        tree = ast.parse(source, filename=str(FACADE))
        functions = [
            node.name for node in tree.body if isinstance(node, ast.FunctionDef)
        ]
        self.assertEqual(["lower_queue_source"], functions)
        self.assertLessEqual(len(source.splitlines()), 300)

    def test_compiler_does_not_import_facade(self) -> None:
        for path in COMPILER.glob("*.py"):
            with self.subTest(path=path.name):
                self.assertNotIn("_queue_frontend", path.read_text(encoding="utf-8"))

    def test_facade_re_exports_canonical_objects(self) -> None:
        from agentic_circuit import _queue_frontend as facade
        from agentic_circuit._queue_compiler import parser, provenance
        from agentic_circuit._queue_compiler.lower_acir import lower_queue_program

        self.assertIs(facade.parse_queue_program, parser.parse_queue_program)
        self.assertIs(facade.lower_queue_program, lower_queue_program)
        self.assertIs(facade.build_queue_acpy, provenance.build_queue_acpy)


if __name__ == "__main__":
    unittest.main()
