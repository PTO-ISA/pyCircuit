"""Static configuration and type analysis for the Queue frontend."""

from __future__ import annotations

import ast
import re
from collections.abc import Collection, Mapping
from dataclasses import dataclass, replace

from _pycircuit_semantics import (
    ArrayType,
    BitsType,
    BoolType,
    Constant,
    Constraint,
    StructType,
    TupleType,
    Unknown,
    ValueType,
    is_primitive_input_width,
    prove_within,
)

from .._canonical_json import canonical_json_bytes, sha256_bytes
from .._static_eval import (
    FrozenMap,
    StaticEnvironment,
    StaticValue,
    evaluate_static,
    static_json_value,
)
from .errors import QueueFrontendError
from .model import (
    BitfieldBinding,
    EnumBinding,
    Payload,
    StaticConfigBinding,
    StaticTypeCheck,
)
from .syntax import _decorator_name

MAX_PACKED_VALUE_WIDTH = 1 << 16


def _table_axis_width(extent: int) -> int:
    return max(1, (extent - 1).bit_length())


def _product(values: tuple[int, ...]) -> int:
    result = 1
    for value in values:
        result *= value
    return result


def _static_constraint(
    node: ast.expr, values: Mapping[str, StaticValue] | None = None
) -> Constraint:
    """Return an exact frontend fact when closed static evaluation succeeds."""

    if isinstance(node, ast.Constant) and type(node.value) in {bool, int, str}:
        return Constant(node.value)
    try:
        value = evaluate_static(node, StaticEnvironment(values or {}))
    except ValueError:
        return Unknown()
    if type(value) in {bool, int, str}:
        return Constant(value)
    return Unknown()


def _constant_integer(
    node: ast.expr, values: Mapping[str, StaticValue] | None = None
) -> int | None:
    if isinstance(node, ast.BinOp) and isinstance(
        node.op, (ast.Add, ast.Sub, ast.Mult)
    ):
        left = _constant_integer(node.left, values)
        right = _constant_integer(node.right, values)
        if left is None or right is None:
            return None
        result = (
            left + right
            if isinstance(node.op, ast.Add)
            else left - right
            if isinstance(node.op, ast.Sub)
            else left * right
        )
        return result if -(1 << 63) <= result <= (1 << 63) - 1 else None
    if isinstance(node, ast.Call) and len(node.args) == 1 and not node.keywords:
        helper = (
            node.func.attr
            if isinstance(node.func, ast.Attribute)
            else node.func.id
            if isinstance(node.func, ast.Name)
            else ""
        )
        if helper in {"index_width", "count_width"}:
            operand = _constant_integer(node.args[0], values)
            if operand is None:
                return None
            from _pycircuit_semantics import count_width, index_width

            try:
                result = (
                    index_width(operand)
                    if helper == "index_width"
                    else count_width(operand)
                )
            except ValueError:
                return None
            assert isinstance(result, int)
            return result
    fact = _static_constraint(node, values)
    if isinstance(fact, Constant) and type(fact.value) is int:
        return fact.value
    return None


def _dependent_static_type_expression(
    node: ast.expr,
    parameter_aliases: Mapping[str, StaticParameterAlias],
    values: Mapping[str, StaticValue] | None,
    *,
    binding_namespace: str = "",
) -> tuple[tuple[str, ...], int] | None:
    """Parse, evaluate, and serialize one dependent type expression.

    Expressions without a declared ``ac.param`` reference return ``None`` so
    ordinary compile-time constants keep using the established static evaluator.
    Once a parameter is referenced, the closed Decision 0246 grammar is
    authoritative and unsupported Python syntax fails closed.
    """

    if not any(
        isinstance(item, ast.Name) and item.id in parameter_aliases
        for item in ast.walk(node)
    ):
        return None

    from _pycircuit_semantics import StaticIntExpression, StaticIntExpressionError

    static_values = values or {}

    def config_path(item: ast.expr) -> tuple[str, tuple[str, ...]] | None:
        fields: list[str] = []
        current = item
        while isinstance(current, ast.Attribute):
            fields.append(current.attr)
            current = current.value
        if not isinstance(current, ast.Name) or current.id not in parameter_aliases:
            return None
        binding = parameter_aliases[current.id]
        if binding.config_type is None:
            return None
        return current.id, tuple(reversed(fields))

    def parse(item: ast.expr) -> int | StaticIntExpression:
        if isinstance(item, ast.Constant) and type(item.value) is int:
            return item.value
        projected = config_path(item)
        if projected is not None:
            alias, fields = projected
            if not fields:
                raise QueueFrontendError(
                    "ACPY-TYPE-008: dependent type config references must select "
                    "an integer field"
                )
            binding = parameter_aliases[alias]
            value = _project_static_config_value(
                static_values.get(alias),
                fields,
                binding.external_name,
            )
            if type(value) is not int:
                raise QueueFrontendError(
                    "ACPY-TYPE-008: config projection "
                    f"{binding.external_name + '.' + '.'.join(fields)!r} "
                    "must resolve to an integer"
                )
            parameter = binding.external_name + "." + ".".join(fields)
            return StaticIntExpression.parameter(binding_namespace + parameter)
        if isinstance(item, ast.Name):
            if item.id in parameter_aliases:
                binding = parameter_aliases[item.id]
                if binding.config_type is not None:
                    raise QueueFrontendError(
                        "ACPY-TYPE-008: dependent type config references must "
                        "select an integer field"
                    )
                return StaticIntExpression.parameter(binding_namespace + item.id)
            value = static_values.get(item.id)
            if type(value) is int:
                return value
            raise QueueFrontendError(
                "ACPY-TYPE-008: dependent type expressions may reference only "
                "declared integer parameters, integer literals, and closed integer constants"
            )
        if isinstance(item, ast.BinOp) and isinstance(
            item.op, (ast.Add, ast.Sub, ast.Mult)
        ):
            left = parse(item.left)
            right = parse(item.right)
            if isinstance(item.op, ast.Add):
                return StaticIntExpression("add", (left, right))
            if isinstance(item.op, ast.Sub):
                return StaticIntExpression("sub", (left, right))
            return StaticIntExpression("mul", (left, right))
        if isinstance(item, ast.Call) and len(item.args) == 1 and not item.keywords:
            helper = _decorator_name(item.func).rsplit(".", 1)[-1]
            operand = parse(item.args[0])
            if helper == "index_width":
                return StaticIntExpression("index_width", (operand,))
            if helper == "count_width":
                return StaticIntExpression("count_width", (operand,))
        raise QueueFrontendError(
            "ACPY-TYPE-008: dependent type expressions admit only literals, "
            "declared parameters, +, -, *, index_width, and count_width"
        )

    try:
        expression = parse(node)
        if type(expression) is int:
            raise QueueFrontendError(
                "ACPY-TYPE-008: dependent type expression lost its parameter reference"
            )
        bindings = {
            token[6:]: _static_parameter_value(
                token[6:],
                parameter_aliases,
                static_values,
                binding_namespace=binding_namespace,
            )
            for token in expression.postfix()
            if token[:6] == "param:"
        }
        result = expression.evaluate(bindings)
        return expression.postfix(), result
    except StaticIntExpressionError as error:
        raise QueueFrontendError(f"ACPY-TYPE-008: {error}") from error


