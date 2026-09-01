"""serial-Python to Queue/Var ACIR construction."""

from __future__ import annotations

import ast
import copy
import json
from collections.abc import Mapping
from dataclasses import dataclass

from ._static_eval import StaticEnvironment, StaticValue, evaluate_static


class QueueFrontendError(ValueError):
    """A stable rejection from the queue frontend."""


@dataclass(frozen=True, slots=True)
class Payload:
    name: str
    fields: tuple[tuple[str, str], ...]

    @property
    def acir_type(self) -> str:
        return f"!ac.struct<@types::@{self.name}>"


@dataclass(frozen=True, slots=True)
class QueueBinding:
    name: str
    payload: str
    depth: int
    latency: int
    input_name: str | None
    argument: str | None = None
    expression: ast.expr | None = None
    scope: tuple[str, ...] = ()
    order: int = 0
    route_output: bool = False
    feedback_output: bool = False
    merge_output: bool = False
    reorder_output: bool = False
    dependency_output: bool = False
    credit_output: bool = False
    memory_output: bool = False
    table_read_output: bool = False
    barrier_output: bool = False
    select_output: bool = False
    firing_effect: bool = False
    atomic_group: int | None = None
    provider: str = "transform"
    rate: int = 1


@dataclass(frozen=True, slots=True)
class ScopeBinding:
    name: str
    path: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class SinkBinding:
    queue: str
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class ObservationBinding:
    queue: str
    name: str
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class ExpectBinding:
    queue: str
    argument: str
    predicate: ast.expr
    message: str
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class RouteBinding:
    input_name: str
    outputs: tuple[str, ...]
    argument: str
    selector: ast.expr
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int
    boolean_selector: bool = False


@dataclass(frozen=True, slots=True)
class ForkBinding:
    input_name: str
    outputs: tuple[str, ...]
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class FeedbackBinding:
    input_name: str
    output_name: str
    argument: str
    condition: ast.expr
    update: ast.expr
    depth: int
    latency: int
    max_iterations: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class MergeBinding:
    inputs: tuple[str, ...]
    output: str
    policy: str
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class ReorderBinding:
    input_name: str
    output_name: str
    argument: str
    key: ast.expr
    capacity: int
    start: int
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class DependencyBinding:
    input_name: str
    output_name: str
    argument: str
    key: ast.expr
    waits_for: ast.expr
    resource: ast.expr
    cost: ast.expr
    capacity: int
    resources: int
    no_dependency: int
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int
    provider: str = "dependency"


@dataclass(frozen=True, slots=True)
class CreditBinding:
    input_name: str
    output_name: str
    argument: str
    cost: ast.expr
    credits: int
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int
    provider: str = "credit"


@dataclass(frozen=True, slots=True)
class BarrierBinding:
    inputs: tuple[str, ...]
    outputs: tuple[str, ...]
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class SelectBinding:
    control: str
    inputs: tuple[str, ...]
    output: str
    argument: str
    selector: ast.expr
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class MemoryInstanceBinding:
    name: str
    data_type: str
    entries: int
    init: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class MemoryRequestBinding:
    instance: str
    input_name: str
    output_name: str
    argument: str
    address: ast.expr
    write: ast.expr
    data: ast.expr
    result_field: str
    depth: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class MemoryBinding:
    input_name: str
    output_name: str
    argument: str
    address: ast.expr
    write: ast.expr
    data: ast.expr
    data_type: str
    entries: int
    init: int
    result_field: str
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class TableBinding:
    name: str
    entry_type: str
    entries: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class EntryViewBinding:
    name: str
    table: str
    argument: str | None
    address: ast.expr
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class TableReadBinding:
    table: str
    input_name: str | None
    output_name: str
    argument: str | None
    address: ast.expr
    when: ast.expr
    view_alias: str | None
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class TableWriteBinding:
    table: str
    input_name: str
    argument: str
    address: ast.expr
    enable: ast.expr
    value: ast.expr | None
    patch_fields: tuple[tuple[str, ast.expr], ...]
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class StaticMemoryArrayBinding:
    name: str
    members: tuple[str, ...]
    data_type: str
    entries: int
    init: int
    latency: int
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class SelectedMemoryBinding:
    name: str
    array: str
    input_name: str
    routed_inputs: tuple[str, ...]
    argument: str
    selector: ast.expr
    depth: int
    latency: int
    scope: tuple[str, ...]
    order: int
    provider: str = "memory"


@dataclass(frozen=True, slots=True)
class AtomicBinding:
    queues: tuple[str, ...]
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class StaticQueueCollection:
    kind: str
    members: tuple[tuple[str | int | bool, str | StaticQueueCollection], ...]


@dataclass(frozen=True, slots=True)
class RecursiveQueueHelper:
    queue_parameter: str
    count_parameter: str
    argument: str
    expression: ast.expr
    apply_call: ast.Call


@dataclass(frozen=True, slots=True)
class CollectionBinding:
    name: str
    value: StaticQueueCollection
    scope: tuple[str, ...]
    order: int


@dataclass(frozen=True, slots=True)
class QueueProgram:
    system: str
    payloads: tuple[Payload, ...]
    queues: tuple[QueueBinding, ...]
    scopes: tuple[ScopeBinding, ...]
    routes: tuple[RouteBinding, ...]
    forks: tuple[ForkBinding, ...]
    feedbacks: tuple[FeedbackBinding, ...]
    merges: tuple[MergeBinding, ...]
    reorders: tuple[ReorderBinding, ...]
    dependencies: tuple[DependencyBinding, ...]
    credits: tuple[CreditBinding, ...]
    barriers: tuple[BarrierBinding, ...]
    selects: tuple[SelectBinding, ...]
    memory_instances: tuple[MemoryInstanceBinding, ...]
    memory_requests: tuple[MemoryRequestBinding, ...]
    memories: tuple[MemoryBinding, ...]
    tables: tuple[TableBinding, ...]
    table_reads: tuple[TableReadBinding, ...]
    table_writes: tuple[TableWriteBinding, ...]
    atomics: tuple[AtomicBinding, ...]
    collections: tuple[CollectionBinding, ...]
    observations: tuple[ObservationBinding, ...]
    expectations: tuple[ExpectBinding, ...]
    sinks: tuple[SinkBinding, ...]
    specialization_fingerprint: str | None = None


def _decorator_name(node: ast.expr) -> str:
    if isinstance(node, ast.Call):
        return _decorator_name(node.func)
    if isinstance(node, ast.Name):
        return node.id
    if isinstance(node, ast.Attribute):
        prefix = _decorator_name(node.value)
        return f"{prefix}.{node.attr}" if prefix else node.attr
    return ""


def _scalar_type(node: ast.expr) -> str:
    name = _decorator_name(node).rsplit(".", 1)[-1]
    if name == "int":
        return "i64"
    if name == "bool":
        return "i1"
    widths = {
        "u1": 1,
        "u2": 2,
        "u4": 4,
        "u8": 8,
        "u16": 16,
        "u32": 32,
        "u64": 64,
        "s8": 8,
        "s16": 16,
        "s32": 32,
        "s64": 64,
    }
    if name in widths:
        return f"i{widths[name]}"
    raise QueueFrontendError("ACPY-QUEUE-002: unsupported field type")


def _payloads(tree: ast.Module) -> tuple[Payload, ...]:
    result: list[Payload] = []
    for node in tree.body:
        if not isinstance(node, ast.ClassDef) or not any(
            _decorator_name(item).rsplit(".", 1)[-1] == "struct"
            for item in node.decorator_list
        ):
            continue
        fields: list[tuple[str, str]] = []
        for statement in node.body:
            if not isinstance(statement, ast.AnnAssign) or not isinstance(
                statement.target, ast.Name
            ):
                raise QueueFrontendError(
                    "ACPY-QUEUE-002: struct body requires annotated fields"
                )
            fields.append((statement.target.id, _scalar_type(statement.annotation)))
        if not fields or len({name for name, _ in fields}) != len(fields):
            raise QueueFrontendError(
                "ACPY-QUEUE-002: struct requires unique compile-time fields"
            )
        result.append(Payload(node.name, tuple(fields)))
    return tuple(result)


def _static_int_value(node: ast.expr, values: Mapping[str, StaticValue]) -> int | None:
    try:
        value = evaluate_static(node, StaticEnvironment(values))
    except ValueError:
        return None
    return value if type(value) is int else None


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


def _payload(node: ast.expr, payloads: dict[str, Payload]) -> str:
    try:
        return _scalar_type(node)
    except QueueFrontendError:
        pass
    if isinstance(node, ast.Name) and node.id in payloads:
        return payloads[node.id].acir_type
    raise QueueFrontendError(
        "ACPY-QUEUE-002: source payload must be a compile-time supported type"
    )


def _lambda_value(node: ast.expr) -> tuple[str, ast.expr]:
    if not isinstance(node, ast.Lambda) or len(node.args.args) != 1:
        raise QueueFrontendError("ACPY-QUEUE-003: apply requires a one-argument lambda")
    return node.args.args[0].arg, node.body


def _constantize_expression(
    node: ast.expr,
    argument: str,
    values: Mapping[str, StaticValue],
) -> ast.expr:
    class Constantizer(ast.NodeTransformer):
        def _constant(self, candidate: ast.expr) -> ast.expr | None:
            try:
                value = evaluate_static(candidate, StaticEnvironment(values))
            except ValueError:
                return None
            if value is None or type(value) in {bool, int, float, str}:
                return ast.copy_location(ast.Constant(value=value), candidate)
            return None

        def visit_Name(self, candidate: ast.Name) -> ast.expr:
            if candidate.id == argument:
                return candidate
            return self._constant(candidate) or candidate

        def visit_Attribute(self, candidate: ast.Attribute) -> ast.expr:
            return self._constant(candidate) or self.generic_visit(candidate)

    result = Constantizer().visit(copy.deepcopy(node))
    assert isinstance(result, ast.expr)
    return ast.fix_missing_locations(result)


def _validate_firing_expression(node: ast.expr, argument: str) -> None:
    if (
        not isinstance(node, ast.Call)
        or not isinstance(node.func, ast.Attribute)
        or node.func.attr != "push"
        or not isinstance(node.func.value, ast.Name)
        or node.func.value.id != argument
        or len(node.args) != 1
        or node.keywords
    ):
        raise QueueFrontendError("ACPY-QUEUE-019: firing must return queue.push(value)")
    pops = 0
    pushes = 0
    for child in ast.walk(node):
        if not isinstance(child, ast.Call) or not isinstance(child.func, ast.Attribute):
            continue
        if (
            not isinstance(child.func.value, ast.Name)
            or child.func.value.id != argument
        ):
            continue
        if child.func.attr in {"peek", "pop"}:
            if child.args or child.keywords:
                raise QueueFrontendError(
                    "ACPY-QUEUE-019: firing peek/pop take no arguments"
                )
            pops += child.func.attr == "pop"
        elif child.func.attr == "push":
            if len(child.args) != 1 or child.keywords:
                raise QueueFrontendError(
                    "ACPY-QUEUE-019: firing push requires one value"
                )
            pushes += 1
        else:
            raise QueueFrontendError("ACPY-QUEUE-019: unsupported queue effect method")
    if pops != 1 or pushes != 1:
        raise QueueFrontendError(
            "ACPY-QUEUE-019: firing requires exactly one pop and one push"
        )


