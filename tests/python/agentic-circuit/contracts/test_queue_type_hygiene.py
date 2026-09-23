from __future__ import annotations

import ast
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
FRONTEND = ROOT / "python/agentic-circuit/src/agentic_circuit/_queue_frontend.py"
PARSER = ROOT / "python/agentic-circuit/src/agentic_circuit/_queue_compiler/parser.py"
MODULES = ROOT / "python/agentic-circuit/src/agentic_circuit/_queue_compiler/modules.py"
ACIR_TEXT = (
    ROOT / "python/agentic-circuit/src/agentic_circuit/_queue_compiler/acir_text.py"
)
TYPE_RENDERING = (
    ROOT
    / "python/agentic-circuit/src/agentic_circuit/_queue_compiler/type_rendering.py"
)
EXPRESSIONS = (
    ROOT / "python/agentic-circuit/src/agentic_circuit/_queue_compiler/expressions.py"
)
LOWER_ACIR = (
    ROOT / "python/agentic-circuit/src/agentic_circuit/_queue_compiler/lower_acir.py"
)
MODEL = ROOT / "python/agentic-circuit/src/agentic_circuit/_queue_compiler/model.py"


class _MlirCallVisitor(ast.NodeVisitor):
    def __init__(self) -> None:
        self.functions: list[str] = []
        self.violations: list[tuple[int, str | None]] = []

    def visit_FunctionDef(self, node: ast.FunctionDef) -> None:
        self.functions.append(node.name)
        self.generic_visit(node)
        self.functions.pop()

    def visit_AsyncFunctionDef(self, node: ast.AsyncFunctionDef) -> None:
        self.functions.append(node.name)
        self.generic_visit(node)
        self.functions.pop()

    def visit_Call(self, node: ast.Call) -> None:
        if isinstance(node.func, ast.Attribute) and node.func.attr == "mlir":
            function = self.functions[-1] if self.functions else None
            if function != "_render_type":
                self.violations.append((node.lineno, function))
        self.generic_visit(node)


class QueueTypeHygieneTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.tree = ast.parse(
            FRONTEND.read_text(encoding="utf-8"), filename=str(FRONTEND)
        )
        cls.parser_tree = ast.parse(
            PARSER.read_text(encoding="utf-8"), filename=str(PARSER)
        )
        cls.modules_tree = ast.parse(
            MODULES.read_text(encoding="utf-8"), filename=str(MODULES)
        )
        cls.acir_text_tree = ast.parse(
            ACIR_TEXT.read_text(encoding="utf-8"), filename=str(ACIR_TEXT)
        )
        cls.expressions_tree = ast.parse(
            EXPRESSIONS.read_text(encoding="utf-8"), filename=str(EXPRESSIONS)
        )
        cls.lower_acir_tree = ast.parse(
            LOWER_ACIR.read_text(encoding="utf-8"), filename=str(LOWER_ACIR)
        )
        cls.model_tree = ast.parse(
            MODEL.read_text(encoding="utf-8"), filename=str(MODEL)
        )

    def test_only_acir_type_renderer_calls_value_type_mlir(self) -> None:
        compiler = ROOT / "python/agentic-circuit/src/agentic_circuit/_queue_compiler"
        for path in sorted(compiler.glob("*.py")):
            with self.subTest(path=path.name):
                tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
                visitor = _MlirCallVisitor()
                visitor.visit(tree)
                if path == TYPE_RENDERING:
                    self.assertEqual([], visitor.violations)
                    self.assertIn(
                        "_render_type",
                        {
                            node.name
                            for node in tree.body
                            if isinstance(node, ast.FunctionDef)
                        },
                    )
                else:
                    self.assertEqual([], visitor.violations)
                    self.assertNotIn(
                        "_render_type",
                        {
                            node.name
                            for node in tree.body
                            if isinstance(node, ast.FunctionDef)
                        },
                    )

    def test_type_bearing_records_store_descriptors(self) -> None:
        expected = {
            "Payload": {"descriptor": "StructType"},
            "EnumBinding": {"descriptor": "ValueType"},
            "RuleStateWriteBinding": {"value_type": "ValueType"},
            "RuleStateReadBinding": {"value_type": "ValueType"},
            "RuleFindBinding": {"value_type": "ValueType"},
            "RuleStateOwnerBinding": {"value_type": "ValueType"},
            "QueueBinding": {
                "payload": "ValueType",
                "rule_payloads": "tuple[ValueType, ...]",
            },
            "MemoryInstanceBinding": {"data_type": "ValueType"},
            "MemoryBinding": {"data_type": "ValueType"},
            "TableBinding": {"entry_type": "ValueType"},
            "VarStateBinding": {"value_type": "ValueType"},
            "SlotBinding": {"payload": "ValueType"},
            "StaticMemoryArrayBinding": {"data_type": "ValueType"},
            "_ModuleRenderSpec": {
                "inputs": "tuple[tuple[str, ValueType], ...]",
                "outputs": "tuple[tuple[str, ValueType], ...]",
            },
            "ModuleState": {"value_type": "ValueType"},
            "ModuleDefinition": {
                "input_type": "ValueType",
                "output_type": "ValueType",
            },
            "RuleModuleDefinition": {
                "inputs": "tuple[tuple[str, ValueType], ...]",
                "outputs": "tuple[tuple[str, ValueType], ...]",
            },
        }
        classes = {
            node.name: node
            for tree in (
                self.tree,
                self.parser_tree,
                self.modules_tree,
                self.model_tree,
                self.lower_acir_tree,
            )
            for node in ast.walk(tree)
            if isinstance(node, ast.ClassDef)
        }
        for class_name, fields in expected.items():
            with self.subTest(class_name=class_name):
                definition = classes.get(class_name)
                self.assertIsNotNone(definition)
                annotations = {
                    statement.target.id: ast.unparse(statement.annotation)
                    for statement in definition.body
                    if isinstance(statement, ast.AnnAssign)
                    and isinstance(statement.target, ast.Name)
                }
                for field, annotation in fields.items():
                    self.assertEqual(annotation, annotations.get(field))

    def test_type_strings_are_not_recovered_from_rendered_spelling(self) -> None:
        # This check validates a declared source path, not a rendered ACIR type.
        allowed = {
            ("implementation_source", "startswith"),
            ("name", "startswith"),
            # Match compiler-generated AST function names, not rendered types.
            ("statement.name", "startswith"),
            ("line", "startswith"),
            ("lines[index]", "startswith"),
            ("concrete_lines[index]", "startswith"),
            ("metadata", "removeprefix"),
            (
                "_render_interface_display_attributes((ast.unparse(expression),), ('result',), projection_module_metadata(projection_name, projection_definition, projection_source))",
                "removeprefix",
            ),
            (
                "_render_interface_display_attributes(tuple((name for name, _ in definition.inputs)), tuple((name for name, _ in definition.outputs)), composite_module_metadata(symbol, function.name))",
                "removeprefix",
            ),
            (
                "_render_interface_display_attributes(tuple((name for name, _ in module.inputs)), tuple((name for name, _ in module.outputs)), _module_attribute_fields(module))",
                "removeprefix",
            ),
            (
                "_render_interface_display_attributes(tuple((name for name, _ in external)), tuple((f'result_{index}' for index in range(len(expected_results)))), module_metadata(system))",
                "removeprefix",
            ),
        }
        found: set[tuple[str, str]] = set()
        for tree in (
            self.tree,
            self.parser_tree,
            self.modules_tree,
            self.expressions_tree,
            self.lower_acir_tree,
        ):
            for node in ast.walk(tree):
                if (
                    isinstance(node, ast.Call)
                    and isinstance(node.func, ast.Attribute)
                    and node.func.attr in {"startswith", "removeprefix", "removesuffix"}
                ):
                    receiver = (
                        node.func.value.id
                        if isinstance(node.func.value, ast.Name)
                        else ast.unparse(node.func.value)
                    )
                    found.add((receiver, node.func.attr))
        self.assertEqual(allowed, found)

    def test_expression_emitter_carries_descriptor_types(self) -> None:
        emitter = next(
            node
            for node in ast.walk(self.expressions_tree)
            if isinstance(node, ast.ClassDef) and node.name == "_ExpressionEmitter"
        )
        methods = {
            node.name: node
            for node in emitter.body
            if isinstance(node, ast.FunctionDef)
        }
        emit = methods["emit"]
        self.assertEqual("tuple[str, ValueType]", ast.unparse(emit.returns))
        parameters = {
            argument.arg: ast.unparse(argument.annotation)
            for argument in emit.args.args
            if argument.annotation is not None
        }
        self.assertIn("ValueType", parameters["expected"])

        initializer = methods["__init__"]
        initializer_parameters = {
            argument.arg: ast.unparse(argument.annotation)
            for argument in (*initializer.args.args, *initializer.args.kwonlyargs)
            if argument.annotation is not None
        }
        for name in (
            "payload",
            "root_values",
            "table_views",
            "slot_views",
            "candidate_values",
            "selection_values",
            "find_values",
            "state_views",
            "table_domains",
        ):
            self.assertIn("ValueType", initializer_parameters[name])


if __name__ == "__main__":
    unittest.main()