def _proven_integer_in(value: int, lower: int, upper: int) -> bool:
    """Use the shared bounded domain for concrete shape/bound checks."""

    return prove_within(Constant(value), lower, upper)


def _is_epoch_05_bool_compatible(value_type: ValueType) -> bool:
    """Preserve the accepted current-contract i1 condition boundary.

    Bool and u1 retain distinct descriptor identities; this predicate exists
    only where the current ACIR contract historically accepts either i1 view.
    """

    return isinstance(value_type, BoolType) or (
        isinstance(value_type, BitsType) and value_type.bit_width() == 1
    )


def _types_equal_in_epoch_05(left: ValueType, right: ValueType) -> bool:
    """Compare semantic types at the current ACIR rendering boundary."""

    return left == right or (
        _is_epoch_05_bool_compatible(left) and _is_epoch_05_bool_compatible(right)
    )


def _epoch_05_integer_width(value_type: ValueType) -> int | None:
    """Return the width accepted by the current integer boundary."""

    from _pycircuit_semantics import RangeType

    if isinstance(value_type, BitsType | RangeType):
        return value_type.width
    if isinstance(value_type, BoolType):
        return 1
    return None


def _scalar_reset_init(value_type: ValueType, init: object, *, code: str) -> int | bool:
    """Accept a constant register reset image for scalar persistent state."""

    from _pycircuit_semantics import RangeType

    if isinstance(value_type, BoolType):
        if type(init) is not bool:
            raise QueueFrontendError(f"{code}: bool variable requires bool init")
        return init
    if type(init) is not int:
        raise QueueFrontendError(f"{code}: integer variable requires integer init")
    if isinstance(value_type, RangeType):
        if not value_type.lower <= init < value_type.upper:
            raise QueueFrontendError(
                f"{code}: range state init is outside declared bounds"
            )
        return init
    width = _epoch_05_integer_width(value_type)
    if width is None:
        raise QueueFrontendError(f"{code}: persistent scalar init requires bits")
    if init < 0 or init >= (1 << width):
        raise QueueFrontendError(
            f"{code}: reset init {init} does not fit {width}-bit state"
        )
    return init