def parse_queue_program(
    text: str,
    system: str,
    static_arguments: Mapping[str, StaticValue] | None = None,
    specialization_fingerprint: str | None = None,
) -> QueueProgram:
    tree = ast.parse(text, filename="<queue-model>", type_comments=True)
    for node in tree.body:
        decorators = getattr(node, "decorator_list", ())
        if any(
            _decorator_name(decorator).rsplit(".", 1)[-1]
            in {"opcode", "provider", "backend"}
            for decorator in decorators
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-010: user opcode or backend providers are forbidden"
            )
    payloads = _payloads(tree)
    payload_map = {item.name: item for item in payloads}
    candidates = [
        node
        for node in tree.body
        if isinstance(node, ast.FunctionDef)
        and node.name == system
        and any(
            _decorator_name(d).rsplit(".", 1)[-1] == "system"
            for d in node.decorator_list
        )
    ]
    if len(candidates) != 1:
        raise QueueFrontendError(
            f"ACPY-QUEUE-001: system {system!r} is missing or ambiguous"
        )
    function = candidates[0]
    if specialization_fingerprint is not None:
        prefix = "sha256:"
        digest = specialization_fingerprint.removeprefix(prefix)
        if (
            not specialization_fingerprint.startswith(prefix)
            or len(digest) != 64
            or any(character not in "0123456789abcdef" for character in digest)
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-022: specialization fingerprint is invalid"
            )
    if function.args.vararg is not None or function.args.kwarg is not None:
        raise QueueFrontendError(
            "ACPY-QUEUE-001: a queue system cannot use variadic parameters"
        )
    parameters = [
        *function.args.posonlyargs,
        *function.args.args,
        *function.args.kwonlyargs,
    ]
    supplied = dict(static_arguments or {})
    parameter_names = {parameter.arg for parameter in parameters}
    extras = sorted(set(supplied) - parameter_names)
    if extras:
        raise QueueFrontendError(
            f"ACPY-QUEUE-001: unknown static argument {extras[0]!r}"
        )

    positional_defaults: dict[str, ast.expr] = {}
    positional = [*function.args.posonlyargs, *function.args.args]
    if function.args.defaults:
        for parameter, default in zip(
            positional[-len(function.args.defaults) :],
            function.args.defaults,
            strict=True,
        ):
            positional_defaults[parameter.arg] = default
    keyword_defaults = {
        parameter.arg: default
        for parameter, default in zip(
            function.args.kwonlyargs,
            function.args.kw_defaults,
            strict=True,
        )
        if default is not None
    }
    for parameter in parameters:
        annotation_name = (
            _decorator_name(parameter.annotation.value).rsplit(".", 1)[-1]
            if isinstance(parameter.annotation, ast.Subscript)
            else ""
        )
        if annotation_name != "const":
            raise QueueFrontendError(
                f"ACPY-QUEUE-022: system parameter {parameter.arg!r} must use ac.const"
            )
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
    system_static_values: Mapping[str, StaticValue] = supplied

    def _static_int(
        node: ast.expr,
        values: Mapping[str, StaticValue] | None = None,
    ) -> int | None:
        return _static_int_value(
            node, system_static_values if values is None else values
        )

    def _positive_int(
        call: ast.Call,
        name: str,
        default: int,
        values: Mapping[str, StaticValue] | None = None,
    ) -> int:
        return _positive_int_value(
            call,
            name,
            default,
            system_static_values if values is None else values,
        )

    def _nonnegative_int(
        call: ast.Call,
        name: str,
        default: int,
        values: Mapping[str, StaticValue] | None = None,
    ) -> int:
        return _nonnegative_int_value(
            call,
            name,
            default,
            system_static_values if values is None else values,
        )

    def _lambda(node: ast.expr) -> tuple[str, ast.expr]:
        argument, expression = _lambda_value(node)
        return argument, _constantize_expression(
            expression, argument, system_static_values
        )

    recursive_helpers: dict[str, RecursiveQueueHelper] = {}
    for helper in tree.body:
        if (
            not isinstance(helper, ast.FunctionDef)
            or helper is function
            or helper.decorator_list
            or len(helper.args.args) != 2
            or helper.args.posonlyargs
            or helper.args.kwonlyargs
            or len(helper.body) != 2
            or not isinstance(helper.body[0], ast.If)
            or not isinstance(helper.body[1], ast.Return)
        ):
            continue
        queue_parameter = helper.args.args[0].arg
        count_parameter = helper.args.args[1].arg
        base = helper.body[0]
        recursive_return = helper.body[1]
        if (
            not isinstance(base.test, ast.Compare)
            or len(base.test.ops) != 1
            or not isinstance(base.test.ops[0], ast.Eq)
            or len(base.test.comparators) != 1
            or not isinstance(base.test.left, ast.Name)
            or base.test.left.id != count_parameter
            or not isinstance(base.test.comparators[0], ast.Constant)
            or base.test.comparators[0].value != 0
            or len(base.body) != 1
            or not isinstance(base.body[0], ast.Return)
            or not isinstance(base.body[0].value, ast.Name)
            or base.body[0].value.id != queue_parameter
            or base.orelse
            or not isinstance(recursive_return.value, ast.Call)
        ):
            continue
        recursive_call = recursive_return.value
        if (
            not isinstance(recursive_call.func, ast.Name)
            or recursive_call.func.id != helper.name
            or len(recursive_call.args) != 2
            or recursive_call.keywords
            or not isinstance(recursive_call.args[0], ast.Call)
            or not isinstance(recursive_call.args[1], ast.BinOp)
            or not isinstance(recursive_call.args[1].op, ast.Sub)
            or not isinstance(recursive_call.args[1].left, ast.Name)
            or recursive_call.args[1].left.id != count_parameter
            or not isinstance(recursive_call.args[1].right, ast.Constant)
            or recursive_call.args[1].right.value != 1
        ):
            continue
        apply_call = recursive_call.args[0]
        if (
            not isinstance(apply_call.func, ast.Attribute)
            or apply_call.func.attr != "apply"
            or not isinstance(apply_call.func.value, ast.Name)
            or apply_call.func.value.id != queue_parameter
            or len(apply_call.args) != 1
        ):
            continue
        argument, expression = _lambda(apply_call.args[0])
        recursive_helpers[helper.name] = RecursiveQueueHelper(
            queue_parameter,
            count_parameter,
            argument,
            expression,
            apply_call,
        )
    queues: list[QueueBinding] = []
    scopes: list[ScopeBinding] = []
    routes: list[RouteBinding] = []
    forks: list[ForkBinding] = []
    feedbacks: list[FeedbackBinding] = []
    merges: list[MergeBinding] = []
    reorders: list[ReorderBinding] = []
    dependencies: list[DependencyBinding] = []
    credits: list[CreditBinding] = []
    barriers: list[BarrierBinding] = []
    selects: list[SelectBinding] = []
    memory_instances: list[MemoryInstanceBinding] = []
    memory_requests: list[MemoryRequestBinding] = []
    memories: list[MemoryBinding] = []
    tables: list[TableBinding] = []
    table_reads: list[TableReadBinding] = []
    table_writes: list[TableWriteBinding] = []
    table_by_name: dict[str, TableBinding] = {}
    entry_views: dict[str, EntryViewBinding] = {}
    memory_by_name: dict[str, MemoryInstanceBinding] = {}
    memory_arrays: dict[str, StaticMemoryArrayBinding] = {}
    selected_memories: dict[str, SelectedMemoryBinding] = {}
    consumed_selected_memories: set[str] = set()
    atomics: list[AtomicBinding] = []
    sinks: list[SinkBinding] = []
    observations: list[ObservationBinding] = []
    expectations: list[ExpectBinding] = []
    by_name: dict[str, QueueBinding] = {}
    collections: dict[str, StaticQueueCollection] = {}
    collection_bindings: list[CollectionBinding] = []
    order = 0

    def call_name(call: ast.Call) -> str:
        return _decorator_name(call.func).rsplit(".", 1)[-1]

    def table_declaration(call: ast.Call) -> tuple[int, str] | None:
        if not isinstance(call.func, ast.Subscript):
            return None
        if _decorator_name(call.func.value).rsplit(".", 1)[-1] != "table":
            return None
        parameters = call.func.slice
        if not isinstance(parameters, ast.Tuple) or len(parameters.elts) != 2:
            raise QueueFrontendError(
                "ACPY-TABLE-001: table requires ac.table[entries, Entry]"
            )
        entries = _static_int(parameters.elts[0])
        if entries is None or entries <= 0:
            raise QueueFrontendError(
                "ACPY-TABLE-001: table entries must be a positive static integer"
            )
        entry_type = _payload(parameters.elts[1], payload_map)
        if call.args or any(
            keyword.arg is None or keyword.arg != "init" for keyword in call.keywords
        ):
            raise QueueFrontendError(
                "ACPY-TABLE-001: table accepts only keyword init=0"
            )
        init_values = [keyword.value for keyword in call.keywords]
        init = 0 if not init_values else _static_int(init_values[0])
        if len(init_values) > 1 or init != 0:
            raise QueueFrontendError(
                "ACPY-TABLE-001: table init must be exactly zero"
            )
        return entries, entry_type

    def parse_view(node: ast.expr, alias: str, scope_path: tuple[str, ...],
                   current_order: int) -> EntryViewBinding | None:
        if not isinstance(node, ast.Call) or not isinstance(node.func, ast.Attribute):
            return None
        if node.func.attr != "view" or not isinstance(node.func.value, ast.Name):
            return None
        table_name = node.func.value.id
        if table_name not in table_by_name:
            return None
        if len(node.args) != 1 or node.keywords:
            raise QueueFrontendError(
                "ACPY-TABLE-002: table.view requires one index or selector lambda"
            )
        selector = node.args[0]
        if isinstance(selector, ast.Lambda):
            argument, address = _lambda(selector)
        else:
            argument, address = None, _constantize_expression(
                selector, "", system_static_values
            )
        return EntryViewBinding(
            alias, table_name, argument, address, scope_path, current_order
        )

    def resolve_view(node: ast.expr, scope_path: tuple[str, ...],
                     current_order: int) -> EntryViewBinding | None:
        if isinstance(node, ast.Name):
            view = entry_views.get(node.id)
            if view and view.scope == scope_path:
                return view
            return None
        return parse_view(node, "", scope_path, current_order)

    def lambda_or_constant(
        node: ast.expr, argument: str, diagnostic: str
    ) -> ast.expr:
        if isinstance(node, ast.Lambda):
            candidate_argument, expression = _lambda(node)
            if candidate_argument != argument:
                raise QueueFrontendError(diagnostic)
            return expression
        return _constantize_expression(node, argument, system_static_values)

    def keyword_value(call: ast.Call, name: str) -> ast.expr:
        matches = [keyword.value for keyword in call.keywords if keyword.arg == name]
        if len(matches) != 1:
            raise QueueFrontendError(
                f"ACPY-QUEUE-024: high-level block requires one {name!r} parameter"
            )
        return matches[0]

    def field_expression(
        node: ast.expr,
        queue: QueueBinding,
        argument: str = "item",
    ) -> ast.expr:
        if not isinstance(node, ast.Attribute) or not isinstance(node.value, ast.Name):
            raise QueueFrontendError(
                "ACPY-QUEUE-024: high-level block requires a typed field descriptor"
            )
        payload = next(
            (item for item in payloads if item.acir_type == queue.payload), None
        )
        if payload is None or node.value.id != payload.name:
            raise QueueFrontendError(
                "ACPY-QUEUE-024: field descriptor payload does not match Queue"
            )
        if node.attr not in {name for name, _ in payload.fields}:
            raise QueueFrontendError(
                f"ACPY-QUEUE-024: payload has no field {node.attr!r}"
            )
        return ast.copy_location(
            ast.Attribute(
                value=ast.Name(id=argument, ctx=ast.Load()),
                attr=node.attr,
                ctx=ast.Load(),
            ),
            node,
        )

    def policy_value(call: ast.Call) -> str:
        matches = [
            keyword.value for keyword in call.keywords if keyword.arg == "policy"
        ]
        if not matches:
            return "priority"
        if len(matches) != 1:
            raise QueueFrontendError("ACPY-QUEUE-024: repeated merge policy")
        node = matches[0]
        if isinstance(node, ast.Constant) and type(node.value) is str:
            policy = node.value
        else:
            policy = _decorator_name(node).rsplit(".", 1)[-1]
        if policy not in {"priority", "round_robin"}:
            raise QueueFrontendError(
                "ACPY-QUEUE-024: merge policy must be priority or round_robin"
            )
        return policy

    def static_reference(
        node: ast.expr,
        aliases: dict[str, str | StaticQueueCollection],
    ) -> str | StaticQueueCollection:
        if isinstance(node, ast.Name):
            if node.id in aliases:
                return aliases[node.id]
            if node.id in by_name:
                return by_name[node.id].name
            if node.id in collections:
                return collections[node.id]
        if (
            isinstance(node, ast.Subscript)
            and isinstance(node.slice, ast.Constant)
            and type(node.slice.value) in {str, int, bool}
        ):
            collection = static_reference(node.value, aliases)
            if not isinstance(collection, StaticQueueCollection):
                raise QueueFrontendError(
                    "ACPY-QUEUE-005: static indexing requires a collection"
                )
            for key, value in collection.members:
                if type(key) is type(node.slice.value) and key == node.slice.value:
                    return value
            raise QueueFrontendError(
                f"ACPY-QUEUE-005: collection has no key {node.slice.value!r}"
            )
        raise QueueFrontendError(
            "ACPY-QUEUE-005: collection reference must be statically resolvable"
        )

    def queue_reference(
        node: ast.expr,
        aliases: dict[str, str | StaticQueueCollection],
    ) -> str:
        value = static_reference(node, aliases)
        if isinstance(value, str):
            return value
        raise QueueFrontendError(
            "ACPY-QUEUE-005: a collection cannot be used as one Queue"
        )

    def collection_signature(
        value: str | StaticQueueCollection,
    ) -> tuple[object, ...]:
        if isinstance(value, str):
            return ("queue", by_name[value].payload)
        keys = tuple(key for key, _ in value.members)
        members = tuple(collection_signature(member) for _, member in value.members)
        return (value.kind, keys, members)

    def stable_collection_identity(value: str | StaticQueueCollection) -> str:
        if isinstance(value, str):
            return value
        return (
            value.kind
            + "("
            + ",".join(
                f"{key}:{stable_collection_identity(member)}"
                for key, member in value.members
            )
            + ")"
        )

    def source_binding(
        name: str,
        call: ast.Call,
        scope_path: tuple[str, ...],
        current_order: int,
        static_values: Mapping[str, StaticValue] | None = None,
    ) -> QueueBinding:
        if call_name(call) != "source" or len(call.args) != 1:
            raise QueueFrontendError(
                "ACPY-QUEUE-005: collection elements must be Queue sources"
            )
        depth = _positive_int(call, "depth", 1, static_values)
        rate = _positive_int(call, "rate", 1, static_values)
        if rate > depth:
            raise QueueFrontendError(
                "ACPY-QUEUE-025: Queue rate must not exceed depth"
            )
        return QueueBinding(
            name,
            _payload(call.args[0], payload_map),
            depth,
            _positive_int(call, "latency", 1, static_values),
            None,
            scope=scope_path,
            order=current_order,
            rate=rate,
        )

    def memory_instance_binding(
        name: str,
        call: ast.Call,
        scope_path: tuple[str, ...],
        current_order: int,
        static_values: dict[str, int] | None = None,
    ) -> MemoryInstanceBinding:
        if call_name(call) != "memory" or len(call.args) != 1:
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory requires one data type"
            )
        if any(
            keyword.arg is None
            or keyword.arg not in {"entries", "init", "latency"}
            for keyword in call.keywords
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory instance has an unsupported keyword"
            )
        data_type = _payload(call.args[0], payload_map)
        if not data_type.startswith("i"):
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory data type must be an integer"
            )
        entries = _positive_int(call, "entries", 16, static_values)
        init = _nonnegative_int(call, "init", 0, static_values)
        latency = _positive_int(call, "latency", 1, static_values)
        if init != 0:
            raise QueueFrontendError("ACPY-QUEUE-015: memory init must be zero")
        return MemoryInstanceBinding(
            name, data_type, entries, init, latency, scope_path, current_order
        )

    def memory_request_parameters(
        call: ast.Call,
        incoming: QueueBinding,
        data_type: str,
        extra_keywords: set[str] | None = None,
    ) -> tuple[str, ast.expr, ast.expr, ast.expr, str, int]:
        allowed_keywords = {
            "address",
            "write",
            "data",
            "result_field",
            "depth",
            *(extra_keywords or set()),
        }
        if any(
            keyword.arg is None or keyword.arg not in allowed_keywords
            for keyword in call.keywords
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory request has an unsupported keyword"
            )
        policies: dict[str, ast.expr] = {}
        for policy in ("address", "write", "data"):
            values = [
                keyword.value for keyword in call.keywords if keyword.arg == policy
            ]
            if len(values) != 1:
                raise QueueFrontendError(
                    "ACPY-QUEUE-015: memory request requires one "
                    f"{policy} lambda"
                )
            policies[policy] = values[0]
        arguments_and_values = [_lambda(policies[item]) for item in policies]
        if len({argument for argument, _ in arguments_and_values}) != 1:
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory request lambdas require one argument name"
            )
        result_fields = [
            keyword.value
            for keyword in call.keywords
            if keyword.arg == "result_field"
        ]
        if (
            len(result_fields) != 1
            or not isinstance(result_fields[0], ast.Constant)
            or type(result_fields[0].value) is not str
            or not result_fields[0].value
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory request requires one static result_field"
            )
        payload = next(
            (
                declaration
                for declaration in payloads
                if declaration.acir_type == incoming.payload
            ),
            None,
        )
        result_field = result_fields[0].value
        field_types = dict(payload.fields) if payload is not None else {}
        if result_field not in field_types:
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory result_field is unknown"
            )
        if field_types[result_field] != data_type:
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory result_field must match instance data type"
            )
        return (
            arguments_and_values[0][0],
            arguments_and_values[0][1],
            arguments_and_values[1][1],
            arguments_and_values[2][1],
            result_field,
            _positive_int(call, "depth", 1),
        )

    def collection_binding(
        name: str,
        call: ast.Call,
        scope_path: tuple[str, ...],
        current_order: int,
        aliases: dict[str, str | StaticQueueCollection],
        static_values: Mapping[str, StaticValue] | None = None,
    ) -> StaticQueueCollection | None:
        static_values = system_static_values if static_values is None else static_values
        kind = call_name(call)
        if kind == "array":
            extent = (
                _static_int(call.args[0], static_values)
                if len(call.args) == 2
                else None
            )
            if len(call.args) != 2 or extent is None or extent <= 0:
                raise QueueFrontendError(
                    "ACPY-QUEUE-005: array requires a positive compile-time extent"
                )
            argument, body = _lambda(call.args[1])
            members: list[tuple[str | int, str | StaticQueueCollection]] = []
            for index in range(extent):
                if not isinstance(body, ast.Call):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-005: array generator must produce a Queue"
                    )
                leaf = f"{name}__{index}"
                values = {**static_values, argument: index}
                if call_name(body) == "source":
                    binding = source_binding(
                        leaf, body, scope_path, current_order, values
                    )
                    queues.append(binding)
                    by_name[leaf] = binding
                    member: str | StaticQueueCollection = leaf
                else:
                    nested = collection_binding(
                        leaf,
                        body,
                        scope_path,
                        current_order,
                        aliases,
                        values,
                    )
                    if nested is None:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-005: array generator must produce a Queue "
                            "or static collection"
                        )
                    member = nested
                members.append((index, member))
            signatures = {collection_signature(member) for _, member in members}
            if len(signatures) != 1:
                raise QueueFrontendError(
                    "ACPY-QUEUE-005: array elements must have one static shape"
                )
            return StaticQueueCollection("array", tuple(members))
        if kind == "map":
            if len(call.args) != 1 or not isinstance(call.args[0], ast.Dict):
                raise QueueFrontendError(
                    "ACPY-QUEUE-005: map requires one compile-time dictionary"
                )
            entries: list[tuple[str | int | bool, str | StaticQueueCollection]] = []
            for key, value in zip(call.args[0].keys, call.args[0].values, strict=True):
                if (
                    not isinstance(key, ast.Constant)
                    or type(key.value) not in {str, int, bool}
                    or (type(key.value) is str and not key.value)
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-005: map keys must be compile-time bool/int/str"
                    )
                entries.append((key.value, static_reference(value, aliases)))
            rank = {bool: 0, int: 1, str: 2}
            entries.sort(key=lambda item: (rank[type(item[0])], item[0]))
            if not entries or len({(type(key), key) for key, _ in entries}) != len(
                entries
            ):
                raise QueueFrontendError(
                    "ACPY-QUEUE-005: map keys must be unique and non-empty"
                )
            if len({collection_signature(value) for _, value in entries}) != 1:
                raise QueueFrontendError(
                    "ACPY-QUEUE-005: map values must have one static shape"
                )
            return StaticQueueCollection("map", tuple(entries))
        if kind == "set":
            if len(call.args) != 1 or not isinstance(
                call.args[0], (ast.Set, ast.List, ast.Tuple)
            ):
                raise QueueFrontendError(
                    "ACPY-QUEUE-005: set requires one compile-time collection"
                )
            members = [static_reference(item, aliases) for item in call.args[0].elts]
            identities = [stable_collection_identity(member) for member in members]
            if not members or len(set(identities)) != len(members):
                raise QueueFrontendError(
                    "ACPY-QUEUE-005: set members must be unique and non-empty"
                )
            members.sort(key=stable_collection_identity)
            if len({collection_signature(member) for member in members}) != 1:
                raise QueueFrontendError(
                    "ACPY-QUEUE-005: set members must have one static shape"
                )
            return StaticQueueCollection(
                "set", tuple((index, member) for index, member in enumerate(members))
            )
        return None

    def visit(
        statements: list[ast.stmt],
        scope_path: tuple[str, ...],
        aliases: dict[str, str | StaticQueueCollection] | None = None,
        atomic_group: int | None = None,
    ) -> None:
        nonlocal order
        aliases = {} if aliases is None else aliases
        for statement in statements:
            current_order = order
            order += 1
            assigned_names: tuple[str, ...] = ()
            if isinstance(statement, ast.Assign) and len(statement.targets) == 1:
                target = statement.targets[0]
                if isinstance(target, ast.Name):
                    assigned_names = (target.id,)
                elif isinstance(target, (ast.Tuple, ast.List)) and all(
                    isinstance(item, ast.Name) for item in target.elts
                ):
                    assigned_names = tuple(item.id for item in target.elts)
            if any(
                name in memory_by_name
                or name in memory_arrays
                or name in selected_memories
                or name in table_by_name
                or name in entry_views
                for name in assigned_names
            ):
                raise QueueFrontendError(
                    "ACPY-QUEUE-015: state binding cannot be rebound"
                )
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Name)
                and isinstance(statement.value, ast.Call)
            ):
                declaration = table_declaration(statement.value)
                if declaration is not None:
                    name = statement.targets[0].id
                    if name in by_name or name in collections or name in table_by_name:
                        raise QueueFrontendError(
                            "ACPY-TABLE-001: table declaration requires a fresh name"
                        )
                    entries, entry_type = declaration
                    binding = TableBinding(
                        name, entry_type, entries, scope_path, current_order
                    )
                    tables.append(binding)
                    table_by_name[name] = binding
                    continue
                view = parse_view(
                    statement.value,
                    statement.targets[0].id,
                    scope_path,
                    current_order,
                )
                if view is not None:
                    if view.name in by_name or view.name in collections:
                        raise QueueFrontendError(
                            "ACPY-TABLE-002: EntryView alias requires a fresh name"
                        )
                    entry_views[view.name] = view
                    continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Name)
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Attribute)
                and statement.value.func.attr == "read"
            ):
                call = statement.value
                view = resolve_view(call.func.value, scope_path, current_order)
                if view is not None:
                    name = statement.targets[0].id
                    if name in by_name or name in collections or name in table_by_name:
                        raise QueueFrontendError(
                            "ACPY-TABLE-003: table read output requires a fresh name"
                        )
                    if len(call.args) > 1 or any(
                        keyword.arg is None
                        or keyword.arg not in {"when", "depth", "latency"}
                        for keyword in call.keywords
                    ):
                        raise QueueFrontendError(
                            "ACPY-TABLE-003: table read parameters are invalid"
                        )
                    input_name: str | None = None
                    argument = view.argument
                    if call.args:
                        if argument is None:
                            raise QueueFrontendError(
                                "ACPY-TABLE-003: Queue-driven read requires a "
                                "selector lambda"
                            )
                        input_name = queue_reference(call.args[0], aliases)
                    elif argument is not None:
                        raise QueueFrontendError(
                            "ACPY-TABLE-003: state-driven read requires a bound index"
                        )
                    when_values = [
                        keyword.value
                        for keyword in call.keywords
                        if keyword.arg == "when"
                    ]
                    if len(when_values) > 1:
                        raise QueueFrontendError(
                            "ACPY-TABLE-003: table read has repeated when"
                        )
                    when_node = when_values[0] if when_values else ast.Constant(True)
                    if argument is not None:
                        when = lambda_or_constant(
                            when_node,
                            argument,
                            "ACPY-TABLE-003: selector and when lambdas require "
                            "one argument name",
                        )
                    else:
                        if isinstance(when_node, ast.Lambda):
                            raise QueueFrontendError(
                                "ACPY-TABLE-003: state-driven when is an "
                                "EntryView expression"
                            )
                        when = _constantize_expression(
                            when_node, "", system_static_values
                        )
                    depth = _positive_int(call, "depth", 1)
                    latency = _positive_int(call, "latency", 1)
                    table = table_by_name[view.table]
                    queue = QueueBinding(
                        name,
                        table.entry_type,
                        depth,
                        latency,
                        None,
                        scope=scope_path,
                        order=current_order,
                        table_read_output=True,
                    )
                    queues.append(queue)
                    by_name[name] = queue
                    table_reads.append(
                        TableReadBinding(
                            view.table,
                            input_name,
                            name,
                            argument,
                            view.address,
                            when,
                            view.name or None,
                            depth,
                            latency,
                            scope_path,
                            current_order,
                        )
                    )
                    continue
            if (
                isinstance(statement, ast.Expr)
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Attribute)
                and statement.value.func.attr in {"write", "patch"}
            ):
                call = statement.value
                view = resolve_view(call.func.value, scope_path, current_order)
                if view is not None:
                    method = call.func.attr
                    if len(call.args) != 1 or view.argument is None:
                        raise QueueFrontendError(
                            "ACPY-TABLE-004: table write/patch requires a "
                            "Queue-driven view"
                        )
                    input_name = queue_reference(call.args[0], aliases)
                    argument = view.argument
                    assert argument is not None
                    enable_values = [
                        keyword.value
                        for keyword in call.keywords
                        if keyword.arg == "enable"
                    ]
                    if len(enable_values) > 1:
                        raise QueueFrontendError(
                            "ACPY-TABLE-004: repeated write enable"
                        )
                    enable = lambda_or_constant(
                        enable_values[0] if enable_values else ast.Constant(True),
                        argument,
                        "ACPY-TABLE-004: selector and enable lambdas require "
                        "one argument name",
                    )
                    value: ast.expr | None = None
                    patch_fields: tuple[tuple[str, ast.expr], ...] = ()
                    table = table_by_name[view.table]
                    if method == "write":
                        if any(
                            keyword.arg is None
                            or keyword.arg not in {"value", "enable"}
                            for keyword in call.keywords
                        ):
                            raise QueueFrontendError(
                                "ACPY-TABLE-004: write accepts only value and enable"
                            )
                        values = [
                            keyword.value
                            for keyword in call.keywords
                            if keyword.arg == "value"
                        ]
                        if len(values) != 1:
                            raise QueueFrontendError(
                                "ACPY-TABLE-004: write requires one value lambda"
                            )
                        value = lambda_or_constant(
                            values[0],
                            argument,
                            "ACPY-TABLE-004: selector and value lambdas require "
                            "one argument name",
                        )
                    else:
                        payload_name = table.entry_type.removeprefix(
                            "!ac.struct<@types::@"
                        ).removesuffix(">")
                        declaration = payload_map.get(payload_name)
                        if declaration is None:
                            raise QueueFrontendError(
                                "ACPY-TABLE-004: patch requires a struct Table Entry"
                            )
                        field_types = dict(declaration.fields)
                        patches: list[tuple[str, ast.expr]] = []
                        for keyword in call.keywords:
                            if keyword.arg == "enable":
                                continue
                            if keyword.arg is None or keyword.arg not in field_types:
                                raise QueueFrontendError(
                                    "ACPY-TABLE-004: patch field is unknown"
                                )
                            patches.append(
                                (
                                    keyword.arg,
                                    lambda_or_constant(
                                        keyword.value,
                                        argument,
                                        "ACPY-TABLE-004: patch lambdas require "
                                        "one argument name",
                                    ),
                                )
                            )
                        if not patches:
                            raise QueueFrontendError(
                                "ACPY-TABLE-004: patch requires at least one field"
                            )
                        patch_fields = tuple(patches)
                    if any(write.table == view.table for write in table_writes):
                        raise QueueFrontendError(
                            "ACPY-TABLE-004: table permits one write/patch endpoint"
                        )
                    table_writes.append(
                        TableWriteBinding(
                            view.table,
                            input_name,
                            argument,
                            view.address,
                            enable,
                            value,
                            patch_fields,
                            scope_path,
                            current_order,
                        )
                    )
                    continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Name)
                and isinstance(statement.value, ast.Call)
                and call_name(statement.value) == "memory"
                and len(statement.value.args) == 1
            ):
                name = statement.targets[0].id
                call = statement.value
                if (
                    name in by_name
                    or name in collections
                    or name in memory_by_name
                    or name in memory_arrays
                    or name in selected_memories
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: memory instance requires one fresh name"
                    )
                instance = memory_instance_binding(
                    name, call, scope_path, current_order
                )
                memory_instances.append(instance)
                memory_by_name[name] = instance
                continue
            if isinstance(statement, ast.If):
                if (
                    isinstance(statement.test, ast.Constant)
                    and type(statement.test.value) is bool
                ):
                    selected = (
                        statement.body if statement.test.value else statement.orelse
                    )
                    visit(selected, scope_path, aliases, atomic_group)
                    continue
                if atomic_group is not None:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-011: runtime if cannot be nested in atomic"
                    )

                def parse_arm(
                    body: list[ast.stmt],
                ) -> tuple[str, str, ast.Call, str, ast.expr]:
                    if (
                        len(body) != 1
                        or not isinstance(body[0], ast.Assign)
                        or len(body[0].targets) != 1
                        or not isinstance(body[0].targets[0], ast.Name)
                        or not isinstance(body[0].value, ast.Call)
                    ):
                        raise QueueFrontendError(
                            "ACPY-QUEUE-011: runtime if requires one apply "
                            "assignment in each branch"
                        )
                    target = body[0].targets[0].id
                    call = body[0].value
                    if (
                        not isinstance(call.func, ast.Attribute)
                        or call.func.attr != "apply"
                        or len(call.args) != 1
                    ):
                        raise QueueFrontendError(
                            "ACPY-QUEUE-011: runtime if requires one apply "
                            "assignment in each branch"
                        )
                    input_name = queue_reference(call.func.value, aliases)
                    argument, expression = _lambda(call.args[0])
                    return target, input_name, call, argument, expression

                false_arm = parse_arm(statement.orelse)
                true_arm = parse_arm(statement.body)
                if false_arm[0] != true_arm[0]:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-011: runtime if branches require one result name"
                    )
                if false_arm[1] != true_arm[1]:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-011: runtime if branches must consume one Queue"
                    )
                name = true_arm[0]
                input_name = true_arm[1]
                if name in by_name or name in collections:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-011: runtime if result requires one fresh name"
                    )
                incoming = by_name[input_name]

                condition_names: dict[str, str] = {}
                for node in ast.walk(statement.test):
                    if not isinstance(node, ast.Name):
                        continue
                    try:
                        referenced = queue_reference(node, aliases)
                    except QueueFrontendError:
                        continue
                    condition_names[node.id] = referenced
                if set(condition_names.values()) != {input_name}:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-011: runtime if condition must read its branch Queue"
                    )

                argument = "item"

                class QueueCondition(ast.NodeTransformer):
                    def visit_Name(self, node: ast.Name) -> ast.expr:
                        if condition_names.get(node.id) == input_name:
                            return ast.copy_location(ast.Name(id=argument), node)
                        return node

                condition = QueueCondition().visit(copy.deepcopy(statement.test))
                assert isinstance(condition, ast.expr)
                _, condition_type = _ExpressionEmitter(
                    payload_map, argument, incoming.payload
                ).emit(condition)
                if condition_type != "i1":
                    raise QueueFrontendError(
                        "ACPY-QUEUE-011: runtime if condition must lower to bool"
                    )
                conditional = len([route for route in routes if route.boolean_selector])
                false_input = f"{name}__if_false{conditional}_in"
                true_input = f"{name}__if_true{conditional}_in"
                false_output = f"{name}__if_false{conditional}"
                true_output = f"{name}__if_true{conditional}"
                for route_name in (false_input, true_input):
                    binding = QueueBinding(
                        route_name,
                        incoming.payload,
                        1,
                        1,
                        None,
                        scope=scope_path,
                        order=current_order,
                        route_output=True,
                    )
                    queues.append(binding)
                    by_name[route_name] = binding
                routes.append(
                    RouteBinding(
                        input_name,
                        (false_input, true_input),
                        argument,
                        condition,
                        1,
                        1,
                        scope_path,
                        current_order,
                        True,
                    )
                )

                for arm, arm_input, arm_output in (
                    (false_arm, false_input, false_output),
                    (true_arm, true_input, true_output),
                ):
                    branch_order = order
                    order += 1
                    binding = QueueBinding(
                        arm_output,
                        incoming.payload,
                        _positive_int(arm[2], "depth", 1),
                        _positive_int(arm[2], "latency", 1),
                        arm_input,
                        arm[3],
                        arm[4],
                        scope_path,
                        branch_order,
                    )
                    queues.append(binding)
                    by_name[arm_output] = binding

                merge_order = order
                order += 1
                output = QueueBinding(
                    name,
                    incoming.payload,
                    1,
                    1,
                    None,
                    scope=scope_path,
                    order=merge_order,
                    merge_output=True,
                )
                queues.append(output)
                by_name[name] = output
                merges.append(
                    MergeBinding(
                        (false_output, true_output),
                        name,
                        "priority",
                        1,
                        1,
                        scope_path,
                        merge_order,
                    )
                )
                continue
            if isinstance(statement, ast.With) and len(statement.items) == 1:
                item = statement.items[0]
                call = item.context_expr
                if (
                    item.optional_vars is None
                    and isinstance(call, ast.Call)
                    and call_name(call) == "scope"
                    and len(call.args) == 1
                    and isinstance(call.args[0], ast.Constant)
                    and type(call.args[0].value) is str
                    and call.args[0].value
                ):
                    path = (*scope_path, call.args[0].value)
                    if any(existing.path == path for existing in scopes):
                        raise QueueFrontendError("ACPY-QUEUE-004: duplicate scope path")
                    scopes.append(ScopeBinding(call.args[0].value, path, current_order))
                    visit(statement.body, path, aliases, atomic_group)
                    continue
            if (
                isinstance(statement, ast.With)
                and len(statement.items) == 1
                and atomic_group is None
            ):
                item = statement.items[0]
                call = item.context_expr
                if (
                    item.optional_vars is None
                    and isinstance(call, ast.Call)
                    and call_name(call) == "atomic"
                    and not call.args
                    and not call.keywords
                ):
                    group = len(atomics)
                    queue_start = len(queues)
                    scope_start = len(scopes)
                    route_start = len(routes)
                    fork_start = len(forks)
                    merge_start = len(merges)
                    reorder_start = len(reorders)
                    dependency_start = len(dependencies)
                    credit_start = len(credits)
                    barrier_start = len(barriers)
                    select_start = len(selects)
                    memory_instance_start = len(memory_instances)
                    memory_request_start = len(memory_requests)
                    feedback_start = len(feedbacks)
                    sink_start = len(sinks)
                    observation_start = len(observations)
                    expectation_start = len(expectations)
                    visit(statement.body, scope_path, aliases, group)
                    created = queues[queue_start:]
                    if (
                        len(created) < 2
                        or any(queue.atomic_group != group for queue in created)
                        or len(scopes) != scope_start
                        or len(routes) != route_start
                        or len(forks) != fork_start
                        or len(merges) != merge_start
                        or len(reorders) != reorder_start
                        or len(dependencies) != dependency_start
                        or len(credits) != credit_start
                        or len(barriers) != barrier_start
                        or len(selects) != select_start
                        or len(memory_instances) != memory_instance_start
                        or len(memory_requests) != memory_request_start
                        or len(feedbacks) != feedback_start
                        or len(sinks) != sink_start
                        or len(observations) != observation_start
                        or len(expectations) != expectation_start
                    ):
                        raise QueueFrontendError(
                            "ACPY-QUEUE-009: atomic requires at least two direct "
                            "Queue apply statements"
                        )
                    inputs = [queue.input_name for queue in created]
                    if None in inputs or len(set(inputs)) != len(inputs):
                        raise QueueFrontendError(
                            "ACPY-QUEUE-009: atomic inputs must be unique Queues"
                        )
                    atomics.append(
                        AtomicBinding(
                            tuple(queue.name for queue in created),
                            scope_path,
                            current_order,
                        )
                    )
                    continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Name)
                and isinstance(statement.value, ast.Call)
                and call_name(statement.value) in {"array", "map", "set"}
            ):
                name = statement.targets[0].id
                if (
                    name in by_name
                    or name in collections
                    or name in memory_by_name
                    or name in memory_arrays
                    or name in selected_memories
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-005: collection assignment requires one fresh name"
                    )
                call = statement.value
                is_memory_array = False
                if call_name(call) == "array" and len(call.args) == 2:
                    _, generator = _lambda(call.args[1])
                    is_memory_array = (
                        isinstance(generator, ast.Call)
                        and call_name(generator) == "memory"
                    )
                if is_memory_array:
                    extent = _static_int(call.args[0], {})
                    if extent is None or extent <= 0:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-015: memory array requires a positive "
                            "compile-time extent"
                        )
                    argument, generator = _lambda(call.args[1])
                    assert isinstance(generator, ast.Call)
                    pending: list[MemoryInstanceBinding] = []
                    for index in range(extent):
                        member_name = f"{name}__{index}"
                        if (
                            member_name in by_name
                            or member_name in collections
                            or member_name in memory_by_name
                            or member_name in memory_arrays
                            or member_name in selected_memories
                        ):
                            raise QueueFrontendError(
                                "ACPY-QUEUE-015: memory array element name "
                                "collides with an existing binding"
                            )
                        pending.append(
                            memory_instance_binding(
                                member_name,
                                generator,
                                scope_path,
                                current_order,
                                {argument: index},
                            )
                        )
                    configurations = {
                        (item.data_type, item.entries, item.init, item.latency)
                        for item in pending
                    }
                    if len(configurations) != 1:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-015: memory array elements must be homogeneous"
                        )
                    for instance in pending:
                        memory_instances.append(instance)
                        memory_by_name[instance.name] = instance
                    first = pending[0]
                    memory_arrays[name] = StaticMemoryArrayBinding(
                        name,
                        tuple(instance.name for instance in pending),
                        first.data_type,
                        first.entries,
                        first.init,
                        first.latency,
                        scope_path,
                        current_order,
                    )
                    continue
                collection = collection_binding(
                    name,
                    call,
                    scope_path,
                    current_order,
                    aliases,
                )
                assert collection is not None
                collections[name] = collection
                collection_bindings.append(
                    CollectionBinding(name, collection, scope_path, current_order)
                )
                continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Name)
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Attribute)
                and statement.value.func.attr == "select"
                and isinstance(statement.value.func.value, ast.Name)
                and statement.value.func.value.id in memory_arrays
            ):
                name = statement.targets[0].id
                if (
                    name in by_name
                    or name in collections
                    or name in memory_by_name
                    or name in memory_arrays
                    or name in selected_memories
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: selected memory requires one fresh name"
                    )
                call = statement.value
                if len(call.args) != 1 or any(
                    keyword.arg is None
                    or keyword.arg not in {"key", "depth", "latency"}
                    for keyword in call.keywords
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: memory array select requires one request Queue"
                    )
                keys = [
                    keyword.value for keyword in call.keywords if keyword.arg == "key"
                ]
                if len(keys) != 1:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: memory array select requires one key lambda"
                    )
                array = memory_arrays[call.func.value.id]
                if len(scope_path) < len(array.scope) or (
                    scope_path[: len(array.scope)] != array.scope
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: memory array is only visible in its "
                        "declaration scope and descendants"
                    )
                input_name = queue_reference(call.args[0], aliases)
                incoming = by_name[input_name]
                argument, selector = _lambda(keys[0])
                depth = _positive_int(call, "depth", 1)
                latency = _positive_int(call, "latency", 1)
                routed_inputs = tuple(
                    f"{name}__bank{index}_request"
                    for index in range(len(array.members))
                )
                for routed in routed_inputs:
                    if routed in by_name:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-015: selected memory synthetic Queue "
                            "name collides with an existing binding"
                        )
                    output = QueueBinding(
                        routed,
                        incoming.payload,
                        depth,
                        latency,
                        None,
                        scope=scope_path,
                        order=current_order,
                        route_output=True,
                    )
                    queues.append(output)
                    by_name[routed] = output
                routes.append(
                    RouteBinding(
                        input_name,
                        routed_inputs,
                        argument,
                        selector,
                        depth,
                        latency,
                        scope_path,
                        current_order,
                    )
                )
                selected_memories[name] = SelectedMemoryBinding(
                    name,
                    array.name,
                    input_name,
                    routed_inputs,
                    argument,
                    selector,
                    depth,
                    latency,
                    scope_path,
                    current_order,
                )
                continue
            if (
                isinstance(statement, ast.For)
                and isinstance(statement.target, ast.Name)
                and isinstance(statement.iter, ast.Call)
                and call_name(statement.iter) == "range"
                and len(statement.iter.args) == 1
                and not statement.iter.keywords
                and not statement.orelse
            ):
                extent = _static_int(statement.iter.args[0])
                if extent is None or extent < 0:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-005: range extent must be a non-negative "
                        "compile-time integer"
                    )

                class StaticIndex(ast.NodeTransformer):
                    def visit_Name(self, node: ast.Name) -> ast.expr:
                        if node.id == statement.target.id:
                            return ast.copy_location(ast.Constant(index), node)
                        return node

                for index in range(extent):
                    expanded = [
                        StaticIndex().visit(copy.deepcopy(body))
                        for body in statement.body
                    ]
                    visit(expanded, scope_path, aliases, atomic_group)
                continue
            if (
                isinstance(statement, ast.For)
                and isinstance(statement.target, ast.Name)
                and not statement.orelse
            ):
                collection = static_reference(statement.iter, aliases)
                if not isinstance(collection, StaticQueueCollection):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-005: compile-time for requires a static collection"
                    )
                for _, member in collection.members:
                    visit(
                        statement.body,
                        scope_path,
                        {**aliases, statement.target.id: member},
                        atomic_group,
                    )
                continue
            if isinstance(statement, ast.While) and not statement.orelse:
                body = list(statement.body)
                break_test: ast.expr | None = None
                continue_test: ast.expr | None = None
                if (
                    body
                    and isinstance(body[0], ast.If)
                    and len(body[0].body) == 1
                    and isinstance(body[0].body[0], ast.Break)
                    and not body[0].orelse
                ):
                    break_test = body.pop(0).test
                if (
                    body
                    and isinstance(body[-1], ast.If)
                    and len(body[-1].body) == 1
                    and isinstance(body[-1].body[0], ast.Continue)
                    and not body[-1].orelse
                ):
                    continue_test = body.pop().test
                if (
                    len(body) != 1
                    or not isinstance(body[0], ast.Assign)
                    or len(body[0].targets) != 1
                    or not isinstance(body[0].targets[0], ast.Name)
                    or not isinstance(body[0].value, ast.Call)
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-007: runtime while requires optional break, "
                        "one Queue update, and optional tail continue"
                    )
                update_statement = body[0]
                variable = update_statement.targets[0].id
                call = update_statement.value
                incoming = by_name.get(variable)
                if (
                    incoming is None
                    or not isinstance(call.func, ast.Attribute)
                    or call.func.attr != "apply"
                    or not isinstance(call.func.value, ast.Name)
                    or call.func.value.id != variable
                    or len(call.args) != 1
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-007: runtime while must rebind one Queue through apply"
                    )
                argument, update = _lambda(call.args[0])

                class QueueCondition(ast.NodeTransformer):
                    def visit_Name(self, node: ast.Name) -> ast.expr:
                        if node.id == variable:
                            return ast.copy_location(ast.Name(id=argument), node)
                        return node

                condition = QueueCondition().visit(copy.deepcopy(statement.test))
                assert isinstance(condition, ast.expr)
                if break_test is not None:
                    rewritten_break = QueueCondition().visit(copy.deepcopy(break_test))
                    assert isinstance(rewritten_break, ast.expr)
                    condition = ast.BoolOp(
                        op=ast.And(),
                        values=[
                            condition,
                            ast.UnaryOp(op=ast.Not(), operand=rewritten_break),
                        ],
                    )
                if continue_test is not None:
                    rewritten_continue = QueueCondition().visit(
                        copy.deepcopy(continue_test)
                    )
                    if not isinstance(rewritten_continue, ast.expr):
                        raise QueueFrontendError(
                            "ACPY-QUEUE-007: continue condition is invalid"
                        )
                    continue_probe = ast.UnaryOp(
                        op=ast.Not(), operand=rewritten_continue
                    )
                    condition = ast.BoolOp(
                        op=ast.And(),
                        values=[
                            condition,
                            ast.Compare(
                                left=continue_probe,
                                ops=[ast.Eq()],
                                comparators=[copy.deepcopy(continue_probe)],
                            ),
                        ],
                    )
                output_name = f"{variable}__feedback{len(feedbacks)}"
                depth = _positive_int(call, "depth", 1)
                latency = _positive_int(call, "latency", 1)
                output = QueueBinding(
                    output_name,
                    incoming.payload,
                    depth,
                    latency,
                    None,
                    scope=scope_path,
                    order=current_order,
                    feedback_output=True,
                )
                queues.append(output)
                by_name[variable] = output
                feedbacks.append(
                    FeedbackBinding(
                        incoming.name,
                        output_name,
                        argument,
                        condition,
                        update,
                        depth,
                        latency,
                        1024,
                        scope_path,
                        current_order,
                    )
                )
                continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Name)
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Attribute)
                and statement.value.func.attr == "merge"
                and isinstance(statement.value.func.value, ast.Name)
                and statement.value.func.value.id in by_name
            ):
                name = statement.targets[0].id
                if name in by_name or name in collections:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-008: merge output requires one fresh name"
                    )
                call = statement.value
                operands = [call.func.value, *call.args]
                inputs = tuple(
                    queue_reference(operand, aliases) for operand in operands
                )
                if len(inputs) < 2:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-008: merge requires at least two Queues"
                    )
                payload = by_name[inputs[0]].payload
                if any(by_name[input_name].payload != payload for input_name in inputs):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-008: merge Queue payloads must match"
                    )
                policies = [
                    keyword.value
                    for keyword in call.keywords
                    if keyword.arg == "policy"
                ]
                if len(policies) > 1 or (
                    policies
                    and (
                        not isinstance(policies[0], ast.Constant)
                        or policies[0].value not in {"round_robin", "priority"}
                    )
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-008: merge policy must be round_robin or priority"
                    )
                policy = policies[0].value if policies else "round_robin"
                depth = _positive_int(call, "depth", 1)
                latency = _positive_int(call, "latency", 1)
                output = QueueBinding(
                    name,
                    payload,
                    depth,
                    latency,
                    None,
                    scope=scope_path,
                    order=current_order,
                    merge_output=True,
                )
                queues.append(output)
                by_name[name] = output
                merges.append(
                    MergeBinding(
                        inputs,
                        name,
                        policy,
                        depth,
                        latency,
                        scope_path,
                        current_order,
                    )
                )
                continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Name)
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Attribute)
                and statement.value.func.attr == "reorder"
                and isinstance(statement.value.func.value, ast.Name)
                and not statement.value.args
            ):
                name = statement.targets[0].id
                if name in by_name or name in collections:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-013: reorder output requires one fresh name"
                    )
                call = statement.value
                incoming = by_name.get(call.func.value.id)
                if incoming is None:
                    raise QueueFrontendError("ACPY-QUEUE-013: reorder input is unbound")
                allowed_keywords = {"key", "capacity", "start", "depth", "latency"}
                if any(
                    keyword.arg is None or keyword.arg not in allowed_keywords
                    for keyword in call.keywords
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-013: reorder has an unsupported keyword"
                    )
                keys = [
                    keyword.value for keyword in call.keywords if keyword.arg == "key"
                ]
                if len(keys) != 1:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-013: reorder requires one key lambda"
                    )
                argument, key = _lambda(keys[0])
                capacity = _positive_int(call, "capacity", 16)
                start = _nonnegative_int(call, "start", 0)
                depth = _positive_int(call, "depth", 1)
                latency = _positive_int(call, "latency", 1)
                output = QueueBinding(
                    name,
                    incoming.payload,
                    depth,
                    latency,
                    None,
                    scope=scope_path,
                    order=current_order,
                    reorder_output=True,
                )
                queues.append(output)
                by_name[name] = output
                reorders.append(
                    ReorderBinding(
                        incoming.name,
                        name,
                        argument,
                        key,
                        capacity,
                        start,
                        depth,
                        latency,
                        scope_path,
                        current_order,
                    )
                )
                continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Name)
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Attribute)
                and statement.value.func.attr == "depend"
                and isinstance(statement.value.func.value, ast.Name)
                and not statement.value.args
            ):
                name = statement.targets[0].id
                if name in by_name or name in collections:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-014: dependency output requires one fresh name"
                    )
                call = statement.value
                incoming = by_name.get(call.func.value.id)
                if incoming is None:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-014: dependency input is unbound"
                    )
                allowed_keywords = {
                    "key",
                    "waits_for",
                    "resource",
                    "cost",
                    "capacity",
                    "resources",
                    "no_dependency",
                    "depth",
                    "latency",
                }
                if any(
                    keyword.arg is None or keyword.arg not in allowed_keywords
                    for keyword in call.keywords
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-014: dependency has an unsupported keyword"
                    )
                policies: dict[str, ast.expr] = {}
                for policy in ("key", "waits_for", "resource", "cost"):
                    values = [
                        keyword.value
                        for keyword in call.keywords
                        if keyword.arg == policy
                    ]
                    if len(values) != 1:
                        raise QueueFrontendError(
                            f"ACPY-QUEUE-014: dependency requires one {policy} lambda"
                        )
                    policies[policy] = values[0]
                key_argument, key = _lambda(policies["key"])
                waits_argument, waits_for = _lambda(policies["waits_for"])
                resource_argument, resource = _lambda(policies["resource"])
                cost_argument, cost = _lambda(policies["cost"])
                if (
                    len(
                        {
                            key_argument,
                            waits_argument,
                            resource_argument,
                            cost_argument,
                        }
                    )
                    != 1
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-014: dependency lambdas require one argument name"
                    )
                capacity = _positive_int(call, "capacity", 16)
                resources = _positive_int(call, "resources", 1)
                no_dependency = _nonnegative_int(call, "no_dependency", 255)
                depth = _positive_int(call, "depth", 1)
                latency = _positive_int(call, "latency", 1)
                output = QueueBinding(
                    name,
                    incoming.payload,
                    depth,
                    latency,
                    None,
                    scope=scope_path,
                    order=current_order,
                    dependency_output=True,
                )
                queues.append(output)
                by_name[name] = output
                dependencies.append(
                    DependencyBinding(
                        incoming.name,
                        name,
                        key_argument,
                        key,
                        waits_for,
                        resource,
                        cost,
                        capacity,
                        resources,
                        no_dependency,
                        depth,
                        latency,
                        scope_path,
                        current_order,
                    )
                )
                continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Name)
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Attribute)
                and statement.value.func.attr == "select"
                and isinstance(statement.value.func.value, ast.Name)
                and statement.value.func.value.id in collections
            ):
                name = statement.targets[0].id
                if name in by_name or name in collections:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-018: select output requires one fresh name"
                    )
                call = statement.value
                if len(call.args) != 1 or any(
                    keyword.arg is None
                    or keyword.arg not in {"key", "depth", "latency"}
                    for keyword in call.keywords
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-018: select requires one control Queue"
                    )
                control = queue_reference(call.args[0], aliases)
                collection = collections[call.func.value.id]
                if any(not isinstance(member, str) for _, member in collection.members):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-018: select requires a flat Queue collection"
                    )
                inputs = tuple(
                    member
                    for _, member in collection.members
                    if isinstance(member, str)
                )
                if len(inputs) < 2 or control in inputs:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-018: select requires two unique data Queues"
                    )
                payload = by_name[inputs[0]].payload
                if any(by_name[input_name].payload != payload for input_name in inputs):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-018: select data Queue payloads must match"
                    )
                keys = [
                    keyword.value for keyword in call.keywords if keyword.arg == "key"
                ]
                if len(keys) != 1:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-018: select requires one key lambda"
                    )
                argument, selector = _lambda(keys[0])
                depth = _positive_int(call, "depth", 1)
                latency = _positive_int(call, "latency", 1)
                output = QueueBinding(
                    name,
                    payload,
                    depth,
                    latency,
                    None,
                    scope=scope_path,
                    order=current_order,
                    select_output=True,
                )
                queues.append(output)
                by_name[name] = output
                selects.append(
                    SelectBinding(
                        control,
                        inputs,
                        name,
                        argument,
                        selector,
                        depth,
                        latency,
                        scope_path,
                        current_order,
                    )
                )
                continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Name)
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Attribute)
                and statement.value.func.attr == "credit"
                and isinstance(statement.value.func.value, ast.Name)
                and not statement.value.args
            ):
                name = statement.targets[0].id
                if name in by_name or name in collections:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-016: credit output requires one fresh name"
                    )
                call = statement.value
                incoming = by_name.get(call.func.value.id)
                if incoming is None:
                    raise QueueFrontendError("ACPY-QUEUE-016: credit input is unbound")
                allowed_keywords = {"cost", "credits", "depth", "latency"}
                if any(
                    keyword.arg is None or keyword.arg not in allowed_keywords
                    for keyword in call.keywords
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-016: credit has an unsupported keyword"
                    )
                costs = [
                    keyword.value for keyword in call.keywords if keyword.arg == "cost"
                ]
                if len(costs) != 1:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-016: credit requires one cost lambda"
                    )
                argument, cost = _lambda(costs[0])
                credit_count = _positive_int(call, "credits", 16)
                depth = _positive_int(call, "depth", 1)
                latency = _positive_int(call, "latency", 1)
                output = QueueBinding(
                    name,
                    incoming.payload,
                    depth,
                    latency,
                    None,
                    scope=scope_path,
                    order=current_order,
                    credit_output=True,
                )
                queues.append(output)
                by_name[name] = output
                credits.append(
                    CreditBinding(
                        incoming.name,
                        name,
                        argument,
                        cost,
                        credit_count,
                        depth,
                        latency,
                        scope_path,
                        current_order,
                    )
                )
                continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Name)
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Attribute)
                and statement.value.func.attr == "request"
                and isinstance(statement.value.func.value, ast.Name)
                and statement.value.func.value.id in selected_memories
                and not statement.value.args
            ):
                name = statement.targets[0].id
                if (
                    name in by_name
                    or name in collections
                    or name in memory_by_name
                    or name in memory_arrays
                    or name in selected_memories
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: memory request output requires one fresh name"
                    )
                call = statement.value
                selected_name = call.func.value.id
                selected = selected_memories[selected_name]
                if selected_name in consumed_selected_memories:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: selected memory may be requested only once"
                    )
                if selected.scope != scope_path:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: selected memory must be requested in the "
                        "same lexical scope"
                    )
                incoming = by_name[selected.input_name]
                array = memory_arrays[selected.array]
                (
                    argument,
                    address,
                    write,
                    data,
                    result_field,
                    depth,
                ) = memory_request_parameters(
                    call,
                    incoming,
                    array.data_type,
                    {"merge_policy", "merge_depth", "merge_latency"},
                )
                merge_policies = [
                    keyword.value
                    for keyword in call.keywords
                    if keyword.arg == "merge_policy"
                ]
                if len(merge_policies) > 1 or (
                    merge_policies
                    and (
                        not isinstance(merge_policies[0], ast.Constant)
                        or merge_policies[0].value not in {"priority", "round_robin"}
                    )
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: merge_policy must be priority or round_robin"
                    )
                merge_policy = (
                    merge_policies[0].value if merge_policies else "priority"
                )
                merge_depth = _positive_int(call, "merge_depth", 1)
                merge_latency = _positive_int(call, "merge_latency", 1)
                response_names = tuple(
                    f"{name}__bank{index}"
                    for index in range(len(array.members))
                )
                for instance_name, input_name, output_name in zip(
                    array.members,
                    selected.routed_inputs,
                    response_names,
                    strict=True,
                ):
                    if output_name in by_name:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-015: selected memory response Queue "
                            "name collides with an existing binding"
                        )
                    output = QueueBinding(
                        output_name,
                        incoming.payload,
                        depth,
                        1,
                        None,
                        scope=scope_path,
                        order=current_order,
                        memory_output=True,
                    )
                    queues.append(output)
                    by_name[output_name] = output
                    memory_requests.append(
                        MemoryRequestBinding(
                            instance_name,
                            input_name,
                            output_name,
                            argument,
                            address,
                            write,
                            data,
                            result_field,
                            depth,
                            scope_path,
                            current_order,
                        )
                    )
                merge_order = current_order + 1
                output = QueueBinding(
                    name,
                    incoming.payload,
                    merge_depth,
                    merge_latency,
                    None,
                    scope=scope_path,
                    order=merge_order,
                    merge_output=True,
                )
                queues.append(output)
                by_name[name] = output
                merges.append(
                    MergeBinding(
                        response_names,
                        name,
                        str(merge_policy),
                        merge_depth,
                        merge_latency,
                        scope_path,
                        merge_order,
                    )
                )
                consumed_selected_memories.add(selected_name)
                order += 1
                continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Name)
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Attribute)
                and statement.value.func.attr == "request"
                and isinstance(statement.value.func.value, ast.Name)
                and len(statement.value.args) == 1
            ):
                name = statement.targets[0].id
                if name in by_name or name in collections:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: memory request output requires one fresh name"
                    )
                call = statement.value
                instance = memory_by_name.get(call.func.value.id)
                if instance is None:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: memory request instance is unbound"
                    )
                if len(scope_path) < len(instance.scope) or (
                    scope_path[: len(instance.scope)] != instance.scope
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: memory instance is only visible in its "
                        "declaration scope and descendants"
                    )
                incoming_name = queue_reference(call.args[0], aliases)
                incoming = by_name.get(incoming_name)
                if incoming is None:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: memory request input is unbound"
                    )
                (
                    argument,
                    address,
                    write,
                    data,
                    result_field,
                    depth,
                ) = memory_request_parameters(call, incoming, instance.data_type)
                output = QueueBinding(
                    name,
                    incoming.payload,
                    depth,
                    1,
                    None,
                    scope=scope_path,
                    order=current_order,
                    memory_output=True,
                )
                queues.append(output)
                by_name[name] = output
                memory_requests.append(
                    MemoryRequestBinding(
                        instance.name,
                        incoming.name,
                        name,
                        argument,
                        address,
                        write,
                        data,
                        result_field,
                        depth,
                        scope_path,
                        current_order,
                    )
                )
                continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Attribute)
                and statement.value.func.attr == "memory"
            ):
                raise QueueFrontendError(
                    "ACPY-QUEUE-015: Queue.memory was removed; declare "
                    "ac.memory(...) and connect it with instance.request(...)"
                )
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Name)
                and isinstance(statement.value, ast.Call)
            ):
                name, call = statement.targets[0].id, statement.value
                if (
                    isinstance(call.func, ast.Name)
                    and call.func.id in recursive_helpers
                ):
                    if (
                        name in by_name
                        or name in collections
                        or len(call.args) != 2
                        or call.keywords
                    ):
                        raise QueueFrontendError(
                            "ACPY-QUEUE-020: recursive helper call is malformed"
                        )
                    input_name = queue_reference(call.args[0], aliases)
                    extent = _static_int(call.args[1])
                    if extent is None or extent < 0 or extent > 1024:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-020: recursion depth must be a compile-time "
                            "integer in [0, 1024]"
                        )
                    helper = recursive_helpers[call.func.id]
                    incoming = by_name[input_name]
                    if extent == 0:
                        by_name[name] = incoming
                        continue
                    previous = incoming
                    for index in range(extent):
                        output_name = (
                            name if index + 1 == extent else f"{name}__rec{index}"
                        )
                        binding = QueueBinding(
                            output_name,
                            incoming.payload,
                            _positive_int(helper.apply_call, "depth", 1),
                            _positive_int(helper.apply_call, "latency", 1),
                            previous.name,
                            helper.argument,
                            copy.deepcopy(helper.expression),
                            scope_path,
                            current_order,
                            atomic_group=atomic_group,
                        )
                        queues.append(binding)
                        by_name[output_name] = binding
                        previous = binding
                    continue
                if name in by_name or name in collections:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-001: queue assignment requires one fresh name"
                    )
                if call_name(call) == "source" and len(call.args) == 1:
                    binding = source_binding(name, call, scope_path, current_order)
                elif call_name(call) == "compute" and len(call.args) == 2:
                    if any(
                        keyword.arg is None
                        or keyword.arg not in {"depth", "latency", "rate"}
                        for keyword in call.keywords
                    ):
                        raise QueueFrontendError(
                            "ACPY-QUEUE-023: compute has an unsupported keyword"
                        )
                    input_name = queue_reference(call.args[0], aliases)
                    incoming = by_name[input_name]
                    argument, expression = _lambda(call.args[1])
                    depth = _positive_int(call, "depth", 1)
                    rate = _positive_int(call, "rate", incoming.rate)
                    if rate > depth:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-025: Queue rate must not exceed depth"
                        )
                    binding = QueueBinding(
                        name,
                        incoming.payload,
                        depth,
                        _positive_int(call, "latency", 1),
                        incoming.name,
                        argument,
                        expression,
                        scope_path,
                        current_order,
                        atomic_group=atomic_group,
                        provider="compute",
                        rate=rate,
                    )
                elif call_name(call) == "pipeline" and len(call.args) == 1:
                    if any(
                        keyword.arg is None
                        or keyword.arg not in {"stages", "depth", "rate"}
                        for keyword in call.keywords
                    ):
                        raise QueueFrontendError(
                            "ACPY-QUEUE-024: pipeline parameters are invalid"
                        )
                    input_name = queue_reference(call.args[0], aliases)
                    incoming = by_name[input_name]
                    stages = _positive_int(call, "stages", 1)
                    depth = _positive_int(call, "depth", 1)
                    rate = _positive_int(call, "rate", incoming.rate)
                    if rate > depth:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-025: Queue rate must not exceed depth"
                        )
                    binding = QueueBinding(
                        name,
                        incoming.payload,
                        depth,
                        stages,
                        incoming.name,
                        "item",
                        ast.Name(id="item", ctx=ast.Load()),
                        scope_path,
                        current_order,
                        atomic_group=atomic_group,
                        provider="pipeline",
                        rate=rate,
                    )
                elif call_name(call) == "merge":
                    if len(call.args) < 2 or any(
                        keyword.arg is None
                        or keyword.arg not in {"policy", "depth", "latency"}
                        for keyword in call.keywords
                    ):
                        raise QueueFrontendError(
                            "ACPY-QUEUE-024: merge requires two or more Queues and "
                            "static policy/depth/latency"
                        )
                    input_names = tuple(
                        queue_reference(argument, aliases) for argument in call.args
                    )
                    if len(set(input_names)) != len(input_names):
                        raise QueueFrontendError(
                            "ACPY-QUEUE-024: merge inputs must be unique Queues"
                        )
                    payloads_used = {by_name[item].payload for item in input_names}
                    if len(payloads_used) != 1:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-024: merge inputs require one payload type"
                        )
                    depth = _positive_int(call, "depth", 1)
                    latency = _positive_int(call, "latency", 1)
                    policy = policy_value(call)
                    binding = QueueBinding(
                        name,
                        by_name[input_names[0]].payload,
                        depth,
                        latency,
                        None,
                        scope=scope_path,
                        order=current_order,
                        merge_output=True,
                    )
                    merges.append(
                        MergeBinding(
                            input_names,
                            name,
                            policy,
                            depth,
                            latency,
                            scope_path,
                            current_order,
                        )
                    )
                elif call_name(call) == "schedule":
                    if len(call.args) != 1 or any(
                        keyword.arg is None
                        or keyword.arg
                        not in {
                            "by",
                            "waits_for",
                            "resource",
                            "cost",
                            "entries",
                            "resources",
                            "no_dependency",
                            "depth",
                            "latency",
                        }
                        for keyword in call.keywords
                    ):
                        raise QueueFrontendError(
                            "ACPY-QUEUE-024: schedule parameters are invalid"
                        )
                    input_name = queue_reference(call.args[0], aliases)
                    incoming = by_name[input_name]
                    argument = "item"
                    key = field_expression(keyword_value(call, "by"), incoming)
                    waits_for = field_expression(
                        keyword_value(call, "waits_for"), incoming
                    )
                    resource = field_expression(
                        keyword_value(call, "resource"), incoming
                    )
                    cost = field_expression(keyword_value(call, "cost"), incoming)
                    capacity = _positive_int(call, "entries", 16)
                    resources = _positive_int(call, "resources", 1)
                    no_dependency = _nonnegative_int(call, "no_dependency", 0)
                    depth = _positive_int(call, "depth", 1)
                    latency = _positive_int(call, "latency", 1)
                    binding = QueueBinding(
                        name,
                        incoming.payload,
                        depth,
                        latency,
                        None,
                        scope=scope_path,
                        order=current_order,
                        dependency_output=True,
                    )
                    dependencies.append(
                        DependencyBinding(
                            input_name,
                            name,
                            argument,
                            key,
                            waits_for,
                            resource,
                            cost,
                            capacity,
                            resources,
                            no_dependency,
                            depth,
                            latency,
                            scope_path,
                            current_order,
                            provider="schedule",
                        )
                    )
                elif call_name(call) == "engine":
                    if len(call.args) != 1 or any(
                        keyword.arg is None
                        or keyword.arg not in {"cost", "lanes", "depth", "latency"}
                        for keyword in call.keywords
                    ):
                        raise QueueFrontendError(
                            "ACPY-QUEUE-024: engine parameters are invalid"
                        )
                    input_name = queue_reference(call.args[0], aliases)
                    incoming = by_name[input_name]
                    argument = "item"
                    cost = field_expression(keyword_value(call, "cost"), incoming)
                    lane_count = _positive_int(call, "lanes", 1)
                    depth = _positive_int(call, "depth", 1)
                    latency = _positive_int(call, "latency", 1)
                    binding = QueueBinding(
                        name,
                        incoming.payload,
                        depth,
                        latency,
                        None,
                        scope=scope_path,
                        order=current_order,
                        credit_output=True,
                    )
                    credits_binding = CreditBinding(
                        input_name,
                        name,
                        argument,
                        cost,
                        lane_count,
                        depth,
                        latency,
                        scope_path,
                        current_order,
                        provider="engine",
                    )
                    credits.append(credits_binding)
                elif call_name(call) == "reorder":
                    if len(call.args) != 1 or any(
                        keyword.arg is None
                        or keyword.arg
                        not in {"by", "entries", "start", "depth", "latency"}
                        for keyword in call.keywords
                    ):
                        raise QueueFrontendError(
                            "ACPY-QUEUE-024: reorder parameters are invalid"
                        )
                    input_name = queue_reference(call.args[0], aliases)
                    incoming = by_name[input_name]
                    argument = "item"
                    key = field_expression(keyword_value(call, "by"), incoming)
                    capacity = _positive_int(call, "entries", 16)
                    start = _nonnegative_int(call, "start", 0)
                    depth = _positive_int(call, "depth", 1)
                    latency = _positive_int(call, "latency", 1)
                    binding = QueueBinding(
                        name,
                        incoming.payload,
                        depth,
                        latency,
                        None,
                        scope=scope_path,
                        order=current_order,
                        reorder_output=True,
                    )
                    reorders.append(
                        ReorderBinding(
                            input_name,
                            name,
                            argument,
                            key,
                            capacity,
                            start,
                            depth,
                            latency,
                            scope_path,
                            current_order,
                        )
                    )
                elif call_name(call) == "table":
                    raise QueueFrontendError(
                        "ACPY-TABLE-000: legacy ac.table(value, ...) was removed; "
                        "use ac.memory for request/response memory or "
                        "ac.table[entries, Entry](init=0) for state Table"
                    )
                elif (
                    isinstance(call.func, ast.Attribute)
                    and call.func.attr in {"apply", "firing"}
                    and isinstance(call.func.value, ast.Name)
                    and len(call.args) == 1
                ):
                    input_name = call.func.value.id
                    incoming = by_name.get(input_name)
                    if incoming is None:
                        raise QueueFrontendError(
                            f"ACPY-QUEUE-001: input queue {input_name!r} is unbound"
                        )
                    argument, expression = _lambda(call.args[0])
                    firing_effect = call.func.attr == "firing"
                    if firing_effect:
                        _validate_firing_expression(expression, argument)
                    binding = QueueBinding(
                        name,
                        incoming.payload,
                        _positive_int(call, "depth", 1),
                        _positive_int(call, "latency", 1),
                        incoming.name,
                        argument,
                        expression,
                        scope_path,
                        current_order,
                        firing_effect=firing_effect,
                        atomic_group=atomic_group,
                    )
                else:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-001: unsupported queue-producing call"
                    )
                queues.append(binding)
                by_name[name] = binding
                continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], (ast.Tuple, ast.List))
                and all(
                    isinstance(item, ast.Name) for item in statement.targets[0].elts
                )
                and isinstance(statement.value, ast.Call)
                and call_name(statement.value) == "barrier"
            ):
                call = statement.value
                if any(
                    keyword.arg is None or keyword.arg not in {"depth", "latency"}
                    for keyword in call.keywords
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-017: barrier has an unsupported keyword"
                    )
                method_style = (
                    isinstance(call.func, ast.Attribute)
                    and isinstance(call.func.value, ast.Name)
                    and call.func.value.id in by_name
                )
                operands = (
                    [call.func.value, *call.args] if method_style else list(call.args)
                )
                inputs = tuple(
                    queue_reference(operand, aliases) for operand in operands
                )
                outputs = tuple(item.id for item in statement.targets[0].elts)
                if len(inputs) < 2 or len(outputs) != len(inputs):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-017: barrier requires matching input/output arity"
                    )
                if len(set(inputs)) != len(inputs):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-017: barrier inputs must be unique Queues"
                    )
                if len(set(outputs)) != len(outputs) or any(
                    output in by_name or output in collections for output in outputs
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-017: barrier outputs require fresh tuple names"
                    )
                depth = _positive_int(call, "depth", 1)
                latency = _positive_int(call, "latency", 1)
                for input_name, output_name in zip(inputs, outputs, strict=True):
                    output = QueueBinding(
                        output_name,
                        by_name[input_name].payload,
                        depth,
                        latency,
                        None,
                        scope=scope_path,
                        order=current_order,
                        barrier_output=True,
                    )
                    queues.append(output)
                    by_name[output_name] = output
                barriers.append(
                    BarrierBinding(
                        inputs,
                        outputs,
                        depth,
                        latency,
                        scope_path,
                        current_order,
                    )
                )
                continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], (ast.Tuple, ast.List))
                and all(
                    isinstance(item, ast.Name) for item in statement.targets[0].elts
                )
                and isinstance(statement.value, ast.Call)
                and call_name(statement.value) == "route"
            ):
                call = statement.value
                method_style = (
                    isinstance(call.func, ast.Attribute)
                    and isinstance(call.func.value, ast.Name)
                    and call.func.value.id in by_name
                )
                if method_style:
                    assert isinstance(call.func, ast.Attribute)
                    assert isinstance(call.func.value, ast.Name)
                    input_name = call.func.value.id
                    if call.args:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-006: method route takes no positional arguments"
                        )
                else:
                    if len(call.args) != 1:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-024: route requires one input Queue"
                        )
                    input_name = queue_reference(call.args[0], aliases)
                incoming = by_name.get(input_name)
                if incoming is None:
                    raise QueueFrontendError(
                        f"ACPY-QUEUE-001: input queue {input_name!r} is unbound"
                    )
                output_count = _positive_int(call, "outputs", 0)
                names = tuple(item.id for item in statement.targets[0].elts)
                if output_count != len(names) or len(set(names)) != len(names):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-006: route outputs must match fresh tuple names"
                    )
                if method_style:
                    key = [
                        keyword.value
                        for keyword in call.keywords
                        if keyword.arg == "key"
                    ]
                    if len(key) != 1:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-006: route requires one key lambda"
                        )
                    argument, selector = _lambda(key[0])
                else:
                    argument = "item"
                    selector = field_expression(
                        keyword_value(call, "by"), incoming, argument
                    )
                depth = _positive_int(call, "depth", 1)
                latency = _positive_int(call, "latency", 1)
                for name in names:
                    if name in by_name:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-006: route output name is already bound"
                        )
                    output = QueueBinding(
                        name,
                        incoming.payload,
                        depth,
                        latency,
                        None,
                        scope=scope_path,
                        order=current_order,
                        route_output=True,
                    )
                    queues.append(output)
                    by_name[name] = output
                routes.append(
                    RouteBinding(
                        incoming.name,
                        names,
                        argument,
                        selector,
                        depth,
                        latency,
                        scope_path,
                        current_order,
                    )
                )
                continue
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], (ast.Tuple, ast.List))
                and all(
                    isinstance(item, ast.Name) for item in statement.targets[0].elts
                )
                and isinstance(statement.value, ast.Call)
                and call_name(statement.value) == "fork"
            ):
                call = statement.value
                method_style = (
                    isinstance(call.func, ast.Attribute)
                    and isinstance(call.func.value, ast.Name)
                    and call.func.value.id in by_name
                )
                if method_style:
                    assert isinstance(call.func, ast.Attribute)
                    assert isinstance(call.func.value, ast.Name)
                    if call.args:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-012: method fork takes no positional arguments"
                        )
                    input_name = call.func.value.id
                else:
                    if len(call.args) != 1:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-024: fork requires one input Queue"
                        )
                    input_name = queue_reference(call.args[0], aliases)
                incoming = by_name.get(input_name)
                if incoming is None:
                    raise QueueFrontendError("ACPY-QUEUE-012: fork input is unbound")
                output_count = _positive_int(call, "outputs", 0)
                names = tuple(item.id for item in statement.targets[0].elts)
                if output_count != len(names) or len(names) < 2:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-012: fork outputs must match tuple arity"
                    )
                depth = _positive_int(call, "depth", 1)
                latency = _positive_int(call, "latency", 1)
                for name in names:
                    if name in by_name:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-012: fork output name is already bound"
                        )
                    output = QueueBinding(
                        name,
                        incoming.payload,
                        depth,
                        latency,
                        None,
                        scope=scope_path,
                        order=current_order,
                        route_output=True,
                    )
                    queues.append(output)
                    by_name[name] = output
                forks.append(
                    ForkBinding(
                        incoming.name,
                        names,
                        depth,
                        latency,
                        scope_path,
                        current_order,
                    )
                )
                continue
            if (
                isinstance(statement, ast.Expr)
                and isinstance(statement.value, ast.Call)
                and call_name(statement.value) == "expect"
                and len(statement.value.args) == 1
            ):
                call = statement.value
                if any(
                    keyword.arg is None or keyword.arg not in {"predicate", "message"}
                    for keyword in call.keywords
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-021: expect has an unsupported keyword"
                    )
                predicates = [
                    keyword.value
                    for keyword in call.keywords
                    if keyword.arg == "predicate"
                ]
                messages = [
                    keyword.value
                    for keyword in call.keywords
                    if keyword.arg == "message"
                ]
                if len(predicates) != 1 or len(messages) != 1:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-021: expect requires predicate and message"
                    )
                if (
                    not isinstance(messages[0], ast.Constant)
                    or type(messages[0].value) is not str
                    or not messages[0].value
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-021: expect message must be a static string"
                    )
                argument, predicate = _lambda(predicates[0])
                expectations.append(
                    ExpectBinding(
                        queue_reference(call.args[0], aliases),
                        argument,
                        predicate,
                        messages[0].value,
                        scope_path,
                        current_order,
                    )
                )
                continue
            if (
                isinstance(statement, ast.Expr)
                and isinstance(statement.value, ast.Call)
                and call_name(statement.value) == "observe"
                and len(statement.value.args) == 1
            ):
                name = queue_reference(statement.value.args[0], aliases)
                observations.append(
                    ObservationBinding(
                        name, f"observe_{current_order}", scope_path, current_order
                    )
                )
                continue
            if (
                isinstance(statement, ast.Expr)
                and isinstance(statement.value, ast.Call)
                and call_name(statement.value) == "sink"
                and len(statement.value.args) == 1
            ):
                name = queue_reference(statement.value.args[0], aliases)
                sinks.append(SinkBinding(name, scope_path, current_order))
                continue
            if isinstance(statement, ast.Return) and statement.value is None:
                continue
            raise QueueFrontendError(
                f"ACPY-QUEUE-001: unsupported statement {type(statement).__name__}"
            )

    visit(function.body, ())
    unused_selected = sorted(set(selected_memories) - consumed_selected_memories)
    if unused_selected:
        raise QueueFrontendError(
            "ACPY-QUEUE-015: selected memory is not requested: "
            + ", ".join(repr(name) for name in unused_selected)
        )
    requests_by_instance: dict[str, list[MemoryRequestBinding]] = {}
    for request in memory_requests:
        requests_by_instance.setdefault(request.instance, []).append(request)
    for instance in memory_instances:
        endpoints = requests_by_instance.get(instance.name, [])
        if not endpoints:
            raise QueueFrontendError(
                f"ACPY-QUEUE-015: memory instance {instance.name!r} is not connected"
            )
        payload_types = {by_name[endpoint.input_name].payload for endpoint in endpoints}
        if len(payload_types) != 1:
            raise QueueFrontendError(
                "ACPY-QUEUE-015: all endpoints of one memory require one payload struct"
            )
    for table in tables:
        endpoint_count = sum(read.table == table.name for read in table_reads) + sum(
            write.table == table.name for write in table_writes
        )
        if endpoint_count == 0:
            raise QueueFrontendError(
                f"ACPY-TABLE-005: table {table.name!r} requires a read/write endpoint"
            )
    if not queues or not sinks:
        raise QueueFrontendError(
            "ACPY-QUEUE-001: a queue system requires source and sink boundaries"
        )
    return QueueProgram(
        system,
        payloads,
        tuple(queues),
        tuple(scopes),
        tuple(routes),
        tuple(forks),
        tuple(feedbacks),
        tuple(merges),
        tuple(reorders),
        tuple(dependencies),
        tuple(credits),
        tuple(barriers),
        tuple(selects),
        tuple(memory_instances),
        tuple(memory_requests),
        tuple(memories),
        tuple(tables),
        tuple(table_reads),
        tuple(table_writes),
        tuple(atomics),
        tuple(collection_bindings),
        tuple(observations),
        tuple(expectations),
        tuple(sinks),
        specialization_fingerprint,
    )


