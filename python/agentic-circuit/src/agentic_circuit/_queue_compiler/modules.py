"""Structured module specialization and ACIR lowering."""

from __future__ import annotations

import ast
import copy
import re
from collections.abc import Mapping
from dataclasses import dataclass

from _pycircuit_semantics import (
    RangeType,
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
    _epoch_05_integer_width,
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
    _types_equal_in_epoch_05,
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
    modules = {
        node.name: node
        for node in tree.body
        if isinstance(node, ast.FunctionDef)
        and any(
            _decorator_name(decorator).rsplit(".", 1)[-1] == "module"
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
        if annotation is None:
            raise QueueFrontendError(
                "ACPY-MODULE-001: module systems require typed returns"
            )
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
        if annotation is None:
            raise QueueFrontendError(
                "ACPY-MODULE-001: module systems require typed returns"
            )
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
        contains_rule_call = any(
            isinstance(node, ast.Call)
            and isinstance(node.func, ast.Name)
            and node.func.id in rule_names
            for node in ast.walk(function)
        )
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
        if (
            len(function.args.args) != 1
            or function.args.posonlyargs
            or function.args.kwonlyargs
            or function.args.vararg is not None
            or function.args.kwarg is not None
            or function.args.defaults
            or function.args.kw_defaults
            or any(
                isinstance(decorator, ast.Call) for decorator in function.decorator_list
            )
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
                if _epoch_05_integer_width(state_type) is None:
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
    parameter_aliases = _static_parameter_aliases(tree)
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
    rule_module_specializations: dict[
        str,
        tuple[
            RuleModuleDefinition,
            QueueProgram,
            tuple[tuple[str, StaticValue], ...],
        ],
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
        module_name: str, call: ast.Call
    ) -> tuple[
        str,
        tuple[tuple[str, StaticValue], ...],
        tuple[tuple[str, ValueType], ...],
        tuple[tuple[str, ValueType], ...],
    ]:
        template = rule_modules[module_name]
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
                system_static_types,
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
                    expression, StaticEnvironment(system_static_values)
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
            specialized_payloads = {item.name: item for item in program.payloads}
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
        definition, _, _ = rule_module_specializations[symbol]
        return symbol, frozen, definition.inputs, definition.outputs

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
            isinstance(statement, ast.Assign)
            and len(statement.targets) == 1
            and isinstance(statement.targets[0], (ast.Name, ast.Tuple, ast.List))
            and isinstance(statement.value, ast.Call)
            and isinstance(statement.value.func, ast.Name)
            and statement.value.func.id in modules
            and all(isinstance(argument, ast.Name) for argument in statement.value.args)
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
            sources = tuple(
                argument.id
                for argument in statement.value.args
                if isinstance(argument, ast.Name)
            )
            if len(sources) != len(input_signature) or len(results) != len(
                output_signature
            ):
                raise QueueFrontendError(
                    "ACPY-MODULE-002: module call arity does not match its signature"
                )
            if any(result in values for result in results) or any(
                source not in values for source in sources
            ):
                raise QueueFrontendError(
                    "ACPY-MODULE-002: module call values must be defined once"
                )
            if any(
                not _types_equal_in_epoch_05(values[source], expected_type)
                for source, (_, expected_type) in zip(
                    sources, input_signature, strict=True
                )
            ):
                raise QueueFrontendError(
                    "ACPY-MODULE-002: module input payload type mismatch"
                )
            for source in sources:
                uses[source] = uses.get(source, 0) + 1
            output_types = tuple(payload for _, payload in output_signature)
            for result, output_type in zip(results, output_types, strict=True):
                values[result] = output_type
                uses[result] = 0
            if module_name not in rule_modules and statement.value.keywords:
                raise QueueFrontendError(
                    "ACPY-MODULE-007: pure module static parameters are not implemented"
                )
            instances.append(
                (
                    results,
                    instance_module_name,
                    sources,
                    output_types,
                    instance_static_arguments,
                    source_frame(statement.value),
                )
            )
            continue
        if isinstance(statement, ast.Return) and statement.value is not None:
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
    if (
        returned_names is None
        or len(returned_names) != len(expected_results)
        or any(
            not _types_equal_in_epoch_05(values[name], expected)
            for name, expected in zip(returned_names, expected_results, strict=True)
        )
    ):
        raise QueueFrontendError(
            "ACPY-MODULE-002: module system return type or arity mismatch"
        )
    if any(count != 1 for count in uses.values()):
        raise QueueFrontendError(
            "ACPY-MODULE-002: every module Queue value requires one consumer"
        )

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
        {binding.root for binding in top_static_configs},
    )
    candidate_static_configs = {binding.root: binding for binding in top_static_configs}
    all_interface_checks = list(system_interface_checks)
    for _, program, _ in rule_module_specializations.values():
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
    all_checks = [
        *(check for payload in all_payloads for check in payload.static_type_checks),
        *all_interface_checks,
    ]
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

    lines = [
        "builtin.module attributes {"
        'ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle"'
        + _render_static_type_attributes(
            tuple(sorted(all_static_bindings.items())),
            all_payloads,
            tuple(all_interface_checks),
            tuple(
                candidate_static_configs[root]
                for root in sorted(candidate_static_configs)
            ),
        )
        + "} {"
    ]
    if all_payloads or enum_bindings or bitfield_bindings:
        lines.append("  ac.type_scope @types {")
        for enumeration in enum_bindings:
            lines.append(_render_enum(enumeration, "    "))
        for payload in all_payloads:
            fields = ", ".join(
                f'{{name = "{field}", type = {typ}}}' for field, typ in payload.fields
            )
            lines.append(
                f"    ac.struct @{payload.descriptor.symbol} fields [{fields}]"
            )
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
    for helper in helper_definitions:
        arguments = ", ".join(
            f"%{name}: !ac.var<{_render_type(value_type)}>"
            for name, value_type in helper.arguments
        )
        attributes = " attributes {ac.inline = true}" if helper.inline else ""
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
    for name, definition in module_types.items():
        argument = definition.argument
        input_type = definition.input_type
        output_type = definition.output_type
        expression = definition.expression
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
            if not _types_equal_in_epoch_05(
                child_input, input_type
            ) or not _types_equal_in_epoch_05(child_output, output_type):
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
                if not _types_equal_in_epoch_05(value_type, state_type):
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
            if not _types_equal_in_epoch_05(value_type, output_type):
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
        )
        value, value_type = emitter.emit(expression, output_type)
        if not _types_equal_in_epoch_05(value_type, output_type):
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
    for name, (
        definition,
        program,
        static_arguments,
    ) in rule_module_specializations.items():
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
                ),
                include_helpers=False,
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
    top_values: dict[str, str] = {}
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
            top_values[name] = f"%inputs#{index}" if len(external) > 1 else "%inputs"
    for (
        results,
        module_name,
        sources,
        output_types,
        static_arguments,
        instance_source,
    ) in instances:
        input_types = tuple(values[source] for source in sources)
        lhs = ", ".join(f"%{result}" for result in results)
        operands = ", ".join(top_values[source] for source in sources)
        input_signature = ", ".join(
            f"!ac.queue<{_render_type(payload)}>" for payload in input_types
        )
        output_signature = ", ".join(
            f"!ac.queue<{_render_type(payload)}>" for payload in output_types
        )
        result_type = (
            output_signature if len(output_types) == 1 else f"({output_signature})"
        )
        instance_name = "__".join(results)
        lines.append(
            f"    {lhs} = ac.instance @{instance_name} of @{module_name}"
            f"({operands}) static "
            f"{_render_static_mlir_dictionary(static_arguments)} "
            f'id "{instance_name}" path "{instance_name}" '
            f": ({input_signature}) -> {result_type}"
            + _render_source_frame_location(instance_source)
        )
        for result in results:
            top_values[result] = f"%{result}"
    returned_operands = [top_values[name] for name in returned_names]
    if host_results:
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