def _candidate_mask_type(entries: int) -> ValueType:
    """Represent compiler-owned candidate sets without widening public bits."""

    if entries <= 64:
        return BitsType(entries)
    return ArrayType((entries + 63) // 64, BitsType(64))


def _contains_declared_range(value_type: ValueType) -> bool:
    from _pycircuit_semantics import RangeType

    if isinstance(value_type, RangeType):
        return True
    if isinstance(value_type, StructType):
        return any(_contains_declared_range(field.type) for field in value_type.fields)
    if isinstance(value_type, TupleType):
        return any(_contains_declared_range(element) for element in value_type.elements)
    if isinstance(value_type, ArrayType):
        return _contains_declared_range(value_type.element)
    return False


def _module_static_values(tree: ast.Module) -> dict[str, StaticValue]:
    """Collect immutable module constants admitted by the source closure."""

    values: dict[str, StaticValue] = {}
    for statement in tree.body:
        name: str | None = None
        expression: ast.expr | None = None
        if (
            isinstance(statement, ast.Assign)
            and len(statement.targets) == 1
            and isinstance(statement.targets[0], ast.Name)
        ):
            name = statement.targets[0].id
            expression = statement.value
        elif (
            isinstance(statement, ast.AnnAssign)
            and isinstance(statement.target, ast.Name)
            and statement.value is not None
        ):
            name = statement.target.id
            expression = statement.value
        if name is None or expression is None or not name.isupper():
            continue
        try:
            value = evaluate_static(expression, StaticEnvironment(values))
        except ValueError:
            continue
        if type(value) not in {bool, int}:
            continue
        if name in values and values[name] != value:
            raise QueueFrontendError(
                f"ACPY-QUEUE-027: module constant {name!r} is ambiguous"
            )
        values[name] = value
    return values


@dataclass(frozen=True, slots=True)
class StaticParameterAlias:
    external_name: str
    config_type: str | None = None


def _config_type_names(tree: ast.Module) -> set[str]:
    return {
        node.name
        for node in tree.body
        if isinstance(node, ast.ClassDef)
        and any(
            _decorator_name(decorator).rsplit(".", 1)[-1] == "config"
            for decorator in node.decorator_list
        )
    }


def _static_config_expression_type(
    node: ast.expr,
    root_types: Mapping[str, str],
    tree: ast.Module,
) -> str | None:
    if isinstance(node, ast.Name):
        return root_types.get(node.id)
    if not isinstance(node, ast.Attribute):
        return None
    owner_type = _static_config_expression_type(node.value, root_types, tree)
    if owner_type is None or owner_type not in _config_type_names(tree):
        return None
    declaration = next(
        (
            candidate
            for candidate in tree.body
            if isinstance(candidate, ast.ClassDef) and candidate.name == owner_type
        ),
        None,
    )
    if declaration is None:
        return None
    for statement in declaration.body:
        if (
            isinstance(statement, ast.AnnAssign)
            and isinstance(statement.target, ast.Name)
            and statement.target.id == node.attr
        ):
            return _decorator_name(statement.annotation).rsplit(".", 1)[-1]
    return None


def _static_parameter_aliases(tree: ast.Module) -> dict[str, StaticParameterAlias]:
    """Collect typed ``NAME = ac.param[T]("argument")`` declarations."""

    aliases: dict[str, StaticParameterAlias] = {}
    external_names: set[str] = set()
    config_types = _config_type_names(tree)
    for statement in tree.body:
        if not (
            isinstance(statement, ast.Assign)
            and len(statement.targets) == 1
            and isinstance(statement.targets[0], ast.Name)
            and isinstance(statement.value, ast.Call)
            and isinstance(statement.value.func, ast.Subscript)
        ):
            continue
        target = statement.targets[0].id
        declaration = statement.value
        family = declaration.func
        family_name = (
            family.value.attr
            if isinstance(family.value, ast.Attribute)
            else family.value.id
            if isinstance(family.value, ast.Name)
            else ""
        )
        parameter_type = family.slice.id if isinstance(family.slice, ast.Name) else ""
        if family_name != "param":
            continue
        if (
            not target.isupper()
            or (parameter_type != "int" and parameter_type not in config_types)
            or len(declaration.args) != 1
            or declaration.keywords
            or not isinstance(declaration.args[0], ast.Constant)
            or type(declaration.args[0].value) is not str
            or not declaration.args[0].value
        ):
            raise QueueFrontendError(
                "ACPY-TYPE-008: static type parameters require "
                'UPPER_SNAKE = ac.param[int | Config]("const_argument")'
            )
        external = declaration.args[0].value
        if target in aliases or external in external_names:
            raise QueueFrontendError(
                "ACPY-TYPE-008: static type parameter names and bindings must be unique"
            )
        aliases[target] = StaticParameterAlias(
            external,
            None if parameter_type == "int" else parameter_type,
        )
        external_names.add(external)
    return aliases


def _validate_static_config_roots(
    function: ast.FunctionDef,
    aliases: Mapping[str, StaticParameterAlias],
    used_roots: Collection[str],
    *,
    binding_namespace: str = "",
) -> None:
    parameters = {
        parameter.arg: parameter
        for parameter in (
            *function.args.posonlyargs,
            *function.args.args,
            *function.args.kwonlyargs,
        )
    }
    for binding in aliases.values():
        if binding.config_type is None:
            continue
        if binding_namespace + binding.external_name not in used_roots:
            continue
        parameter = parameters.get(binding.external_name)
        annotation = None if parameter is None else parameter.annotation
        actual_type = (
            _decorator_name(annotation.slice).rsplit(".", 1)[-1]
            if isinstance(annotation, ast.Subscript)
            and _decorator_name(annotation.value).rsplit(".", 1)[-1] == "const"
            else ""
        )
        if actual_type != binding.config_type:
            raise QueueFrontendError(
                "ACPY-TYPE-008: typed config root "
                f"{binding.external_name!r} requires matching "
                f"ac.const[{binding.config_type}] entry parameter"
            )


def _project_static_config_value(
    root: StaticValue | None,
    fields: tuple[str, ...],
    external_name: str,
) -> StaticValue:
    if root is None:
        raise QueueFrontendError(
            f"ACPY-TYPE-008: unbound static config parameter {external_name!r}"
        )
    value = root
    traversed: list[str] = [external_name]
    for field in fields:
        if not isinstance(value, FrozenMap):
            raise QueueFrontendError(
                "ACPY-TYPE-008: config projection "
                f"{'.'.join(traversed)!r} is not a nested config record"
            )
        try:
            value = value[field]
        except KeyError as error:
            raise QueueFrontendError(
                f"ACPY-TYPE-008: unknown config field {field!r} on "
                f"{'.'.join(traversed)!r}"
            ) from error
        traversed.append(field)
    return value


def _static_parameter_value(
    parameter: str,
    aliases: Mapping[str, StaticParameterAlias],
    values: Mapping[str, StaticValue],
    *,
    binding_namespace: str = "",
) -> int:
    canonical = (
        parameter[len(binding_namespace) :]
        if binding_namespace
        and parameter[: len(binding_namespace)] == binding_namespace
        else parameter
    )
    for alias, binding in aliases.items():
        if binding.config_type is None:
            if canonical != alias:
                continue
            value = values.get(alias)
            if value is None:
                raise QueueFrontendError(
                    f"ACPY-TYPE-008: unbound static integer parameter {canonical!r}"
                )
        else:
            prefix = binding.external_name + "."
            if canonical[: len(prefix)] != prefix:
                continue
            value = _project_static_config_value(
                values.get(alias),
                tuple(canonical[len(prefix) :].split(".")),
                binding.external_name,
            )
        if type(value) is not int:
            raise QueueFrontendError(
                f"ACPY-TYPE-008: static type parameter {canonical!r} "
                "must resolve to an integer"
            )
        return value
    raise QueueFrontendError(
        f"ACPY-TYPE-008: unknown static type parameter {canonical!r}"
    )


def _static_type_bindings_for_checks(
    checks: Collection[StaticTypeCheck],
    aliases: Mapping[str, StaticParameterAlias],
    values: Mapping[str, StaticValue],
    *,
    binding_namespace: str = "",
) -> tuple[tuple[str, int], ...]:
    parameters = {
        token[6:]
        for check in checks
        for token in check.program
        if token[:6] == "param:"
    }
    return tuple(
        (
            parameter,
            _static_parameter_value(
                parameter,
                aliases,
                values,
                binding_namespace=binding_namespace,
            ),
        )
        for parameter in sorted(parameters)
    )


def _config_schema_document(
    tree: ast.Module,
    type_name: str,
    active: tuple[str, ...] = (),
) -> dict[str, object]:
    declarations = {
        node.name: node
        for node in tree.body
        if isinstance(node, ast.ClassDef) and node.name in _config_type_names(tree)
    }
    node = declarations.get(type_name)
    if node is None:
        raise QueueFrontendError(
            f"ACPY-TYPE-008: config type {type_name!r} is unavailable"
        )
    if type_name in active:
        cycle = " -> ".join((*active, type_name))
        raise QueueFrontendError(
            f"ACPY-TYPE-008: recursive config schema is unsupported: {cycle}"
        )
    fields: list[dict[str, object]] = []
    for statement in node.body:
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
                "ACPY-TYPE-008: config schemas require annotated data fields"
            )
        annotation = _decorator_name(statement.annotation).rsplit(".", 1)[-1]
        if annotation in declarations:
            field_type: object = _config_schema_document(
                tree,
                annotation,
                (*active, type_name),
            )
        else:
            annotation_text = ast.unparse(statement.annotation)
            if annotation_text not in {"int", "bool", "float", "str"}:
                raise QueueFrontendError(
                    "ACPY-TYPE-008: typed config roots support only int, bool, "
                    "float, str, or nested @ac.config fields"
                )
            field_type = {
                "kind": "scalar",
                "name": annotation_text,
                "version": 1,
            }
        fields.append({"name": statement.target.id, "type": field_type})
    if not fields or len({field["name"] for field in fields}) != len(fields):
        raise QueueFrontendError(
            "ACPY-TYPE-008: config schemas require unique annotated fields"
        )
    return {
        "fields": fields,
        "kind": "config",
        "name": type_name,
        "version": 1,
    }


