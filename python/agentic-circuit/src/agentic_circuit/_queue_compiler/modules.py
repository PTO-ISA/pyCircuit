"""Structured typed module-family and ACIR lowering."""

from __future__ import annotations

import ast
import copy
from collections.abc import Mapping
from dataclasses import dataclass

from _pycircuit_semantics import (
    RangeType,
    StructType,
    ValueType,
)

from .._canonical_json import canonical_mlir_string
from .._source_map import (
    SourceFrame,
    SourceNodeLocations,
    apply_source_node_locations,
    source_frame,
)
from .._static_eval import (
    FrozenMap,
    StaticEnvironment,
    StaticValue,
    evaluate_static,
)
from .acir_text import (
    _enum_layout_entry,
    _payload_layout_entry,
    _render_bitfield,
    _render_enum,
    _render_interface_display_attributes,
    _render_type,
)
from .definitions import _invariant_definitions, _pure_helper_definitions
from .errors import QueueFrontendError
from .expressions import (
    _ExpressionEmitter,
)
from .lower_acir import (
    _module_attribute_fields,
    _ModuleRenderSpec,
    lower_queue_program,
)
from .model import (
    Payload,
    QueueProgram,
    StaticTypeCheck,
)
from .normalize import (
    _desugar_nested_rule_captures,
    _strip_static_assertions,
)
from .parser import _reject_reserved_declarations, parse_queue_program
from .provenance import DefinitionNdfMetadata, NdfMetadata
from .source import (
    _normalize_queue_source_path,
    _render_source_frame_location,
)
from .static_types import (
    _bitfields,
    _bounded_annotation_static_checks,
    _config_type_names,
    _enums,
    _integer_width,
    _module_static_values,
    _payload,
    _payloads,
    _resolved_config_values_for_checks,
    _resolved_type_bindings_for_checks,
    _scalar_annotation_static_check,
    _scalar_reset_init,
    _static_config_expression_type,
    _static_parameter_aliases,
    _type_static_values,
    _types_compatible,
    _validate_static_config_roots,
)
from .syntax import _decorator_name
from .type_rendering import _nominal_declarations


