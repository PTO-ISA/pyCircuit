from __future__ import annotations

import ast
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
PACKAGE = ROOT / "python/agentic-circuit/src/agentic_circuit"
FACADE = PACKAGE / "_queue_frontend.py"
COMPILER = PACKAGE / "_queue_compiler"
PARSER = COMPILER / "parser.py"
STATEMENT_MODULES = (
    COMPILER / "graph_statements.py",
    COMPILER / "memory_statements.py",
    COMPILER / "endpoint_statements.py",
    COMPILER / "state_statements.py",
    COMPILER / "state_semantics.py",
)
LEGACY_FRONTEND_DEFINITIONS = frozenset(
    {
        "BarrierBinding",
        "BitfieldBinding",
        "CandidateSetBinding",
        "CollectionBinding",
        "CreditBinding",
        "DependencyBinding",
        "EntryViewBinding",
        "EnumBinding",
        "ExpectBinding",
        "FeedbackBinding",
        "ForkBinding",
        "InvariantDefinition",
        "MaskedEntryViewBinding",
        "MaskedTableWriteBinding",
        "MemoryBinding",
        "MemoryInstanceBinding",
        "MemoryRequestBinding",
        "MergeBinding",
        "ObservationBinding",
        "Payload",
        "ProjectedTableViewBinding",
        "PureHelperDefinition",
        "QueueBinding",
        "QueueFrontendError",
        "QueueProgram",
        "RecursiveQueueHelper",
        "ReorderBinding",
        "RouteBinding",
        "RuleDefinition",
        "RuleFindBinding",
        "RuleFindDefinition",
        "RuleLocalBinding",
        "RuleLocalDefinition",
        "RuleSlotOwnerBinding",
        "RuleSlotReleaseBinding",
        "RuleSlotReleaseDefinition",
        "RuleStateOwnerBinding",
        "RuleStateReadBinding",
        "RuleStateReadDefinition",
        "RuleStateWriteBinding",
        "RuleStateWriteDefinition",
        "ScopeBinding",
        "SelectBinding",
        "SelectedMemoryBinding",
        "SelectionBinding",
        "SinkBinding",
        "SlotBinding",
        "SlotReleaseBinding",
        "StaticConfigBinding",
        "StaticMemoryArrayBinding",
        "StaticParameterAlias",
        "StaticQueueCollection",
        "StaticTypeCheck",
        "TableBinding",
        "TableReadBinding",
        "TableWriteBinding",
        "VarStateBinding",
        "_ExpressionEmitter",
        "_ExpressionFact",
        "_ModuleRenderSpec",
        "_abi_layout",
        "_align",
        "_bitfields",
        "_bounded_annotation_static_checks",
        "_candidate_mask_type",
        "_config_schema_document",
        "_config_type_names",
        "_constant_integer",
        "_constantize_expression",
        "_contains_declared_range",
        "_decorator_name",
        "_dependent_static_type_expression",
        "_desugar_nested_rule_captures",
        "_enum_layout_entry",
        "_enums",
        "_integer_width",
        "_extract_conditional_effect_guard",
        "_helper_type",
        "_invariant_definitions",
        "_is_bool_like",
        "_is_none_return",
        "_lambda_value",
        "_lower_simple_module_source",
        "_module_static_values",
        "_nonnegative_int_value",
        "_normalize_queue_source_path",
        "_normalize_rule_field_assignments",
        "_payload",
        "_payload_layout_entry",
        "_payloads",
        "_positive_int_value",
        "_primitive_integer_width",
        "_product",
        "_project_static_config_value",
        "_proven_integer_in",
        "_pure_helper_definitions",
        "_render_bitfield",
        "_render_callsite_location",
        "_render_dense_i64",
        "_render_enum",
        "_render_fused_source_locations",
        "_render_interface_display_attributes",
        "_render_queue_type",
        "_render_source_frame_location",
        "_render_static_mlir_dictionary",
        "_render_static_mlir_value",
        "_render_static_type_attributes",
        "_render_table_domain_attributes",
        "_render_table_init_value",
        "_render_type",
        "_resolve_invariant_call",
        "_scalar_annotation_static_check",
        "_scalar_reset_init",
        "_scalar_type_descriptor",
        "_static_config_bindings_for_checks",
        "_static_config_expression_type",
        "_static_constraint",
        "_static_int_value",
        "_static_parameter_aliases",
        "_static_parameter_value",
        "_static_type_bindings_for_checks",
        "_strip_static_assertions",
        "_table_axis_width",
        "_table_schema_id",
        "_type_static_values",
        "_types_compatible",
        "_validate_static_config_roots",
        "build_queue_acpy",
        "lower_queue_program",
        "lower_queue_source",
        "parse_queue_program",
    }
)


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

    def test_facade_re_exports_complete_legacy_definition_inventory(self) -> None:
        from agentic_circuit import _queue_frontend as facade

        missing = sorted(
            name for name in LEGACY_FRONTEND_DEFINITIONS if not hasattr(facade, name)
        )
        self.assertEqual([], missing)
        for name in LEGACY_FRONTEND_DEFINITIONS - {"lower_queue_source"}:
            with self.subTest(name=name):
                self.assertTrue(
                    getattr(facade, name).__module__.startswith(
                        "agentic_circuit._queue_compiler"
                    )
                )

    def test_statement_handlers_keep_parser_priority_order(self) -> None:
        tree = ast.parse(PARSER.read_text(encoding="utf-8"), filename=str(PARSER))
        parse = next(
            node
            for node in tree.body
            if isinstance(node, ast.FunctionDef) and node.name == "parse_queue_program"
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
                "handle_state_statement",
                "handle_memory_declaration",
                "handle_memory_array_declaration",
                "handle_memory_array_select",
                "handle_memory_request",
                "handle_queue_graph_operation",
                "handle_multi_output_operation",
                "handle_expect",
                "handle_observe",
                "handle_sink",
                "handle_return",
            ],
            handlers,
        )
        self.assertLessEqual(visit.end_lineno - visit.lineno + 1, 2200)

    def test_statement_handlers_do_not_depend_on_parser_or_facade(self) -> None:
        for path in STATEMENT_MODULES:
            with self.subTest(path=path.name):
                source = path.read_text(encoding="utf-8")
                self.assertNotIn("_queue_frontend", source)
                imports = {
                    node.module
                    for node in ast.walk(ast.parse(source, filename=str(path)))
                    if isinstance(node, ast.ImportFrom)
                }
                self.assertNotIn("parser", imports)

    def test_statement_dependencies_flow_toward_parser(self) -> None:
        context = (COMPILER / "parser_context.py").read_text(encoding="utf-8")
        common = (COMPILER / "statement_common.py").read_text(encoding="utf-8")
        self.assertNotIn("_statements", context)
        self.assertNotIn("_statements", common)
        self.assertFalse((COMPILER / "statements.py").exists())


if __name__ == "__main__":
    unittest.main()