def _static_config_bindings_for_checks(
    tree: ast.Module,
    checks: Collection[StaticTypeCheck],
    aliases: Mapping[str, StaticParameterAlias],
    values: Mapping[str, StaticValue],
    *,
    binding_namespace: str = "",
) -> tuple[StaticConfigBinding, ...]:
    parameters = {
        token[6:]
        for check in checks
        for token in check.program
        if token[:6] == "param:"
    }
    result: list[StaticConfigBinding] = []
    for alias, binding in aliases.items():
        if binding.config_type is None:
            continue
        root = binding_namespace + binding.external_name
        if not any(
            parameter[: len(root) + 1] == root + "." for parameter in parameters
        ):
            continue
        value = values.get(alias)
        if not isinstance(value, FrozenMap):
            raise QueueFrontendError(
                f"ACPY-TYPE-008: unbound static config parameter {root!r}"
            )
        schema = canonical_json_bytes(
            _config_schema_document(tree, binding.config_type)
        ).decode("utf-8")
        result.append(
            StaticConfigBinding(
                root=root,
                type_name=binding.config_type,
                schema=schema,
                schema_sha256=sha256_bytes(schema.encode("utf-8")),
                value=canonical_json_bytes(static_json_value(value)).decode("utf-8"),
            )
        )
    return tuple(sorted(result, key=lambda binding: binding.root))


def _type_static_values(
    tree: ast.Module,
    static_arguments: Mapping[str, StaticValue] | None,
) -> dict[str, StaticValue]:
    values = _module_static_values(tree)
    supplied = dict(static_arguments or {})
    for alias, binding in _static_parameter_aliases(tree).items():
        if binding.external_name not in supplied:
            continue
        value = supplied[binding.external_name]
        if binding.config_type is None and type(value) is not int:
            raise QueueFrontendError(
                "ACPY-TYPE-008: static type parameter "
                f"{binding.external_name!r} must bind an integer"
            )
        if binding.config_type is not None and not isinstance(value, FrozenMap):
            raise QueueFrontendError(
                "ACPY-TYPE-008: static config parameter "
                f"{binding.external_name!r} must bind an @ac.config record"
            )
        values[alias] = value
    return values


