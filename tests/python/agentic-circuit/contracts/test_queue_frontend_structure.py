from __future__ import annotations

import ast
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
PACKAGE = ROOT / "python/agentic-circuit/src/agentic_circuit"
FACADE = PACKAGE / "_queue_frontend.py"
COMPILER = PACKAGE / "_queue_compiler"
PARSER = COMPILER / "parser.py"
STATEMENTS = COMPILER / "statements.py"


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

    def test_statement_handlers_keep_parser_priority_order(self) -> None:
        tree = ast.parse(PARSER.read_text(encoding="utf-8"), filename=str(PARSER))
        parse = next(
            node
            for node in tree.body
            if isinstance(node, ast.FunctionDef)
            and node.name == "parse_queue_program"
        )
        visit = next(
            node
            for node in parse.body
            if isinstance(node, ast.FunctionDef) and node.name == "visit"
        )
        handlers = [
            node.func.id
            for node in ast.walk(visit)
            if isinstance(node, ast.Call)
            and isinstance(node.func, ast.Name)
            and node.func.id.startswith("handle_")
        ]
        self.assertEqual(
            [
                "handle_memory_array_select",
                "handle_queue_graph_operation",
                "handle_multi_output_operation",
                "handle_expect",
                "handle_observe",
                "handle_sink",
                "handle_return",
            ],
            handlers,
        )
        self.assertLessEqual(visit.end_lineno - visit.lineno + 1, 3000)

    def test_statement_handlers_do_not_depend_on_parser_or_facade(self) -> None:
        source = STATEMENTS.read_text(encoding="utf-8")
        self.assertNotIn("_queue_frontend", source)
        self.assertNotIn(".parser", source)


if __name__ == "__main__":
    unittest.main()