def _lower_simple_module_source(
    text: str,
    system: str,
    *,
    static_arguments: Mapping[str, StaticValue] | None = None,
    host_results: bool = False,
    source_path: str | None = None,
    definition_locations: Mapping[str, tuple[str, int, int]] | None = None,
    static_assert_locations: (
        Mapping[str, tuple[tuple[str, int, int], ...]] | None
    ) = None,
    source_node_locations: SourceNodeLocations | None = None,
    definition_ndf: DefinitionNdfMetadata | None = None,
) -> str | None:
    definition_ndf = definition_ndf or {}
    normalized_source_path = _normalize_queue_source_path(source_path)
    tree = ast.parse(text, filename=normalized_source_path, type_comments=True)
    apply_source_node_locations(
        tree,
        source_node_locations,
        normalized_source_path,
    )
    _reject_reserved_declarations(tree)
    definition_sources = dict(definition_locations or {})
    if normalized_source_path.endswith(".py"):
        for node in ast.walk(tree):
            if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)):
                definition_sources.setdefault(
                    node.name,
                    (normalized_source_path, node.lineno, node.col_offset + 1),
                )

    def module_metadata(name: str) -> tuple[str, ...]:
        source = definition_sources.get(name)
        return _module_attribute_fields(
            _ModuleRenderSpec(
                name,
                (),
                (),
                ndf=definition_ndf.get(name, NdfMetadata()),
                source_file="" if source is None else source[0],
                source_line=0 if source is None else source[1],
                source_column=0 if source is None else source[2],
            )
        )

    def projection_module_metadata(
        name: str,
        definition_name: str,
        frame: SourceFrame | None,
    ) -> tuple[str, ...]:
        return _module_attribute_fields(
            _ModuleRenderSpec(
                name,
                (),
                (),
                ndf=definition_ndf.get(system, NdfMetadata()),
                source_file="" if frame is None else frame.file,
                source_line=0 if frame is None else frame.line,
                source_column=0 if frame is None else frame.column,
                definition_name=definition_name,
            )
        )

    def composite_module_metadata(
        symbol: str,
        definition_name: str,
    ) -> tuple[str, ...]:
        source = definition_sources.get(definition_name)
        return _module_attribute_fields(
            _ModuleRenderSpec(
                symbol,
                (),
                (),
                ndf=definition_ndf.get(definition_name, NdfMetadata()),
                source_file="" if source is None else source[0],
                source_line=0 if source is None else source[1],
                source_column=0 if source is None else source[2],
                definition_name=definition_name,
            )
        )

    module_names = [
        node.name
        for node in tree.body
        if isinstance(node, ast.FunctionDef)
        and any(
            _decorator_name(decorator).rsplit(".", 1)[-1] == "module"
            for decorator in node.decorator_list
        )
    ]
    for module_name in module_names:
        tree = _desugar_nested_rule_captures(tree, module_name, "module")
    module_static_values = _module_static_values(tree)
    type_static_values = _type_static_values(tree, static_arguments)
    parameter_aliases = _static_parameter_aliases(tree)
    enum_bindings = _enums(tree)
    enum_map = {item.name: item.descriptor for item in enum_bindings}
    payloads = _payloads(
        tree,
        enum_bindings,
        type_static_values,
        allow_unbound=True,
    )
    payload_map = {payload.name: payload for payload in payloads}
    bitfield_bindings = _bitfields(tree)
    bitfield_map = {binding.name: binding.layout for binding in bitfield_bindings}
    invariants = {
        definition.function_name: definition
        for definition in _invariant_definitions(
            tree, payload_map, bitfield_map, enum_map
        )
    }
    helper_definitions = _pure_helper_definitions(
        tree,
        payload_map,
        enum_map,
        entry=system,
        defer_unbound_unreachable=True,
    )
    helpers = {definition.name: definition for definition in helper_definitions}
    module_declarations: dict[str, str] = {}
    module_declaration_nodes: dict[str, ast.FunctionDef] = {}
    module_family_schemas: dict[str, tuple[str, str]] = {}
    module_family_interfaces: dict[str, str] = {}
    module_family_nominals: dict[str, tuple[str, ...]] = {}
    module_family_parameter_specs: dict[str, list[dict[str, object]]] = {}
    module_family_cases: dict[
        str, tuple[tuple[tuple[str, StaticValue], ...], ...]
    ] = {}
    module_family_case_attrs: dict[str, tuple[str, ...]] = {}

    def require_literal_tuple(node: ast.expr, subject: str) -> ast.Tuple:
        if not isinstance(node, ast.Tuple) or any(
            isinstance(item, ast.Starred) for item in node.elts
        ):
            raise QueueFrontendError(
                f"ACPY-FAMILY-008: {subject} must be a tuple literal"
            )
        return node

    def validate_static_value_literal(node: ast.expr) -> None:
        if isinstance(node, ast.Constant) and type(node.value) in {bool, int}:
            return
        if (
            isinstance(node, ast.UnaryOp)
            and isinstance(node.op, ast.USub)
            and isinstance(node.operand, ast.Constant)
            and type(node.operand.value) is int
        ):
            return
        if isinstance(node, ast.Attribute) and isinstance(node.value, ast.Name):
            return
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Name):
            if node.args or any(keyword.arg is None for keyword in node.keywords):
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: config values require keyword-only literal fields"
                )
            for keyword in node.keywords:
                validate_static_value_literal(keyword.value)
            return
        raise QueueFrontendError(
            "ACPY-FAMILY-008: static family value must use the closed literal grammar"
        )

    def validate_case_literal(node: ast.expr) -> None:
        if not isinstance(node, ast.Call) or node.keywords or (
            _decorator_name(node.func).rsplit(".", 1)[-1] != "case"
        ):
            raise QueueFrontendError(
                "ACPY-FAMILY-008: finite_cases entries must be literal case() calls"
            )
        for binding in node.args:
            pair = require_literal_tuple(binding, "case binding")
            if (
                len(pair.elts) != 2
                or not isinstance(pair.elts[0], ast.Constant)
                or type(pair.elts[0].value) is not str
            ):
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: case binding must be a literal (name, value) pair"
                )
            validate_static_value_literal(pair.elts[1])

    def literal_bool(node: ast.expr, subject: str) -> bool:
        if isinstance(node, ast.Constant) and type(node.value) is bool:
            return node.value
        raise QueueFrontendError(f"ACPY-FAMILY-008: {subject} must be a Boolean literal")

    def literal_int(node: ast.expr, subject: str) -> int:
        if isinstance(node, ast.Constant) and type(node.value) is int:
            return node.value
        if (
            isinstance(node, ast.UnaryOp)
            and isinstance(node.op, ast.USub)
            and isinstance(node.operand, ast.Constant)
            and type(node.operand.value) is int
        ):
            return -node.operand.value
        raise QueueFrontendError(f"ACPY-FAMILY-008: {subject} must be an integer literal")

    def call_kind(node: ast.expr) -> str:
        return (
            _decorator_name(node.func).rsplit(".", 1)[-1]
            if isinstance(node, ast.Call)
            else ""
        )

    def parse_static_parameters(
        parameters: ast.Tuple, frame: tuple[str, int, int]
    ) -> tuple[list[dict[str, object]], str]:
        class_definitions = {
            node.name: node
            for node in tree.body
            if isinstance(node, ast.ClassDef)
        }
        config_names = {
            name
            for name, node in class_definitions.items()
            if any(
                _decorator_name(decorator).rsplit(".", 1)[-1] == "config"
                for decorator in node.decorator_list
            )
        }
        enum_names = {
            name
            for name, node in class_definitions.items()
            if any(
                isinstance(base, ast.Name) and base.id in {"Enum", "IntEnum"}
                for base in node.bases
            )
        }

        def config_fields(
            name: str, active: tuple[str, ...] = ()
        ) -> list[tuple[str, tuple[str, object]]]:
            if name in active:
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: static config type is recursively defined"
                )
            definition = class_definitions.get(name)
            if definition is None or name not in config_names:
                raise QueueFrontendError(
                    f"ACPY-FAMILY-008: static config {name!r} is not declared"
                )
            if definition.bases or definition.keywords:
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: static config inheritance is not supported"
                )
            fields: list[tuple[str, tuple[str, object]]] = []
            for statement in definition.body:
                if (
                    isinstance(statement, ast.Expr)
                    and isinstance(statement.value, ast.Constant)
                    and isinstance(statement.value.value, str)
                ):
                    continue
                if not isinstance(statement, ast.AnnAssign) or not isinstance(
                    statement.target, ast.Name
                ):
                    raise QueueFrontendError(
                        "ACPY-FAMILY-008: static config admits only "
                        "annotation-only fields"
                    )
                if statement.value is not None:
                    raise QueueFrontendError(
                        "ACPY-FAMILY-008: static config fields cannot have defaults"
                    )
                annotation = statement.annotation
                if isinstance(annotation, ast.Name) and annotation.id == "bool":
                    descriptor: tuple[str, object] = ("bool", None)
                elif (
                    isinstance(annotation, ast.Call)
                    and call_kind(annotation) == "static_int"
                    and not annotation.args
                ):
                    keywords = {item.arg: item.value for item in annotation.keywords}
                    if None in keywords or set(keywords) != {"width", "signed"}:
                        raise QueueFrontendError(
                            "ACPY-FAMILY-008: config static_int requires literal "
                            "width and signedness"
                        )
                    width = literal_int(keywords["width"], "config static_int width")
                    signed = literal_bool(
                        keywords["signed"], "config static_int signedness"
                    )
                    if width <= 0:
                        raise QueueFrontendError(
                            "ACPY-FAMILY-008: config static_int width must be positive"
                        )
                    descriptor = ("int", (width, signed))
                elif not isinstance(annotation, ast.Name):
                    raise QueueFrontendError(
                        "ACPY-FAMILY-008: config fields require closed named types"
                    )
                elif annotation.id in enum_names:
                    descriptor = ("enum", annotation.id)
                elif annotation.id in config_names:
                    descriptor = (
                        "config",
                        (annotation.id, config_fields(annotation.id, (*active, name))),
                    )
                else:
                    raise QueueFrontendError(
                        "ACPY-FAMILY-008: config field type is not closed; "
                        "integers require ac.static_int(width=..., signed=...)"
                    )
                if any(field_name == statement.target.id for field_name, _ in fields):
                    raise QueueFrontendError(
                        "ACPY-FAMILY-008: static config field names must be unique"
                    )
                fields.append((statement.target.id, descriptor))
            if not fields:
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: static config requires at least one field"
                )
            return fields

        def descriptor_type(descriptor: tuple[str, object]) -> str:
            field_kind, payload = descriptor
            if field_kind == "bool":
                return "#ac.static_bool_type"
            if field_kind == "int":
                width, signed = payload
                return (
                    f"#ac.static_int_type<{width}, "
                    f"{'true' if signed else 'false'}>"
                )
            if field_kind == "enum":
                return f"#ac.static_enum_type<@{payload}>"
            nested_name, nested_fields = payload
            return render_config_type(str(nested_name), nested_fields)

        def render_config_type(
            name: str, fields: list[tuple[str, tuple[str, object]]]
        ) -> str:
            rendered_fields = ", ".join(
                "#ac.static_config_field<"
                + canonical_mlir_string(field_name)
                + ", "
                + descriptor_type(descriptor)
                + ">"
                for field_name, descriptor in fields
            )
            return (
                f"#ac.static_config_type<@{name}, "
                f"#ac.static_config_fields<[{rendered_fields}]>>"
            )

        def parse_config_value(
            node: ast.expr,
            name: str,
            fields: list[tuple[str, tuple[str, object]]],
            subject: str,
        ) -> FrozenMap:
            if (
                not isinstance(node, ast.Call)
                or not isinstance(node.func, ast.Name)
                or node.func.id != name
                or node.args
                or any(keyword.arg is None for keyword in node.keywords)
            ):
                raise QueueFrontendError(
                    f"ACPY-FAMILY-008: {subject} must construct {name} with literal fields"
                )
            supplied = {str(keyword.arg): keyword.value for keyword in node.keywords}
            if set(supplied) != {field_name for field_name, _ in fields}:
                raise QueueFrontendError(
                    f"ACPY-FAMILY-008: {subject} has incomplete config fields"
                )
            values: list[tuple[str, StaticValue]] = []
            for field_name, descriptor in fields:
                field_kind, payload = descriptor
                value_node = supplied[field_name]
                if field_kind == "bool":
                    value: StaticValue = literal_bool(value_node, subject)
                elif field_kind == "int":
                    value = literal_int(value_node, subject)
                    width, signed = payload
                    minimum = -(1 << (width - 1)) if signed else 0
                    maximum = (1 << (width - (1 if signed else 0))) - 1
                    if value < minimum or value > maximum:
                        raise QueueFrontendError(
                            f"ACPY-FAMILY-008: {subject} integer config field "
                            "is out of range"
                        )
                elif (
                    field_kind == "enum"
                    and isinstance(value_node, ast.Attribute)
                    and isinstance(value_node.value, ast.Name)
                    and value_node.value.id == payload
                ):
                    value = value_node.attr
                elif field_kind == "config":
                    nested_name, nested_fields = payload
                    value = parse_config_value(
                        value_node, str(nested_name), nested_fields, subject
                    )
                else:
                    raise QueueFrontendError(
                        f"ACPY-FAMILY-008: {subject} has a mistyped config field"
                    )
                values.append((field_name, value))
            return FrozenMap(tuple(values))

        def render_config_value(
            name: str,
            fields: list[tuple[str, tuple[str, object]]],
            value: FrozenMap,
        ) -> str:
            supplied = dict(value.entries)
            rendered_fields: list[str] = []
            for field_name, descriptor in fields:
                field_kind, payload = descriptor
                item = supplied[field_name]
                if field_kind == "bool":
                    raw = f"#ac.static_bool_value<{'true' if item else 'false'}>"
                elif field_kind == "int":
                    width, signed = payload
                    raw = (
                        f"#ac.static_int_value<#ac.static_int_type<{width}, "
                        f"{'true' if signed else 'false'}>, {item} : i{width}>"
                    )
                elif field_kind == "enum":
                    raw = (
                        f"#ac.static_enum_value<@{payload}, "
                        f"{canonical_mlir_string(str(item))}>"
                    )
                else:
                    nested_name, nested_fields = payload
                    assert isinstance(item, FrozenMap)
                    raw = render_config_value(
                        str(nested_name), nested_fields, item
                    )
                rendered_fields.append(
                    "#ac.static_config_field_value<"
                    + canonical_mlir_string(field_name)
                    + ", "
                    + raw
                    + ">"
                )
            return (
                f"#ac.static_config_value<@{name}, "
                "#ac.static_config_field_values<["
                + ", ".join(rendered_fields)
                + "]>>"
            )

        parsed: list[dict[str, object]] = []
        rendered: list[str] = []
        names: set[str] = set()
        for expression in parameters.elts:
            if not isinstance(expression, ast.Call) or call_kind(expression) != "static_parameter" or len(expression.args) != 2:
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: parameters entries must be literal static_parameter() calls"
                )
            name_node, type_node = expression.args
            if not isinstance(name_node, ast.Constant) or type(name_node.value) is not str or not name_node.value.isidentifier():
                raise QueueFrontendError("ACPY-FAMILY-008: static parameter name must be a literal identifier")
            name = name_node.value
            if name in names:
                raise QueueFrontendError("ACPY-FAMILY-008: static parameter names must be unique")
            names.add(name)
            if not isinstance(type_node, ast.Call):
                raise QueueFrontendError("ACPY-FAMILY-008: static parameter type must use a closed constructor")
            kind = call_kind(type_node)
            enum_name: str | None = None
            config_name: str | None = None
            config_schema: list[tuple[str, tuple[str, object]]] | None = None
            if kind == "static_bool" and not type_node.args and not type_node.keywords:
                type_text = "#ac.static_bool_type"
                width = None
                signed = None
            elif kind == "static_int" and not type_node.args:
                type_keywords = {item.arg: item.value for item in type_node.keywords}
                if set(type_keywords) != {"width", "signed"}:
                    raise QueueFrontendError("ACPY-FAMILY-008: static_int requires literal width and signedness")
                width = literal_int(type_keywords["width"], "static_int width")
                signed = literal_bool(type_keywords["signed"], "static_int signedness")
                if width <= 0:
                    raise QueueFrontendError("ACPY-FAMILY-008: static_int width must be positive")
                type_text = f"#ac.static_int_type<{width}, {'true' if signed else 'false'}>"
            elif (
                kind == "static_enum"
                and len(type_node.args) == 1
                and not type_node.keywords
                and isinstance(type_node.args[0], ast.Name)
            ):
                enum_name = type_node.args[0].id
                type_text = f"#ac.static_enum_type<@{enum_name}>"
                width = None
                signed = None
            elif (
                kind == "static_config"
                and len(type_node.args) == 1
                and not type_node.keywords
                and isinstance(type_node.args[0], ast.Name)
            ):
                config_name = type_node.args[0].id
                config_schema = config_fields(config_name)
                type_text = render_config_type(config_name, config_schema)
                width = None
                signed = None
            else:
                raise QueueFrontendError("ACPY-FAMILY-008: unsupported static parameter type constructor")
            options = {item.arg: item.value for item in expression.keywords}
            if None in options or set(options) - {"default", "constraints"}:
                raise QueueFrontendError("ACPY-FAMILY-008: static_parameter has unsupported keyword")
            default_node = options.get("default")
            def parse_value(
                node: ast.expr,
                subject: str,
                *,
                value_kind: str = kind,
                value_enum: str | None = enum_name,
                value_config_name: str | None = config_name,
                value_config_schema: (
                    list[tuple[str, tuple[str, object]]] | None
                ) = config_schema,
            ) -> object:
                if value_kind == "static_bool":
                    return literal_bool(node, subject)
                if value_kind == "static_int":
                    return literal_int(node, subject)
                if (
                    value_kind == "static_enum"
                    and isinstance(node, ast.Attribute)
                    and isinstance(node.value, ast.Name)
                    and node.value.id == value_enum
                ):
                    return node.attr
                if value_kind == "static_config":
                    assert (
                        value_config_name is not None
                        and value_config_schema is not None
                    )
                    return parse_config_value(
                        node, value_config_name, value_config_schema, subject
                    )
                raise QueueFrontendError(
                    f"ACPY-FAMILY-008: {subject} has the wrong static type"
                )

            default = None if default_node is None else parse_value(default_node, "default")
            required = default_node is None
            def render_value(
                value: object,
                *,
                value_kind: str = kind,
                value_type: str = type_text,
                value_width: int | None = width,
                value_enum: str | None = enum_name,
                value_config_name: str | None = config_name,
                value_config_schema: (
                    list[tuple[str, tuple[str, object]]] | None
                ) = config_schema,
            ) -> str:
                if value_kind == "static_bool":
                    return f"#ac.static_value<#ac.static_bool_value<{'true' if value else 'false'}>>"
                if value_kind == "static_enum":
                    return (
                        "#ac.static_value<#ac.static_enum_value<"
                        f"@{value_enum}, {canonical_mlir_string(str(value))}>>"
                    )
                if value_kind == "static_config":
                    assert (
                        value_config_name is not None
                        and value_config_schema is not None
                    )
                    assert isinstance(value, FrozenMap)
                    return "#ac.static_value<" + render_config_value(
                        value_config_name, value_config_schema, value
                    ) + ">"
                return f"#ac.static_value<#ac.static_int_value<{value_type}, {value} : i{value_width}>>"
            constraint_texts: list[str] = []
            constraints_node = options.get("constraints")
            if constraints_node is not None:
                for constraint in require_literal_tuple(constraints_node, "constraints").elts:
                    if not isinstance(constraint, ast.Call):
                        raise QueueFrontendError("ACPY-FAMILY-008: constraints must use closed constructors")
                    constraint_kind = call_kind(constraint)
                    if constraint_kind == "one_of" and not constraint.keywords and constraint.args:
                        values = [parse_value(value, "one_of value") for value in constraint.args]
                        constraint_texts.append("#ac.static_constraint<#ac.one_of<[" + ", ".join(render_value(value) for value in values) + "]>>")
                    elif constraint_kind == "integer_range" and kind == "static_int" and not constraint.keywords and len(constraint.args) == 2:
                        minimum, maximum = (literal_int(value, "integer_range bound") for value in constraint.args)
                        def raw(
                            value: int,
                            value_type: str = type_text,
                            value_width: int | None = width,
                        ) -> str:
                            return (
                                f"#ac.static_int_value<{value_type}, {value} "
                                f": i{value_width}>"
                            )
                        constraint_texts.append(f"#ac.static_constraint<#ac.integer_range<{raw(minimum)}, {raw(maximum)}>>")
                    else:
                        raise QueueFrontendError("ACPY-FAMILY-008: malformed static constraint")
            provenance = (
                "#ac.source_provenance<"
                + canonical_mlir_string(frame[0])
                + f", {frame[1]}, {frame[2]}, {frame[1]}, {frame[2]}>"
            )
            rendered.append(
                f"#ac.static_parameter<{canonical_mlir_string(name)}, #ac.static_type<{type_text}>, "
                + ("true" if required else f"false default {render_value(default)}")
                + f", [{', '.join(constraint_texts)}], {provenance}>"
            )
            parsed.append({"name": name, "kind": kind, "type": type_text, "width": width, "enum": enum_name, "config_name": config_name, "config_schema": config_schema, "default": default, "required": required, "parse": parse_value, "render": render_value})
        return parsed, "#ac.static_parameters<[" + ", ".join(rendered) + "]>"

    def render_static_cases(
        cases: ast.expr, parameters: list[dict[str, object]]
    ) -> tuple[str, tuple[tuple[tuple[str, StaticValue], ...], ...]]:
        case_nodes = require_literal_tuple(cases, "finite_cases").elts
        rendered_cases: list[str] = []
        values_by_case: list[tuple[tuple[str, StaticValue], ...]] = []
        for case_node in case_nodes:
            validate_case_literal(case_node)
            assert isinstance(case_node, ast.Call)
            supplied = {binding.elts[0].value: binding.elts[1] for binding in case_node.args if isinstance(binding, ast.Tuple)}
            arguments: list[str] = []
            values: list[tuple[str, StaticValue]] = []
            for parameter in parameters:
                name = str(parameter["name"])
                value_node = supplied.pop(name, None)
                if value_node is None:
                    if parameter["required"]:
                        raise QueueFrontendError(f"ACPY-FAMILY-008: finite case is missing {name!r}")
                    value = parameter["default"]
                else:
                    parse_value = parameter["parse"]
                    assert callable(parse_value)
                    value = parse_value(value_node, "case value")
                render_value = parameter["render"]
                assert callable(render_value)
                arguments.append(f"#ac.static_argument<{canonical_mlir_string(name)}, {render_value(value)}>")
                values.append((name, value))
            if supplied:
                raise QueueFrontendError(f"ACPY-FAMILY-008: finite case has unknown binding {next(iter(supplied))!r}")
            rendered_cases.append("#ac.static_arguments<[" + ", ".join(arguments) + "]>")
            values_by_case.append(tuple(values))
        return (
            "#ac.static_cases<[" + ", ".join(rendered_cases) + "]>",
            tuple(values_by_case),
        )

    def queue_annotation_parts(
        annotation: ast.expr,
    ) -> tuple[ast.expr, ast.expr, ast.expr] | None:
        if (
            not isinstance(annotation, ast.Subscript)
            or _decorator_name(annotation.value).rsplit(".", 1)[-1] != "Queue"
        ):
            return None
        if not isinstance(annotation.slice, ast.Tuple) or len(annotation.slice.elts) != 3:
            raise QueueFrontendError(
                "ACPY-FAMILY-008: Queue annotation requires payload, lanes, and rate"
            )
        payload, lanes, rate = annotation.slice.elts
        return payload, lanes, rate

    def evaluate_dependent_integer(
        expression: ast.expr, values: Mapping[str, StaticValue]
    ) -> int:
        if isinstance(expression, ast.Constant) and type(expression.value) is int:
            return expression.value
        if (
            isinstance(expression, ast.UnaryOp)
            and isinstance(expression.op, ast.USub)
            and isinstance(expression.operand, ast.Constant)
            and type(expression.operand.value) is int
        ):
            return -expression.operand.value
        if isinstance(expression, ast.Name) and expression.id in values:
            value = values[expression.id]
        elif isinstance(expression, ast.Attribute):
            fields: list[str] = []
            cursor: ast.expr = expression
            while isinstance(cursor, ast.Attribute):
                fields.append(cursor.attr)
                cursor = cursor.value
            if not isinstance(cursor, ast.Name) or cursor.id not in values:
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: Queue shape field has an unknown root"
                )
            value = values[cursor.id]
            for field in reversed(fields):
                if not isinstance(value, FrozenMap):
                    raise QueueFrontendError(
                        "ACPY-FAMILY-008: Queue shape field crosses a non-config value"
                    )
                try:
                    value = value[field]
                except KeyError as error:
                    raise QueueFrontendError(
                        f"ACPY-FAMILY-008: Queue shape field {field!r} is unknown"
                    ) from error
        elif isinstance(expression, ast.BinOp) and isinstance(
            expression.op, (ast.Add, ast.Sub, ast.Mult)
        ):
            lhs = evaluate_dependent_integer(expression.left, values)
            rhs = evaluate_dependent_integer(expression.right, values)
            if isinstance(expression.op, ast.Add):
                return lhs + rhs
            if isinstance(expression.op, ast.Sub):
                return lhs - rhs
            return lhs * rhs
        elif (
            isinstance(expression, ast.Call)
            and len(expression.args) == 1
            and not expression.keywords
            and _decorator_name(expression.func).rsplit(".", 1)[-1]
            in {"index_width", "count_width"}
        ):
            operand = evaluate_dependent_integer(expression.args[0], values)
            kind = _decorator_name(expression.func).rsplit(".", 1)[-1]
            if kind == "index_width":
                if operand <= 0:
                    raise QueueFrontendError(
                        "ACPY-FAMILY-008: index_width operand must be positive"
                    )
                return max(1, (operand - 1).bit_length())
            if operand < 0:
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: count_width operand must be non-negative"
                )
            return max(1, operand.bit_length())
        else:
            raise QueueFrontendError(
                "ACPY-FAMILY-008: unsupported dependent Queue shape expression"
            )
        if type(value) is not int:
            raise QueueFrontendError(
                "ACPY-FAMILY-008: Queue shape must resolve to an integer"
            )
        return value

    def materialized_queue_shape(
        annotation: ast.expr, values: Mapping[str, StaticValue]
    ) -> tuple[int, int]:
        parts = queue_annotation_parts(annotation)
        if parts is None:
            return (1, 1)
        _, lanes_node, rate_node = parts
        lanes = evaluate_dependent_integer(lanes_node, values)
        rate = evaluate_dependent_integer(rate_node, values)
        if lanes <= 0 or rate <= 0 or rate > lanes:
            raise QueueFrontendError(
                "ACPY-FAMILY-008: Queue requires lanes > 0 and 1 <= rate <= lanes"
            )
        return lanes, rate

    def render_concrete_queue(value_type: ValueType, shape: tuple[int, int]) -> str:
        lanes, rate = shape
        suffix = "" if shape == (1, 1) else f", lanes={lanes}, rate={rate}"
        return f"!ac.queue<{_render_type(value_type)}{suffix}>"

    def render_family_interface(
        declaration: ast.FunctionDef, frame: tuple[str, int, int]
    ) -> tuple[str, tuple[ValueType, ...]]:
        provenance = (
            "#ac.source_provenance<"
            f"{canonical_mlir_string(frame[0])}, {frame[1]}, {frame[2]}, "
            f"{frame[1]}, {frame[2]}>"
        )
        one = "#ac.dependent_value<#ac.dependent_integer<1>>"
        port_payloads: list[ValueType] = []

        parameter_names = {
            str(parameter["name"])
            for parameter in module_family_parameter_specs.get(declaration.name, [])
        }

        def dependent_value(expression: ast.expr) -> str:
            if isinstance(expression, ast.Constant) and type(expression.value) is int:
                record = f"#ac.dependent_integer<{expression.value}>"
            elif (
                isinstance(expression, ast.UnaryOp)
                and isinstance(expression.op, ast.USub)
                and isinstance(expression.operand, ast.Constant)
                and type(expression.operand.value) is int
            ):
                record = f"#ac.dependent_integer<{-expression.operand.value}>"
            elif isinstance(expression, ast.Name) and expression.id in parameter_names:
                record = f"#ac.dependent_parameter<{canonical_mlir_string(expression.id)}>"
            elif isinstance(expression, ast.Attribute):
                fields: list[str] = []
                cursor: ast.expr = expression
                while isinstance(cursor, ast.Attribute):
                    fields.append(cursor.attr)
                    cursor = cursor.value
                if not isinstance(cursor, ast.Name) or cursor.id not in parameter_names:
                    raise QueueFrontendError(
                        "ACPY-FAMILY-008: dependent field must root at a static config parameter"
                    )
                fields.reverse()
                record = (
                    "#ac.dependent_field<#ac.dependent_parameter<"
                    f"{canonical_mlir_string(cursor.id)}>, ["
                    + ", ".join(canonical_mlir_string(field) for field in fields)
                    + "]>"
                )
            elif isinstance(expression, ast.BinOp) and isinstance(
                expression.op, (ast.Add, ast.Sub, ast.Mult)
            ):
                mnemonic = {
                    ast.Add: "dependent_add",
                    ast.Sub: "dependent_sub",
                    ast.Mult: "dependent_mul",
                }[type(expression.op)]
                record = (
                    f"#ac.{mnemonic}<{dependent_value(expression.left)}, "
                    f"{dependent_value(expression.right)}>"
                )
            elif (
                isinstance(expression, ast.Call)
                and len(expression.args) == 1
                and not expression.keywords
                and _decorator_name(expression.func).rsplit(".", 1)[-1]
                in {"index_width", "count_width"}
            ):
                kind = _decorator_name(expression.func).rsplit(".", 1)[-1]
                record = f"#ac.dependent_{kind}<{dependent_value(expression.args[0])}>"
            else:
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: unsupported dependent integer expression"
                )
            return f"#ac.dependent_value<{record}>"

        def logical_type(annotation: ast.expr) -> str:
            kind = _decorator_name(
                annotation.value if isinstance(annotation, ast.Subscript) else annotation
            ).rsplit(".", 1)[-1]
            if isinstance(annotation, ast.Subscript) and kind == "bits":
                return (
                    "#ac.type_expr<#ac.type_expr_bits<"
                    f"{dependent_value(annotation.slice)}, false>>"
                )
            if isinstance(annotation, ast.Subscript) and kind in {"tuple", "Tuple"}:
                elements = (
                    annotation.slice.elts
                    if isinstance(annotation.slice, ast.Tuple)
                    else (annotation.slice,)
                )
                return (
                    "#ac.type_expr<#ac.type_expr_tuple<["
                    + ", ".join(logical_type(element) for element in elements)
                    + "]>>"
                )
            if isinstance(annotation, ast.Subscript) and kind == "array":
                if not isinstance(annotation.slice, ast.Tuple) or len(annotation.slice.elts) != 2:
                    raise QueueFrontendError(
                        "ACPY-FAMILY-008: dependent array requires length and element"
                    )
                length, element = annotation.slice.elts
                return (
                    "#ac.type_expr<#ac.type_expr_value_array<"
                    f"{dependent_value(length)}, {logical_type(element)}>>"
                )
            value_type = _payload(
                annotation,
                payload_map,
                enum_map,
                static_values=type_static_values,
            )
            port_payloads.append(value_type)
            return (
                "#ac.type_expr<#ac.type_expr_concrete<"
                f"{_render_type(value_type)}>>"
            )

        def logical_queue(annotation: ast.expr) -> str:
            if (
                isinstance(annotation, ast.Subscript)
                and _decorator_name(annotation.value).rsplit(".", 1)[-1]
                == "Queue"
            ):
                if (
                    not isinstance(annotation.slice, ast.Tuple)
                    or len(annotation.slice.elts) != 3
                ):
                    raise QueueFrontendError(
                        "ACPY-FAMILY-008: Queue annotation requires payload, "
                        "lanes, and rate"
                    )
                payload, lanes, rate = annotation.slice.elts
                return (
                    "#ac.type_expr<#ac.type_expr_queue<"
                    f"{logical_type(payload)}, {dependent_value(lanes)}, "
                    f"{dependent_value(rate)}>>"
                )
            return (
                "#ac.type_expr<#ac.type_expr_queue<"
                f"{logical_type(annotation)}, {one}, {one}>>"
            )

        ports: list[str] = []
        for parameter in declaration.args.args:
            if parameter.annotation is None:
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: module declaration ports require annotations"
                )
            ports.append(
                "#ac.interface_port<"
                f"{canonical_mlir_string(parameter.arg)}, \"input\", "
                f"{logical_queue(parameter.annotation)}, {provenance}>"
            )
        result_nodes: tuple[ast.expr, ...]
        if declaration.returns is None or (
            isinstance(declaration.returns, ast.Constant)
            and declaration.returns.value is None
        ):
            result_nodes = ()
        elif (
            isinstance(declaration.returns, ast.Subscript)
            and _decorator_name(declaration.returns.value).rsplit(".", 1)[-1]
            in {"tuple", "Tuple"}
        ):
            result_nodes = (
                tuple(declaration.returns.slice.elts)
                if isinstance(declaration.returns.slice, ast.Tuple)
                else (declaration.returns.slice,)
            )
        else:
            result_nodes = (declaration.returns,)
        for index, annotation in enumerate(result_nodes):
            name = "result" if len(result_nodes) == 1 else f"result{index}"
            ports.append(
                "#ac.interface_port<"
                f"{canonical_mlir_string(name)}, \"output\", "
                f"{logical_queue(annotation)}, {provenance}>"
            )
        return (
            "#ac.module_interface<[" + ", ".join(ports) + "]>",
            tuple(port_payloads),
        )

    for node in tree.body:
        if not isinstance(node, ast.FunctionDef):
            continue
        declarations = [
            decorator
            for decorator in node.decorator_list
            if _decorator_name(decorator).rsplit(".", 1)[-1] == "module_decl"
        ]
        if not declarations:
            continue
        decorator = declarations[0]
        if len(declarations) != 1 or not isinstance(decorator, ast.Call) or decorator.args:
            raise QueueFrontendError(
                "ACPY-MODULE-009: module declaration requires keyword-only "
                "source, parameters, and finite_cases"
            )
        keywords = {keyword.arg: keyword.value for keyword in decorator.keywords}
        if (
            None in keywords
            or set(keywords) - {"source", "parameters", "finite_cases"}
            or "source" not in keywords
            or not isinstance(keywords["source"], ast.Constant)
            or type(keywords["source"].value) is not str
            or not keywords["source"].value
        ):
            raise QueueFrontendError(
                "ACPY-MODULE-009: module declaration requires one literal source"
            )
        parameters = require_literal_tuple(
            keywords.get("parameters", ast.Tuple(elts=[], ctx=ast.Load())),
            "module parameters",
        )
        finite_cases = keywords.get("finite_cases", ast.Constant(value=None))
        if parameters.elts:
            cases = require_literal_tuple(finite_cases, "finite_cases")
            if not cases.elts:
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: parameterized module requires finite_cases"
                )
            for family_case in cases.elts:
                validate_case_literal(family_case)
        elif not (
            isinstance(finite_cases, ast.Constant) and finite_cases.value is None
        ):
            cases = require_literal_tuple(finite_cases, "finite_cases")
            if len(cases.elts) != 1:
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: zero-parameter family admits one case()"
                )
            validate_case_literal(cases.elts[0])
        implementation_source = keywords["source"].value
        path = implementation_source.split("/")
        if (
            implementation_source.startswith("/")
            or "\\" in implementation_source
            or implementation_source[-3:] != ".py"
            or any(not component or component in {".", ".."} for component in path)
        ):
            raise QueueFrontendError(
                "ACPY-MODULE-009: module declaration source must be a safe "
                "relative POSIX .py path"
            )
        module_declarations[node.name] = implementation_source
        module_declaration_nodes[node.name] = node
        frame = (implementation_source, 1, 1)
        parsed_parameters, rendered_parameters = parse_static_parameters(parameters, frame)
        if parsed_parameters:
            rendered_cases, case_values = render_static_cases(
                finite_cases, parsed_parameters
            )
        else:
            rendered_cases = "#ac.static_cases<[#ac.static_arguments<[]>]>"
            case_values = ((),)
        module_family_schemas[node.name] = (rendered_parameters, rendered_cases)
        module_family_parameter_specs[node.name] = parsed_parameters
        interface, port_payloads = render_family_interface(node, frame)
        nominals = [
            str(parameter["enum"])
            for parameter in parsed_parameters
            if parameter.get("enum") is not None
        ]
        nominals.extend(
            nominal
            for nominal in _nominal_declarations(port_payloads)
            if nominal not in nominals
        )
        module_family_nominals[node.name] = tuple(nominals)
        module_family_interfaces[node.name] = interface
        module_family_cases[node.name] = case_values
        rendered_argument_sets: list[str] = []
        for values in case_values:
            value_map = dict(values)
            arguments: list[str] = []
            for parameter in parsed_parameters:
                parameter_name = str(parameter["name"])
                render_value = parameter["render"]
                assert callable(render_value)
                arguments.append(
                    f"#ac.static_argument<{canonical_mlir_string(parameter_name)}, "
                    f"{render_value(value_map[parameter_name])}>"
                )
            rendered_argument_sets.append(
                "#ac.static_arguments<[" + ", ".join(arguments) + "]>"
            )
        module_family_case_attrs[node.name] = tuple(rendered_argument_sets)

    modules: dict[str, ast.FunctionDef] = {}
    module_implementations: set[str] = set()
    for node in tree.body:
        if not isinstance(node, ast.FunctionDef):
            continue
        kinds = {
            _decorator_name(decorator).rsplit(".", 1)[-1]
            for decorator in node.decorator_list
        }
        if not kinds & {"module", "module_decl"}:
            continue
        if "module" in kinds:
            implementations = [
                decorator
                for decorator in node.decorator_list
                if _decorator_name(decorator).rsplit(".", 1)[-1] == "module"
            ]
            if (
                len(implementations) != 1
                or not isinstance(implementations[0], ast.Call)
                or implementations[0].args
                or [keyword.arg for keyword in implementations[0].keywords]
                != ["declaration"]
            ):
                raise QueueFrontendError(
                    "ACPY-MODULE-009: module implementation requires exact "
                    "@module(declaration=...)"
                )
            module_implementations.add(node.name)
        if node.name not in modules or "module" in kinds:
            modules[node.name] = node
    if not modules:
        return None
    systems = [
        node
        for node in tree.body
        if isinstance(node, ast.FunctionDef)
        and node.name == system
        and any(
            _decorator_name(decorator).rsplit(".", 1)[-1] == "system"
            for decorator in node.decorator_list
        )
    ]
    if len(systems) != 1:
        raise QueueFrontendError(
            f"ACPY-MODULE-001: system {system!r} is missing or ambiguous"
        )
    reserved_definitions = sorted(
        name for name in modules if name.startswith("compiler_")
    )
    reserved_values = sorted(
        {
            name
            for function in (systems[0], *modules.values())
            for name in (
                function.name,
                *(
                    parameter.arg
                    for parameter in (
                        *function.args.posonlyargs,
                        *function.args.args,
                        *function.args.kwonlyargs,
                    )
                ),
                *(
                    node.id
                    for node in ast.walk(function)
                    if isinstance(node, ast.Name)
                    and isinstance(node.ctx, ast.Store)
                ),
            )
        }
    )
    reserved_values = [
        name for name in reserved_values if name.startswith("compiler_")
    ]
    if reserved_definitions or reserved_values:
        reserved = (reserved_definitions + reserved_values)[0]
        raise QueueFrontendError(
            "ACPY-MODULE-008: names beginning with 'compiler_' are compiler-owned: "
            f"{reserved!r}"
        )
    if not any(
        isinstance(node, ast.Call)
        and isinstance(node.func, ast.Name)
        and node.func.id in modules
        for node in ast.walk(systems[0])
    ):
        return None

    reachable_modules: set[str] = set()
    pending_modules = [
        node.func.id
        for node in ast.walk(systems[0])
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Name)
        and node.func.id in modules
    ]
    while pending_modules:
        module_name = pending_modules.pop()
        if module_name in reachable_modules:
            continue
        reachable_modules.add(module_name)
        pending_modules.extend(
            node.func.id
            for node in ast.walk(modules[module_name])
            if isinstance(node, ast.Call)
            and isinstance(node.func, ast.Name)
            and node.func.id in modules
        )
    modules = {
        name: function
        for name, function in modules.items()
        if name in reachable_modules
    }

    def result_payloads(annotation: ast.expr | None) -> tuple[ValueType, ...]:
        if annotation is None or (
            isinstance(annotation, ast.Constant) and annotation.value is None
        ):
            return ()
        if isinstance(annotation, ast.Subscript) and _decorator_name(
            annotation.value
        ).rsplit(".", 1)[-1] in {"tuple", "Tuple"}:
            elements = (
                annotation.slice.elts
                if isinstance(annotation.slice, ast.Tuple)
                else (annotation.slice,)
            )
            return tuple(
                _payload(
                    element,
                    payload_map,
                    enum_map,
                    static_values=type_static_values,
                )
                for element in elements
            )
        return (
            _payload(
                annotation,
                payload_map,
                enum_map,
                static_values=type_static_values,
            ),
        )

    def result_annotations(annotation: ast.expr | None) -> tuple[ast.expr, ...]:
        if annotation is None or (
            isinstance(annotation, ast.Constant) and annotation.value is None
        ):
            return ()
        if isinstance(annotation, ast.Subscript) and _decorator_name(
            annotation.value
        ).rsplit(".", 1)[-1] in {"tuple", "Tuple"}:
            return (
                tuple(copy.deepcopy(annotation.slice.elts))
                if isinstance(annotation.slice, ast.Tuple)
                else (copy.deepcopy(annotation.slice),)
            )
        return (copy.deepcopy(annotation),)

    @dataclass(frozen=True, slots=True)
    class ModuleState:
        name: str
        value_type: ValueType
        init: int

    @dataclass(frozen=True, slots=True)
    class ModuleAssignment:
        state: str
        expression: ast.expr

    @dataclass(frozen=True, slots=True)
    class ModuleDefinition:
        argument: str
        input_type: ValueType
        output_type: ValueType
        expression: ast.expr
        states: tuple[ModuleState, ...] = ()
        assignments: tuple[ModuleAssignment, ...] = ()

    @dataclass(frozen=True, slots=True)
    class RuleModuleDefinition:
        inputs: tuple[tuple[str, ValueType], ...]
        outputs: tuple[tuple[str, ValueType], ...]
        static_parameters: tuple[str, ...]
        static_defaults: tuple[tuple[str, ast.expr], ...]

    @dataclass(frozen=True, slots=True)
    class RuleModuleTemplate:
        input_annotations: tuple[tuple[str, ast.expr], ...]
        output_names: tuple[str, ...]
        output_annotations: tuple[ast.expr, ...]
        static_parameters: tuple[str, ...]
        static_defaults: tuple[tuple[str, ast.expr], ...]
        static_parameter_types: tuple[tuple[str, str], ...]

    module_types: dict[str, ModuleDefinition] = {}
    empty_modules: set[str] = set()
    empty_module_children: dict[
        str, tuple[tuple[str, SourceFrame | None], ...]
    ] = {}
    composite_modules: dict[str, ast.FunctionDef] = {}
    rule_modules: dict[str, RuleModuleTemplate] = {}
    rule_names = {
        node.name
        for node in tree.body
        if isinstance(node, ast.FunctionDef)
        and any(
            _decorator_name(decorator).rsplit(".", 1)[-1] == "rule"
            for decorator in node.decorator_list
        )
    }
    family_case_functions: dict[
        str, tuple[tuple[tuple[str, StaticValue], ...], ast.FunctionDef]
    ] = {}

    def specialize_family_function(
        function: ast.FunctionDef,
        values: tuple[tuple[str, StaticValue], ...],
    ) -> ast.FunctionDef:
        environment = dict(values)

        class Specialize(ast.NodeTransformer):
            def visit_Name(self, node: ast.Name) -> ast.expr:
                if isinstance(node.ctx, ast.Load) and node.id in environment:
                    value = environment[node.id]
                    if type(value) in {bool, int}:
                        return ast.copy_location(ast.Constant(value=value), node)
                return node

            def visit_If(self, node: ast.If):
                node = self.generic_visit(node)
                if isinstance(node.test, ast.Constant) and type(node.test.value) is bool:
                    return node.body if node.test.value else node.orelse
                return node

            def visit_IfExp(self, node: ast.IfExp):
                node = self.generic_visit(node)
                if isinstance(node.test, ast.Constant) and type(node.test.value) is bool:
                    return node.body if node.test.value else node.orelse
                return node

        specialized = Specialize().visit(copy.deepcopy(function))
        assert isinstance(specialized, ast.FunctionDef)
        for index, statement in enumerate(specialized.body):
            if isinstance(statement, ast.Return):
                specialized.body = specialized.body[: index + 1]
                break
        return ast.fix_missing_locations(specialized)

    for family_name, cases in module_family_cases.items():
        if (
            family_name not in module_implementations
            or family_name not in modules
            or not cases
        ):
            continue
        original = modules[family_name]
        case_functions = tuple(
            (values, specialize_family_function(original, values)) for values in cases
        )
        family_case_functions[family_name] = case_functions
        modules[family_name] = case_functions[0][1]

    def materialized_family_case_source(
        family_name: str,
        values: tuple[tuple[str, StaticValue], ...],
    ) -> str | None:
        """Render one selected family body with its static values bound.

        ``parse_queue_program`` discovers ``ac.const`` parameters from the
        entry signature. Family parameters instead belong to the source-owned
        declaration, so passing them as parser arguments would correctly reject
        them as unknown. Materialize the selected finite case first, including
        nested rules desugared out of the module body, then parse the resulting
        closed body without inventing a second parameter surface.
        """
        if not module_family_parameter_specs.get(family_name):
            return None
        cases = family_case_functions.get(family_name)
        if cases is None:
            return None
        selected = next(
            (function for arguments, function in cases if arguments == values),
            None,
        )
        if selected is None:
            raise QueueFrontendError(
                "ACPY-FAMILY-008: static arguments do not select a declared case"
            )
        case_tree = copy.deepcopy(tree)
        nested_prefix = f"compiler_nested_{family_name}_"
        rewritten: list[ast.stmt] = []
        for statement in case_tree.body:
            if isinstance(statement, ast.FunctionDef):
                decorators = {
                    _decorator_name(decorator).rsplit(".", 1)[-1]
                    for decorator in statement.decorator_list
                }
                if statement.name == family_name and "module" in decorators:
                    rewritten.append(copy.deepcopy(selected))
                    continue
                if statement.name.startswith(nested_prefix) and "rule" in decorators:
                    rewritten.append(specialize_family_function(statement, values))
                    continue
            rewritten.append(statement)
        case_tree.body = rewritten
        return ast.unparse(ast.fix_missing_locations(case_tree))

    def template_static_fields(
        name: str, function: ast.FunctionDef
    ) -> tuple[
        tuple[str, ...],
        tuple[tuple[str, ast.expr], ...],
        tuple[tuple[str, str], ...],
    ]:
        """Static parameter fields for one module template.

        A declared static family owns its parameters in the family declaration,
        so its specs win for every body shape. A parameterized family body that
        instantiates a child (composite) otherwise degrades to its empty Python
        keyword-only signature and then rejects the family's own
        `static=case(...)` selection as an unknown argument. Non-family modules
        keep the keyword-only `ac.const` fields unchanged.
        """
        family_specs = module_family_parameter_specs.get(name, [])
        if family_specs:
            return (
                tuple(str(parameter["name"]) for parameter in family_specs),
                tuple(
                    (
                        str(parameter["name"]),
                        ast.Constant(value=parameter["default"]),
                    )
                    for parameter in family_specs
                    if not parameter["required"]
                ),
                tuple(
                    (str(parameter["name"]), str(parameter["kind"]))
                    for parameter in family_specs
                ),
            )
        return (
            tuple(parameter.arg for parameter in function.args.kwonlyargs),
            tuple(
                (parameter.arg, default)
                for parameter, default in zip(
                    function.args.kwonlyargs,
                    function.args.kw_defaults,
                    strict=True,
                )
                if default is not None
            ),
            tuple(
                (
                    parameter.arg,
                    _decorator_name(parameter.annotation.slice).rsplit(".", 1)[-1],
                )
                for parameter in function.args.kwonlyargs
                if isinstance(parameter.annotation, ast.Subscript)
            ),
        )

    for name, function in modules.items():
        if name in module_declarations:
            declaration = module_declaration_nodes[name]
            body = list(declaration.body)
            if (
                body
                and isinstance(body[0], ast.Expr)
                and isinstance(body[0].value, ast.Constant)
                and isinstance(body[0].value.value, str)
            ):
                body.pop(0)
            if (
                len(body) != 1
                or not isinstance(body[0], ast.Expr)
                or not isinstance(body[0].value, ast.Constant)
                or body[0].value.value is not Ellipsis
                or declaration.args.posonlyargs
                or declaration.args.vararg is not None
                or declaration.args.kwarg is not None
                or declaration.args.defaults
                or any(
                    not isinstance(parameter.annotation, ast.Subscript)
                    or _decorator_name(parameter.annotation.value).rsplit(".", 1)[-1]
                    != "const"
                    for parameter in declaration.args.kwonlyargs
                )
            ):
                raise QueueFrontendError(
                    "ACPY-MODULE-009: module declaration requires typed runtime "
                    "parameters, optional keyword-only ac.const parameters, and "
                    "an ellipsis body"
                )
            output_annotations = result_annotations(declaration.returns)
            family_specs = module_family_parameter_specs.get(name, [])
            family_static_parameters = tuple(
                str(parameter["name"]) for parameter in family_specs
            )
            family_defaults = tuple(
                (str(parameter["name"]), ast.Constant(value=parameter["default"]))
                for parameter in family_specs
                if not parameter["required"]
            )
            rule_modules[name] = RuleModuleTemplate(
                tuple(
                    (parameter.arg, copy.deepcopy(parameter.annotation))
                    for parameter in declaration.args.args
                ),
                tuple(
                    "result" if len(output_annotations) == 1 else f"result{index}"
                    for index in range(len(output_annotations))
                ),
                output_annotations,
                family_static_parameters
                or tuple(parameter.arg for parameter in declaration.args.kwonlyargs),
                family_defaults or tuple(
                    (parameter.arg, default)
                    for parameter, default in zip(
                        declaration.args.kwonlyargs,
                        declaration.args.kw_defaults,
                        strict=True,
                    )
                    if default is not None
                ),
                tuple(
                    (str(parameter["name"]), str(parameter["kind"]))
                    for parameter in family_specs
                )
                or tuple(
                    (
                        parameter.arg,
                        _decorator_name(parameter.annotation.slice).rsplit(".", 1)[-1],
                    )
                    for parameter in declaration.args.kwonlyargs
                ),
            )
            if name not in module_implementations:
                continue
            if not declaration.args.kwonlyargs and not family_specs:
                rule_modules.pop(name)
        empty_body = list(function.body)
        if (
            empty_body
            and isinstance(empty_body[0], ast.Expr)
            and isinstance(empty_body[0].value, ast.Constant)
            and isinstance(empty_body[0].value.value, str)
        ):
            empty_body.pop(0)
        if empty_body and (
            isinstance(empty_body[-1], ast.Pass)
            or isinstance(empty_body[-1], ast.Return)
            and (
                empty_body[-1].value is None
                or isinstance(empty_body[-1].value, ast.Constant)
                and empty_body[-1].value.value is None
            )
        ):
            empty_body.pop()
        children: list[tuple[str, SourceFrame | None]] = []
        for statement in empty_body:
            if not (
                isinstance(statement, ast.Expr)
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Name)
                and statement.value.func.id in module_declarations
                and not statement.value.args
                and not statement.value.keywords
            ):
                children = []
                break
            children.append((statement.value.func.id, source_frame(statement.value)))
        is_empty_body = not empty_body or bool(children)
        if (
            is_empty_body
            and not function.args.posonlyargs
            and not function.args.args
            and not function.args.kwonlyargs
            and function.args.vararg is None
            and function.args.kwarg is None
            and not result_annotations(function.returns)
        ):
            empty_modules.add(name)
            empty_module_children[name] = tuple(children)
            continue
        contains_rule_call = any(
            isinstance(node, ast.Call)
            and isinstance(node.func, ast.Name)
            and node.func.id in rule_names
            for node in ast.walk(function)
        )
        contains_module_call = any(
            isinstance(node, ast.Call)
            and isinstance(node.func, ast.Name)
            and node.func.id in modules
            for node in ast.walk(function)
        )
        if contains_module_call and not contains_rule_call:
            if (
                function.args.posonlyargs
                or function.args.vararg is not None
                or function.args.kwarg is not None
                or function.args.defaults
                or any(
                    not isinstance(parameter.annotation, ast.Subscript)
                    or _decorator_name(parameter.annotation.value).rsplit(".", 1)[-1]
                    != "const"
                    for parameter in function.args.kwonlyargs
                )
            ):
                raise QueueFrontendError(
                    "ACPY-MODULE-010: composite modules require positional "
                    "typed runtime inputs and optional keyword-only ac.const parameters"
                )
            output_annotations = result_annotations(function.returns)
            (
                template_static_parameters,
                template_static_defaults,
                template_static_parameter_types,
            ) = template_static_fields(name, function)
            rule_modules[name] = RuleModuleTemplate(
                tuple(
                    (parameter.arg, copy.deepcopy(parameter.annotation))
                    for parameter in function.args.args
                ),
                tuple(
                    "result" if len(output_annotations) == 1 else f"result{index}"
                    for index in range(len(output_annotations))
                ),
                output_annotations,
                template_static_parameters,
                template_static_defaults,
                template_static_parameter_types,
            )
            composite_modules[name] = function
            continue
        if contains_rule_call:
            if (
                not function.args.args
                or function.args.posonlyargs
                or function.args.vararg is not None
                or function.args.kwarg is not None
                or function.args.defaults
            ):
                raise QueueFrontendError(
                    "ACPY-MODULE-005: rule modules require positional typed "
                    "runtime inputs and optional keyword-only ac.const parameters"
                )
            if any(
                not isinstance(parameter.annotation, ast.Subscript)
                or _decorator_name(parameter.annotation.value).rsplit(".", 1)[-1]
                != "const"
                for parameter in function.args.kwonlyargs
            ):
                raise QueueFrontendError(
                    "ACPY-MODULE-005: keyword-only rule module parameters must "
                    "use ac.const"
                )
            input_annotations = tuple(
                (parameter.arg, copy.deepcopy(parameter.annotation))
                for parameter in function.args.args
            )
            output_annotations = result_annotations(function.returns)
            body = list(function.body)
            if (
                body
                and isinstance(body[0], ast.Expr)
                and isinstance(body[0].value, ast.Constant)
                and isinstance(body[0].value.value, str)
            ):
                body.pop(0)
            returned = body[-1] if body else None
            if not isinstance(returned, ast.Return) or returned.value is None:
                raise QueueFrontendError(
                    "ACPY-MODULE-005: rule module requires a typed return"
                )
            result_nodes = (
                tuple(returned.value.elts)
                if isinstance(returned.value, (ast.Tuple, ast.List))
                else (returned.value,)
            )
            if len(result_nodes) != len(output_annotations) or not all(
                isinstance(result, ast.Name) for result in result_nodes
            ):
                raise QueueFrontendError(
                    "ACPY-MODULE-005: rule module return names must match its arity"
                )
            (
                template_static_parameters,
                template_static_defaults,
                template_static_parameter_types,
            ) = template_static_fields(name, function)
            rule_modules[name] = RuleModuleTemplate(
                input_annotations,
                tuple(
                    result.id for result in result_nodes if isinstance(result, ast.Name)
                ),
                output_annotations,
                template_static_parameters,
                template_static_defaults,
                template_static_parameter_types,
            )
            continue
        pure_module_checks: list[StaticTypeCheck] = []
        for parameter in function.args.args:
            check = _scalar_annotation_static_check(
                f"interface.module.{name}.input.{parameter.arg}",
                parameter.annotation,
                parameter_aliases,
                type_static_values,
            )
            if check is not None:
                pure_module_checks.append(check)
            pure_module_checks.extend(
                _bounded_annotation_static_checks(
                    f"interface.module.{name}.input.{parameter.arg}",
                    parameter.annotation,
                    parameter_aliases,
                    type_static_values,
                )
            )
            parameter_type = _payload(
                parameter.annotation,
                payload_map,
                enum_map,
                static_values=type_static_values,
            )
            pure_module_checks.extend(
                check
                for payload in payloads
                if payload.descriptor == parameter_type
                for check in payload.resolved_type_checks
            )
        for index, annotation in enumerate(result_annotations(function.returns)):
            check = _scalar_annotation_static_check(
                f"interface.module.{name}.output.{index}",
                annotation,
                parameter_aliases,
                type_static_values,
            )
            if check is not None:
                pure_module_checks.append(check)
            pure_module_checks.extend(
                _bounded_annotation_static_checks(
                    f"interface.module.{name}.output.{index}",
                    annotation,
                    parameter_aliases,
                    type_static_values,
                )
            )
            result_type = _payload(
                annotation,
                payload_map,
                enum_map,
                static_values=type_static_values,
            )
            pure_module_checks.extend(
                check
                for payload in payloads
                if payload.descriptor == result_type
                for check in payload.resolved_type_checks
            )
        _validate_static_config_roots(
            function,
            parameter_aliases,
            {
                token[6:]
                for check in pure_module_checks
                for token in check.program
                if token[:6] == "param:"
            },
        )
        if (
            len(function.args.args) != 1
            or function.args.posonlyargs
            or function.args.kwonlyargs
            or function.args.vararg is not None
            or function.args.kwarg is not None
            or function.args.defaults
            or function.args.kw_defaults
        ):
            raise QueueFrontendError(
                "ACPY-MODULE-001: first module slice requires one typed "
                "positional parameter"
            )
        parameter = function.args.args[0]
        body = list(function.body)
        if (
            body
            and isinstance(body[0], ast.Expr)
            and isinstance(body[0].value, ast.Constant)
            and isinstance(body[0].value.value, str)
        ):
            body.pop(0)
        outputs = result_payloads(function.returns)
        if len(outputs) != 1:
            raise QueueFrontendError(
                "ACPY-MODULE-001: first module slice requires one typed result"
            )
        input_type = _payload(
            parameter.annotation,
            payload_map,
            enum_map,
            static_values=type_static_values,
        )
        if (
            len(body) == 1
            and isinstance(body[0], ast.Return)
            and body[0].value is not None
        ):
            module_types[name] = ModuleDefinition(
                parameter.arg,
                input_type,
                outputs[0],
                copy.deepcopy(body[0].value),
            )
            continue
        if body and isinstance(body[-1], ast.Return) and body[-1].value is not None:
            states: list[ModuleState] = []
            state_names: set[str] = set()
            cursor = 0
            while cursor < len(body) - 1 and isinstance(body[cursor], ast.AnnAssign):
                declaration = body[cursor]
                assert isinstance(declaration, ast.AnnAssign)
                if (
                    not isinstance(declaration.target, ast.Name)
                    or declaration.value is None
                    or not isinstance(declaration.value, ast.Constant)
                    or type(declaration.value.value) is not int
                ):
                    raise QueueFrontendError(
                        "ACPY-MODULE-004: module state requires a typed static "
                        "integer initializer"
                    )
                state_name = declaration.target.id
                if state_name == parameter.arg:
                    raise QueueFrontendError(
                        "ACPY-MODULE-004: module state must not shadow its parameter"
                    )
                if state_name in state_names:
                    raise QueueFrontendError(
                        "ACPY-MODULE-004: module state names must be unique"
                    )
                state_type = _payload(
                    declaration.annotation,
                    payload_map,
                    enum_map,
                    static_values=type_static_values,
                )
                state_init = _scalar_reset_init(
                    state_type,
                    declaration.value.value,
                    code="ACPY-MODULE-004",
                )
                if type(state_init) is not int:
                    raise QueueFrontendError(
                        "ACPY-MODULE-004: module bits state requires integer init"
                    )
                if _integer_width(state_type) is None:
                    raise QueueFrontendError(
                        "ACPY-MODULE-004: first module state slice requires scalars"
                    )
                state_names.add(state_name)
                states.append(ModuleState(state_name, state_type, state_init))
                cursor += 1
            assignment_nodes = body[cursor:-1]
            if states and assignment_nodes:
                assignments: list[ModuleAssignment] = []
                assigned: set[str] = set()
                for statement in assignment_nodes:
                    if (
                        not isinstance(statement, ast.Assign)
                        or len(statement.targets) != 1
                        or not isinstance(statement.targets[0], ast.Name)
                        or statement.targets[0].id not in state_names
                    ):
                        raise QueueFrontendError(
                            "ACPY-MODULE-004: stateful module statements must "
                            "assign declared lexical state"
                        )
                    state_name = statement.targets[0].id
                    if state_name in assigned:
                        raise QueueFrontendError(
                            "ACPY-MODULE-004: each module state may be assigned once"
                        )
                    assigned.add(state_name)
                    assignments.append(
                        ModuleAssignment(state_name, copy.deepcopy(statement.value))
                    )
                if assigned != state_names:
                    raise QueueFrontendError(
                        "ACPY-MODULE-004: first stateful module slice requires "
                        "one assignment per declared state"
                    )
                module_types[name] = ModuleDefinition(
                    parameter.arg,
                    input_type,
                    outputs[0],
                    copy.deepcopy(body[-1].value),
                    tuple(states),
                    tuple(assignments),
                )
                continue
        raise QueueFrontendError(
            "ACPY-MODULE-001: module body requires one expression return or "
            "zero-initialized typed state assignments followed by return"
        )

    def module_signature(
        name: str,
    ) -> tuple[tuple[tuple[str, ValueType], ...], tuple[tuple[str, ValueType], ...]]:
        if name in empty_modules:
            return (), ()
        definition = module_types[name]
        return (
            ((definition.argument, definition.input_type),),
            (("result", definition.output_type),),
        )

    def selected_module_queue_shapes(
        name: str, static_arguments: tuple[tuple[str, StaticValue], ...]
    ) -> tuple[tuple[tuple[int, int], ...], tuple[tuple[int, int], ...]]:
        declaration = module_declaration_nodes.get(name)
        if declaration is None:
            if name in module_types or name in empty_modules:
                inputs, outputs = module_signature(name)
                return ((1, 1),) * len(inputs), ((1, 1),) * len(outputs)
            # A composite body (one that instantiates a child instead of
            # returning a single pure expression) owns no `module_types` entry.
            # Its Queue shapes live in the specialized template annotations,
            # which is also where a finite family case keeps them during
            # concrete case lowering after the family declaration is replaced
            # by a concrete module definition.
            template = rule_modules.get(name)
            if template is None:
                raise QueueFrontendError(
                    f"ACPY-MODULE-002: module {name!r} has no materialized "
                    "Queue interface"
                )
            values = dict(static_arguments)
            return (
                tuple(
                    materialized_queue_shape(annotation, values)
                    for _, annotation in template.input_annotations
                ),
                tuple(
                    materialized_queue_shape(annotation, values)
                    for annotation in template.output_annotations
                ),
            )
        values = dict(static_arguments)
        input_annotations = (
            *declaration.args.posonlyargs,
            *declaration.args.args,
        )
        return (
            tuple(
                materialized_queue_shape(parameter.annotation, values)
                for parameter in input_annotations
            ),
            tuple(
                materialized_queue_shape(annotation, values)
                for annotation in result_annotations(declaration.returns)
            ),
        )

    function = systems[0]
    if function.args.vararg is not None or function.args.kwarg is not None:
        raise QueueFrontendError(
            "ACPY-MODULE-001: module systems cannot use variadic parameters"
        )
    parameters = [
        *function.args.posonlyargs,
        *function.args.args,
        *function.args.kwonlyargs,
    ]
    positional = [*function.args.posonlyargs, *function.args.args]
    positional_with_defaults = (
        positional[-len(function.args.defaults) :] if function.args.defaults else []
    )
    positional_defaults = {
        parameter.arg: default
        for parameter, default in zip(
            positional_with_defaults,
            function.args.defaults,
            strict=True,
        )
    }
    keyword_defaults = {
        parameter.arg: default
        for parameter, default in zip(
            function.args.kwonlyargs,
            function.args.kw_defaults,
            strict=True,
        )
        if default is not None
    }
    supplied = dict(static_arguments or {})
    static_parameter_names: set[str] = set()
    external: list[tuple[str, ValueType]] = []
    external_queue_shapes: dict[str, tuple[int, int]] = {}
    for parameter in parameters:
        if (
            isinstance(parameter.annotation, ast.Subscript)
            and _decorator_name(parameter.annotation.value).rsplit(".", 1)[-1]
            == "const"
        ):
            static_parameter_names.add(parameter.arg)
            if parameter.arg in supplied:
                continue
            default = positional_defaults.get(parameter.arg) or keyword_defaults.get(
                parameter.arg
            )
            if default is None:
                raise QueueFrontendError(
                    f"ACPY-QUEUE-022: system requires static argument {parameter.arg!r}"
                )
            try:
                supplied[parameter.arg] = evaluate_static(
                    default, StaticEnvironment(supplied)
                )
            except ValueError as error:
                raise QueueFrontendError(
                    f"ACPY-QUEUE-022: default for {parameter.arg!r} is not static"
                ) from error
            continue
        if parameter.arg in supplied:
            raise QueueFrontendError(
                "ACPY-QUEUE-022: supplied static arguments must use ac.const"
            )
        if parameter.arg in positional_defaults or parameter.arg in keyword_defaults:
            raise QueueFrontendError(
                "ACPY-QUEUE-022: external system values cannot have defaults"
            )
        external.append(
            (
                parameter.arg,
                _payload(
                    parameter.annotation,
                    payload_map,
                    enum_map,
                    static_values=type_static_values,
                ),
            )
        )
        external_queue_shapes[parameter.arg] = materialized_queue_shape(
            parameter.annotation, type_static_values
        )
    extras = sorted(set(supplied) - static_parameter_names)
    if extras:
        raise QueueFrontendError(
            f"ACPY-MODULE-001: unknown static argument {extras[0]!r}"
        )
    system_static_values: Mapping[str, StaticValue] = {
        **module_static_values,
        **supplied,
    }
    system_static_types = {
        parameter.arg: _decorator_name(parameter.annotation.slice).rsplit(".", 1)[-1]
        for parameter in parameters
        if isinstance(parameter.annotation, ast.Subscript)
        and _decorator_name(parameter.annotation.value).rsplit(".", 1)[-1] == "const"
    }
    _strip_static_assertions(
        function,
        system_static_values,
        normalized_source_path,
        definition_locations,
        static_assert_locations,
    )
    expected_results = result_payloads(function.returns)
    result_queue_shapes = tuple(
        materialized_queue_shape(annotation, type_static_values)
        for annotation in result_annotations(function.returns)
    )
    system_interface_checks: list[StaticTypeCheck] = []
    for parameter in parameters:
        if (
            isinstance(parameter.annotation, ast.Subscript)
            and _decorator_name(parameter.annotation.value).rsplit(".", 1)[-1]
            == "const"
        ):
            continue
        check = _scalar_annotation_static_check(
            f"interface.system.{system}.input.{parameter.arg}",
            parameter.annotation,
            parameter_aliases,
            type_static_values,
        )
        if check is not None:
            system_interface_checks.append(check)
        system_interface_checks.extend(
            _bounded_annotation_static_checks(
                f"interface.system.{system}.input.{parameter.arg}",
                parameter.annotation,
                parameter_aliases,
                type_static_values,
            )
        )
    for index, annotation in enumerate(result_annotations(function.returns)):
        check = _scalar_annotation_static_check(
            f"interface.system.{system}.output.{index}",
            annotation,
            parameter_aliases,
            type_static_values,
        )
        if check is not None:
            system_interface_checks.append(check)
        system_interface_checks.extend(
            _bounded_annotation_static_checks(
                f"interface.system.{system}.output.{index}",
                annotation,
                parameter_aliases,
                type_static_values,
            )
        )
    values = dict(external)
    value_queue_shapes = dict(external_queue_shapes)
    uses = {name: 0 for name, _ in external}
    projections: list[
        tuple[
            str,
            str,
            str,
            tuple[tuple[str, StaticValue], ...],
            ast.Attribute,
            ValueType,
            SourceFrame | None,
        ]
    ] = []
    projection_definitions: dict[
        str,
        tuple[
            str,
            str,
            tuple[tuple[str, StaticValue], ...],
            ValueType,
            ast.Attribute,
            ValueType,
            SourceFrame | None,
        ],
    ] = {}
    instances: list[
        tuple[
            tuple[str, ...],
            str,
            tuple[str, ...],
            tuple[ValueType, ...],
            tuple[tuple[str, StaticValue], ...],
            SourceFrame | None,
        ]
    ] = []
    operation_order: list[tuple[str, int]] = []
    module_bodies: dict[
        str,
        tuple[
            RuleModuleDefinition,
            QueueProgram | None,
            tuple[tuple[str, StaticValue], ...],
        ],
    ] = {}
    declaration_sources: dict[str, str] = {}
    composite_functions: dict[
        str, tuple[ast.FunctionDef, tuple[tuple[str, StaticValue], ...]]
    ] = {}
    returned_names: tuple[str, ...] | None = None

    def specialize_system_statements(
        statements: list[ast.stmt],
    ) -> list[ast.stmt]:
        specialized: list[ast.stmt] = []
        for statement in statements:
            if not isinstance(statement, ast.If):
                specialized.append(statement)
                continue
            try:
                condition = evaluate_static(
                    statement.test, StaticEnvironment(system_static_values)
                )
            except ValueError as error:
                raise QueueFrontendError(
                    "ACPY-MODULE-007: system control flow must depend only on "
                    "ac.const values"
                ) from error
            if type(condition) is not bool:
                raise QueueFrontendError(
                    "ACPY-MODULE-007: static system condition must be bool"
                )
            selected = statement.body if condition else statement.orelse
            specialized.extend(specialize_system_statements(selected))
        return specialized

    def specialize_rule_module(
        module_name: str,
        call: ast.Call,
        *,
        context_static_values: Mapping[str, StaticValue] | None = None,
        context_static_types: Mapping[str, str] | None = None,
    ) -> tuple[
        str,
        tuple[tuple[str, StaticValue], ...],
        tuple[tuple[str, ValueType], ...],
        tuple[tuple[str, ValueType], ...],
    ]:
        template = rule_modules[module_name]
        active_static_values = (
            system_static_values
            if context_static_values is None
            else context_static_values
        )
        active_static_types = (
            system_static_types
            if context_static_types is None
            else context_static_types
        )
        supplied_keywords: dict[str, ast.expr] = {}
        call_keywords = list(call.keywords)
        if module_family_parameter_specs.get(module_name):
            if len(call_keywords) != 1 or call_keywords[0].arg != "static":
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: parameterized child requires static=case(...)"
                )
            static_case = call_keywords[0].value
            validate_case_literal(static_case)
            assert isinstance(static_case, ast.Call)
            call_keywords = [
                ast.keyword(arg=binding.elts[0].value, value=binding.elts[1])
                for binding in static_case.args
                if isinstance(binding, ast.Tuple)
            ]
        for keyword in call_keywords:
            if keyword.arg is None or keyword.arg in supplied_keywords:
                raise QueueFrontendError(
                    "ACPY-MODULE-007: module static arguments require unique names"
                )
            supplied_keywords[keyword.arg] = keyword.value
        unknown = sorted(set(supplied_keywords) - set(template.static_parameters))
        if unknown:
            raise QueueFrontendError(
                f"ACPY-MODULE-007: unknown module static argument {unknown[0]!r}"
            )
        defaults = dict(template.static_defaults)
        expected_types = dict(template.static_parameter_types)
        family_specs_by_name = {
            str(parameter["name"]): parameter
            for parameter in module_family_parameter_specs.get(module_name, [])
        }
        static_values: list[tuple[str, StaticValue]] = []
        for name in template.static_parameters:
            expression = supplied_keywords.get(name, defaults.get(name))
            if expression is None:
                raise QueueFrontendError(
                    f"ACPY-MODULE-007: module requires static argument {name!r}"
                )
            expected_type = expected_types.get(name)
            actual_type = _static_config_expression_type(
                expression,
                active_static_types,
                tree,
            )
            if expected_type in _config_type_names(tree):
                if actual_type is None:
                    raise QueueFrontendError(
                        "ACPY-MODULE-007: module static argument "
                        f"{name!r} requires nominal ac.const[{expected_type}] "
                        "provenance"
                    )
                if actual_type != expected_type:
                    raise QueueFrontendError(
                        "ACPY-MODULE-007: module static argument "
                        f"{name!r} requires ac.const[{expected_type}], got "
                        f"ac.const[{actual_type}]"
                    )
            elif (
                isinstance(expression, ast.Name)
                and actual_type is not None
                and expected_type is not None
                and actual_type != expected_type
            ):
                raise QueueFrontendError(
                    "ACPY-MODULE-007: module static argument "
                    f"{name!r} requires ac.const[{expected_type}], got "
                    f"ac.const[{actual_type}]"
                )
            family_spec = family_specs_by_name.get(name)
            if family_spec is not None:
                parse_value = family_spec["parse"]
                assert callable(parse_value)
                value = parse_value(expression, "module static argument")
            else:
                try:
                    value = evaluate_static(
                        expression, StaticEnvironment(active_static_values)
                    )
                except ValueError as error:
                    raise QueueFrontendError(
                        f"ACPY-MODULE-007: module static argument {name!r} is not closed"
                    ) from error
            static_values.append((name, value))
        frozen = tuple(static_values)
        declared_cases = module_family_cases.get(module_name)
        if declared_cases is not None and frozen not in declared_cases:
            raise QueueFrontendError(
                "ACPY-FAMILY-008: static arguments do not select a declared case"
            )
        if module_name in module_implementations and (
            module_name in module_types
            or module_name in empty_modules
        ):
            inputs, outputs = module_signature(module_name)
            return module_name, frozen, inputs, outputs
        symbol = module_name
        existing = module_bodies.get(symbol)
        if existing is not None and existing[2] != frozen:
            if existing[1] is not None:
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: one family symbol cannot carry an "
                    "undeclared concrete body"
                )
            # A declaration without a local implementation publishes exactly one
            # `ac.module.import`, which carries no concrete case. Each placement
            # keeps its own ordered static arguments, so only the first
            # registration owns the import.
            imported_definition, _, _ = existing
            return (
                symbol,
                frozen,
                imported_definition.inputs,
                imported_definition.outputs,
            )
        if symbol not in module_bodies:
            namespace = ""
            program: QueueProgram | None = None
            specialized_payloads = payload_map

            def resolve_child_module(
                child_name: str,
                child_call: ast.Call,
            ) -> tuple[
                str,
                tuple[tuple[str, StaticValue], ...],
                tuple[tuple[str, ValueType], ...],
                tuple[tuple[str, ValueType], ...],
            ]:
                if child_name in rule_modules:
                    return specialize_rule_module(
                        child_name,
                        child_call,
                        context_static_values=active_static_values,
                        context_static_types=active_static_types,
                    )
                if child_name in empty_modules:
                    if child_call.keywords:
                        raise QueueFrontendError(
                            "ACPY-MODULE-007: pure module static parameters "
                            "are not implemented"
                        )
                    return child_name, (), (), ()
                if child_name in module_types:
                    inputs, outputs = module_signature(child_name)
                    if child_call.keywords:
                        raise QueueFrontendError(
                            "ACPY-MODULE-007: pure module static parameters "
                            "are not implemented"
                        )
                    return child_name, (), inputs, outputs
                raise QueueFrontendError(
                    f"ACPY-MODULE-011: child module {child_name!r} is not callable"
                )

            if (
                module_name in module_implementations
                and module_name not in composite_modules
            ):
                materialized_source = materialized_family_case_source(
                    module_name, frozen
                )
                try:
                    program = parse_queue_program(
                        text if materialized_source is None else materialized_source,
                        module_name,
                        static_arguments=(
                            {} if module_name in module_family_schemas else dict(frozen)
                        ),
                        entry_kind="module",
                        source_path=normalized_source_path,
                        static_type_namespace=namespace,
                        definition_locations=definition_locations,
                        static_assert_locations=static_assert_locations,
                        source_node_locations=(
                            source_node_locations
                            if materialized_source is None
                            else None
                        ),
                        resolve_child_module=resolve_child_module,
                    )
                except QueueFrontendError as error:
                    raise QueueFrontendError(
                        f"ACPY-MODULE-002: rule-backed module {module_name!r} "
                        f"could not specialize: {error}"
                    ) from error
                specialized_payloads = {
                    **payload_map,
                    **{item.name: item for item in program.payloads},
                }
            specialized_values = {
                **_type_static_values(tree, dict(frozen)),
                **dict(frozen),
            }
            inputs = tuple(
                (
                    name,
                    _payload(
                        annotation,
                        specialized_payloads,
                        enum_map,
                        specialized_values,
                    ),
                )
                for name, annotation in template.input_annotations
            )
            outputs = tuple(
                (
                    name,
                    _payload(
                        annotation,
                        specialized_payloads,
                        enum_map,
                        specialized_values,
                    ),
                )
                for name, annotation in zip(
                    template.output_names,
                    template.output_annotations,
                    strict=True,
                )
            )
            definition = RuleModuleDefinition(
                inputs,
                outputs,
                template.static_parameters,
                template.static_defaults,
            )
            module_bodies[symbol] = (
                definition,
                program,
                frozen,
            )
            if module_name in composite_modules:
                composite_functions[symbol] = (
                    composite_modules[module_name],
                    frozen,
                )
            if module_name in module_declarations:
                declaration_sources[symbol] = (
                    module_declarations[module_name]
                )
        definition, _, _ = module_bodies[symbol]
        return symbol, frozen, definition.inputs, definition.outputs

    @dataclass(frozen=True, slots=True)
    class CompositeInstance:
        results: tuple[str, ...]
        module_name: str
        sources: tuple[str, ...]
        output_types: tuple[ValueType, ...]
        static_arguments: tuple[tuple[str, StaticValue], ...]
        source: SourceFrame | None

    @dataclass(slots=True)
    class CompositePlan:
        inputs: tuple[tuple[str, ValueType], ...]
        outputs: tuple[tuple[str, ValueType], ...]
        values: dict[str, ValueType]
        uses: dict[str, int]
        instances: list[CompositeInstance]
        returned_names: tuple[str, ...]

    composite_plans: dict[str, CompositePlan] = {}

    def parse_composite_plan(
        symbol: str,
        function: ast.FunctionDef,
        frozen: tuple[tuple[str, StaticValue], ...],
    ) -> CompositePlan:
        definition, _, _ = module_bodies[symbol]
        active_static_values: Mapping[str, StaticValue] = {
            **module_static_values,
            **dict(frozen),
        }
        active_static_types = dict(
            rule_modules[function.name].static_parameter_types
        )
        specialized_function = copy.deepcopy(function)
        _strip_static_assertions(
            specialized_function,
            active_static_values,
            normalized_source_path,
            definition_locations,
            static_assert_locations,
        )

        def specialize_statements(statements: list[ast.stmt]) -> list[ast.stmt]:
            specialized: list[ast.stmt] = []
            for statement in statements:
                if not isinstance(statement, ast.If):
                    specialized.append(statement)
                    continue
                try:
                    condition = evaluate_static(
                        statement.test,
                        StaticEnvironment(active_static_values),
                    )
                except ValueError as error:
                    raise QueueFrontendError(
                        "ACPY-MODULE-010: composite control flow must depend "
                        "only on ac.const values"
                    ) from error
                if type(condition) is not bool:
                    raise QueueFrontendError(
                        "ACPY-MODULE-010: composite static condition must be bool"
                    )
                specialized.extend(
                    specialize_statements(
                        statement.body if condition else statement.orelse
                    )
                )
            return specialized

        statements = specialize_statements(specialized_function.body)
        normalized: list[ast.stmt] = []
        for statement in statements:
            if (
                isinstance(statement, ast.Return)
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Name)
                and statement.value.func.id in modules
            ):
                module_name = statement.value.func.id
                if module_name in rule_modules:
                    _, _, _, child_outputs = specialize_rule_module(
                        module_name,
                        statement.value,
                        context_static_values=active_static_values,
                        context_static_types=active_static_types,
                    )
                else:
                    _, child_outputs = module_signature(module_name)
                names = tuple(
                    name for name, _ in definition.outputs
                )
                if len(names) != len(child_outputs):
                    names = tuple(
                        f"compiler_return_{index}"
                        for index in range(len(child_outputs))
                    )
                target: ast.expr = (
                    ast.Name(id=names[0], ctx=ast.Store())
                    if len(names) == 1
                    else ast.Tuple(
                        elts=[
                            ast.Name(id=name, ctx=ast.Store()) for name in names
                        ],
                        ctx=ast.Store(),
                    )
                )
                returned: ast.expr = (
                    ast.Name(id=names[0], ctx=ast.Load())
                    if len(names) == 1
                    else ast.Tuple(
                        elts=[ast.Name(id=name, ctx=ast.Load()) for name in names],
                        ctx=ast.Load(),
                    )
                )
                normalized.extend(
                    [
                        ast.Assign(targets=[target], value=statement.value),
                        ast.Return(value=returned),
                    ]
                )
                continue
            normalized.append(statement)

        values = dict(definition.inputs)
        uses = {name: 0 for name, _ in definition.inputs}
        instances: list[CompositeInstance] = []
        returned_names: tuple[str, ...] | None = None

        def resolve_child(call: ast.Call) -> tuple[
            str,
            tuple[tuple[str, StaticValue], ...],
            tuple[tuple[str, ValueType], ...],
            tuple[tuple[str, ValueType], ...],
        ]:
            assert isinstance(call.func, ast.Name)
            module_name = call.func.id
            if module_name in rule_modules:
                return specialize_rule_module(
                    module_name,
                    call,
                    context_static_values=active_static_values,
                    context_static_types=active_static_types,
                )
            inputs, outputs = module_signature(module_name)
            if call.keywords:
                raise QueueFrontendError(
                    "ACPY-MODULE-007: pure module static parameters are not implemented"
                )
            return module_name, (), inputs, outputs

        def append_instance(
            call: ast.Call,
            results: tuple[str, ...],
        ) -> None:
            child, static_arguments, input_signature, output_signature = (
                resolve_child(call)
            )
            if len(call.args) != len(input_signature) or len(results) != len(
                output_signature
            ):
                raise QueueFrontendError(
                    "ACPY-MODULE-010: composite child call arity does not "
                    "match its declaration"
                )
            if len(set(results)) != len(results):
                raise QueueFrontendError(
                    "ACPY-MODULE-010: composite results require unique names"
                )
            if any(result in values for result in results):
                raise QueueFrontendError(
                    "ACPY-MODULE-010: composite results require fresh names"
                )
            sources: list[str] = []
            for argument, (_, expected_type) in zip(
                call.args, input_signature, strict=True
            ):
                if not isinstance(argument, ast.Name) or argument.id not in values:
                    raise QueueFrontendError(
                        "ACPY-MODULE-010: composite child inputs must be named "
                        "parent or prior-child Queue values"
                    )
                actual_type = values[argument.id]
                if not _types_compatible(actual_type, expected_type):
                    raise QueueFrontendError(
                        "ACPY-MODULE-010: composite child input type mismatch"
                    )
                uses[argument.id] += 1
                sources.append(argument.id)
            output_types = tuple(payload for _, payload in output_signature)
            for result, output_type in zip(results, output_types, strict=True):
                values[result] = output_type
                uses[result] = 0
            instances.append(
                CompositeInstance(
                    results,
                    child,
                    tuple(sources),
                    output_types,
                    static_arguments,
                    source_frame(call),
                )
            )

        for statement in normalized:
            if (
                isinstance(statement, ast.Expr)
                and isinstance(statement.value, ast.Constant)
                and isinstance(statement.value.value, str)
            ):
                continue
            if (
                isinstance(statement, ast.Expr)
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Name)
                and statement.value.func.id in modules
            ):
                append_instance(statement.value, ())
                continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], (ast.Name, ast.Tuple, ast.List))
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Name)
                and statement.value.func.id in modules
            ):
                target = statement.targets[0]
                results = (
                    (target.id,)
                    if isinstance(target, ast.Name)
                    else tuple(
                        item.id for item in target.elts if isinstance(item, ast.Name)
                    )
                )
                if not results or (
                    not isinstance(target, ast.Name)
                    and len(results) != len(target.elts)
                ):
                    raise QueueFrontendError(
                        "ACPY-MODULE-010: composite results require named values"
                    )
                append_instance(statement.value, results)
                continue
            if isinstance(statement, ast.Return):
                if statement.value is None or (
                    isinstance(statement.value, ast.Constant)
                    and statement.value.value is None
                ):
                    returned_names = ()
                    continue
                returned = (
                    tuple(statement.value.elts)
                    if isinstance(statement.value, (ast.Tuple, ast.List))
                    else (statement.value,)
                )
                if not all(isinstance(item, ast.Name) for item in returned):
                    raise QueueFrontendError(
                        "ACPY-MODULE-010: composite returns require named Queue values"
                    )
                returned_names = tuple(
                    item.id for item in returned if isinstance(item, ast.Name)
                )
                for returned_name in returned_names:
                    if returned_name not in values:
                        raise QueueFrontendError(
                            "ACPY-MODULE-010: composite returned value is undefined"
                        )
                    uses[returned_name] += 1
                continue
            raise QueueFrontendError(
                "ACPY-MODULE-010: unsupported composite module statement "
                f"{type(statement).__name__} at line "
                f"{getattr(statement, 'lineno', 0)}"
            )
        if returned_names is None and not definition.outputs:
            returned_names = ()
        if (
            returned_names is None
            or len(returned_names) != len(definition.outputs)
            or any(
                not _types_compatible(values[name], expected)
                for name, (_, expected) in zip(
                    returned_names, definition.outputs, strict=True
                )
            )
        ):
            raise QueueFrontendError(
                "ACPY-MODULE-010: composite return type or arity mismatch"
            )
        unused = sorted(name for name, count in uses.items() if count < 1)
        if unused:
            raise QueueFrontendError(
                "ACPY-MODULE-010: every composite Queue value requires a "
                f"consumer: {unused[0]!r}"
            )
        return CompositePlan(
            definition.inputs,
            definition.outputs,
            values,
            uses,
            instances,
            returned_names,
        )

    for children in empty_module_children.values():
        for child, _ in children:
            specialize_rule_module(
                child,
                ast.Call(
                    func=ast.Name(id=child, ctx=ast.Load()),
                    args=[],
                    keywords=[],
                ),
            )

    specialized_statements = specialize_system_statements(function.body)
    normalized_statements: list[ast.stmt] = []
    for statement in specialized_statements:
        if (
            isinstance(statement, ast.Return)
            and isinstance(statement.value, ast.Call)
            and isinstance(statement.value.func, ast.Name)
            and statement.value.func.id in modules
        ):
            if statement.value.func.id in rule_modules:
                _, _, _, outputs = specialize_rule_module(
                    statement.value.func.id, statement.value
                )
            else:
                _, outputs = module_signature(statement.value.func.id)
            names = tuple(f"compiler_return_{index}" for index in range(len(outputs)))
            target: ast.expr = (
                ast.Name(id=names[0], ctx=ast.Store())
                if len(names) == 1
                else ast.Tuple(
                    elts=[ast.Name(id=name, ctx=ast.Store()) for name in names],
                    ctx=ast.Store(),
                )
            )
            returned: ast.expr = (
                ast.Name(id=names[0], ctx=ast.Load())
                if len(names) == 1
                else ast.Tuple(
                    elts=[ast.Name(id=name, ctx=ast.Load()) for name in names],
                    ctx=ast.Load(),
                )
            )
            normalized_statements.extend(
                [
                    ast.Assign(targets=[target], value=statement.value),
                    ast.Return(returned),
                ]
            )
            continue
        normalized_statements.append(statement)

    for statement in normalized_statements:
        if (
            isinstance(statement, ast.Expr)
            and isinstance(statement.value, ast.Constant)
            and isinstance(statement.value.value, str)
        ):
            continue
        if (
            isinstance(statement, ast.Expr)
            and isinstance(statement.value, ast.Call)
            and isinstance(statement.value.func, ast.Name)
            and statement.value.func.id in modules
        ):
            module_name = statement.value.func.id
            instance_static_arguments: tuple[tuple[str, StaticValue], ...] = ()
            instance_module_name = module_name
            if module_name in rule_modules:
                (
                    instance_module_name,
                    instance_static_arguments,
                    input_signature,
                    output_signature,
                ) = specialize_rule_module(module_name, statement.value)
            else:
                input_signature, output_signature = module_signature(module_name)
            input_queue_shapes, output_queue_shapes = selected_module_queue_shapes(
                module_name, instance_static_arguments
            )
            if statement.value.args or input_signature or output_signature:
                raise QueueFrontendError(
                    "ACPY-MODULE-002: expression module calls require a "
                    "zero-input zero-output signature"
                )
            if module_name not in rule_modules and statement.value.keywords:
                raise QueueFrontendError(
                    "ACPY-MODULE-007: pure module static parameters are not implemented"
                )
            instances.append(
                (
                    (),
                    instance_module_name,
                    (),
                    (),
                    instance_static_arguments,
                    source_frame(statement.value),
                )
            )
            operation_order.append(("instance", len(instances) - 1))
            continue
        if (
            isinstance(statement, ast.Assign)
            and len(statement.targets) == 1
            and isinstance(statement.targets[0], (ast.Name, ast.Tuple, ast.List))
            and isinstance(statement.value, ast.Call)
            and isinstance(statement.value.func, ast.Name)
            and statement.value.func.id in modules
        ):
            target = statement.targets[0]
            results = (
                (target.id,)
                if isinstance(target, ast.Name)
                else tuple(
                    item.id for item in target.elts if isinstance(item, ast.Name)
                )
            )
            if not results or (
                not isinstance(target, ast.Name) and len(results) != len(target.elts)
            ):
                raise QueueFrontendError(
                    "ACPY-MODULE-002: module results require fresh tuple names"
                )
            module_name = statement.value.func.id
            instance_static_arguments: tuple[tuple[str, StaticValue], ...] = ()
            instance_module_name = module_name
            if module_name in rule_modules:
                (
                    instance_module_name,
                    instance_static_arguments,
                    input_signature,
                    output_signature,
                ) = specialize_rule_module(module_name, statement.value)
            else:
                input_signature, output_signature = module_signature(module_name)
            input_queue_shapes, output_queue_shapes = selected_module_queue_shapes(
                module_name, instance_static_arguments
            )
            if len(statement.value.args) != len(input_signature) or len(results) != len(
                output_signature
            ):
                raise QueueFrontendError(
                    "ACPY-MODULE-002: module call arity does not match its signature"
                )
            if any(result in values for result in results):
                raise QueueFrontendError(
                    "ACPY-MODULE-002: module call values must be defined once"
                )
            sources: list[str] = []
            for argument_index, (argument, (_, expected_type), expected_shape) in enumerate(
                zip(
                    statement.value.args,
                    input_signature,
                    input_queue_shapes,
                    strict=True,
                )
            ):
                root = argument
                while isinstance(root, ast.Attribute):
                    root = root.value
                if not isinstance(root, ast.Name):
                    raise QueueFrontendError(
                        "ACPY-MODULE-002: module inputs require named values or "
                        f"field projections at line {getattr(argument, 'lineno', 0)}: "
                        f"{ast.unparse(argument)}"
                    )
                if root.id not in values:
                    raise QueueFrontendError(
                        "ACPY-MODULE-002: module call values must be defined once"
                    )
                if isinstance(argument, ast.Name):
                    source = argument.id
                    actual_type = values[source]
                else:
                    emitter = _ExpressionEmitter(
                        payload_map,
                        root.id,
                        values[root.id],
                        enum_types=enum_map,
                        bitfields=bitfield_map,
                        invariants=invariants,
                        helpers=helpers,
                        inline_explicit_helpers=True,
                    )
                    _, actual_type = emitter.emit(argument, expected_type)
                    root_type = values[root.id]
                    if not isinstance(root_type, StructType):
                        raise QueueFrontendError(
                            "ACPY-MODULE-002: module field projection requires "
                            f"a struct root at line {getattr(argument, 'lineno', 0)}"
                        )
                    fields: list[str] = []
                    field_cursor: ast.expr = argument
                    while isinstance(field_cursor, ast.Attribute):
                        fields.append(field_cursor.attr)
                        field_cursor = field_cursor.value
                    fields.reverse()
                    projection_definition = (
                        f"compiler_project_{root_type.name}_{'_'.join(fields)}"
                    )
                    projection_static_arguments = tuple(
                        root_type.resolved_static_bindings
                    )
                    readable_projection_parameters = "".join(
                        f"compiler_{name}_{'neg_' if value < 0 else ''}{abs(value)}"
                        for name, value in projection_static_arguments
                    )
                    projection_module = (
                        projection_definition + readable_projection_parameters
                    )
                    projection_expression: ast.expr = ast.copy_location(
                        ast.Name(id="value", ctx=ast.Load()), argument
                    )
                    for field in fields:
                        projection_expression = ast.copy_location(
                            ast.Attribute(
                                value=projection_expression,
                                attr=field,
                                ctx=ast.Load(),
                            ),
                            argument,
                        )
                    projection_expression = ast.fix_missing_locations(
                        projection_expression
                    )
                    definition = (
                        "value",
                        projection_definition,
                        projection_static_arguments,
                        root_type,
                        projection_expression,
                        actual_type,
                        source_frame(argument),
                    )
                    existing_projection = projection_definitions.get(projection_module)
                    if existing_projection is not None and (
                        existing_projection[1] != definition[1]
                        or existing_projection[2] != definition[2]
                        or existing_projection[3] != definition[3]
                        or ast.dump(existing_projection[4]) != ast.dump(definition[4])
                        or existing_projection[5] != definition[5]
                    ):
                        raise QueueFrontendError(
                            "ACPY-MODULE-002: readable projection module name "
                            f"collision for {projection_module!r}"
                        )
                    projection_definitions.setdefault(projection_module, definition)
                    projection_name = (
                        f"compiler_projection_{len(projections)}_{root.id}_{argument_index}"
                    )
                    while projection_name in values:
                        projection_name += "_"
                    values[projection_name] = actual_type
                    value_queue_shapes[projection_name] = value_queue_shapes.get(
                        root.id, (1, 1)
                    )
                    uses[projection_name] = 0
                    projections.append(
                        (
                            projection_name,
                            root.id,
                            projection_module,
                            projection_static_arguments,
                            argument,
                            actual_type,
                            source_frame(argument),
                        )
                    )
                    operation_order.append(("projection", len(projections) - 1))
                    uses[root.id] = uses.get(root.id, 0) + 1
                    source = projection_name
                if not _types_compatible(actual_type, expected_type):
                    raise QueueFrontendError(
                        "ACPY-MODULE-002: module input payload type mismatch at "
                        f"line {getattr(argument, 'lineno', 0)}: "
                        f"{ast.unparse(argument)}"
                    )
                actual_shape = value_queue_shapes.get(source, (1, 1))
                if actual_shape != expected_shape:
                    raise QueueFrontendError(
                        "ACPY-MODULE-002: module input Queue shape mismatch at "
                        f"line {getattr(argument, 'lineno', 0)}"
                    )
                uses[source] = uses.get(source, 0) + 1
                sources.append(source)
            output_types = tuple(payload for _, payload in output_signature)
            for result, output_type, output_shape in zip(
                results, output_types, output_queue_shapes, strict=True
            ):
                values[result] = output_type
                value_queue_shapes[result] = output_shape
                uses[result] = 0
            if module_name not in rule_modules and statement.value.keywords:
                raise QueueFrontendError(
                    "ACPY-MODULE-007: pure module static parameters are not implemented"
                )
            instances.append(
                (
                    results,
                    instance_module_name,
                    tuple(sources),
                    output_types,
                    instance_static_arguments,
                    source_frame(statement.value),
                )
            )
            operation_order.append(("instance", len(instances) - 1))
            continue
        if isinstance(statement, ast.Return) and statement.value is None:
            returned_names = ()
            continue
        if isinstance(statement, ast.Return) and statement.value is not None:
            if isinstance(statement.value, ast.Constant) and statement.value.value is None:
                returned_names = ()
                continue
            returned = (
                tuple(statement.value.elts)
                if isinstance(statement.value, (ast.Tuple, ast.List))
                else (statement.value,)
            )
            if not all(isinstance(value, ast.Name) for value in returned):
                raise QueueFrontendError(
                    "ACPY-MODULE-002: module system returns require named values"
                )
            returned_names = tuple(value.id for value in returned)
            for name in returned_names:
                if name not in values:
                    raise QueueFrontendError(
                        "ACPY-MODULE-002: returned module value is undefined"
                    )
                uses[name] = uses.get(name, 0) + 1
            continue
        raise QueueFrontendError(
            f"ACPY-MODULE-002: unsupported module system statement "
            f"{type(statement).__name__} at line "
            f"{getattr(statement, 'lineno', 0)}: {ast.unparse(statement)}"
        )
    if returned_names is None and not expected_results:
        returned_names = ()
    if (
        returned_names is None
        or len(returned_names) != len(expected_results)
        or any(
            not _types_compatible(values[name], expected)
            for name, expected in zip(returned_names, expected_results, strict=True)
        )
        or any(
            value_queue_shapes.get(name, (1, 1)) != expected
            for name, expected in zip(
                returned_names, result_queue_shapes, strict=True
            )
        )
    ):
        raise QueueFrontendError(
            "ACPY-MODULE-002: module system return type or arity mismatch"
        )
    if any(count < 1 for count in uses.values()):
        raise QueueFrontendError(
            "ACPY-MODULE-002: every module Queue value requires a consumer"
        )

    pending_composites = list(composite_functions)
    composite_cursor = 0
    while composite_cursor < len(pending_composites):
        symbol = pending_composites[composite_cursor]
        composite_cursor += 1
        if symbol in composite_plans:
            continue
        composite_function, composite_frozen = (
            composite_functions[symbol]
        )
        composite_plans[symbol] = parse_composite_plan(
            symbol, composite_function, composite_frozen
        )
        for candidate in composite_functions:
            if candidate not in pending_composites:
                pending_composites.append(candidate)

    all_payloads_by_symbol: dict[str, Payload] = {
        payload.descriptor.symbol: payload for payload in payloads
    }
    top_static_checks = (
        *(check for payload in payloads for check in payload.resolved_type_checks),
        *system_interface_checks,
    )
    candidate_static_bindings = dict(
        _resolved_type_bindings_for_checks(
            top_static_checks,
            parameter_aliases,
            type_static_values,
        )
    )
    top_static_configs = _resolved_config_values_for_checks(
        tree,
        top_static_checks,
        parameter_aliases,
        type_static_values,
    )
    _validate_static_config_roots(
        function,
        parameter_aliases,
        {
            token[6:]
            for check in top_static_checks
            for token in check.program
            if token[:6] == "param:"
        },
    )
    candidate_static_configs = {binding.root: binding for binding in top_static_configs}
    all_interface_checks = list(system_interface_checks)
    for _, program, _ in module_bodies.values():
        if program is None:
            continue
        for payload in program.payloads:
            existing = all_payloads_by_symbol.get(payload.descriptor.symbol)
            if existing is not None and existing.descriptor != payload.descriptor:
                raise QueueFrontendError(
                    "ACPY-TYPE-008: specialized struct symbol collision"
                )
            if existing is None:
                all_payloads_by_symbol[payload.descriptor.symbol] = payload
        for name, value in program.resolved_type_bindings:
            if (
                name in candidate_static_bindings
                and candidate_static_bindings[name] != value
            ):
                raise QueueFrontendError(
                    "ACPY-TYPE-008: specialized static binding collision"
                )
            candidate_static_bindings[name] = value
        for binding in program.resolved_config_values:
            existing_config = candidate_static_configs.get(binding.root)
            if existing_config is not None and existing_config != binding:
                raise QueueFrontendError(
                    "ACPY-TYPE-008: specialized static config binding collision"
                )
            candidate_static_configs[binding.root] = binding
        all_interface_checks.extend(program.resolved_type_checks)
    all_payloads = tuple(all_payloads_by_symbol.values())
    candidate_checks = [
        *(check for payload in all_payloads for check in payload.resolved_type_checks),
        *all_interface_checks,
    ]
    unique_checks: dict[str, StaticTypeCheck] = {}
    for check in candidate_checks:
        existing_check = unique_checks.get(check.target)
        if existing_check is None:
            unique_checks[check.target] = check
            continue
        if (
            existing_check.result != check.result
            or existing_check.concrete_type != check.concrete_type
        ):
            raise QueueFrontendError(
                "ACPY-TYPE-008: static type check target collision for "
                f"{check.target!r}"
            )
    def family_owner(source: str) -> str:
        normalized = source or "generated/module.py"
        quoted = canonical_mlir_string(normalized)
        return f"#ac.source_owner<{quoted}, {quoted}>"

    def concrete_family_interface(
        inputs: tuple[tuple[str, ValueType], ...],
        outputs: tuple[tuple[str, ValueType], ...],
        provenance: str,
    ) -> str:
        """Interface one resolved module signature materializes to.

        ``ac.module.case`` verification materializes the family interface
        against the concrete case signature, so a module emitted for one
        resolved signature needs exactly one port per input and per result.
        The declared interface of a same-named family already provides that;
        this is the fallback for a symbol whose declaration is registered under
        a different name, or for a module that has no declaration entry at all.
        """
        ports: list[str] = []
        for direction, signature in (("input", inputs), ("output", outputs)):
            for name, payload in signature:
                queue_type = f"!ac.queue<{_render_type(payload)}>"
                ports.append(
                    "#ac.interface_port<"
                    f"{canonical_mlir_string(name)}, \"{direction}\", "
                    f"#ac.type_expr<#ac.type_expr_concrete<{queue_type}>>, "
                    f"{provenance}>"
                )
        return "#ac.module_interface<[" + ", ".join(ports) + "]>"

    def family_schema(
        name: str,
        source: str,
        fallback_interface: str | None = None,
        fallback_nominals: tuple[str, ...] = (),
    ) -> str:
        parameters, cases = module_family_schemas.get(
            name,
            ("#ac.static_parameters<[]>", "#ac.static_cases<[#ac.static_arguments<[]>]>")
        )
        interface = module_family_interfaces.get(name)
        if interface is None:
            interface = (
                fallback_interface
                if fallback_interface is not None
                else "#ac.module_interface<[]>"
            )
        nominals = list(module_family_nominals.get(name, ()))
        nominals.extend(item for item in fallback_nominals if item not in nominals)
        return (
            f"#ac.module_family_schema<{parameters}, {cases}, "
            f"{interface}, "
            f"{family_owner(source)}, ["
            + ", ".join(f"@{nominal}" for nominal in nominals)
            + "]>"
        )

    def family_arguments(
        name: str, values: tuple[tuple[str, StaticValue], ...]
    ) -> str:
        specs = module_family_parameter_specs.get(name, [])
        if not specs:
            return "#ac.static_arguments<[]>"
        supplied = dict(values)
        arguments: list[str] = []
        for parameter in specs:
            parameter_name = str(parameter["name"])
            render_value = parameter["render"]
            assert callable(render_value)
            arguments.append(
                f"#ac.static_argument<{canonical_mlir_string(parameter_name)}, "
                f"{render_value(supplied[parameter_name])}>"
            )
        return "#ac.static_arguments<[" + ", ".join(arguments) + "]>"

    def family_provenance(source: str, line: int = 1, column: int = 1) -> str:
        normalized = source or "generated/module.py"
        return (
            "#ac.source_provenance<"
            f"{canonical_mlir_string(normalized)}, {max(line, 1)}, "
            f"{max(column, 1)}, {max(line, 1)}, {max(column, 1)}>"
        )

    lines = [
        "builtin.module attributes {"
        'ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle"'
        + "} {"
    ]
    if all_payloads or enum_bindings or bitfield_bindings:
        lines.append("  ac.type_scope @types {")
        for enumeration in enum_bindings:
            rendered = _render_enum(enumeration, "    ")
            location = definition_sources.get(enumeration.name)
            if location is not None:
                rendered += (
                    " {ac.source_file = "
                    + canonical_mlir_string(location[0])
                    + "}"
                )
            lines.append(rendered)
        for payload in all_payloads:
            fields = ", ".join(
                f'{{name = "{field}", type = {typ}}}' for field, typ in payload.fields
            )
            rendered = (
                f"    ac.struct @{payload.descriptor.symbol} fields [{fields}]"
            )
            location = definition_sources.get(payload.name)
            if location is not None:
                rendered += (
                    " {ac.source_file = "
                    + canonical_mlir_string(location[0])
                    + "}"
                )
            lines.append(rendered)
        for bitfield in bitfield_bindings:
            lines.append(_render_bitfield(bitfield, "    "))
        layouts = [
            *(_enum_layout_entry(enumeration) for enumeration in enum_bindings),
            *(_payload_layout_entry(payload) for payload in all_payloads),
        ]
        if layouts:
            lines.append(
                "  } {dlti.dl_spec = #dlti.dl_spec<" + ", ".join(layouts) + ">}"
            )
        else:
            lines.append("  }")
    emitted_helpers = {helper.name: helper for helper in helper_definitions}
    for helper in helper_definitions:
        arguments = ", ".join(
            f"%{name}: !ac.var<{_render_type(value_type)}>"
            for name, value_type in helper.arguments
        )
        attribute_fields = []
        if helper.inline:
            attribute_fields.append("ac.inline = true")
        if helper.source is not None:
            attribute_fields.append(
                "ac.source_file = " + canonical_mlir_string(helper.source.file)
            )
        attributes = (
            " attributes {" + ", ".join(attribute_fields) + "}"
            if attribute_fields
            else ""
        )
        lines.append(
            f"  func.func private @{helper.name}({arguments}) -> "
            f"!ac.var<{_render_type(helper.result)}>{attributes} {{"
        )
        helper_emitter = _ExpressionEmitter(
            payload_map,
            "",
            helper.result,
            root_values={
                name: (name, value_type) for name, value_type in helper.arguments
            },
            enum_types=enum_map,
            bitfields=bitfield_map,
            invariants=invariants,
            helpers=helpers,
            strict_descriptors=True,
        )
        result, result_type = helper_emitter.emit(helper.expression, helper.result)
        if result_type != helper.result:
            raise QueueFrontendError(
                f"ACPY-HELPER-002: helper {helper.name!r} result type does not match"
            )
        lines.extend("  " + line for line in helper_emitter.lines)
        lines.append(
            f"    func.return %{result} : !ac.var<{_render_type(helper.result)}>"
        )
        lines.append("  }" + _render_source_frame_location(helper.source))
    lines.append(
        f'  ac.system @{system} root @Top as "root" tick 0 "cycle" '
        'seed {kind = "fixed", value = 0 : i64} instrumentation [] '
        'results {id = "default", format = "json"} selected true'
    )

    def family_module_open(
        name: str,
        arguments: tuple[tuple[str, ValueType], ...],
        results: tuple[ValueType, ...],
        metadata: str,
    ) -> list[str]:
        frame = definition_sources.get(name, (normalized_source_path, 1, 1))
        source = frame[0]
        argument_declarations = ", ".join(
            f"%{argument}: !ac.queue<{_render_type(value_type)}>"
            for argument, value_type in arguments
        )
        input_types = ", ".join(
            f"!ac.queue<{_render_type(value_type)}>" for _, value_type in arguments
        )
        result_types = ", ".join(
            f"!ac.queue<{_render_type(value_type)}>" for value_type in results
        )
        result_type = (
            "()" if not results else result_types if len(results) == 1 else f"({result_types})"
        )
        result_ports = tuple(
            ("result" if len(results) == 1 else f"result{index}", value_type)
            for index, value_type in enumerate(results)
        )
        interface = concrete_family_interface(
            arguments, result_ports, family_provenance(*frame)
        )
        nominals = _nominal_declarations(
            [payload for _, payload in arguments]
            + [payload for _, payload in result_ports]
        )
        return [
            f"  ac.module @{name} source {family_owner(source)} "
            f"schema {family_schema(name, source, interface, nominals)} {{",
            f"    ac.module.case arguments #ac.static_arguments<[]> type ({input_types}) -> {result_type}"
            + metadata.removeprefix(" attributes")
            + f" source {family_provenance(*frame)} graph {{",
            f"    ^bb0({argument_declarations}):" if arguments else "    ^bb0:",
        ]

    def family_module_close() -> list[str]:
        return ["    }", "  }"]

    for name in sorted(empty_modules):
        lines.extend(family_module_open(
            name, (), (), _render_interface_display_attributes((), (), module_metadata(name))
        ))
        for index, (child, child_source) in enumerate(
            empty_module_children.get(name, ())
        ):
            instance = f"{child}_{index}"
            lines.append(
                f"    ac.instance @{instance} of @{child}() static #ac.static_arguments<[]> "
                f'id "{instance}" path "{instance}" : () -> ()'
                + _render_source_frame_location(child_source)
            )
        lines.extend(["    ac.return", *family_module_close()])
    for name, definition in module_types.items():
        argument = definition.argument
        input_type = definition.input_type
        output_type = definition.output_type
        expression = definition.expression
        if (
            not definition.states
            and isinstance(expression, ast.Call)
            and isinstance(expression.func, ast.Name)
            and expression.func.id in module_declarations
        ):
            if (
                len(expression.args) != 1
                or not isinstance(expression.args[0], ast.Name)
                or expression.args[0].id != argument
            ):
                raise QueueFrontendError(
                    "ACPY-MODULE-003: imported nested module call requires the "
                    "module parameter as its sole runtime argument"
                )
            child, static_arguments, inputs, outputs = specialize_rule_module(
                expression.func.id, expression
            )
            if (
                len(inputs) != 1
                or len(outputs) != 1
                or not _types_compatible(inputs[0][1], input_type)
                or not _types_compatible(outputs[0][1], output_type)
            ):
                raise QueueFrontendError(
                    "ACPY-MODULE-003: imported nested module call signature mismatch"
                )
            lines.extend(
                [
                    *family_module_open(
                        name,
                        (("input", input_type),),
                        (output_type,),
                        _render_interface_display_attributes(
                            (argument,), ("result",), module_metadata(name)
                        ),
                    ),
                    f"    %output = ac.instance @result of @{child}(%input) "
                    f"static {family_arguments(child, static_arguments)} "
                    'id "result" path "result" '
                    f": (!ac.queue<{_render_type(input_type)}>) -> "
                    f"!ac.queue<{_render_type(output_type)}>"
                    + _render_source_frame_location(source_frame(expression)),
                    f"    ac.return %output : !ac.queue<{_render_type(output_type)}>",
                    *family_module_close(),
                ]
            )
            continue
        if (
            not definition.states
            and isinstance(expression, ast.Call)
            and isinstance(expression.func, ast.Name)
            and expression.func.id in module_types
        ):
            if (
                len(expression.args) != 1
                or expression.keywords
                or not isinstance(expression.args[0], ast.Name)
                or expression.args[0].id != argument
            ):
                raise QueueFrontendError(
                    "ACPY-MODULE-003: nested module call requires the module "
                    "parameter as its sole argument"
                )
            child = expression.func.id
            child_definition = module_types[child]
            child_input = child_definition.input_type
            child_output = child_definition.output_type
            if not _types_compatible(
                child_input, input_type
            ) or not _types_compatible(child_output, output_type):
                raise QueueFrontendError(
                    "ACPY-MODULE-003: nested module call signature mismatch"
                )
            lines.extend(
                [
                    *family_module_open(
                        name, (("input", input_type),), (output_type,),
                        _render_interface_display_attributes(
                            (argument,), ("result",), module_metadata(name)
                        ),
                    ),
                    "    %output = ac.scope @body(%input) {",
                    f"    ^bb0(%borrowed: !ac.queue<{_render_type(input_type)}>):",
                    f"    %output = ac.instance @result of @{child}(%input) "
                    'static #ac.static_arguments<[]> id "result" path "result" '
                    f": (!ac.queue<{_render_type(input_type)}>) -> "
                    f"!ac.queue<{_render_type(output_type)}>"
                    + _render_source_frame_location(source_frame(expression)),
                    f"    ac.return %output : !ac.queue<{_render_type(output_type)}>",
                    *family_module_close(),
                ]
            )
            continue
        if definition.states:
            state_types = {state.name: state.value_type for state in definition.states}
            root_values = {
                state.name: (
                    "old" if index == 0 else f"old_{index}",
                    state.value_type,
                )
                for index, state in enumerate(definition.states)
            }
            emitter = _ExpressionEmitter(
                payload_map,
                argument,
                input_type,
                root_values=root_values,
                enum_types=enum_map,
                bitfields=bitfield_map,
                invariants=invariants,
                helpers=helpers,
                inline_explicit_helpers=True,
            )
            lines.extend(
                [
                    *family_module_open(
                        name, (("input", input_type),), (output_type,),
                        _render_interface_display_attributes(
                            (argument,), ("result",), module_metadata(name)
                        ),
                    ),
                ]
            )
            for state in definition.states:
                init_type = (
                    f"i{state.value_type.width}"
                    if isinstance(state.value_type, RangeType)
                    else _render_type(state.value_type)
                )
                lines.append(
                    f"      ac.var.decl @{state.name} "
                    f"type {_render_type(state.value_type)} "
                    f'init {state.init} : {init_type} owner "/body" '
                    "stable_id "
                    f'"var/body/{state.name}"'
                )
            lines.extend(
                [
                    "      %next = ac.rule %borrowed depths [1] latencies [1] ",
                    f'          name "{name}" stable_id "{name}_0" domain "cycle" ',
                    "          type exact {",
                    f"      ^body(%item: !ac.var<{_render_type(input_type)}>):",
                ]
            )
            for index, state in enumerate(definition.states):
                old = "old" if index == 0 else f"old_{index}"
                lines.append(
                    f"        %{old} = ac.var.read @{state.name} : "
                    f"!ac.var<{_render_type(state.value_type)}>"
                )
            emitted_lines = 0
            for assignment in definition.assignments:
                state_type = state_types[assignment.state]
                value, value_type = emitter.emit(assignment.expression, state_type)
                if not _types_compatible(value_type, state_type):
                    raise QueueFrontendError(
                        "ACPY-MODULE-004: assigned module state type mismatch"
                    )
                lines.extend("    " + line for line in emitter.lines[emitted_lines:])
                emitted_lines = len(emitter.lines)
                lines.append(
                    f"        ac.var.assign @{assignment.state} = %{value} : "
                    f"!ac.var<{_render_type(state_type)}>"
                )
                emitter.root_values[assignment.state] = (value, state_type)
            value, value_type = emitter.emit(expression, output_type)
            if not _types_compatible(value_type, output_type):
                raise QueueFrontendError(
                    "ACPY-MODULE-004: stateful module result type mismatch"
                )
            lines.extend("    " + line for line in emitter.lines[emitted_lines:])
            lines.extend(
                [
                    f"        %rule_ready = ac.marker.obligation %{value} "
                    "state pending resolver handshake "
                    f'origin "{name}:return" path "true" : '
                    f"!ac.var<{_render_type(output_type)}>",
                    f"        ac.rule.return %rule_ready : "
                    f"!ac.var<{_render_type(output_type)}>",
                    f'      }} {{ac.name = "result"}} : '
                    f"(!ac.queue<{_render_type(input_type)}>) "
                    f"-> !ac.queue<{_render_type(output_type)}>",
                    f"      ac.scope.yield %next : "
                    f"!ac.queue<{_render_type(output_type)}>",
                    f"    }} : (!ac.queue<{_render_type(input_type)}>) -> "
                    f"!ac.queue<{_render_type(output_type)}>",
                    f"    ac.return %output : !ac.queue<{_render_type(output_type)}>",
                    *family_module_close(),
                ]
            )
            continue
        emitter = _ExpressionEmitter(
            payload_map,
            argument,
            input_type,
            enum_types=enum_map,
            bitfields=bitfield_map,
            invariants=invariants,
            helpers=helpers,
            inline_explicit_helpers=True,
        )
        value, value_type = emitter.emit(expression, output_type)
        if not _types_compatible(value_type, output_type):
            raise QueueFrontendError(
                "ACPY-MODULE-001: module result payload type mismatch"
            )
        lines.extend(
            [
                *family_module_open(
                    name, (("input", input_type),), (output_type,),
                    _render_interface_display_attributes(
                        (argument,), ("result",), module_metadata(name)
                    ),
                ),
                "    %output = ac.scope @body(%input) {",
                f"    ^bb0(%borrowed: !ac.queue<{_render_type(input_type)}>):",
                "      %transformed = ac.transform %borrowed depths [1] "
                "latencies [1] {",
                f"      ^bb0(%item: !ac.var<{_render_type(input_type)}>):",
            ]
        )
        lines.extend("    " + line for line in emitter.lines)
        lines.extend(
            [
                f"        ac.transform.yield %{value} : "
                f"!ac.var<{_render_type(output_type)}>",
                f'      }} {{ac.name = "result"}} : '
                f"(!ac.queue<{_render_type(input_type)}>) "
                f"-> !ac.queue<{_render_type(output_type)}>",
                f"      ac.scope.yield %transformed : "
                f"!ac.queue<{_render_type(output_type)}>",
                f"    }} : (!ac.queue<{_render_type(input_type)}>) -> "
                f"!ac.queue<{_render_type(output_type)}>",
                f"    ac.return %output : !ac.queue<{_render_type(output_type)}>",
                *family_module_close(),
            ]
        )
    for projection_name, (
        argument,
        projection_definition,
        projection_static_arguments,
        input_type,
        expression,
        output_type,
        projection_source,
    ) in projection_definitions.items():
        emitter = _ExpressionEmitter(
            payload_map,
            argument,
            input_type,
            enum_types=enum_map,
            bitfields=bitfield_map,
            invariants=invariants,
            helpers=helpers,
            prefix=f"{projection_name}_",
            inline_explicit_helpers=True,
        )
        value, observed_type = emitter.emit(expression, output_type)
        if not _types_compatible(observed_type, output_type):
            raise AssertionError("module projection type changed during rendering")
        if projection_static_arguments:
            raise QueueFrontendError(
                "ACPY-FAMILY-008: compiler projection requires typed family "
                "arguments, not a static dictionary"
            )
        projection_owner = (
            projection_source.file
            if projection_source is not None
            else "generated/compiler_projection.py"
        )
        projection_provenance = family_provenance(projection_owner)
        projection_interface = concrete_family_interface(
            (("value", input_type),),
            (("result", output_type),),
            projection_provenance,
        )
        projection_nominals = _nominal_declarations([input_type, output_type])
        projection_schema = (
            "#ac.module_family_schema<#ac.static_parameters<[]>, "
            "#ac.static_cases<[#ac.static_arguments<[]>]>, "
            f"{projection_interface}, {family_owner(projection_owner)}, ["
            + ", ".join(f"@{nominal}" for nominal in projection_nominals)
            + "]>"
        )
        projection_metadata = _render_interface_display_attributes(
            (ast.unparse(expression),),
            ("result",),
            projection_module_metadata(
                projection_name,
                projection_definition,
                projection_source,
            ),
        ).removeprefix(" attributes")
        lines.extend(
            [
                f"  ac.module @{projection_name} source "
                f"{family_owner(projection_owner)} schema {projection_schema} {{",
                "    ac.module.case arguments #ac.static_arguments<[]> type "
                f"(!ac.queue<{_render_type(input_type)}>) -> "
                f"!ac.queue<{_render_type(output_type)}> {projection_metadata} "
                f"source {projection_provenance} graph {{",
                f"    ^bb0(%input: !ac.queue<{_render_type(input_type)}>):",
                "    %output = ac.scope @body(%input) {",
                f"    ^bb0(%borrowed: !ac.queue<{_render_type(input_type)}>):",
                "      %projected = ac.transform %borrowed depths [1] "
                "latencies [1] {",
                f"      ^transform(%item: !ac.var<{_render_type(input_type)}>):",
            ]
        )
        lines.extend("    " + line for line in emitter.lines)
        lines.extend(
            [
                f"        ac.transform.yield %{value} : "
                f"!ac.var<{_render_type(output_type)}>",
                "      } {ac.name = "
                + canonical_mlir_string(ast.unparse(expression))
                + f"}} : (!ac.queue<{_render_type(input_type)}>) -> "
                f"!ac.queue<{_render_type(output_type)}>",
                f"      ac.scope.yield %projected : "
                f"!ac.queue<{_render_type(output_type)}>",
                f"    }} : (!ac.queue<{_render_type(input_type)}>) -> "
                f"!ac.queue<{_render_type(output_type)}>",
                f"    ac.return %output : !ac.queue<{_render_type(output_type)}>",
                "    }",
                "  }" + _render_source_frame_location(projection_source),
            ]
        )

    def render_composite_module(
        symbol: str,
        definition: RuleModuleDefinition,
        static_arguments: tuple[tuple[str, StaticValue], ...],
        plan: CompositePlan,
        function: ast.FunctionDef,
    ) -> None:
        argument_types = ", ".join(
            f"%arg{index}: !ac.queue<{_render_type(payload)}>"
            for index, (_, payload) in enumerate(definition.inputs)
        )
        result_types = ", ".join(
            f"!ac.queue<{_render_type(payload)}>"
            for _, payload in definition.outputs
        )
        result_signature = (
            result_types if len(definition.outputs) == 1 else f"({result_types})"
        )
        source_frame = definition_sources.get(function.name, ("generated/module.py", 1, 1))
        source = source_frame[0]
        physical_inputs = ", ".join(
            f"!ac.queue<{_render_type(payload)}>"
            for _, payload in definition.inputs
        )
        physical_results = result_signature if definition.outputs else "()"
        interface = concrete_family_interface(
            definition.inputs,
            definition.outputs,
            family_provenance(*source_frame),
        )
        nominals = _nominal_declarations(
            [payload for _, payload in definition.inputs]
            + [payload for _, payload in definition.outputs]
        )
        lines.extend([
            f"  ac.module @{symbol} source {family_owner(source)} "
            f"schema {family_schema(function.name, source, interface, nominals)} {{",
            f"    ac.module.case arguments #ac.static_arguments<[]> "
            f"type ({physical_inputs}) -> {physical_results}"
            + _render_interface_display_attributes(
                tuple(name for name, _ in definition.inputs),
                tuple(name for name, _ in definition.outputs),
                composite_module_metadata(symbol, function.name),
            ).removeprefix(" attributes")
            + f" source {family_provenance(*source_frame)} graph {{",
            f"    ^bb0({argument_types}):" if argument_types else "    ^bb0:",
        ])
        available: dict[str, list[str]] = {}

        def bind_value(name: str, ssa: str) -> None:
            use_count = plan.uses[name]
            if use_count == 1:
                available[name] = [ssa]
                return
            outputs = [f"{name}_fanout_{index}" for index in range(use_count)]
            payload = plan.values[name]
            rendered_type = _render_type(payload)
            rendered_outputs = ", ".join(f"%{item}" for item in outputs)
            local_outputs = ", ".join(f"%{item}_local" for item in outputs)
            output_types = ", ".join(
                f"!ac.queue<{rendered_type}>" for _ in outputs
            )
            depths = ", ".join("1" for _ in outputs)
            output_names = "[" + ", ".join(
                canonical_mlir_string(f"{item}_local") for item in outputs
            ) + "]"
            lines.extend(
                [
                    f"    {rendered_outputs} = ac.scope "
                    f"@fanout_{name}(%{ssa}) {{",
                    f"    ^bb0(%borrowed: !ac.queue<{rendered_type}>):",
                    f"      {local_outputs} = ac.broadcast %borrowed "
                    f"depths [{depths}] latencies [{depths}] "
                    f"{{ac.output_names = {output_names}}} : "
                    f"!ac.queue<{rendered_type}> -> ({output_types})",
                    "      ac.scope.yield "
                    + ", ".join(f"%{item}_local" for item in outputs)
                    + " : "
                    + output_types,
                    f"    }} : (!ac.queue<{rendered_type}>) -> "
                    f"({output_types})",
                ]
            )
            available[name] = outputs

        def take_value(name: str) -> str:
            values_for_name = available.get(name)
            if not values_for_name:
                raise AssertionError(
                    f"composite Queue value {name!r} is unavailable"
                )
            return values_for_name.pop(0)

        for index, (name, _) in enumerate(definition.inputs):
            bind_value(name, f"arg{index}")
        for index, instance in enumerate(plan.instances):
            operands = ", ".join(
                f"%{take_value(source)}" for source in instance.sources
            )
            input_signature = ", ".join(
                f"!ac.queue<{_render_type(plan.values[source])}>"
                for source in instance.sources
            )
            output_signature = ", ".join(
                f"!ac.queue<{_render_type(payload)}>"
                for payload in instance.output_types
            )
            result_type = (
                output_signature
                if len(instance.output_types) == 1
                else f"({output_signature})"
            )
            lhs = ", ".join(f"%{result}" for result in instance.results)
            assignment = f"{lhs} = " if lhs else ""
            instance_name = f"{instance.module_name}_{index}"
            lines.append(
                f"    {assignment}ac.instance @{instance_name} of "
                f"@{instance.module_name}({operands}) static "
                f"{family_arguments(instance.module_name, instance.static_arguments)} "
                f'id "{instance_name}" path "{instance_name}" '
                f": ({input_signature}) -> {result_type}"
                + _render_source_frame_location(instance.source)
            )
            for result in instance.results:
                bind_value(result, result)
        if definition.outputs:
            returned = [
                f"%{take_value(name)}" for name in plan.returned_names
            ]
            lines.append(
                "    ac.return " + ", ".join(returned) + " : " + result_types
            )
        else:
            lines.append("    ac.return")
        lines.append("    }")
        lines.append("  }")

    for name, (
        definition,
        program,
        static_arguments,
    ) in module_bodies.items():
        if program is None:
            if name in composite_plans:
                composite_function, _ = composite_functions[name]
                render_composite_module(
                    name,
                    definition,
                    static_arguments,
                    composite_plans[name],
                    composite_function,
                )
                continue
            source = declaration_sources[name]
            lines.append(
                f"  ac.module.import @{name} source {family_owner(source)} "
                f"schema {family_schema(name, source)}"
            )
            continue
        helper_names_to_emit: set[str] = set()
        for helper in program.helpers:
            existing_helper = emitted_helpers.get(helper.name)
            if existing_helper is None:
                emitted_helpers[helper.name] = helper
                helper_names_to_emit.add(helper.name)
                continue
            if (
                existing_helper.arguments != helper.arguments
                or existing_helper.result != helper.result
                or existing_helper.inline != helper.inline
                or ast.dump(existing_helper.expression)
                != ast.dump(helper.expression)
            ):
                raise QueueFrontendError(
                    "ACPY-HELPER-002: concrete helper symbol collision for "
                    f"{helper.name!r}; make its typed family case explicit"
                )
        module_frame = definition_sources.get(program.system, ("", 0, 0))
        module_interface = concrete_family_interface(
            definition.inputs,
            definition.outputs,
            family_provenance(*module_frame),
        )
        module_nominals = _nominal_declarations(
            [payload for _, payload in definition.inputs]
            + [payload for _, payload in definition.outputs]
        )
        lines.extend(
            lower_queue_program(
                program,
                module=_ModuleRenderSpec(
                    name,
                    definition.inputs,
                    definition.outputs,
                    static_arguments,
                    definition_ndf.get(program.system, NdfMetadata()),
                    *module_frame,
                    definition_name=program.system,
                    schema=family_schema(
                        name,
                        module_frame[0],
                        module_interface,
                        module_nominals,
                    ),
                ),
                include_helpers=True,
                helper_names_to_emit=frozenset(helper_names_to_emit),
                definition_locations=dict(definition_locations or {}),
                definition_ndf=definition_ndf,
                render_instance_static_arguments=family_arguments,
            )
            .rstrip()
            .splitlines()
        )
    root_result_types = ", ".join(
        render_concrete_queue(payload, shape)
        for payload, shape in zip(
            expected_results, result_queue_shapes, strict=True
        )
    )
    root_result_signature = (
        root_result_types if len(expected_results) == 1 else f"({root_result_types})"
    )
    top_source_frame = definition_sources.get(system, ("generated/core.py", 1, 1))
    top_source = top_source_frame[0]
    root_input_types = ", ".join(
        render_concrete_queue(payload, external_queue_shapes[name])
        for name, payload in external
    )
    root_ports: list[str] = []
    root_provenance = family_provenance(*top_source_frame)
    for name, payload in external:
        queue_type = render_concrete_queue(payload, external_queue_shapes[name])
        root_ports.append(
            "#ac.interface_port<"
            f"{canonical_mlir_string(name)}, \"input\", "
            f"#ac.type_expr<#ac.type_expr_concrete<{queue_type}>>, "
            f"{root_provenance}>"
        )
    for index, (payload, shape) in enumerate(
        zip(expected_results, result_queue_shapes, strict=True)
    ):
        queue_type = render_concrete_queue(payload, shape)
        root_ports.append(
            "#ac.interface_port<"
            f"{canonical_mlir_string(f'result_{index}')}, \"output\", "
            f"#ac.type_expr<#ac.type_expr_concrete<{queue_type}>>, "
            f"{root_provenance}>"
        )
    root_nominals = _nominal_declarations(
        [payload for _, payload in external] + list(expected_results)
    )
    root_schema = (
        "#ac.module_family_schema<#ac.static_parameters<[]>, "
        "#ac.static_cases<[#ac.static_arguments<[]>]>, "
        "#ac.module_interface<[" + ", ".join(root_ports) + "]>, "
        f"{family_owner(top_source)}, ["
        + ", ".join(f"@{nominal}" for nominal in root_nominals)
        + "]>"
    )
    root_arguments = ", ".join(
        f"%input_{index}: {render_concrete_queue(payload, external_queue_shapes[name])}"
        for index, (name, payload) in enumerate(external)
    )
    lines.extend([
        f"  ac.module @Top source {family_owner(top_source)} "
        f"schema {root_schema} {{",
        "    ac.module.case arguments #ac.static_arguments<[]> type ("
        + root_input_types + ") -> "
        + (root_result_signature if expected_results else "()")
        + _render_interface_display_attributes(
            tuple(name for name, _ in external),
            tuple(f"result_{index}" for index in range(len(expected_results))),
            module_metadata(system),
        ).removeprefix(" attributes")
        + f" source {family_provenance(*top_source_frame)} graph {{",
        f"    ^bb0({root_arguments}):" if root_arguments else "    ^bb0:",
    ])
    top_values: dict[str, list[str]] = {}

    def bind_top_value(name: str, ssa: str) -> None:
        use_count = uses[name]
        if use_count == 1:
            top_values[name] = [ssa]
            return
        outputs = [f"{name}_fanout_{index}" for index in range(use_count)]
        payload = values[name]
        rendered_type = _render_type(payload)
        rendered_outputs = ", ".join(f"%{output}" for output in outputs)
        rendered_local_outputs = ", ".join(
            f"%{output}_local" for output in outputs
        )
        depths = ", ".join("1" for _ in outputs)
        output_types = ", ".join(
            f"!ac.queue<{rendered_type}>" for _ in outputs
        )
        output_names = "[" + ", ".join(
            canonical_mlir_string(f"{output}_local") for output in outputs
        ) + "]"
        lines.append(
            f"    {rendered_outputs} = ac.scope @fanout_{name}(%{ssa}) {{"
        )
        lines.append(
            f"    ^bb0(%borrowed: !ac.queue<{rendered_type}>):"
        )
        lines.append(
            f"      {rendered_local_outputs} = ac.broadcast %borrowed "
            f"depths [{depths}] "
            f"latencies [{depths}] "
            f"{{ac.output_names = {output_names}}} : "
            f"!ac.queue<{rendered_type}> -> "
            f"({output_types})"
        )
        lines.append(
            "      ac.scope.yield "
            + ", ".join(f"%{output}_local" for output in outputs)
            + " : "
            + output_types
        )
        lines.append(
            f"    }} : (!ac.queue<{rendered_type}>) -> ({output_types})"
        )
        top_values[name] = outputs

    def take_top_value(name: str) -> str:
        available = top_values.get(name)
        if not available:
            raise AssertionError(f"module Queue value {name!r} is unavailable")
        return available.pop(0)

    for index, (name, _) in enumerate(external):
        bind_top_value(name, f"input_{index}")
    for operation_kind, operation_index in operation_order:
        if operation_kind == "projection":
            (
                result,
                source,
                projection_module,
                projection_static_arguments,
                expression,
                output_type,
                projection_source,
            ) = projections[operation_index]
            input_type = values[source]
            operand = take_top_value(source)
            lines.append(
                f"    %{result} = ac.instance @{result} of @{projection_module}"
                f"(%{operand}) static "
                "#ac.static_arguments<[]> "
                f"id \"{result}\" path \"{result}\" "
                f": (!ac.queue<{_render_type(input_type)}>) -> "
                f"!ac.queue<{_render_type(output_type)}>"
                + _render_source_frame_location(projection_source)
            )
            bind_top_value(result, result)
            continue
        if operation_kind != "instance":
            raise AssertionError(f"unknown module operation {operation_kind!r}")
        (
            results,
            module_name,
            sources,
            output_types,
            static_arguments,
            instance_source,
        ) = instances[operation_index]
        input_types = tuple(values[source] for source in sources)
        lhs = ", ".join(f"%{result}" for result in results)
        operands = ", ".join(f"%{take_top_value(source)}" for source in sources)
        input_signature = ", ".join(
            render_concrete_queue(payload, value_queue_shapes[source])
            for source, payload in zip(sources, input_types, strict=True)
        )
        output_signature = ", ".join(
            render_concrete_queue(payload, value_queue_shapes[result])
            for result, payload in zip(results, output_types, strict=True)
        )
        result_type = (
            output_signature if len(output_types) == 1 else f"({output_signature})"
        )
        instance_name = f"{module_name}_{operation_index}"
        assignment = f"{lhs} = " if lhs else ""
        lines.append(
            f"    {assignment}ac.instance @{instance_name} of @{module_name}"
            f"({operands}) static "
            f"{family_arguments(module_name, static_arguments)} "
            f'id "{instance_name}" path "{instance_name}" '
            f": ({input_signature}) -> {result_type}"
            + _render_source_frame_location(instance_source)
        )
        for result in results:
            bind_top_value(result, result)
    returned_operands = [f"%{take_top_value(name)}" for name in returned_names]
    if not expected_results:
        lines.append("    ac.return")
    else:
        lines.append(
            "    ac.return " + ", ".join(returned_operands) + " : " + root_result_types
        )
    lines.extend(["    }", "  }", "}"])

    def lower_concrete_family_case(
        family_name: str, values: tuple[tuple[str, StaticValue], ...]
    ) -> list[str]:
        declaration = module_declaration_nodes[family_name]
        static_by_name = dict(values)

        def evaluate_queue_integer(expression: ast.expr) -> int:
            if isinstance(expression, ast.Constant) and type(expression.value) is int:
                return expression.value
            if (
                isinstance(expression, ast.UnaryOp)
                and isinstance(expression.op, ast.USub)
                and isinstance(expression.operand, ast.Constant)
                and type(expression.operand.value) is int
            ):
                return -expression.operand.value
            if isinstance(expression, ast.Name) and expression.id in static_by_name:
                value = static_by_name[expression.id]
            elif isinstance(expression, ast.Attribute):
                fields: list[str] = []
                cursor: ast.expr = expression
                while isinstance(cursor, ast.Attribute):
                    fields.append(cursor.attr)
                    cursor = cursor.value
                if not isinstance(cursor, ast.Name) or cursor.id not in static_by_name:
                    raise QueueFrontendError(
                        "ACPY-FAMILY-008: Queue shape field has an unknown root"
                    )
                value = static_by_name[cursor.id]
                for field in reversed(fields):
                    if not isinstance(value, FrozenMap):
                        raise QueueFrontendError(
                            "ACPY-FAMILY-008: Queue shape field crosses a "
                            "non-config value"
                        )
                    try:
                        value = value[field]
                    except KeyError as error:
                        raise QueueFrontendError(
                            f"ACPY-FAMILY-008: Queue shape field {field!r} is unknown"
                        ) from error
            elif isinstance(expression, ast.BinOp) and isinstance(
                expression.op, (ast.Add, ast.Sub, ast.Mult)
            ):
                lhs = evaluate_queue_integer(expression.left)
                rhs = evaluate_queue_integer(expression.right)
                if isinstance(expression.op, ast.Add):
                    return lhs + rhs
                if isinstance(expression.op, ast.Sub):
                    return lhs - rhs
                return lhs * rhs
            elif (
                isinstance(expression, ast.Call)
                and len(expression.args) == 1
                and not expression.keywords
                and _decorator_name(expression.func).rsplit(".", 1)[-1]
                in {"index_width", "count_width"}
            ):
                operand = evaluate_queue_integer(expression.args[0])
                kind = _decorator_name(expression.func).rsplit(".", 1)[-1]
                if kind == "index_width":
                    if operand <= 0:
                        raise QueueFrontendError(
                            "ACPY-FAMILY-008: Queue index_width operand must be positive"
                        )
                    return max(1, (operand - 1).bit_length())
                if operand < 0:
                    raise QueueFrontendError(
                        "ACPY-FAMILY-008: Queue count_width operand must be non-negative"
                    )
                return max(1, operand.bit_length())
            else:
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: unsupported dependent Queue shape expression"
                )
            if type(value) is not int:
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: Queue shape must resolve to an integer"
                )
            return value

        def queue_parts(annotation: ast.expr) -> tuple[ast.expr, int, int]:
            if (
                not isinstance(annotation, ast.Subscript)
                or _decorator_name(annotation.value).rsplit(".", 1)[-1] != "Queue"
                or not isinstance(annotation.slice, ast.Tuple)
                or len(annotation.slice.elts) != 3
            ):
                return annotation, 1, 1
            payload, lanes_node, rate_node = annotation.slice.elts
            lanes = evaluate_queue_integer(lanes_node)
            rate = evaluate_queue_integer(rate_node)
            if lanes <= 0 or rate <= 0 or rate > lanes:
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: Queue requires lanes > 0 and 1 <= rate <= lanes"
                )
            return payload, lanes, rate

        def strip_queue_annotation(annotation: ast.expr | None) -> ast.expr | None:
            if annotation is None:
                return None
            if (
                isinstance(annotation, ast.Subscript)
                and _decorator_name(annotation.value).rsplit(".", 1)[-1] == "Queue"
                and isinstance(annotation.slice, ast.Tuple)
                and len(annotation.slice.elts) == 3
            ):
                return copy.deepcopy(annotation.slice.elts[0])
            if (
                isinstance(annotation, ast.Subscript)
                and _decorator_name(annotation.value).rsplit(".", 1)[-1]
                in {"tuple", "Tuple"}
            ):
                result = copy.deepcopy(annotation)
                elements = (
                    list(result.slice.elts)
                    if isinstance(result.slice, ast.Tuple)
                    else [result.slice]
                )
                stripped = [strip_queue_annotation(element) for element in elements]
                assert all(item is not None for item in stripped)
                result.slice = ast.Tuple(
                    elts=[item for item in stripped if item is not None],
                    ctx=ast.Load(),
                )
                return result
            return copy.deepcopy(annotation)

        queue_shapes: dict[str, tuple[int, int]] = {}
        declaration_annotations = [
            parameter.annotation
            for parameter in (*declaration.args.posonlyargs, *declaration.args.args)
        ]
        declaration_annotations.extend(result_annotations(declaration.returns))
        for annotation in declaration_annotations:
            payload_annotation, lanes, rate = queue_parts(annotation)
            payload = _payload(
                payload_annotation,
                payload_map,
                enum_map,
                static_by_name,
            )
            rendered_payload = _render_type(payload)
            previous = queue_shapes.get(rendered_payload)
            if previous is not None and previous != (lanes, rate):
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: one payload type cannot use multiple Queue "
                    "shapes in the same F4 family case"
                )
            queue_shapes[rendered_payload] = (lanes, rate)
        non_scalar_shapes = {
            shape for shape in queue_shapes.values() if shape != (1, 1)
        }
        if len(non_scalar_shapes) > 1:
            raise QueueFrontendError(
                "ACPY-FAMILY-008: one F4 family case requires a uniform "
                "multi-lane Queue shape"
            )

        case_tree = copy.deepcopy(tree)
        implementation: ast.FunctionDef | None = None
        filtered: list[ast.stmt] = []
        for statement in case_tree.body:
            if isinstance(statement, ast.FunctionDef) and statement.name == family_name:
                kinds = {
                    _decorator_name(decorator).rsplit(".", 1)[-1]
                    for decorator in statement.decorator_list
                }
                if "module_decl" in kinds:
                    continue
                if "module" in kinds:
                    specialized = specialize_family_function(statement, values)
                    for parameter in (
                        *specialized.args.posonlyargs,
                        *specialized.args.args,
                    ):
                        parameter.annotation = strip_queue_annotation(
                            parameter.annotation
                        )
                    specialized.returns = strip_queue_annotation(
                        specialized.returns
                    )
                    specialized.decorator_list = [
                        ast.Call(
                            func=ast.Name(id="module", ctx=ast.Load()),
                            args=[],
                            keywords=[
                                ast.keyword(
                                    arg="declaration",
                                    value=ast.Name(
                                        id=family_name, ctx=ast.Load()
                                    ),
                                )
                            ],
                        )
                    ]
                    implementation = specialized
                    filtered.append(specialized)
                    continue
            if isinstance(statement, ast.FunctionDef) and any(
                _decorator_name(decorator).rsplit(".", 1)[-1] == "system"
                for decorator in statement.decorator_list
            ):
                continue
            filtered.append(statement)
        if implementation is None:
            raise QueueFrontendError(
                f"ACPY-FAMILY-008: implementation for family {family_name!r} is missing"
            )
        root_name = "family_case_root"
        occupied = {
            statement.name
            for statement in filtered
            if isinstance(statement, (ast.FunctionDef, ast.ClassDef))
        }
        while root_name in occupied:
            root_name += "_"
        runtime_parameters = (*implementation.args.posonlyargs, *implementation.args.args)
        call = ast.Call(
            func=ast.Name(id=family_name, ctx=ast.Load()),
            args=[ast.Name(id=parameter.arg, ctx=ast.Load()) for parameter in runtime_parameters],
            keywords=[],
        )
        has_result = not (
            implementation.returns is None
            or isinstance(implementation.returns, ast.Constant)
            and implementation.returns.value is None
        )
        root = ast.FunctionDef(
            name=root_name,
            args=copy.deepcopy(implementation.args),
            body=(
                [ast.Return(value=call)]
                if has_result
                else [ast.Expr(value=call), ast.Return(value=ast.Constant(value=None))]
            ),
            decorator_list=[ast.Name(id="system", ctx=ast.Load())],
            returns=copy.deepcopy(implementation.returns),
            type_comment=None,
        )
        filtered.append(ast.fix_missing_locations(root))
        case_tree.body = filtered
        concrete = _lower_simple_module_source(
            ast.unparse(ast.fix_missing_locations(case_tree)),
            root_name,
            source_path=normalized_source_path,
            host_results=True,
        )
        if concrete is None:
            raise QueueFrontendError(
                f"ACPY-FAMILY-008: concrete case for {family_name!r} did not lower"
            )
        concrete_lines = concrete.splitlines()
        start = next(
            index
            for index, line in enumerate(concrete_lines)
            if line.startswith(f"  ac.module @{family_name} ")
        )
        end = next(
            index
            for index in range(start + 1, len(concrete_lines))
            if concrete_lines[index].startswith("  ac.")
            or concrete_lines[index] == "}"
        )
        result = concrete_lines[start + 1 : end - 1]
        for rendered_payload, (lanes, rate) in queue_shapes.items():
            if (lanes, rate) == (1, 1):
                continue
            scalar = f"!ac.queue<{rendered_payload}>"
            shaped = (
                f"!ac.queue<{rendered_payload}, lanes={lanes}, rate={rate}>"
            )
            result = [line.replace(scalar, shaped) for line in result]
        if non_scalar_shapes:
            _, rate = next(iter(non_scalar_shapes))
            result = [line.replace("depths [1]", f"depths [{rate}]") for line in result]
        return result

    # A finite implementation owns one concrete case region for every declared
    # case, including declared cases unused by the selected caller graph.
    for family_name, case_attrs in module_family_case_attrs.items():
        if (
            family_name not in module_implementations
            or not module_family_parameter_specs.get(family_name)
        ):
            continue
        prefix = f"  ac.module @{family_name} "
        start = next(
            (index for index, line in enumerate(lines) if line.startswith(prefix)),
            None,
        )
        if start is None:
            continue
        end = next(
            (
                index
                for index in range(start + 1, len(lines))
                if lines[index].startswith("  ac.") or lines[index] == "}"
            ),
            len(lines),
        )
        module_lines = lines[start:end]
        if len(module_lines) < 3 or module_lines[-1] != "  }":
            raise QueueFrontendError(
                "ACPY-FAMILY-008: family implementation has malformed emitted region"
            )
        expanded = [module_lines[0]]
        for arguments, values in zip(
            case_attrs, module_family_cases[family_name], strict=True
        ):
            concrete = lower_concrete_family_case(family_name, values)
            header_index = next(
                (
                    index
                    for index, line in enumerate(concrete)
                    if "ac.module.case arguments #ac.static_arguments<[]>" in line
                ),
                None,
            )
            if header_index is None:
                raise QueueFrontendError(
                    "ACPY-FAMILY-008: concrete family case has no canonical region"
                )
            concrete[header_index] = concrete[header_index].replace(
                "#ac.static_arguments<[]>", arguments, 1
            )
            expanded.extend(concrete)
        expanded.append("  }")
        lines[start:end] = expanded
    return "\n".join(lines) + "\n"