def _primitive_integer_width(operation: str, value_type: ValueType) -> int:
    """Apply the shared exact-width contract for scalar value primitives."""

    width = _epoch_05_integer_width(value_type)
    if width is None:
        raise QueueFrontendError(
            "ACPY-VAR-003", f"{operation} operand must be an integer payload"
        )
    if not is_primitive_input_width(width):
        raise QueueFrontendError(
            "ACPY-VAR-003", f"{operation} operand width must be in [1, 64]"
        )
    return width


def _scalar_annotation_static_check(
    target: str,
    annotation: ast.expr,
    parameter_aliases: Mapping[str, StaticParameterAlias],
    static_values: Mapping[str, StaticValue],
    *,
    binding_namespace: str = "",
) -> StaticTypeCheck | None:
    if not (
        isinstance(annotation, ast.Subscript)
        and _decorator_name(annotation.value).rsplit(".", 1)[-1] == "bits"
    ):
        return None
    dependent = _dependent_static_type_expression(
        annotation.slice,
        parameter_aliases,
        static_values,
        binding_namespace=binding_namespace,
    )
    if dependent is None:
        return None
    program, width = dependent
    if not _proven_integer_in(width, 1, 64):
        raise QueueFrontendError("ACPY-TYPE-003: bits width must be in [1, 64]")
    return StaticTypeCheck(target + ":bits", program, width, BitsType(width))


def _bounded_annotation_static_checks(
    target: str,
    annotation: ast.expr,
    parameter_aliases: Mapping[str, StaticParameterAlias],
    static_values: Mapping[str, StaticValue],
    *,
    binding_namespace: str = "",
) -> tuple[StaticTypeCheck, ...]:
    if not isinstance(annotation, ast.Subscript):
        return ()
    kind = _decorator_name(annotation.value).rsplit(".", 1)[-1]
    if kind not in {"index", "range"}:
        return ()
    bounds = (
        (ast.Constant(0), annotation.slice)
        if kind == "index"
        else (
            tuple(annotation.slice.elts)
            if isinstance(annotation.slice, ast.Tuple)
            else ()
        )
    )
    if len(bounds) != 2:
        return ()
    concrete = _scalar_type_descriptor(annotation, static_values)
    checks: list[StaticTypeCheck] = []
    for suffix, bound in zip(("range_lower", "range_upper"), bounds, strict=True):
        dependent = _dependent_static_type_expression(
            bound,
            parameter_aliases,
            static_values,
            binding_namespace=binding_namespace,
        )
        if dependent is not None:
            program, result = dependent
            checks.append(
                StaticTypeCheck(f"{target}:{suffix}", program, result, concrete)
            )
    return tuple(checks)


def _scalar_type_descriptor(
    node: ast.expr,
    static_values: Mapping[str, StaticValue] | None = None,
) -> ValueType:
    from _pycircuit_semantics import BitsType, BoolType, RangeType

    if (
        isinstance(node, ast.Subscript)
        and _decorator_name(node.value).rsplit(".", 1)[-1] == "bits"
    ):
        width_node = node.slice
        width = _constant_integer(width_node, static_values)
        if width is None:
            raise QueueFrontendError(
                "ACPY-TYPE-003: bits width must be a static integer"
            )
        if not _proven_integer_in(width, 1, 64):
            raise QueueFrontendError("ACPY-TYPE-003: bits width must be in [1, 64]")
        return BitsType(width)
    if isinstance(node, ast.Subscript):
        bounded_kind = _decorator_name(node.value).rsplit(".", 1)[-1]
        if bounded_kind in {"index", "range"}:
            bounds = (
                (ast.Constant(0), node.slice)
                if bounded_kind == "index"
                else (
                    tuple(node.slice.elts) if isinstance(node.slice, ast.Tuple) else ()
                )
            )
            if len(bounds) != 2:
                raise QueueFrontendError(
                    "ACPY-TYPE-009: range requires two static bounds"
                )
            lower = _constant_integer(bounds[0], static_values)
            upper = _constant_integer(bounds[1], static_values)
            if lower is None or upper is None:
                raise QueueFrontendError(
                    "ACPY-TYPE-009: bounded type bounds must be static integers"
                )
            try:
                return RangeType(lower, upper)
            except ValueError as error:
                raise QueueFrontendError(f"ACPY-TYPE-009: {error}") from error
    name = _decorator_name(node).rsplit(".", 1)[-1]
    if name == "int":
        return BitsType(64)
    if name == "bool":
        return BoolType()
    unsigned = re.fullmatch(r"u([0-9]+)", name)
    if unsigned is not None:
        width = int(unsigned.group(1))
        if 1 <= width <= 64:
            return BitsType(width)
        raise QueueFrontendError("ACPY-QUEUE-002: bit width must be in [1, 64]")
    widths = {
        "s8": 8,
        "s16": 16,
        "s32": 32,
        "s64": 64,
    }
    if name in widths:
        return BitsType(widths[name])
    raise QueueFrontendError("ACPY-QUEUE-002: unsupported field type")


