"""Structured module specialization and ACIR lowering."""

from __future__ import annotations

import ast
import copy
import re
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
    StaticEnvironment,
    StaticValue,
    evaluate_static,
    static_json_value,
)
from .acir_text import (
    _enum_layout_entry,
    _payload_layout_entry,
    _render_bitfield,
    _render_enum,
    _render_interface_display_attributes,
    _render_static_mlir_dictionary,
    _render_static_mlir_value,
    _render_static_type_attributes,
    _render_type,
)
from .definitions import _invariant_definitions, _pure_helper_definitions
from .errors import QueueFrontendError
from .expressions import (
    _ExpressionEmitter,
)
from .lower_acir import (
    _ModuleRenderSpec,
    _module_attribute_fields,
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
from .parser import parse_queue_program
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
    _scalar_annotation_static_check,
    _scalar_reset_init,
    _static_config_bindings_for_checks,
    _static_config_expression_type,
    _static_parameter_aliases,
    _static_type_bindings_for_checks,
    _type_static_values,
    _types_compatible,
    _validate_static_config_roots,
)
from .syntax import _decorator_name


def _readable_static_value(value: StaticValue) -> str:
    rendered = str(static_json_value(value))
    token = re.sub("[^A-Za-z0-9]+", "_", rendered).strip("_")
    return token or "empty"


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
        if (
            len(declarations) != 1
            or not isinstance(decorator, ast.Call)
            or decorator.args
            or len(decorator.keywords) != 1
            or decorator.keywords[0].arg != "source"
            or not isinstance(decorator.keywords[0].value, ast.Constant)
            or type(decorator.keywords[0].value.value) is not str
            or not decorator.keywords[0].value.value
        ):
            raise QueueFrontendError(
                "ACPY-MODULE-009: module declaration requires exactly "
                "@ac.module_decl(source=\"relative/source.py\")"
            )
        implementation_source = decorator.keywords[0].value.value
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

    modules = {
        node.name: node
        for node in tree.body
        if isinstance(node, ast.FunctionDef)
        and any(
            _decorator_name(decorator).rsplit(".", 1)[-1]
            in {"module", "module_decl"}
            for decorator in node.decorator_list
        )
    }
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
        name for name in modules if name[:5] == "__ac_"
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
    reserved_values = [name for name in reserved_values if name[:5] == "__ac_"]
    if reserved_definitions or reserved_values:
        reserved = (reserved_definitions + reserved_values)[0]
        raise QueueFrontendError(
            "ACPY-MODULE-008: names beginning with '__ac_' are compiler-owned: "
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
    for name, function in modules.items():
        if name in module_declarations:
            body = list(function.body)
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
                or function.args.posonlyargs
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
                    "ACPY-MODULE-009: module declaration requires typed runtime "
                    "parameters, optional keyword-only ac.const parameters, and "
                    "an ellipsis body"
                )
            output_annotations = result_annotations(function.returns)
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
                ),
            )
            continue
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
                ),
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
                or any(
                    isinstance(decorator, ast.Call)
                    for decorator in function.decorator_list
                )
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
            rule_modules[name] = RuleModuleTemplate(
                input_annotations,
                tuple(
                    result.id for result in result_nodes if isinstance(result, ast.Name)
                ),
                output_annotations,
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
                for check in payload.static_type_checks
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
                for check in payload.static_type_checks
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
            or function.args.vararg is not None
            or function.args.kwarg is not None
            or function.args.defaults
            or any(
                isinstance(decorator, ast.Call) for decorator in function.decorator_list
            )
        ):
            raise QueueFrontendError(
                "ACPY-MODULE-001: first module slice requires one typed "
                "positional parameter"
            )
        if any(
            not isinstance(parameter.annotation, ast.Subscript)
            or _decorator_name(parameter.annotation.value).rsplit(".", 1)[-1]
            != "const"
            for parameter in function.args.kwonlyargs
        ) or any(default is None for default in function.args.kw_defaults):
            raise QueueFrontendError(
                "ACPY-MODULE-001: keyword-only module parameters must use "
                "ac.const and provide defaults"
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
    rule_module_specializations: dict[
        str,
        tuple[
            RuleModuleDefinition,
            QueueProgram | None,
            tuple[tuple[str, StaticValue], ...],
        ],
    ] = {}
    declaration_specialization_sources: dict[str, str] = {}
    composite_specialization_functions: dict[
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
        for keyword in call.keywords:
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
            try:
                value = evaluate_static(
                    expression, StaticEnvironment(active_static_values)
                )
            except ValueError as error:
                raise QueueFrontendError(
                    f"ACPY-MODULE-007: module static argument {name!r} is not closed"
                ) from error
            _render_static_mlir_value(value)
            static_values.append((name, value))
        frozen = tuple(static_values)
        readable_parameters = "__".join(
            f"{name}_{_readable_static_value(value)}" for name, value in frozen
        )
        symbol = (
            module_name
            if not readable_parameters
            else f"{module_name}__{readable_parameters}"
        )
        existing = rule_module_specializations.get(symbol)
        if existing is not None and existing[2] != frozen:
            raise QueueFrontendError(
                "ACPY-MODULE-007: readable specialization name collision for "
                f"{module_name!r}"
            )
        if symbol not in rule_module_specializations:
            namespace = "" if not readable_parameters else f"{symbol}__"
            program: QueueProgram | None = None
            specialized_payloads = payload_map
            if (
                module_name not in module_declarations
                and module_name not in composite_modules
            ):
                try:
                    program = parse_queue_program(
                        text,
                        module_name,
                        static_arguments=dict(frozen),
                        entry_kind="module",
                        source_path=normalized_source_path,
                        static_type_namespace=namespace,
                        definition_locations=definition_locations,
                        static_assert_locations=static_assert_locations,
                        source_node_locations=source_node_locations,
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
            specialized_values = _type_static_values(tree, dict(frozen))
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
            rule_module_specializations[symbol] = (
                definition,
                program,
                frozen,
            )
            if module_name in composite_modules:
                composite_specialization_functions[symbol] = (
                    composite_modules[module_name],
                    frozen,
                )
            if module_name in module_declarations:
                declaration_specialization_sources[symbol] = (
                    module_declarations[module_name]
                )
        definition, _, _ = rule_module_specializations[symbol]
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
        definition, _, _ = rule_module_specializations[symbol]
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
                        f"__return_{index}"
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
            names = tuple(f"__return_{index}" for index in range(len(outputs)))
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
            if statement.value.args or input_signature or output_signature:
                raise QueueFrontendError(
                    "ACPY-MODULE-002: expression module calls require a "
                    "zero-input zero-output signature"
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
            for argument_index, (argument, (_, expected_type)) in enumerate(
                zip(statement.value.args, input_signature, strict=True)
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
                        f"__ac_project_{root_type.name}__{'__'.join(fields)}"
                    )
                    projection_static_arguments = tuple(
                        root_type.specialization_bindings
                    )
                    readable_projection_parameters = "".join(
                        f"__{name}_{'neg_' if value < 0 else ''}{abs(value)}"
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
                        f"__ac_projection_{len(projections)}_{root.id}_{argument_index}"
                    )
                    while projection_name in values:
                        projection_name += "_"
                    values[projection_name] = actual_type
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
                uses[source] = uses.get(source, 0) + 1
                sources.append(source)
            output_types = tuple(payload for _, payload in output_signature)
            for result, output_type in zip(results, output_types, strict=True):
                values[result] = output_type
                uses[result] = 0
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
    ):
        raise QueueFrontendError(
            "ACPY-MODULE-002: module system return type or arity mismatch"
        )
    if any(count < 1 for count in uses.values()):
        raise QueueFrontendError(
            "ACPY-MODULE-002: every module Queue value requires a consumer"
        )

    pending_composites = list(composite_specialization_functions)
    composite_cursor = 0
    while composite_cursor < len(pending_composites):
        symbol = pending_composites[composite_cursor]
        composite_cursor += 1
        if symbol in composite_plans:
            continue
        composite_function, composite_frozen = (
            composite_specialization_functions[symbol]
        )
        composite_plans[symbol] = parse_composite_plan(
            symbol, composite_function, composite_frozen
        )
        for candidate in composite_specialization_functions:
            if candidate not in pending_composites:
                pending_composites.append(candidate)

    all_payloads_by_symbol: dict[str, Payload] = {
        payload.descriptor.symbol: payload for payload in payloads
    }
    top_static_checks = (
        *(check for payload in payloads for check in payload.static_type_checks),
        *system_interface_checks,
    )
    candidate_static_bindings = dict(
        _static_type_bindings_for_checks(
            top_static_checks,
            parameter_aliases,
            type_static_values,
        )
    )
    top_static_configs = _static_config_bindings_for_checks(
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
    for _, program, _ in rule_module_specializations.values():
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
        for name, value in program.static_type_bindings:
            if (
                name in candidate_static_bindings
                and candidate_static_bindings[name] != value
            ):
                raise QueueFrontendError(
                    "ACPY-TYPE-008: specialized static binding collision"
                )
            candidate_static_bindings[name] = value
        for binding in program.static_config_bindings:
            existing_config = candidate_static_configs.get(binding.root)
            if existing_config is not None and existing_config != binding:
                raise QueueFrontendError(
                    "ACPY-TYPE-008: specialized static config binding collision"
                )
            candidate_static_configs[binding.root] = binding
        all_interface_checks.extend(program.static_type_checks)
    all_payloads = tuple(all_payloads_by_symbol.values())
    candidate_checks = [
        *(check for payload in all_payloads for check in payload.static_type_checks),
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
    all_checks = list(unique_checks.values())
    payload_check_targets = {
        check.target
        for payload in all_payloads
        for check in payload.static_type_checks
    }
    rendered_extra_checks = tuple(
        check for check in all_checks if check.target not in payload_check_targets
    )
    used_static_parameters = {
        token[6:]
        for check in all_checks
        for token in check.program
        if token[:6] == "param:"
    }
    all_static_bindings = {
        name: value
        for name, value in candidate_static_bindings.items()
        if name in used_static_parameters
    }
    used_static_configs = {
        root: binding
        for root, binding in candidate_static_configs.items()
        if any(
            parameter[: len(root) + 1] == root + "."
            for parameter in used_static_parameters
        )
    }

    lines = [
        "builtin.module attributes {"
        'ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle"'
        + _render_static_type_attributes(
            tuple(sorted(all_static_bindings.items())),
            all_payloads,
            rendered_extra_checks,
            tuple(
                used_static_configs[root]
                for root in sorted(used_static_configs)
            ),
        )
        + "} {"
    ]
    if all_payloads or enum_bindings or bitfield_bindings:
        lines.append("  ac.type_scope @types {")
        for enumeration in enum_bindings:
            rendered = _render_enum(enumeration, "    ")
            location = (definition_locations or {}).get(enumeration.name)
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
            location = (definition_locations or {}).get(payload.name)
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
    for name in sorted(empty_modules):
        lines.append(
            f"  ac.module @{name}() parameters {{}}"
            + _render_interface_display_attributes(
                (), (), module_metadata(name)
            )
            + " graph {"
        )
        for index, (child, child_source) in enumerate(
            empty_module_children.get(name, ())
        ):
            instance = f"{child}_{index}"
            lines.append(
                f"    ac.instance @{instance} of @{child}() static {{}} "
                f'id "{instance}" path "{instance}" : () -> ()'
                + _render_source_frame_location(child_source)
            )
        lines.extend(["    ac.return", "  }"])
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
                    f"  ac.module @{name}(%input: "
                    f"!ac.queue<{_render_type(input_type)}>) -> "
                    f"!ac.queue<{_render_type(output_type)}> parameters {{}}"
                    f"{_render_interface_display_attributes((argument,), ('result',), module_metadata(name))} "
                    "graph {",
                    f"    %output = ac.instance @result of @{child}(%input) "
                    f"static {_render_static_mlir_dictionary(static_arguments)} "
                    'id "result" path "result" '
                    f": (!ac.queue<{_render_type(input_type)}>) -> "
                    f"!ac.queue<{_render_type(output_type)}>"
                    + _render_source_frame_location(source_frame(expression)),
                    f"    ac.return %output : !ac.queue<{_render_type(output_type)}>",
                    "  }",
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
                    f"  ac.module @{name}(%input: "
                    f"!ac.queue<{_render_type(input_type)}>) -> "
                    f"!ac.queue<{_render_type(output_type)}> parameters {{}}"
                    f"{_render_interface_display_attributes((argument,), ('result',), module_metadata(name))} "
                    "graph {",
                    f"    %output = ac.instance @result of @{child}(%input) "
                    'static {} id "result" path "result" '
                    f": (!ac.queue<{_render_type(input_type)}>) -> "
                    f"!ac.queue<{_render_type(output_type)}>"
                    + _render_source_frame_location(source_frame(expression)),
                    f"    ac.return %output : !ac.queue<{_render_type(output_type)}>",
                    "  }",
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
                    f"  ac.module @{name}(%input: "
                    f"!ac.queue<{_render_type(input_type)}>) -> "
                    f"!ac.queue<{_render_type(output_type)}> parameters {{}}"
                    f"{_render_interface_display_attributes((argument,), ('result',), module_metadata(name))} "
                    "graph {",
                    "    %output = ac.scope @body(%input) {",
                    f"    ^bb0(%borrowed: !ac.queue<{_render_type(input_type)}>):",
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
                    "  }",
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
                f"  ac.module @{name}(%input: "
                f"!ac.queue<{_render_type(input_type)}>) -> "
                f"!ac.queue<{_render_type(output_type)}> parameters {{}}"
                f"{_render_interface_display_attributes((argument,), ('result',), module_metadata(name))} "
                "graph {",
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
                "  }",
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
        lines.extend(
            [
                f"  ac.module @{projection_name}(%input: "
                f"!ac.queue<{_render_type(input_type)}>) -> "
                f"!ac.queue<{_render_type(output_type)}> parameters "
                f"{_render_static_mlir_dictionary(projection_static_arguments)}"
                + _render_interface_display_attributes(
                    (ast.unparse(expression),),
                    ("result",),
                    projection_module_metadata(
                        projection_name,
                        projection_definition,
                        projection_source,
                    ),
                )
                + " graph {",
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
        lines.append(
            f"  ac.module @{symbol}({argument_types})"
            + (f" -> {result_signature}" if definition.outputs else "")
            + " parameters "
            + _render_static_mlir_dictionary(static_arguments)
            + _render_interface_display_attributes(
                tuple(name for name, _ in definition.inputs),
                tuple(name for name, _ in definition.outputs),
                composite_module_metadata(symbol, function.name),
            )
            + " graph {"
        )
        available: dict[str, list[str]] = {}

        def bind_value(name: str, ssa: str) -> None:
            use_count = plan.uses[name]
            if use_count == 1:
                available[name] = [ssa]
                return
            outputs = [f"{name}__fanout{index}" for index in range(use_count)]
            payload = plan.values[name]
            rendered_type = _render_type(payload)
            rendered_outputs = ", ".join(f"%{item}" for item in outputs)
            local_outputs = ", ".join(f"%{item}__local" for item in outputs)
            output_types = ", ".join(
                f"!ac.queue<{rendered_type}>" for _ in outputs
            )
            depths = ", ".join("1" for _ in outputs)
            output_names = "[" + ", ".join(
                canonical_mlir_string(f"{item}__local") for item in outputs
            ) + "]"
            lines.extend(
                [
                    f"    {rendered_outputs} = ac.scope "
                    f"@__ac_fanout_{name}(%{ssa}) {{",
                    f"    ^bb0(%borrowed: !ac.queue<{rendered_type}>):",
                    f"      {local_outputs} = ac.broadcast %borrowed "
                    f"depths [{depths}] latencies [{depths}] "
                    f"{{ac.output_names = {output_names}}} : "
                    f"!ac.queue<{rendered_type}> -> ({output_types})",
                    "      ac.scope.yield "
                    + ", ".join(f"%{item}__local" for item in outputs)
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
                f"{_render_static_mlir_dictionary(instance.static_arguments)} "
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
        lines.append("  }")

    for name, (
        definition,
        program,
        static_arguments,
    ) in rule_module_specializations.items():
        if program is None:
            if name in composite_plans:
                composite_function, _ = composite_specialization_functions[name]
                render_composite_module(
                    name,
                    definition,
                    static_arguments,
                    composite_plans[name],
                    composite_function,
                )
                continue
            argument_types = ", ".join(
                f"!ac.queue<{_render_type(payload)}>"
                for _, payload in definition.inputs
            )
            result_types = ", ".join(
                f"!ac.queue<{_render_type(payload)}>"
                for _, payload in definition.outputs
            )
            function_type = f"({argument_types}) -> " + (
                result_types if len(definition.outputs) == 1 else f"({result_types})"
            )
            source = declaration_specialization_sources[name]
            lines.append(
                f"  ac.module.import @{name} : {function_type} parameters "
                f"{_render_static_mlir_dictionary(static_arguments)} from "
                "{source = " + canonical_mlir_string(source) + "}"
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
                    f"{helper.name!r}; make its typed specialization explicit"
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
                    *(definition_sources.get(program.system, ("", 0, 0))),
                    definition_name=program.system,
                ),
                include_helpers=True,
                helper_names_to_emit=frozenset(helper_names_to_emit),
                definition_locations=dict(definition_locations or {}),
                definition_ndf=definition_ndf,
            )
            .rstrip()
            .splitlines()
        )
    root_result_types = ", ".join(
        f"!ac.queue<{_render_type(payload)}>" for payload in expected_results
    )
    root_result_signature = (
        root_result_types if len(expected_results) == 1 else f"({root_result_types})"
    )
    top_static_parameters = (
        "{"
        + ", ".join(
            f"{name} = {_render_static_mlir_value(value)}"
            for name, value in sorted(system_static_values.items())
        )
        + "}"
    )
    lines.append(
        "  ac.module @Top()"
        + (f" -> {root_result_signature}" if host_results else "")
        + f" parameters {top_static_parameters}"
        + _render_interface_display_attributes(
            (),
            (),
            module_metadata(system),
        )
        + " graph {"
    )
    source_values = [f"%source_{index}" for index in range(len(external))]
    top_values: dict[str, list[str]] = {}

    def bind_top_value(name: str, ssa: str) -> None:
        use_count = uses[name]
        if use_count == 1:
            top_values[name] = [ssa]
            return
        outputs = [f"{name}__fanout{index}" for index in range(use_count)]
        payload = values[name]
        rendered_type = _render_type(payload)
        rendered_outputs = ", ".join(f"%{output}" for output in outputs)
        rendered_local_outputs = ", ".join(
            f"%{output}__local" for output in outputs
        )
        depths = ", ".join("1" for _ in outputs)
        output_types = ", ".join(
            f"!ac.queue<{rendered_type}>" for _ in outputs
        )
        output_names = "[" + ", ".join(
            canonical_mlir_string(f"{output}__local") for output in outputs
        ) + "]"
        lines.append(
            f"    {rendered_outputs} = ac.scope @__ac_fanout_{name}(%{ssa}) {{"
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
            + ", ".join(f"%{output}__local" for output in outputs)
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

    if external:
        result_name = "%inputs"
        suffix = f":{len(external)}" if len(external) > 1 else ""
        lines.append(f"    {result_name}{suffix} = ac.scope @inputs() {{")
        for index, (name, payload) in enumerate(external):
            lines.append(
                f"      {source_values[index]} = ac.source depth 1 latency 1 "
                f'{{ac.name = "{name}"}} : '
                f"!ac.queue<{_render_type(payload)}>"
            )
        lines.append(
            "      ac.scope.yield "
            + ", ".join(source_values)
            + " : "
            + ", ".join(
                f"!ac.queue<{_render_type(payload)}>" for _, payload in external
            )
        )
        lines.append(
            "    } : () -> ("
            + ", ".join(
                f"!ac.queue<{_render_type(payload)}>" for _, payload in external
            )
            + ")"
        )
        for index, (name, _) in enumerate(external):
            bind_top_value(
                name,
                f"inputs#{index}" if len(external) > 1 else "inputs",
            )
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
                f"{_render_static_mlir_dictionary(projection_static_arguments)} "
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
            f"!ac.queue<{_render_type(payload)}>" for payload in input_types
        )
        output_signature = ", ".join(
            f"!ac.queue<{_render_type(payload)}>" for payload in output_types
        )
        result_type = (
            output_signature if len(output_types) == 1 else f"({output_signature})"
        )
        instance_name = "__".join(results) or f"{module_name}_{operation_index}"
        assignment = f"{lhs} = " if lhs else ""
        lines.append(
            f"    {assignment}ac.instance @{instance_name} of @{module_name}"
            f"({operands}) static "
            f"{_render_static_mlir_dictionary(static_arguments)} "
            f'id "{instance_name}" path "{instance_name}" '
            f": ({input_signature}) -> {result_type}"
            + _render_source_frame_location(instance_source)
        )
        for result in results:
            bind_top_value(result, result)
    returned_operands = [f"%{take_top_value(name)}" for name in returned_names]
    if not expected_results:
        lines.append("    ac.return")
    elif host_results:
        lines.append(
            "    ac.return " + ", ".join(returned_operands) + " : " + root_result_types
        )
    else:
        lines.append("    ac.scope @outputs(" + ", ".join(returned_operands) + ") {")
        lines.append(
            "    ^bb0("
            + ", ".join(
                f"%result_{index}: !ac.queue<{_render_type(values[name])}>"
                for index, name in enumerate(returned_names)
            )
            + "):"
        )
        for index, _ in enumerate(returned_names):
            lines.append(
                f'      ac.sink %result_{index} {{ac.name = "sink_{index}"}} '
                f": !ac.queue<{_render_type(expected_results[index])}>"
            )
        lines.extend(
            [
                "      ac.scope.yield",
                "    } : ("
                + ", ".join(
                    f"!ac.queue<{_render_type(payload)}>"
                    for payload in expected_results
                )
                + ") -> ()",
                "    ac.return",
            ]
        )
    lines.extend(["  }", "}"])
    return "\n".join(lines) + "\n"