class _ExpressionEmitter:
    def __init__(
        self,
        payloads: dict[str, Payload],
        argument: str,
        payload: str,
        *,
        root_name: str = "item",
        prefix: str = "",
        allow_queue_effects: bool = False,
        table_views: Mapping[str, tuple[str, ast.expr, str]] | None = None,
    ) -> None:
        self.payloads = payloads
        self.argument = argument
        self.payload = payload
        self.root_name = root_name
        self.prefix = prefix
        self.allow_queue_effects = allow_queue_effects
        self.table_views = dict(table_views or {})
        self.lines: list[str] = []
        self.index = 0

    def _new(self) -> str:
        name = f"{self.prefix}v{self.index}"
        self.index += 1
        return name

    def emit(self, node: ast.expr, expected: str | None = None) -> tuple[str, str]:
        if isinstance(node, ast.Name) and node.id == self.argument:
            return self.root_name, self.payload
        if isinstance(node, ast.Name) and node.id in self.table_views:
            table, address, entry_type = self.table_views[node.id]
            index, index_type = self.emit(address)
            if not index_type.startswith("i"):
                raise QueueFrontendError(
                    "ACPY-TABLE-003: table index must lower to an integer"
                )
            name = self._new()
            self.lines.append(
                f"    %{name} = ac.table.get @{table} [%{index}] : "
                f"!ac.var<{index_type}> -> !ac.var<{entry_type}>"
            )
            return name, entry_type
        if (
            isinstance(node, ast.Call)
            and isinstance(node.func, ast.Attribute)
            and isinstance(node.func.value, ast.Name)
            and node.func.value.id == self.argument
            and node.func.attr in {"peek", "pop", "push"}
        ):
            if not self.allow_queue_effects:
                raise QueueFrontendError(
                    "ACPY-QUEUE-019: queue effects require firing()"
                )
            if node.func.attr in {"peek", "pop"}:
                if node.args or node.keywords:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-019: firing peek/pop take no arguments"
                    )
                return self.root_name, self.payload
            if len(node.args) != 1 or node.keywords:
                raise QueueFrontendError(
                    "ACPY-QUEUE-019: firing push requires one value"
                )
            return self.emit(node.args[0], self.payload)
        if isinstance(node, ast.Constant) and type(node.value) in {int, bool}:
            typ = expected or ("i1" if type(node.value) is bool else "i64")
            name = self._new()
            value = (
                "true"
                if node.value is True
                else "false"
                if node.value is False
                else str(node.value)
            )
            attribute = value if type(node.value) is bool else f"{value} : {typ}"
            self.lines.append(
                f"    %{name} = ac.var.constant {attribute} as !ac.var<{typ}>"
            )
            return name, typ
        if isinstance(node, ast.Attribute):
            record, record_type = self.emit(node.value)
            payload_name = record_type.removeprefix(
                "!ac.struct<@types::@"
            ).removesuffix(">")
            definition = self.payloads.get(payload_name)
            field_type = dict(definition.fields).get(node.attr) if definition else None
            if field_type is None:
                raise QueueFrontendError(f"ACPY-QUEUE-003: unknown field {node.attr!r}")
            name = self._new()
            self.lines.append(
                f'    %{name} = ac.var.get %{record} field "{node.attr}" : !ac.var<{record_type}> -> !ac.var<{field_type}>'
            )
            return name, field_type
        if isinstance(node, ast.BinOp) and isinstance(
            node.op, (ast.Add, ast.Sub, ast.Mult)
        ):
            left, left_type = self.emit(node.left)
            right, right_type = self.emit(node.right, left_type)
            if left_type != right_type:
                raise QueueFrontendError(
                    "ACPY-QUEUE-003: arithmetic operands must match"
                )
            opcode = {ast.Add: "add", ast.Sub: "sub", ast.Mult: "mul"}[type(node.op)]
            name = self._new()
            self.lines.append(
                f"    %{name} = ac.var.{opcode} %{left}, %{right} : !ac.var<{left_type}>"
            )
            return name, left_type
        if isinstance(node, ast.BoolOp) and isinstance(node.op, ast.And):
            if len(node.values) < 2:
                raise QueueFrontendError(
                    "ACPY-QUEUE-003: boolean and requires two operands"
                )
            current, current_type = self.emit(node.values[0], "i1")
            if current_type != "i1":
                raise QueueFrontendError("ACPY-QUEUE-003: boolean operands must be i1")
            for operand in node.values[1:]:
                value, value_type = self.emit(operand, "i1")
                if value_type != "i1":
                    raise QueueFrontendError(
                        "ACPY-QUEUE-003: boolean operands must be i1"
                    )
                name = self._new()
                self.lines.append(
                    f"    %{name} = ac.var.mul %{current}, %{value} : !ac.var<i1>"
                )
                current = name
            return current, "i1"
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.Not):
            value, value_type = self.emit(node.operand, "i1")
            if value_type != "i1":
                raise QueueFrontendError("ACPY-QUEUE-003: boolean not requires i1")
            false_value = self._new()
            self.lines.append(
                f"    %{false_value} = ac.var.constant false as !ac.var<i1>"
            )
            name = self._new()
            self.lines.append(
                f'    %{name} = ac.var.cmp "eq" %{value}, %{false_value} : '
                "!ac.var<i1> -> !ac.var<i1>"
            )
            return name, "i1"
        if (
            isinstance(node, ast.Compare)
            and len(node.ops) == len(node.comparators) == 1
        ):
            left, left_type = self.emit(node.left)
            right, right_type = self.emit(node.comparators[0], left_type)
            if left_type != right_type:
                raise QueueFrontendError(
                    "ACPY-QUEUE-003: comparison operands must match"
                )
            predicates = {
                ast.Eq: "eq",
                ast.NotEq: "ne",
                ast.Lt: "slt",
                ast.LtE: "sle",
                ast.Gt: "sgt",
                ast.GtE: "sge",
            }
            predicate = predicates.get(type(node.ops[0]))
            if predicate is None:
                raise QueueFrontendError("ACPY-QUEUE-003: unsupported comparison")
            name = self._new()
            self.lines.append(
                f'    %{name} = ac.var.cmp "{predicate}" %{left}, %{right} : '
                f"!ac.var<{left_type}> -> !ac.var<i1>"
            )
            return name, "i1"
        if (
            isinstance(node, ast.Call)
            and _decorator_name(node.func).rsplit(".", 1)[-1] == "popcount"
        ):
            if len(node.args) != 1 or node.keywords:
                raise QueueFrontendError(
                    "ACPY-QUEUE-003: popcount requires exactly one positional operand"
                )
            value, value_type = self.emit(node.args[0])
            if not value_type.startswith("i") or not value_type[1:].isdigit():
                raise QueueFrontendError(
                    "ACPY-QUEUE-003: popcount operand must be an integer payload"
                )
            width = int(value_type[1:])
            if width <= 0:
                raise QueueFrontendError(
                    "ACPY-QUEUE-003: popcount operand width must be positive"
                )
            result_width = width.bit_length()
            name = self._new()
            self.lines.append(
                f"    %{name} = ac.var.popcount %{value} : !ac.var<{value_type}> -> !ac.var<i{result_width}>"
            )
            return name, f"i{result_width}"
        if (
            isinstance(node, ast.Call)
            and isinstance(node.func, ast.Attribute)
            and node.func.attr == "with_fields"
            and not node.args
        ):
            record, record_type = self.emit(node.func.value)
            current = record
            for keyword in node.keywords:
                if keyword.arg is None:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-003: field unpacking is forbidden"
                    )
                payload_name = record_type.removeprefix(
                    "!ac.struct<@types::@"
                ).removesuffix(">")
                definition = self.payloads.get(payload_name)
                field_type = (
                    dict(definition.fields).get(keyword.arg) if definition else None
                )
                if field_type is None:
                    raise QueueFrontendError(
                        f"ACPY-QUEUE-003: unknown field {keyword.arg!r}"
                    )
                value, value_type = self.emit(keyword.value, field_type)
                if value_type != field_type:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-003: field update type mismatch"
                    )
                name = self._new()
                self.lines.append(
                    f'    %{name} = ac.var.with %{current}, %{value} field "{keyword.arg}" : !ac.var<{record_type}>, !ac.var<{field_type}> -> !ac.var<{record_type}>'
                )
                current = name
            return current, record_type
        raise QueueFrontendError("ACPY-QUEUE-003: unsupported lambda expression")