def _enums(tree: ast.Module) -> tuple[EnumBinding, ...]:
    from _pycircuit_semantics import EnumType

    result: list[EnumBinding] = []
    names: set[str] = set()
    for node in tree.body:
        if not isinstance(node, ast.ClassDef) or not any(
            _decorator_name(base).rsplit(".", 1)[-1] == "Enum" for base in node.bases
        ):
            continue
        if node.name in names:
            raise QueueFrontendError(
                f"ACPY-TYPE-005: enum {node.name!r} is defined more than once"
            )
        enumerants: list[str] = []
        values: list[int] = []
        encoding_width: int | None = None
        for decorator in node.decorator_list:
            if (
                not isinstance(decorator, ast.Call)
                or _decorator_name(decorator.func).rsplit(".", 1)[-1] != "encoding"
                or decorator.args
                or len(decorator.keywords) != 1
                or decorator.keywords[0].arg != "width"
            ):
                raise QueueFrontendError(
                    "ACPY-TYPE-005: enum decorators require @ac.encoding(width=N)"
                )
            encoding_width = _constant_integer(decorator.keywords[0].value)
            if encoding_width is None or not 1 <= encoding_width <= 64:
                raise QueueFrontendError(
                    "ACPY-TYPE-005: enum encoding width must be in [1, 64]"
                )
        for statement in node.body:
            if (
                isinstance(statement, ast.Expr)
                and isinstance(statement.value, ast.Constant)
                and type(statement.value.value) is str
            ):
                continue
            if (
                not isinstance(statement, ast.Assign)
                or len(statement.targets) != 1
                or not isinstance(statement.targets[0], ast.Name)
                or not isinstance(statement.value, ast.Constant)
                or type(statement.value.value) is not int
            ):
                raise QueueFrontendError(
                    "ACPY-TYPE-005: enum body requires integer member assignments"
                )
            enumerants.append(statement.targets[0].id)
            values.append(statement.value.value)
        if encoding_width is None and values != list(range(len(values))):
            raise QueueFrontendError(
                "ACPY-TYPE-005: enum values must be contiguous from zero in declaration order"
            )
        if encoding_width is not None and (
            any(value < 0 or value >= (1 << encoding_width) for value in values)
            or len(set(values)) != len(values)
        ):
            raise QueueFrontendError(
                "ACPY-TYPE-005: explicit enum values must be unique, nonnegative, "
                "and fit the declared width"
            )
        if len(set(enumerants)) != len(enumerants):
            raise QueueFrontendError("ACPY-TYPE-005: enum member names must be unique")
        descriptor = EnumType(
            node.name,
            tuple(enumerants),
            None if encoding_width is None else tuple(values),
            encoding_width,
        )
        names.add(node.name)
        result.append(EnumBinding(node.name, descriptor))
    return tuple(result)


def _payloads(
    tree: ast.Module,
    enums: tuple[EnumBinding, ...] = (),
    static_values: Mapping[str, StaticValue] | None = None,
    *,
    static_type_namespace: str = "",
    allow_unbound: bool = False,
) -> tuple[Payload, ...]:
    from _pycircuit_semantics import ArrayType, StructType, TupleType, ValueField

    declarations = [
        node
        for node in tree.body
        if isinstance(node, ast.ClassDef)
        and any(
            _decorator_name(item).rsplit(".", 1)[-1] == "struct"
            for item in node.decorator_list
        )
    ]
    by_name = {node.name: node for node in declarations}
    if len(by_name) != len(declarations):
        raise QueueFrontendError("ACPY-TYPE-004: struct names must be unique")
    resolved: dict[str, StructType] = {}
    active: list[str] = []
    enum_types = {binding.name: binding.descriptor for binding in enums}
    parameter_aliases = _static_parameter_aliases(tree)
    checks_by_name: dict[str, tuple[StaticTypeCheck, ...]] = {}

    def annotation_checks(
        struct_name: str, field_name: str, node: ast.expr
    ) -> tuple[StaticTypeCheck, ...]:
        checks: list[StaticTypeCheck] = []

        def visit(annotation: ast.expr, path: str) -> None:
            if not isinstance(annotation, ast.Subscript):
                return
            kind = _decorator_name(annotation.value).rsplit(".", 1)[-1]
            if kind == "bits":
                dependent = _dependent_static_type_expression(
                    annotation.slice,
                    parameter_aliases,
                    static_values,
                    binding_namespace=static_type_namespace,
                )
                if dependent is not None:
                    program, result = dependent
                    checks.append(
                        StaticTypeCheck(
                            f"{struct_name}.{field_name}{path}:bits",
                            program,
                            result,
                        )
                    )
                return
            if kind in {"index", "range"}:
                bounds = (
                    (ast.Constant(0), annotation.slice)
                    if kind == "index"
                    else (
                        tuple(annotation.slice.elts)
                        if isinstance(annotation.slice, ast.Tuple)
                        else ()
                    )
                )
                if len(bounds) != 2:
                    return
                for bound_name, bound_node in zip(
                    ("range_lower", "range_upper"), bounds, strict=True
                ):
                    dependent = _dependent_static_type_expression(
                        bound_node,
                        parameter_aliases,
                        static_values,
                        binding_namespace=static_type_namespace,
                    )
                    if dependent is not None:
                        program, result = dependent
                        checks.append(
                            StaticTypeCheck(
                                f"{struct_name}.{field_name}{path}:{bound_name}",
                                program,
                                result,
                            )
                        )
                return
            if kind == "array":
                if (
                    not isinstance(annotation.slice, ast.Tuple)
                    or len(annotation.slice.elts) != 2
                ):
                    return
                length, element = annotation.slice.elts
                dependent = _dependent_static_type_expression(
                    length,
                    parameter_aliases,
                    static_values,
                    binding_namespace=static_type_namespace,
                )
                if dependent is not None:
                    program, result = dependent
                    checks.append(
                        StaticTypeCheck(
                            f"{struct_name}.{field_name}{path}:array_length",
                            program,
                            result,
                        )
                    )
                visit(element, path + ".array_element")
                return
            if kind in {"tuple", "Tuple"}:
                elements = (
                    tuple(annotation.slice.elts)
                    if isinstance(annotation.slice, ast.Tuple)
                    else (annotation.slice,)
                )
                for index, element in enumerate(elements):
                    visit(element, path + f".tuple_{index}")

        visit(node, "")
        return tuple(checks)

    def annotation_type(node: ast.expr) -> ValueType:
        node_kind = _decorator_name(
            node.value if isinstance(node, ast.Subscript) else node
        ).rsplit(".", 1)[-1]
        if isinstance(node, ast.Subscript) and node_kind == "bits":
            dependent = _dependent_static_type_expression(
                node.slice,
                parameter_aliases,
                static_values,
                binding_namespace=static_type_namespace,
            )
            if dependent is not None:
                from _pycircuit_semantics import BitsType

                width = dependent[1]
                if not _proven_integer_in(width, 1, 64):
                    raise QueueFrontendError(
                        "ACPY-TYPE-003: bits width must be in [1, 64]"
                    )
                return BitsType(width)
        try:
            return _scalar_type_descriptor(node, static_values)
        except QueueFrontendError as scalar_error:
            name = node_kind
            if isinstance(node, ast.Subscript) and name in {"tuple", "Tuple"}:
                elements = (
                    tuple(node.slice.elts)
                    if isinstance(node.slice, ast.Tuple)
                    else (node.slice,)
                )
                return TupleType(
                    tuple(annotation_type(element) for element in elements)
                )
            if isinstance(node, ast.Subscript) and name == "array":
                if not isinstance(node.slice, ast.Tuple) or len(node.slice.elts) != 2:
                    raise QueueFrontendError(
                        "ACPY-TYPE-006: value array requires static [length, element]"
                    ) from scalar_error
                dependent = _dependent_static_type_expression(
                    node.slice.elts[0],
                    parameter_aliases,
                    static_values,
                    binding_namespace=static_type_namespace,
                )
                length = (
                    dependent[1]
                    if dependent is not None
                    else _constant_integer(node.slice.elts[0], static_values)
                )
                if length is None:
                    raise QueueFrontendError(
                        "ACPY-TYPE-006: value array requires static [length, element]"
                    ) from scalar_error
                if not _proven_integer_in(length, 1, (1 << 63) - 1):
                    raise QueueFrontendError(
                        "ACPY-TYPE-006: value array length must be positive"
                    ) from scalar_error
                return ArrayType(
                    length,
                    annotation_type(node.slice.elts[1]),
                )
            if name in enum_types:
                return enum_types[name]
            if name in by_name:
                return resolve(name)
            raise scalar_error

    def resolve(name: str) -> StructType:
        cached = resolved.get(name)
        if cached is not None:
            return cached
        if name in active:
            cycle = " -> ".join((*active[active.index(name) :], name))
            raise QueueFrontendError(
                f"ACPY-TYPE-004: recursive struct cycle is unsupported: {cycle}"
            )
        active.append(name)
        node = by_name[name]
        fields: list[ValueField] = []
        static_checks: list[StaticTypeCheck] = []
        for statement in node.body:
            if not isinstance(statement, ast.AnnAssign) or not isinstance(
                statement.target, ast.Name
            ):
                raise QueueFrontendError(
                    "ACPY-QUEUE-002: struct body requires annotated fields"
                )
            try:
                field_type = annotation_type(statement.annotation)
            except QueueFrontendError as error:
                raise QueueFrontendError(
                    f"{error}; field {name}.{statement.target.id} has annotation "
                    f"{ast.unparse(statement.annotation)!r}"
                ) from error
            if field_type.bit_width() > MAX_PACKED_VALUE_WIDTH:
                raise QueueFrontendError(
                    "ACPY-TYPE-006: aggregate field width exceeds the backend "
                    "template domain"
                )
            fields.append(ValueField(statement.target.id, field_type))
            static_checks.extend(
                annotation_checks(name, statement.target.id, statement.annotation)
            )
        if not fields or len({field.name for field in fields}) != len(fields):
            raise QueueFrontendError(
                "ACPY-QUEUE-002: struct requires unique compile-time fields"
            )
        binding_values: dict[str, int] = {}
        for check in static_checks:
            for token in check.program:
                if token[:6] != "param:":
                    continue
                parameter = token[6:]
                binding_values[parameter] = _static_parameter_value(
                    parameter,
                    parameter_aliases,
                    static_values or {},
                    binding_namespace=static_type_namespace,
                )
        bindings: dict[str, int] = {}
        for check in static_checks:
            for token in check.program:
                if token[:6] != "param:":
                    continue
                parameter = token[6:]
                if parameter not in binding_values:
                    raise QueueFrontendError(
                        f"ACPY-TYPE-008: unbound static integer parameter {parameter!r}"
                    )
                canonical_parameter = (
                    parameter[len(static_type_namespace) :]
                    if static_type_namespace
                    and parameter[: len(static_type_namespace)] == static_type_namespace
                    else parameter
                )
                bindings[canonical_parameter] = binding_values[parameter]
        descriptor = StructType(node.name, tuple(fields), tuple(bindings.items()))
        static_checks = [
            replace(
                check,
                target=descriptor.symbol + check.target[len(node.name) :],
            )
            for check in static_checks
        ]
        if descriptor.bit_width() > MAX_PACKED_VALUE_WIDTH:
            raise QueueFrontendError(
                "ACPY-TYPE-006: payload width exceeds the backend template domain"
            )
        active.pop()
        resolved[name] = descriptor
        checks_by_name[name] = tuple(static_checks)
        return descriptor

    descriptors: list[StructType] = []
    for node in declarations:
        try:
            descriptors.append(resolve(node.name))
        except QueueFrontendError as error:
            active.clear()
            if allow_unbound and (
                "unbound static integer parameter" in str(error)
                or "unbound static config parameter" in str(error)
            ):
                continue
            raise
    return tuple(
        Payload(descriptor, checks_by_name.get(descriptor.name, ()))
        for descriptor in descriptors
    )