def lower_queue_program(program: QueueProgram) -> str:
    specialization = (
        ""
        if program.specialization_fingerprint is None
        else f', ac.specialization = "{program.specialization_fingerprint}"'
    )
    lines = [
        f'module attributes {{ac.contract_epoch = "0.4", '
        f'ac.system = "{program.system}"{specialization}}} {{'
    ]
    payloads = {item.name: item for item in program.payloads}
    if program.payloads:
        lines.append("  ac.type_scope @types {")
        for payload in program.payloads:
            fields = ", ".join(
                f'{{name = "{name}", type = {typ}}}' for name, typ in payload.fields
            )
            lines.append(f"    ac.struct @{payload.name} fields [{fields}]")
        layouts: list[str] = []
        for payload in program.payloads:
            sizes = [
                max(1, (int(typ.removeprefix("i")) + 7) // 8)
                for _, typ in payload.fields
            ]
            alignment = max(sizes)
            offset = 0
            for size in sizes:
                offset = ((offset + size - 1) // size) * size + size
            total = ((offset + alignment - 1) // alignment) * alignment
            layouts.append(
                f"!ac.struct<@types::@{payload.name}> = "
                f'{{abi_alignment = {alignment} : i64, endianness = "little", '
                f"preferred_alignment = {alignment} : i64, size = {total} : i64}}"
            )
        lines.append("  } {dlti.dl_spec = #dlti.dl_spec<" + ", ".join(layouts) + ">}")
    for instance in sorted(
        program.memory_instances, key=lambda value: (value.scope, value.order, value.name)
    ):
        owner = "/" + "/".join(instance.scope) if instance.scope else "/"
        stable_id = (
            "/".join((*instance.scope, instance.name))
            if instance.scope
            else instance.name
        )
        lines.append(
            f'  ac.memory.instance @{instance.name} data {instance.data_type} '
            f'entries {instance.entries} init {instance.init} '
            f'latency {instance.latency} owner "{owner}" '
            f'stable_id "memory/{stable_id}"'
        )
    for table in sorted(
        program.tables, key=lambda value: (value.scope, value.order, value.name)
    ):
        owner = "/" + "/".join(table.scope) if table.scope else "/"
        stable_id = "/".join((*table.scope, table.name)) if table.scope else table.name
        lines.append(
            f"  ac.table @{table.name} entry {table.entry_type} "
            f"entries {table.entries} init 0 owner \"{owner}\" "
            f"stable_id \"table/{stable_id}\""
        )
    by_name = {item.name: item for item in program.queues}
    memory_ordinals: dict[tuple[str, str], int] = {}
    requests_by_instance: dict[str, list[MemoryRequestBinding]] = {}
    for request in program.memory_requests:
        requests_by_instance.setdefault(request.instance, []).append(request)
    for instance, requests in requests_by_instance.items():
        for ordinal, request in enumerate(
            sorted(requests, key=lambda value: (value.scope, value.order, value.output_name))
        ):
            memory_ordinals[(instance, request.output_name)] = ordinal

    def name_array(names: list[str] | tuple[str, ...]) -> str:
        return "[" + ", ".join(f'"{name}"' for name in names) + "]"

    consumers: dict[str, list[QueueBinding]] = {}
    for queue in program.queues:
        if queue.input_name is not None:
            consumers.setdefault(queue.input_name, []).append(queue)
    fanouts: dict[str, tuple[tuple[str, ...], tuple[QueueBinding, ...]]] = {}

    def common_scope(scopes: list[tuple[str, ...]]) -> tuple[str, ...]:
        common: list[str] = []
        for parts in zip(*scopes, strict=False):
            if len(set(parts)) != 1:
                break
            common.append(parts[0])
        return tuple(common)

    for source_name, group in consumers.items():
        if len(group) < 2:
            continue
        fanouts[source_name] = (
            common_scope([consumer.scope for consumer in group]),
            tuple(group),
        )
    payload_by_queue = {name: queue.payload for name, queue in by_name.items()}
    queue_scope = {name: queue.scope for name, queue in by_name.items()}
    effective_input: dict[str, str] = {}
    for source_name, (fanout_scope, group) in fanouts.items():
        for index, consumer in enumerate(group):
            synthetic = f"{source_name}__fanout{index}"
            effective_input[consumer.name] = synthetic
            payload_by_queue[synthetic] = by_name[source_name].payload
            queue_scope[synthetic] = fanout_scope

    uses: dict[str, list[tuple[str, ...]]] = {name: [] for name in payload_by_queue}
    for queue in program.queues:
        if queue.input_name is None:
            continue
        selected = effective_input.get(queue.name, queue.input_name)
        uses[selected].append(queue.scope)
    for source_name, (fanout_scope, _) in fanouts.items():
        uses[source_name].append(fanout_scope)
    for sink_binding in program.sinks:
        uses[sink_binding.queue].append(sink_binding.scope)
    for observation in program.observations:
        uses[observation.queue].append(observation.scope)
    for expectation in program.expectations:
        uses[expectation.queue].append(expectation.scope)
    for route in program.routes:
        uses[route.input_name].append(route.scope)
    for fork in program.forks:
        uses[fork.input_name].append(fork.scope)
    for feedback in program.feedbacks:
        uses[feedback.input_name].append(feedback.scope)
    for merge in program.merges:
        for input_name in merge.inputs:
            uses[input_name].append(merge.scope)
    for reorder in program.reorders:
        uses[reorder.input_name].append(reorder.scope)
    for dependency in program.dependencies:
        uses[dependency.input_name].append(dependency.scope)
    for credit in program.credits:
        uses[credit.input_name].append(credit.scope)
    for barrier in program.barriers:
        for input_name in barrier.inputs:
            uses[input_name].append(barrier.scope)
    for select in program.selects:
        uses[select.control].append(select.scope)
        for input_name in select.inputs:
            uses[input_name].append(select.scope)
    for request in program.memory_requests:
        uses[request.input_name].append(request.scope)
    for read in program.table_reads:
        if read.input_name is not None:
            uses[read.input_name].append(read.scope)
    for write in program.table_writes:
        uses[write.input_name].append(write.scope)

    def inside(container: tuple[str, ...], candidate: tuple[str, ...]) -> bool:
        return candidate[: len(container)] == container

    def scope_io(path: tuple[str, ...]) -> tuple[list[str], list[str]]:
        inputs = [
            name
            for name, producer_scope in queue_scope.items()
            if not inside(path, producer_scope)
            and any(inside(path, use) for use in uses[name])
        ]
        outputs = [
            name
            for name, producer_scope in queue_scope.items()
            if inside(path, producer_scope)
            and any(not inside(path, use) for use in uses[name])
        ]
        return inputs, outputs

    def queue_attributes(name: str, rates: tuple[int, ...]) -> str:
        attributes = [f'ac.name = "{name}"']
        if any(rate != 1 for rate in rates):
            attributes.append(
                "ac.output_rates = array<i64: "
                + ", ".join(str(rate) for rate in rates)
                + ">"
            )
        return "{" + ", ".join(attributes) + "}"

    def emit_queue(
        queue: QueueBinding,
        output_ssa: str,
        mapping: dict[str, str],
        indent: str,
    ) -> None:
        if queue.input_name is None:
            lines.append(
                f"{indent}%{output_ssa} = ac.source depth {queue.depth} "
                f"latency {queue.latency} "
                f"{queue_attributes(queue.name, (queue.rate,))} : "
                f"!ac.queue<{queue.payload}>"
            )
            mapping[queue.name] = output_ssa
            return
        assert queue.argument is not None and queue.expression is not None
        emitter = _ExpressionEmitter(
            payloads,
            queue.argument,
            queue.payload,
            allow_queue_effects=queue.firing_effect,
        )
        result, result_type = emitter.emit(queue.expression)
        if result_type != queue.payload:
            raise QueueFrontendError(
                "ACPY-QUEUE-003: lambda result must preserve Queue payload type"
            )
        input_name = effective_input.get(queue.name, queue.input_name)
        input_ssa = mapping[input_name]
        lines.append(
            f"{indent}%{output_ssa} = ac.transform %{input_ssa} "
            f"depths [{queue.depth}] latencies [{queue.latency}] {{"
        )
        lines.append(f"{indent}^transform(%item: !ac.var<{queue.payload}>):")
        lines.extend(indent + line[2:] for line in emitter.lines)
        lines.append(
            f"{indent}  ac.transform.yield %{result} : !ac.var<{queue.payload}>"
        )
        lines.append(
            f"{indent}}} {queue_attributes(queue.name, (queue.rate,))} : "
            f"(!ac.queue<{queue.payload}>) -> "
            f"!ac.queue<{queue.payload}>"
        )
        mapping[queue.name] = output_ssa

    def render_items(
        path: tuple[str, ...], mapping: dict[str, str], indent: str
    ) -> None:
        def visible_order(consumer: QueueBinding) -> int:
            if consumer.scope == path:
                return consumer.order
            child_path = (*path, consumer.scope[len(path)])
            return next(
                scope.order for scope in program.scopes if scope.path == child_path
            )

        events: list[tuple[float, str, object]] = []
        events.extend(
            (queue.order, "queue", queue)
            for queue in program.queues
            if queue.scope == path
            and not queue.route_output
            and not queue.feedback_output
            and not queue.merge_output
            and not queue.reorder_output
            and not queue.dependency_output
            and not queue.credit_output
            and not queue.memory_output
            and not queue.table_read_output
            and not queue.barrier_output
            and not queue.select_output
            and queue.atomic_group is None
        )
        events.extend(
            (atomic.order, "atomic", atomic)
            for atomic in program.atomics
            if atomic.scope == path
        )
        events.extend(
            (fork.order, "fork", fork) for fork in program.forks if fork.scope == path
        )
        events.extend(
            (route.order, "route", route)
            for route in program.routes
            if route.scope == path
        )
        events.extend(
            (merge.order, "merge", merge)
            for merge in program.merges
            if merge.scope == path
        )
        events.extend(
            (feedback.order, "feedback", feedback)
            for feedback in program.feedbacks
            if feedback.scope == path
        )
        events.extend(
            (reorder.order, "reorder", reorder)
            for reorder in program.reorders
            if reorder.scope == path
        )
        events.extend(
            (dependency.order, "dependency", dependency)
            for dependency in program.dependencies
            if dependency.scope == path
        )
        events.extend(
            (credit.order, "credit", credit)
            for credit in program.credits
            if credit.scope == path
        )
        events.extend(
            (barrier.order, "barrier", barrier)
            for barrier in program.barriers
            if barrier.scope == path
        )
        events.extend(
            (select.order, "select", select)
            for select in program.selects
            if select.scope == path
        )
        events.extend(
            (request.order, "memory_request", request)
            for request in program.memory_requests
            if request.scope == path
        )
        events.extend(
            (read.order, "table_read", read)
            for read in program.table_reads
            if read.scope == path
        )
        events.extend(
            (write.order, "table_write", write)
            for write in program.table_writes
            if write.scope == path
        )
        events.extend(
            (
                min(visible_order(consumer) for consumer in group) - 0.5,
                "broadcast",
                source,
            )
            for source, (fanout_scope, group) in fanouts.items()
            if fanout_scope == path
        )
        events.extend(
            (scope.order, "scope", scope)
            for scope in program.scopes
            if scope.path[:-1] == path
        )
        events.extend(
            (observation.order, "observe", observation)
            for observation in program.observations
            if observation.scope == path
        )
        events.extend(
            (expectation.order, "expect", expectation)
            for expectation in program.expectations
            if expectation.scope == path
        )
        events.extend(
            (sink_binding.order, "sink", sink_binding)
            for sink_binding in program.sinks
            if sink_binding.scope == path
        )
        for _, kind, item in sorted(events, key=lambda event: event[0]):
            if kind == "queue":
                queue = item
                assert isinstance(queue, QueueBinding)
                output = queue.name if not path else f"{queue.name}__local"
                emit_queue(queue, output, mapping, indent)
            elif kind == "scope":
                scope = item
                assert isinstance(scope, ScopeBinding)
                render_scope(scope, mapping, indent)
            elif kind == "broadcast":
                source = item
                assert isinstance(source, str)
                _, group = fanouts[source]
                outputs = [f"{source}__fanout{index}" for index in range(len(group))]
                lhs = ", ".join(f"%{name}" for name in outputs)
                depths = ", ".join("1" for _ in outputs)
                payload = payload_by_queue[source]
                output_types = ", ".join(f"!ac.queue<{payload}>" for _ in outputs)
                lines.append(
                    f"{indent}{lhs} = ac.broadcast %{mapping[source]} depths "
                    f"[{depths}] latencies [{depths}] "
                    f"{{ac.output_names = {name_array(outputs)}}} : "
                    f"!ac.queue<{payload}> -> "
                    f"({output_types})"
                )
                for consumer, output in zip(group, outputs, strict=True):
                    mapping[effective_input[consumer.name]] = output
            elif kind == "barrier":
                barrier = item
                assert isinstance(barrier, BarrierBinding)
                output_names = [
                    name if not path else f"{name}__local" for name in barrier.outputs
                ]
                lhs = ", ".join(f"%{name}" for name in output_names)
                operands = ", ".join(
                    f"%{mapping[input_name]}" for input_name in barrier.inputs
                )
                depths = ", ".join(str(barrier.depth) for _ in output_names)
                latencies = ", ".join(str(barrier.latency) for _ in output_names)
                input_types = ", ".join(
                    f"!ac.queue<{by_name[input_name].payload}>"
                    for input_name in barrier.inputs
                )
                output_types = ", ".join(
                    f"!ac.queue<{by_name[input_name].payload}>"
                    for input_name in barrier.inputs
                )
                lines.append(
                    f"{indent}{lhs} = ac.barrier {operands} depths [{depths}] "
                    f"latencies [{latencies}] "
                    f"{{ac.output_names = {name_array(barrier.outputs)}}} : "
                    f"({input_types}) -> ({output_types})"
                )
                for name, output in zip(barrier.outputs, output_names, strict=True):
                    mapping[name] = output
            elif kind == "select":
                select = item
                assert isinstance(select, SelectBinding)
                control = by_name[select.control]
                emitter = _ExpressionEmitter(payloads, select.argument, control.payload)
                selector, selector_type = emitter.emit(select.selector)
                if not selector_type.startswith("i"):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-018: select key must lower to an integer"
                    )
                output = select.output if not path else f"{select.output}__local"
                operands = ", ".join(
                    f"%{mapping[name]}" for name in (select.control, *select.inputs)
                )
                input_types = ", ".join(
                    f"!ac.queue<{by_name[name].payload}>"
                    for name in (select.control, *select.inputs)
                )
                lines.append(
                    f"{indent}%{output} = ac.select {operands} "
                    f"depth {select.depth} latency {select.latency} key {{"
                )
                lines.append(f"{indent}^key(%item: !ac.var<{control.payload}>):")
                lines.extend(indent + line[2:] for line in emitter.lines)
                lines.append(
                    f"{indent}  ac.select.yield %{selector} : !ac.var<{selector_type}>"
                )
                lines.append(
                    f'{indent}}} {{ac.name = "{select.output}"}} : '
                    f"({input_types}) -> !ac.queue<{by_name[select.output].payload}>"
                )
                mapping[select.output] = output
            elif kind == "route":
                route = item
                assert isinstance(route, RouteBinding)
                incoming = by_name[route.input_name]
                emitter = _ExpressionEmitter(payloads, route.argument, incoming.payload)
                selector, selector_type = emitter.emit(route.selector)
                if route.boolean_selector and selector_type != "i1":
                    raise QueueFrontendError(
                        "ACPY-QUEUE-011: runtime if condition must lower to bool"
                    )
                if not route.boolean_selector and not selector_type.startswith("i"):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-006: route key must lower to an integer"
                    )
                output_names = [
                    name if not path else f"{name}__local" for name in route.outputs
                ]
                lhs = ", ".join(f"%{name}" for name in output_names)
                depths = ", ".join(str(route.depth) for _ in output_names)
                latencies = ", ".join(str(route.latency) for _ in output_names)
                output_types = ", ".join(
                    f"!ac.queue<{incoming.payload}>" for _ in output_names
                )
                lines.append(
                    f"{indent}{lhs} = ac.route %{mapping[route.input_name]} "
                    f"depths [{depths}] latencies [{latencies}] {{"
                )
                lines.append(f"{indent}^selector(%item: !ac.var<{incoming.payload}>):")
                lines.extend(indent + line[2:] for line in emitter.lines)
                lines.append(
                    f"{indent}  ac.route.yield %{selector} : !ac.var<{selector_type}>"
                )
                lines.append(
                    f"{indent}}} "
                    f"{{ac.output_names = {name_array(route.outputs)}}} : "
                    f"!ac.queue<{incoming.payload}> -> ({output_types})"
                )
                for name, output in zip(route.outputs, output_names, strict=True):
                    mapping[name] = output
            elif kind == "fork":
                fork = item
                assert isinstance(fork, ForkBinding)
                incoming = by_name[fork.input_name]
                output_names = [
                    name if not path else f"{name}__local" for name in fork.outputs
                ]
                lhs = ", ".join(f"%{name}" for name in output_names)
                depths = ", ".join(str(fork.depth) for _ in output_names)
                latencies = ", ".join(str(fork.latency) for _ in output_names)
                output_types = ", ".join(
                    f"!ac.queue<{incoming.payload}>" for _ in output_names
                )
                lines.append(
                    f"{indent}{lhs} = ac.fork %{mapping[fork.input_name]} "
                    f"depths [{depths}] latencies [{latencies}] "
                    f"{{ac.output_names = {name_array(fork.outputs)}}} : "
                    f"!ac.queue<{incoming.payload}> -> ({output_types})"
                )
                for name, output in zip(fork.outputs, output_names, strict=True):
                    mapping[name] = output
            elif kind == "feedback":
                feedback = item
                assert isinstance(feedback, FeedbackBinding)
                incoming = by_name[feedback.input_name]
                emitter = _ExpressionEmitter(
                    payloads, feedback.argument, incoming.payload
                )
                condition, condition_type = emitter.emit(feedback.condition)
                update, update_type = emitter.emit(feedback.update)
                if condition_type != "i1" or update_type != incoming.payload:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-007: while condition must be bool and update "
                        "must preserve Queue payload"
                    )
                output = (
                    feedback.output_name
                    if not path
                    else f"{feedback.output_name}__local"
                )
                lines.append(
                    f"{indent}%{output} = ac.feedback %{mapping[feedback.input_name]} "
                    f"depth {feedback.depth} latency {feedback.latency} "
                    f"max_iterations {feedback.max_iterations} {{"
                )
                lines.append(f"{indent}^body(%item: !ac.var<{incoming.payload}>):")
                lines.extend(indent + line[2:] for line in emitter.lines)
                lines.append(
                    f"{indent}  ac.feedback.yield %{update} continue %{condition} : "
                    f"!ac.var<{incoming.payload}>, !ac.var<i1>"
                )
                lines.append(
                    f'{indent}}} {{ac.name = "{feedback.output_name}"}} : '
                    f"!ac.queue<{incoming.payload}> -> "
                    f"!ac.queue<{incoming.payload}>"
                )
                mapping[feedback.output_name] = output
            elif kind == "reorder":
                reorder = item
                assert isinstance(reorder, ReorderBinding)
                incoming = by_name[reorder.input_name]
                emitter = _ExpressionEmitter(
                    payloads, reorder.argument, incoming.payload
                )
                key, key_type = emitter.emit(reorder.key)
                if not key_type.startswith("i"):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-013: reorder key must lower to an integer"
                    )
                output = (
                    reorder.output_name if not path else f"{reorder.output_name}__local"
                )
                lines.append(
                    f"{indent}%{output} = ac.reorder "
                    f"%{mapping[reorder.input_name]} capacity {reorder.capacity} "
                    f"start {reorder.start} depth {reorder.depth} "
                    f"latency {reorder.latency} {{"
                )
                lines.append(f"{indent}^key(%item: !ac.var<{incoming.payload}>):")
                lines.extend(indent + line[2:] for line in emitter.lines)
                lines.append(f"{indent}  ac.reorder.yield %{key} : !ac.var<{key_type}>")
                lines.append(
                    f'{indent}}} {{ac.name = "{reorder.output_name}"}} : '
                    f"!ac.queue<{incoming.payload}> -> "
                    f"!ac.queue<{incoming.payload}>"
                )
                mapping[reorder.output_name] = output
            elif kind == "dependency":
                dependency = item
                assert isinstance(dependency, DependencyBinding)
                incoming = by_name[dependency.input_name]
                policies = (
                    ("key", dependency.key),
                    ("waits_for", dependency.waits_for),
                    ("resource", dependency.resource),
                    ("cost", dependency.cost),
                )
                emitted: list[tuple[str, str, list[str]]] = []
                for policy_name, expression in policies:
                    emitter = _ExpressionEmitter(
                        payloads, dependency.argument, incoming.payload
                    )
                    value, value_type = emitter.emit(expression)
                    if not value_type.startswith("i"):
                        raise QueueFrontendError(
                            "ACPY-QUEUE-014: dependency policies must lower to integers"
                        )
                    emitted.append((value, value_type, emitter.lines))
                if emitted[0][1] != emitted[1][1]:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-014: key and waits_for types must match"
                    )
                output = (
                    dependency.output_name
                    if not path
                    else f"{dependency.output_name}__local"
                )
                lines.append(
                    f"{indent}%{output} = ac.dependency "
                    f"%{mapping[dependency.input_name]} capacity "
                    f"{dependency.capacity} resources {dependency.resources} "
                    f"no_dependency "
                    f"{dependency.no_dependency} depth {dependency.depth} "
                    f"latency {dependency.latency} key {{"
                )
                for index, policy_name in enumerate(
                    ("key", "waits_for", "resource", "cost")
                ):
                    if index:
                        lines.append(f"{indent}}} {policy_name} {{")
                    lines.append(
                        f"{indent}^{policy_name}(%item: !ac.var<{incoming.payload}>):"
                    )
                    value, value_type, policy_lines = emitted[index]
                    lines.extend(indent + line[2:] for line in policy_lines)
                    lines.append(
                        f"{indent}  ac.dependency.yield %{value} : "
                        f"!ac.var<{value_type}>"
                    )
                lines.append(
                    f'{indent}}} {{ac.name = "{dependency.output_name}"}} : '
                    f"!ac.queue<{incoming.payload}> -> "
                    f"!ac.queue<{incoming.payload}>"
                )
                mapping[dependency.output_name] = output
            elif kind == "credit":
                credit = item
                assert isinstance(credit, CreditBinding)
                incoming = by_name[credit.input_name]
                emitter = _ExpressionEmitter(
                    payloads, credit.argument, incoming.payload
                )
                cost, cost_type = emitter.emit(credit.cost)
                if not cost_type.startswith("i"):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-016: credit cost must lower to an integer"
                    )
                output = (
                    credit.output_name if not path else f"{credit.output_name}__local"
                )
                lines.append(
                    f"{indent}%{output} = ac.credit "
                    f"%{mapping[credit.input_name]} credits {credit.credits} "
                    f"depth {credit.depth} latency {credit.latency} cost {{"
                )
                lines.append(f"{indent}^cost(%item: !ac.var<{incoming.payload}>):")
                lines.extend(indent + line[2:] for line in emitter.lines)
                lines.append(
                    f"{indent}  ac.credit.yield %{cost} : !ac.var<{cost_type}>"
                )
                lines.append(
                    f'{indent}}} {{ac.name = "{credit.output_name}"}} : '
                    f"!ac.queue<{incoming.payload}> -> "
                    f"!ac.queue<{incoming.payload}>"
                )
                mapping[credit.output_name] = output
            elif kind == "memory_request":
                memory = item
                assert isinstance(memory, MemoryRequestBinding)
                incoming = by_name[memory.input_name]
                instance = next(
                    value
                    for value in program.memory_instances
                    if value.name == memory.instance
                )
                policies = (
                    ("address", memory.address),
                    ("write", memory.write),
                    ("data", memory.data),
                )
                emitted: list[tuple[str, str, list[str]]] = []
                for policy_name, expression in policies:
                    emitter = _ExpressionEmitter(
                        payloads, memory.argument, incoming.payload
                    )
                    value, value_type = emitter.emit(expression)
                    emitted.append((value, value_type, emitter.lines))
                if not emitted[0][1].startswith("i"):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: memory address must lower to an integer"
                    )
                if emitted[1][1] != "i1":
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: memory write must lower to bool"
                    )
                if emitted[2][1] != instance.data_type:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: memory data must match result_field"
                    )
                output = (
                    memory.output_name if not path else f"{memory.output_name}__local"
                )
                lines.append(
                    f"{indent}%{output} = ac.memory.request @{memory.instance}, "
                    f"%{mapping[memory.input_name]} ordinal "
                    f"{memory_ordinals[(memory.instance, memory.output_name)]} "
                    f'result_field "{memory.result_field}" '
                    f"depth {memory.depth} address {{"
                )
                for index, policy_name in enumerate(("address", "write", "data")):
                    if index:
                        lines.append(f"{indent}}} {policy_name} {{")
                    lines.append(
                        f"{indent}^{policy_name}(%item: !ac.var<{incoming.payload}>):"
                    )
                    value, value_type, policy_lines = emitted[index]
                    lines.extend(indent + line[2:] for line in policy_lines)
                    lines.append(
                        f"{indent}  ac.memory.yield %{value} : !ac.var<{value_type}>"
                    )
                lines.append(
                    f'{indent}}} {{ac.endpoint_path = "'
                    f'{"/" + "/".join((*memory.scope, memory.output_name))}", '
                    f'ac.name = "{memory.output_name}"}} : '
                    f"!ac.queue<{incoming.payload}> -> "
                    f"!ac.queue<{incoming.payload}>"
                )
                mapping[memory.output_name] = output
            elif kind == "table_read":
                read = item
                assert isinstance(read, TableReadBinding)
                table = next(
                    value for value in program.tables if value.name == read.table
                )
                input_payload = (
                    table.entry_type
                    if read.input_name is None
                    else by_name[read.input_name].payload
                )
                argument = read.argument or ""
                table_views = (
                    {
                        read.view_alias: (
                            read.table,
                            read.address,
                            table.entry_type,
                        )
                    }
                    if read.view_alias
                    else {}
                )
                address_emitter = _ExpressionEmitter(
                    payloads, argument, input_payload
                )
                address, address_type = address_emitter.emit(read.address)
                when_emitter = _ExpressionEmitter(
                    payloads,
                    argument,
                    input_payload,
                    table_views=table_views,
                )
                condition, condition_type = when_emitter.emit(read.when, "i1")
                if not address_type.startswith("i") or condition_type != "i1":
                    raise QueueFrontendError(
                        "ACPY-TABLE-003: read address/when type mismatch"
                    )
                output = read.output_name if not path else f"{read.output_name}__local"
                operand = (
                    ""
                    if read.input_name is None
                    else f", %{mapping[read.input_name]} : !ac.queue<{input_payload}> "
                )
                lines.append(
                    f"{indent}%{output} = ac.table.read @{read.table}{operand}"
                    f" depth {read.depth} latency {read.latency} address {{"
                )
                block_argument = (
                    ""
                    if read.input_name is None
                    else f"(%item: !ac.var<{input_payload}>)"
                )
                lines.append(f"{indent}^address{block_argument}:")
                lines.extend(indent + line[2:] for line in address_emitter.lines)
                lines.append(
                    f"{indent}  ac.table.yield %{address} : !ac.var<{address_type}>"
                )
                lines.append(f"{indent}}} when {{")
                lines.append(f"{indent}^when{block_argument}:")
                lines.extend(indent + line[2:] for line in when_emitter.lines)
                lines.append(
                    f"{indent}  ac.table.yield %{condition} : !ac.var<i1>"
                )
                lines.append(
                    f'{indent}}} {{ac.endpoint_path = "'
                    f'{"/" + "/".join((*read.scope, read.output_name))}", '
                    f'ac.name = "{read.output_name}"}} -> '
                    f"!ac.queue<{table.entry_type}>"
                )
                mapping[read.output_name] = output
            elif kind == "table_write":
                write = item
                assert isinstance(write, TableWriteBinding)
                table = next(
                    value for value in program.tables if value.name == write.table
                )
                input_payload = by_name[write.input_name].payload
                address_emitter = _ExpressionEmitter(
                    payloads, write.argument, input_payload
                )
                address, address_type = address_emitter.emit(write.address)
                enable_emitter = _ExpressionEmitter(
                    payloads, write.argument, input_payload
                )
                enabled, enable_type = enable_emitter.emit(write.enable, "i1")
                value_emitter = _ExpressionEmitter(
                    payloads,
                    write.argument,
                    input_payload,
                    table_views={
                        "__old": (write.table, write.address, table.entry_type)
                    },
                )
                if write.value is not None:
                    value, value_type = value_emitter.emit(
                        write.value, table.entry_type
                    )
                else:
                    patch_call = ast.Call(
                        func=ast.Attribute(
                            value=ast.Name(id="__old", ctx=ast.Load()),
                            attr="with_fields",
                            ctx=ast.Load(),
                        ),
                        args=[],
                        keywords=[
                            ast.keyword(arg=name, value=expression)
                            for name, expression in write.patch_fields
                        ],
                    )
                    value, value_type = value_emitter.emit(
                        patch_call, table.entry_type
                    )
                if (
                    not address_type.startswith("i")
                    or enable_type != "i1"
                    or value_type != table.entry_type
                ):
                    raise QueueFrontendError(
                        "ACPY-TABLE-004: write address/enable/value type mismatch"
                    )
                lines.append(
                    f"{indent}ac.table.write @{write.table}, "
                    f"%{mapping[write.input_name]} address {{"
                )
                block_argument = f"(%item: !ac.var<{input_payload}>)"
                policies = (
                    ("address", address, address_type, address_emitter.lines),
                    ("enable", enabled, enable_type, enable_emitter.lines),
                    ("value", value, value_type, value_emitter.lines),
                )
                for index, (
                    policy_name,
                    policy_value,
                    policy_type,
                    policy_lines,
                ) in enumerate(policies):
                    if index:
                        lines.append(f"{indent}}} {policy_name} {{")
                    lines.append(f"{indent}^{policy_name}{block_argument}:")
                    lines.extend(indent + line[2:] for line in policy_lines)
                    lines.append(
                        f"{indent}  ac.table.yield %{policy_value} : "
                        f"!ac.var<{policy_type}>"
                    )
                endpoint_name = f"{write.table}__write"
                lines.append(
                    f'{indent}}} {{ac.endpoint_path = "'
                    f'{"/" + "/".join((*write.scope, endpoint_name))}", '
                    f'ac.name = "{endpoint_name}"}} : '
                    f"!ac.queue<{input_payload}>"
                )
            elif kind == "atomic":
                atomic = item
                assert isinstance(atomic, AtomicBinding)
                members = [by_name[name] for name in atomic.queues]
                inputs = [
                    effective_input.get(queue.name, queue.input_name)
                    for queue in members
                ]
                if any(input_name is None for input_name in inputs):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-009: atomic input resolution failed"
                    )
                input_names = [str(input_name) for input_name in inputs]
                outputs = [
                    queue.name if not path else f"{queue.name}__local"
                    for queue in members
                ]
                lhs = ", ".join(f"%{name}" for name in outputs)
                operands = ", ".join(
                    f"%{mapping[input_name]}" for input_name in input_names
                )
                depths = ", ".join(str(queue.depth) for queue in members)
                latencies = ", ".join(str(queue.latency) for queue in members)
                input_types = ", ".join(
                    f"!ac.queue<{payload_by_queue[input_name]}>"
                    for input_name in input_names
                )
                output_types = ", ".join(
                    f"!ac.queue<{queue.payload}>" for queue in members
                )
                lines.append(
                    f"{indent}{lhs} = ac.transform {operands} depths [{depths}] "
                    f"latencies [{latencies}] {{"
                )
                arguments = ", ".join(
                    f"%item{index}: !ac.var<{payload_by_queue[input_name]}>"
                    for index, input_name in enumerate(input_names)
                )
                lines.append(f"{indent}^transform({arguments}):")
                yielded: list[str] = []
                for index, queue in enumerate(members):
                    assert queue.argument is not None and queue.expression is not None
                    emitter = _ExpressionEmitter(
                        payloads,
                        queue.argument,
                        payload_by_queue[input_names[index]],
                        root_name=f"item{index}",
                        prefix=f"a{index}_",
                    )
                    result, result_type = emitter.emit(queue.expression)
                    if result_type != queue.payload:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-009: atomic lambda result type mismatch"
                        )
                    lines.extend(indent + line[2:] for line in emitter.lines)
                    yielded.append(f"%{result}")
                lines.append(
                    f"{indent}  ac.transform.yield {', '.join(yielded)} : "
                    + ", ".join(f"!ac.var<{queue.payload}>" for queue in members)
                )
                names_attr = name_array(tuple(queue.name for queue in members))
                lines.append(
                    f"{indent}}} {{ac.output_names = {names_attr}}} : "
                    f"({input_types}) -> ({output_types})"
                )
                for queue, output in zip(members, outputs, strict=True):
                    mapping[queue.name] = output
            elif kind == "merge":
                merge = item
                assert isinstance(merge, MergeBinding)
                output = merge.output if not path else f"{merge.output}__local"
                operands = ", ".join(f"%{mapping[name]}" for name in merge.inputs)
                input_types = ", ".join(
                    f"!ac.queue<{by_name[name].payload}>" for name in merge.inputs
                )
                payload = by_name[merge.output].payload
                lines.append(
                    f'{indent}%{output} = ac.merge {operands} policy "{merge.policy}" '
                    f"depth {merge.depth} latency {merge.latency} "
                    f'{{ac.name = "{merge.output}"}} : '
                    f"({input_types}) -> !ac.queue<{payload}>"
                )
                mapping[merge.output] = output
            elif kind == "expect":
                expectation = item
                assert isinstance(expectation, ExpectBinding)
                queue = by_name[expectation.queue]
                emitter = _ExpressionEmitter(
                    payloads, expectation.argument, queue.payload
                )
                condition, condition_type = emitter.emit(expectation.predicate)
                if condition_type != "i1":
                    raise QueueFrontendError(
                        "ACPY-QUEUE-021: expect predicate must lower to bool"
                    )
                lines.append(
                    f"{indent}ac.expect %{mapping[expectation.queue]} message "
                    f"{json.dumps(expectation.message)} {{"
                )
                lines.append(f"{indent}^predicate(%item: !ac.var<{queue.payload}>):")
                lines.extend(indent + line[2:] for line in emitter.lines)
                lines.append(f"{indent}  ac.expect.yield %{condition} : !ac.var<i1>")
                lines.append(
                    f'{indent}}} {{ac.name = "expect_{expectation.order}"}} : '
                    f"!ac.queue<{queue.payload}>"
                )
            elif kind == "observe":
                observation = item
                assert isinstance(observation, ObservationBinding)
                queue = by_name[observation.queue]
                lines.append(
                    f"{indent}ac.observe %{mapping[observation.queue]} name "
                    f'"{observation.name}" : !ac.queue<{queue.payload}>'
                )
            else:
                sink_binding = item
                assert isinstance(sink_binding, SinkBinding)
                queue = by_name[sink_binding.queue]
                lines.append(
                    f"{indent}ac.sink %{mapping[sink_binding.queue]} "
                    f'{{ac.name = "sink_{sink_binding.order}"}} : '
                    f"!ac.queue<{queue.payload}>"
                )

    def render_scope(
        scope: ScopeBinding, parent_mapping: dict[str, str], indent: str
    ) -> None:
        inputs, outputs = scope_io(scope.path)
        result_names = [
            name if len(scope.path) == 1 else f"{name}__inner" for name in outputs
        ]
        lhs = (
            ""
            if not result_names
            else ", ".join(f"%{name}" for name in result_names) + " = "
        )
        operands = ", ".join(f"%{parent_mapping[name]}" for name in inputs)
        input_types = ", ".join(
            f"!ac.queue<{payload_by_queue[name]}>" for name in inputs
        )
        output_types = ", ".join(
            f"!ac.queue<{payload_by_queue[name]}>" for name in outputs
        )
        lines.append(f"{indent}{lhs}ac.scope @{scope.name}({operands}) {{")
        local_mapping = dict(parent_mapping)
        if inputs:
            args = ", ".join(
                f"%{name}__in: !ac.queue<{payload_by_queue[name]}>" for name in inputs
            )
            lines.append(f"{indent}^body({args}):")
            for name in inputs:
                local_mapping[name] = f"{name}__in"
        else:
            lines.append(f"{indent}^body:")
        render_items(scope.path, local_mapping, indent + "  ")
        yielded = ", ".join(f"%{local_mapping[name]}" for name in outputs)
        yield_types = ", ".join(
            f"!ac.queue<{payload_by_queue[name]}>" for name in outputs
        )
        lines.append(
            f"{indent}  ac.scope.yield"
            + (f" {yielded} : {yield_types}" if outputs else "")
        )
        result_signature = output_types if len(outputs) == 1 else f"({output_types})"
        lines.append(f"{indent}}} : ({input_types}) -> {result_signature}")
        for name, result in zip(outputs, result_names, strict=True):
            parent_mapping[name] = result

    render_items((), {}, "  ")
    lines.append("}")
    return "\n".join(lines) + "\n"


def lower_queue_source(
    text: str,
    system: str,
    static_arguments: Mapping[str, StaticValue] | None = None,
    specialization_fingerprint: str | None = None,
) -> str:
    return lower_queue_program(
        parse_queue_program(
            text,
            system,
            static_arguments=static_arguments,
            specialization_fingerprint=specialization_fingerprint,
        )
    )