def _bitfields(tree: ast.Module) -> tuple[BitfieldBinding, ...]:
    result: list[BitfieldBinding] = []
    names: set[str] = set()
    for statement in tree.body:
        value: ast.expr | None = None
        target: ast.expr | None = None
        if isinstance(statement, ast.Assign) and len(statement.targets) == 1:
            target = statement.targets[0]
            value = statement.value
        elif isinstance(statement, ast.AnnAssign):
            target = statement.target
            value = statement.value
        if (
            not isinstance(target, ast.Name)
            or not isinstance(value, ast.Call)
            or _decorator_name(value.func).rsplit(".", 1)[-1] != "BitfieldSpec"
        ):
            continue
        if target.id in names:
            raise QueueFrontendError(
                f"ACPY-BITFIELD-001: BitfieldSpec {target.id!r} is duplicated"
            )
        if any(keyword.arg is None for keyword in value.keywords):
            raise QueueFrontendError(
                "ACPY-BITFIELD-001: BitfieldSpec does not accept keyword unpacking"
            )
        keyword_values = {keyword.arg: keyword.value for keyword in value.keywords}
        if len(keyword_values) != len(value.keywords) or set(keyword_values) - {
            "width",
            "fields",
        }:
            raise QueueFrontendError(
                "ACPY-BITFIELD-001: BitfieldSpec accepts only width and fields"
            )
        if len(value.args) > 2:
            raise QueueFrontendError(
                "ACPY-BITFIELD-001: BitfieldSpec requires width and fields"
            )
        width_node = value.args[0] if value.args else keyword_values.get("width")
        fields_node = (
            value.args[1] if len(value.args) == 2 else keyword_values.get("fields")
        )
        if (
            width_node is None
            or fields_node is None
            or (value.args and "width" in keyword_values)
            or (len(value.args) == 2 and "fields" in keyword_values)
        ):
            raise QueueFrontendError(
                "ACPY-BITFIELD-001: BitfieldSpec requires width and fields once"
            )
        try:
            width = ast.literal_eval(width_node)
            fields = ast.literal_eval(fields_node)
        except (ValueError, TypeError, SyntaxError) as exc:
            raise QueueFrontendError(
                "ACPY-BITFIELD-001: BitfieldSpec width and fields must be static literals"
            ) from exc
        if not isinstance(fields, Mapping):
            raise QueueFrontendError(
                "ACPY-BITFIELD-001: BitfieldSpec fields must be a static mapping"
            )
        from _pycircuit_semantics import BitfieldLayout, BitfieldLayoutError

        try:
            layout = BitfieldLayout(width, fields)
        except BitfieldLayoutError as exc:
            raise QueueFrontendError(f"ACPY-BITFIELD-001: {exc}") from exc
        if layout.width > 64:
            raise QueueFrontendError(
                "ACPY-BITFIELD-001: BitfieldSpec width must be in [1, 64]"
            )
        names.add(target.id)
        result.append(BitfieldBinding(target.id, layout))
    return tuple(result)


def _static_int_value(node: ast.expr, values: Mapping[str, StaticValue]) -> int | None:
    return _constant_integer(node, values)


def _positive_int_value(
    call: ast.Call,
    name: str,
    default: int,
    static_values: Mapping[str, StaticValue] | None = None,
) -> int:
    matches = [keyword for keyword in call.keywords if keyword.arg == name]
    if len(matches) > 1:
        raise QueueFrontendError(f"ACPY-QUEUE-001: repeated {name!r}")
    if not matches:
        return default
    value = _static_int_value(matches[0].value, static_values or {})
    if value is None:
        raise QueueFrontendError(
            f"ACPY-QUEUE-001: {name} must be a compile-time integer"
        )
    if value <= 0:
        raise QueueFrontendError(f"ACPY-QUEUE-001: {name} must be positive")
    return value


def _nonnegative_int_value(
    call: ast.Call,
    name: str,
    default: int,
    static_values: Mapping[str, StaticValue] | None = None,
) -> int:
    matches = [keyword for keyword in call.keywords if keyword.arg == name]
    if len(matches) > 1:
        raise QueueFrontendError(f"ACPY-QUEUE-001: repeated {name!r}")
    if not matches:
        return default
    value = _static_int_value(matches[0].value, static_values or {})
    if value is None:
        raise QueueFrontendError(
            f"ACPY-QUEUE-001: {name} must be a compile-time integer"
        )
    if value < 0:
        raise QueueFrontendError(f"ACPY-QUEUE-001: {name} must be non-negative")
    return value


def _payload(
    node: ast.expr,
    payloads: dict[str, Payload],
    enums: Mapping[str, ValueType] | None = None,
    static_values: Mapping[str, StaticValue] | None = None,
) -> ValueType:
    try:
        return _scalar_type_descriptor(node, static_values)
    except QueueFrontendError:
        pass
    if isinstance(node, ast.Name) and node.id in payloads:
        return payloads[node.id].descriptor
    if isinstance(node, ast.Name) and enums is not None and node.id in enums:
        return enums[node.id]
    raise QueueFrontendError(
        "ACPY-QUEUE-002: source payload must be a compile-time supported type"
    )
