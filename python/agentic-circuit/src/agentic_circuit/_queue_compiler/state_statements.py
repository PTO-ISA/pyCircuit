"""Persistent Var, Table, Slot, and selection statement handlers."""

from __future__ import annotations

import ast
import copy

from _pycircuit_semantics import ArrayType, EnumType, StructType, TupleType

from .._diagnostics import SourceSpan
from .._source_map import source_frame
from .errors import QueueFrontendError
from .model import (
    CandidateSetBinding,
    EntryViewBinding,
    MaskedEntryViewBinding,
    MaskedTableWriteBinding,
    ProjectedTableViewBinding,
    QueueBinding,
    SelectionBinding,
    SlotBinding,
    SlotReleaseBinding,
    TableBinding,
    TableReadBinding,
    TableWriteBinding,
    VarStateBinding,
)
from .normalize import _constantize_expression
from .parser_context import (
    HANDLED,
    UNHANDLED,
    _ParserEnvironment,
    _ParserState,
    _StatementContext,
    _StatementResult,
)
from .state_semantics import _StateSemantics
from .statement_common import _call_name, _lambda, _queue_reference
from .static_types import (
    _nonnegative_int_value,
    _payload,
    _positive_int_value,
    _product,
    _scalar_reset_init,
    _static_int_value,
)
from .syntax import _decorator_name


def _lambda_or_constant(
    environment: _ParserEnvironment,
    node: ast.expr,
    argument: str,
    diagnostic: str,
) -> ast.expr:
    if isinstance(node, ast.Lambda):
        candidate_argument, expression = _lambda(environment, node)
        if candidate_argument != argument:
            raise QueueFrontendError(diagnostic)
        return expression
    return _constantize_expression(node, argument, environment.static_values)


def handle_masked_table_write(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
    call: ast.Call,
    view: MaskedEntryViewBinding,
    method: str,
    arbitration_rank: int | None,
) -> _StatementResult:
    scope_path = context.scope
    current_order = context.current_order
    semantics = _StateSemantics(environment, state)
    if method == "allocate":
        raise QueueFrontendError("ACPY-TABLE-009: allocation requires a scalar view")
    if call.args:
        raise QueueFrontendError(
            "ACPY-TABLE-008: masked write/patch is state-driven and takes no Queue"
        )
    enable_values = [
        keyword.value for keyword in call.keywords if keyword.arg == "enable"
    ]
    if len(enable_values) > 1:
        raise QueueFrontendError("ACPY-TABLE-008: repeated masked write enable")
    enable_node = enable_values[0] if enable_values else ast.Constant(True)
    if isinstance(enable_node, ast.Lambda):
        raise QueueFrontendError("ACPY-TABLE-008: masked enable must be an expression")
    enable = _constantize_expression(enable_node, "", environment.static_values)
    table = state.table_by_name[view.table]
    value: ast.expr | None = None
    patch_fields: tuple[tuple[str, ast.expr], ...] = ()
    if method == "write":
        if any(
            keyword.arg is None or keyword.arg not in {"value", "enable", "arbitration"}
            for keyword in call.keywords
        ):
            raise QueueFrontendError(
                "ACPY-TABLE-008: masked write accepts only value and enable"
            )
        values = [keyword.value for keyword in call.keywords if keyword.arg == "value"]
        if len(values) != 1:
            raise QueueFrontendError("ACPY-TABLE-008: masked write requires one value")
        if isinstance(values[0], ast.Lambda):
            raise QueueFrontendError(
                "ACPY-TABLE-008: masked write value must be a "
                "uniform expression, not a lambda"
            )
        value = _constantize_expression(values[0], "", environment.static_values)
    else:
        if not isinstance(table.entry_type, StructType):
            raise QueueFrontendError(
                "ACPY-TABLE-008: masked patch requires a struct Table Entry"
            )
        field_types = {field.name: field.type for field in table.entry_type.fields}
        patches: list[tuple[str, ast.expr]] = []
        for keyword in call.keywords:
            if keyword.arg in {"enable", "arbitration"}:
                continue
            if keyword.arg is None or keyword.arg not in field_types:
                raise QueueFrontendError(
                    "ACPY-TABLE-008: masked patch field is unknown"
                )
            expression = keyword.value
            if isinstance(expression, ast.Lambda):
                old_name, expression = _lambda(environment, expression)

                class OldEntryName(ast.NodeTransformer):
                    def __init__(self, name: str) -> None:
                        self.name = name

                    def visit_Name(self, node: ast.Name) -> ast.expr:  # noqa: N802
                        if node.id == self.name:
                            return ast.copy_location(
                                ast.Name(
                                    id="compiler_old",
                                    ctx=node.ctx,
                                ),
                                node,
                            )
                        return node

                expression = OldEntryName(old_name).visit(copy.deepcopy(expression))
            else:
                expression = _constantize_expression(
                    expression, "", environment.static_values
                )
            patches.append((keyword.arg, expression))
        if not patches:
            raise QueueFrontendError(
                "ACPY-TABLE-008: masked patch requires at least one field"
            )
        if len({name for name, _ in patches}) != len(patches):
            raise QueueFrontendError("ACPY-TABLE-008: masked patch field is repeated")
        patch_fields = tuple(patches)
    write_fields = semantics.normalized_write_fields(table, value, patch_fields)
    semantics.reject_overlapping_table_writer(
        view.table, write_fields, "field", arbitration_rank
    )
    state.masked_table_writes.append(
        MaskedTableWriteBinding(
            view.table,
            view.candidates,
            enable,
            value,
            patch_fields,
            write_fields,
            "field",
            arbitration_rank,
            scope_path,
            current_order,
        )
    )
    return HANDLED


def handle_scalar_table_write(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
    call: ast.Call,
    view: EntryViewBinding,
    method: str,
    arbitration_rank: int | None,
) -> _StatementResult:
    scope_path = context.scope
    aliases = context.aliases
    current_order = context.current_order
    semantics = _StateSemantics(environment, state)
    queue_driven = view.argument is not None
    if method == "allocate" and queue_driven:
        raise QueueFrontendError("ACPY-TABLE-009: allocation must be state-driven")
    if len(call.args) != (1 if queue_driven else 0):
        raise QueueFrontendError(
            "ACPY-TABLE-004: Queue-driven table write/patch requires "
            "one Queue; state-driven write/patch takes no Queue"
        )
    input_name = (
        _queue_reference(state, call.args[0], aliases) if queue_driven else None
    )
    argument = view.argument
    enable_values = [
        keyword.value for keyword in call.keywords if keyword.arg == "enable"
    ]
    if len(enable_values) > 1:
        raise QueueFrontendError("ACPY-TABLE-004: repeated write enable")
    enable_node = enable_values[0] if enable_values else ast.Constant(True)
    if queue_driven:
        assert argument is not None
        enable = _lambda_or_constant(
            environment,
            enable_node,
            argument,
            "ACPY-TABLE-004: selector and enable lambdas require one argument name",
        )
    else:
        if isinstance(enable_node, ast.Lambda):
            raise QueueFrontendError(
                "ACPY-TABLE-004: state-driven enable must be an "
                "expression, not a lambda"
            )
        enable = _constantize_expression(enable_node, "", environment.static_values)
    value: ast.expr | None = None
    patch_fields: tuple[tuple[str, ast.expr], ...] = ()
    table = state.table_by_name[view.table]
    if method in {"write", "allocate"}:
        if any(
            keyword.arg is None or keyword.arg not in {"value", "enable", "arbitration"}
            for keyword in call.keywords
        ):
            raise QueueFrontendError(
                "ACPY-TABLE-004: write/allocation accepts only value and enable"
            )
        values = [keyword.value for keyword in call.keywords if keyword.arg == "value"]
        if len(values) != 1:
            raise QueueFrontendError(
                "ACPY-TABLE-004: write/allocation requires one value"
            )
        if queue_driven:
            assert argument is not None
            value = _lambda_or_constant(
                environment,
                values[0],
                argument,
                "ACPY-TABLE-004: selector and value lambdas require one argument name",
            )
        else:
            if isinstance(values[0], ast.Lambda):
                raise QueueFrontendError(
                    "ACPY-TABLE-004: state-driven value must be an "
                    "expression, not a lambda"
                )
            value = _constantize_expression(values[0], "", environment.static_values)
    else:
        if not isinstance(table.entry_type, StructType):
            raise QueueFrontendError(
                "ACPY-TABLE-004: patch requires a struct Table Entry"
            )
        field_types = {field.name: field.type for field in table.entry_type.fields}
        patches: list[tuple[str, ast.expr]] = []
        for keyword in call.keywords:
            if keyword.arg in {"enable", "arbitration"}:
                continue
            if keyword.arg is None or keyword.arg not in field_types:
                raise QueueFrontendError("ACPY-TABLE-004: patch field is unknown")
            patches.append(
                (
                    keyword.arg,
                    (
                        _lambda_or_constant(
                            environment,
                            keyword.value,
                            argument or "",
                            "ACPY-TABLE-004: patch lambdas require one argument name",
                        )
                        if queue_driven
                        else _constantize_expression(
                            keyword.value, "", environment.static_values
                        )
                    ),
                )
            )
        if not patches:
            raise QueueFrontendError(
                "ACPY-TABLE-004: patch requires at least one field"
            )
        if len({name for name, _ in patches}) != len(patches):
            raise QueueFrontendError("ACPY-TABLE-004: patch field is repeated")
        patch_fields = tuple(patches)
    write_fields = semantics.normalized_write_fields(table, value, patch_fields)
    write_mode = "replace" if method == "allocate" else "field"
    semantics.reject_overlapping_table_writer(
        view.table, write_fields, write_mode, arbitration_rank
    )
    state.table_writes.append(
        TableWriteBinding(
            view.table,
            input_name,
            argument,
            view.address,
            enable,
            value,
            patch_fields,
            write_fields,
            write_mode,
            arbitration_rank,
            scope_path,
            current_order,
        )
    )
    return HANDLED


def handle_table_write(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
) -> _StatementResult:
    statement = context.statement
    if not (
        isinstance(statement, ast.Expr)
        and isinstance(statement.value, ast.Call)
        and isinstance(statement.value.func, ast.Attribute)
        and statement.value.func.attr in {"write", "patch", "allocate"}
    ):
        return UNHANDLED
    call = statement.value
    semantics = _StateSemantics(environment, state)
    view = semantics.resolve_view(call.func.value, context.scope, context.current_order)
    if view is None:
        return UNHANDLED
    if isinstance(view, ProjectedTableViewBinding):
        raise QueueFrontendError(
            "ACPY-TABLE-004: projected Table view requires a complete index before write"
        )
    method = call.func.attr
    arbitration_rank = semantics.table_writer_arbitration(
        call, "ACPY-TABLE-011", view.table
    )
    if isinstance(view, MaskedEntryViewBinding):
        return handle_masked_table_write(
            environment, state, context, call, view, method, arbitration_rank
        )
    return handle_scalar_table_write(
        environment, state, context, call, view, method, arbitration_rank
    )


def handle_state_statement(
    environment: _ParserEnvironment,
    state: _ParserState,
    context: _StatementContext,
) -> _StatementResult:
    statement = context.statement
    scope_path = context.scope
    aliases = context.aliases
    current_order = context.current_order
    semantics = _StateSemantics(environment, state)
    call_name = _call_name

    def _static_int(node: ast.expr) -> int | None:
        return _static_int_value(node, environment.static_values)

    def _positive_int(call: ast.Call, name: str, default: int) -> int:
        return _positive_int_value(call, name, default, environment.static_values)

    def _nonnegative_int(call: ast.Call, name: str, default: int) -> int:
        return _nonnegative_int_value(call, name, default, environment.static_values)

    def lambda_or_constant(node: ast.expr, argument: str, diagnostic: str) -> ast.expr:
        if isinstance(node, ast.Lambda):
            candidate_argument, expression = _lambda(environment, node)
            if candidate_argument != argument:
                raise QueueFrontendError(diagnostic)
            return expression
        return _constantize_expression(node, argument, environment.static_values)

    if (
        isinstance(statement, ast.Assign)
        and len(statement.targets) == 1
        and isinstance(statement.targets[0], ast.Name)
        and isinstance(statement.value, ast.Call)
        and _decorator_name(statement.value.func).rsplit(".", 1)[-1]
        == "writer_priority"
    ):
        name = statement.targets[0].id
        if name in state.arbitration_descriptors:
            raise QueueFrontendError(
                "ACPY-TABLE-011: arbitration descriptor requires a fresh name"
            )
        state.arbitration_descriptors[name] = semantics.writer_priority_rank(
            statement.value, "ACPY-TABLE-011"
        )
        return HANDLED
    if (
        isinstance(statement, ast.AnnAssign)
        and isinstance(statement.target, ast.Name)
        and statement.value is not None
    ):
        name = statement.target.id
        if name in state.variable_by_name or name in state.by_name:
            raise QueueFrontendError(
                "ACPY-VAR-001: persistent variable requires a fresh name"
            )
        entries = 1
        annotation = statement.annotation
        if isinstance(annotation, ast.Subscript) and _decorator_name(
            annotation.value
        ).rsplit(".", 1)[-1] in {"list", "List"}:
            initializer = statement.value
            count: int | None = None
            if isinstance(initializer, ast.BinOp) and isinstance(
                initializer.op, ast.Mult
            ):
                if isinstance(initializer.left, ast.List):
                    count = _static_int(initializer.right)
                elif isinstance(initializer.right, ast.List):
                    count = _static_int(initializer.left)
            if count is None and isinstance(initializer, ast.List):
                count = len(initializer.elts)
            entry_spelling = ast.unparse(annotation.slice)
            extent_spelling = str(count) if count is not None else "N"
            frame = source_frame(statement)
            location = (
                None
                if frame is None
                else SourceSpan(
                    frame.file,
                    frame.line,
                    frame.column,
                    frame.end_line,
                    frame.end_column,
                )
            )
            raise QueueFrontendError(
                "ACPY-VAR-002",
                "persistent indexed state must be declared explicitly; "
                f"replace {name}: list[{entry_spelling}] with "
                f"{name} = ac.table[{extent_spelling}, {entry_spelling}](init=0)",
                location,
            )
        else:
            value_type = _payload(
                annotation,
                environment.payload_map,
                environment.enum_map,
                environment.type_static_values,
            )
            if isinstance(value_type, EnumType):
                if (
                    not isinstance(statement.value, ast.Attribute)
                    or _decorator_name(statement.value.value).rsplit(".", 1)[-1]
                    != value_type.name
                    or statement.value.attr != value_type.enumerants[0]
                    or value_type.encoding_values[0] != 0
                ):
                    raise QueueFrontendError(
                        "ACPY-VAR-001: persistent enum init must be its "
                        "zero-encoded first declared member"
                    )
                init = 0
            else:
                if not isinstance(statement.value, ast.Constant) or type(
                    statement.value.value
                ) not in {bool, int}:
                    raise QueueFrontendError(
                        "ACPY-VAR-001: persistent scalar init must be constant"
                    )
                init = statement.value.value
        if isinstance(value_type, StructType | TupleType | ArrayType | EnumType) and (
            type(init) is not int or init != 0
        ):
            raise QueueFrontendError(
                "ACPY-VAR-001: persistent struct init must be zero"
            )
        if not isinstance(value_type, StructType | TupleType | ArrayType | EnumType):
            init = _scalar_reset_init(value_type, init, code="ACPY-VAR-001")
        binding = VarStateBinding(
            name,
            value_type,
            init,
            scope_path,
            current_order,
            entries,
            (entries,) if entries != 1 else (),
            source_frame(statement),
        )
        state.variables.append(binding)
        state.variable_by_name[name] = binding
        return HANDLED
    assigned_names: tuple[str, ...] = ()
    if isinstance(statement, ast.Assign) and len(statement.targets) == 1:
        target = statement.targets[0]
        if isinstance(target, ast.Name):
            assigned_names = (target.id,)
        elif isinstance(target, ast.Tuple | ast.List) and all(
            isinstance(item, ast.Name) for item in target.elts
        ):
            assigned_names = tuple(item.id for item in target.elts)
    if any(
        name in state.memory_by_name
        or name in state.memory_arrays
        or name in state.selected_memories
        or name in state.table_by_name
        or name in state.entry_views
        or name in state.slot_by_name
        or name in state.candidate_by_name
        or name in state.selection_by_name
        for name in assigned_names
    ):
        raise QueueFrontendError("ACPY-QUEUE-015: state binding cannot be rebound")
    selection_unpack: tuple[str, ...] = ()
    if (
        isinstance(statement, ast.Assign)
        and len(statement.targets) == 1
        and isinstance(statement.targets[0], ast.Tuple | ast.List)
        and isinstance(statement.value, ast.Call)
        and isinstance(statement.value.func, ast.Attribute)
        and statement.value.func.attr == "choose"
    ):
        if not all(isinstance(item, ast.Name) for item in statement.targets[0].elts):
            raise QueueFrontendError(
                "ACPY-TABLE-012: TableChoice tuple unpack requires names"
            )
        selection_unpack = tuple(
            item.id for item in statement.targets[0].elts if isinstance(item, ast.Name)
        )
        statement.targets[0] = ast.copy_location(
            ast.Name(id=f"compiler_table_selection_{current_order}", ctx=ast.Store()),
            statement.targets[0],
        )
    if (
        isinstance(statement, ast.Assign)
        and len(statement.targets) == 1
        and isinstance(statement.targets[0], ast.Name)
        and isinstance(statement.value, ast.Call)
    ):
        call = statement.value
        declaration = semantics.table_declaration(statement.value)
        if declaration is not None:
            name = statement.targets[0].id
            if (
                name in state.by_name
                or name in state.collections
                or name in state.table_by_name
                or name in state.variable_by_name
            ):
                raise QueueFrontendError(
                    "ACPY-TABLE-001: table declaration requires a fresh name"
                )
            entries, shape, entry_type, init_image = declaration
            if environment.entry_kind == "module":
                variable = VarStateBinding(
                    name,
                    entry_type,
                    0,
                    scope_path,
                    current_order,
                    entries,
                    shape,
                    source_frame(statement),
                )
                state.variables.append(variable)
                state.variable_by_name[name] = variable
                return HANDLED
            binding = TableBinding(
                name,
                entry_type,
                entries,
                shape,
                init_image,
                scope_path,
                current_order,
                source_frame(statement),
            )
            state.tables.append(binding)
            state.table_by_name[name] = binding
            return HANDLED
        if call_name(call) == "slot":
            if len(call.args) != 1 or call.keywords:
                raise QueueFrontendError(
                    "ACPY-SLOT-001: ac.slot requires exactly one Queue"
                )
            name = statement.targets[0].id
            if (
                name in state.by_name
                or name in state.collections
                or name in state.slot_by_name
            ):
                raise QueueFrontendError(
                    "ACPY-SLOT-001: slot declaration requires a fresh name"
                )
            input_name = _queue_reference(state, call.args[0], aliases)
            binding = SlotBinding(
                name,
                input_name,
                state.by_name[input_name].payload,
                scope_path,
                current_order,
            )
            state.slots.append(binding)
            state.slot_by_name[name] = binding
            return HANDLED
        if (
            isinstance(call.func, ast.Attribute)
            and call.func.attr == "match"
            and isinstance(call.func.value, ast.Name)
            and (
                call.func.value.id in state.table_by_name
                or isinstance(
                    state.entry_views.get(call.func.value.id),
                    ProjectedTableViewBinding,
                )
            )
        ):
            name = statement.targets[0].id
            projected = state.entry_views.get(call.func.value.id)
            table_name = (
                projected.table
                if isinstance(projected, ProjectedTableViewBinding)
                else call.func.value.id
            )
            table = state.table_by_name[table_name]
            domain_axes = (
                projected.domain_axes
                if isinstance(projected, ProjectedTableViewBinding)
                else tuple(range(len(table.shape)))
            )
            domain_shape = (
                projected.domain_shape
                if isinstance(projected, ProjectedTableViewBinding)
                else table.shape
            )
            domain_strides = (
                projected.domain_strides
                if isinstance(projected, ProjectedTableViewBinding)
                else tuple(
                    _product(table.shape[axis + 1 :])
                    for axis in range(len(table.shape))
                )
            )
            domain_offset = (
                projected.domain_offset
                if isinstance(projected, ProjectedTableViewBinding)
                else 0
            )
            domain_entries = _product(domain_shape)
            if domain_entries > 64:
                raise QueueFrontendError(
                    "ACPY-TABLE-006: table.match domain must contain 1..64 entries"
                )
            if len(call.args) != 1 or call.keywords:
                raise QueueFrontendError(
                    "ACPY-TABLE-006: table.match requires one predicate lambda"
                )
            argument, predicate = _lambda(environment, call.args[0])
            binding = CandidateSetBinding(
                name,
                table_name,
                domain_entries,
                domain_axes,
                domain_shape,
                domain_strides,
                domain_offset,
                argument,
                predicate,
                scope_path,
                current_order,
            )
            state.candidates.append(binding)
            state.candidate_by_name[name] = binding
            return HANDLED
        if (
            isinstance(call.func, ast.Attribute)
            and call.func.attr == "choose"
            and isinstance(call.func.value, ast.Name)
            and (
                call.func.value.id in state.table_by_name
                or isinstance(
                    state.entry_views.get(call.func.value.id),
                    ProjectedTableViewBinding,
                )
            )
        ):
            name = statement.targets[0].id
            projected = state.entry_views.get(call.func.value.id)
            table_name = (
                projected.table
                if isinstance(projected, ProjectedTableViewBinding)
                else call.func.value.id
            )
            if len(call.args) != 1 or not isinstance(call.args[0], ast.Name):
                raise QueueFrontendError(
                    "ACPY-TABLE-007: table.choose requires one CandidateSet"
                )
            candidate = state.candidate_by_name.get(call.args[0].id)
            if candidate is None or candidate.table != table_name:
                raise QueueFrontendError(
                    "ACPY-TABLE-007: CandidateSet belongs to a different Table"
                )
            if isinstance(projected, ProjectedTableViewBinding) and (
                candidate.domain_axes != projected.domain_axes
                or candidate.domain_shape != projected.domain_shape
                or candidate.domain_strides != projected.domain_strides
                or candidate.domain_offset != projected.domain_offset
            ):
                raise QueueFrontendError(
                    "ACPY-TABLE-007: CandidateSet belongs to a different Table view"
                )
            keywords = {keyword.arg: keyword.value for keyword in call.keywords}
            if None in keywords or set(keywords) - {
                "count",
                "policy",
                "key",
                "initial_cursor",
            }:
                raise QueueFrontendError(
                    "ACPY-TABLE-007: table.choose parameters are invalid"
                )
            count = _static_int(keywords.get("count", ast.Constant(1)))
            if count is None or not 1 <= count <= candidate.entries:
                raise QueueFrontendError(
                    "ACPY-TABLE-012: table.choose count must be a static "
                    "integer within the candidate domain"
                )
            policy_node = keywords.get("policy", ast.Constant("first"))
            policy = (
                policy_node.value
                if isinstance(policy_node, ast.Constant)
                and isinstance(policy_node.value, str)
                else None
            )
            if policy not in {"first", "min", "max", "round_robin"}:
                raise QueueFrontendError(
                    "ACPY-TABLE-007: choose policy must be first, min, max, "
                    "or round_robin"
                )
            key_node = keywords.get("key")
            key_argument: str | None = None
            key: ast.expr | None = None
            key_ordering: str | None = None
            if policy in {"first", "round_robin"}:
                if key_node is not None:
                    raise QueueFrontendError(
                        "ACPY-TABLE-007: first/round_robin policy does not accept key"
                    )
            else:
                if key_node is None:
                    raise QueueFrontendError(
                        "ACPY-TABLE-007: min/max policy requires key lambda"
                    )
                key_argument, key = _lambda(environment, key_node)
                key_ordering = semantics.table_key_ordering(
                    state.table_by_name[table_name], key_argument, key
                )
            initial_cursor = _nonnegative_int(call, "initial_cursor", 0)
            if initial_cursor >= candidate.entries:
                raise QueueFrontendError(
                    "ACPY-TABLE-012: initial cursor is outside the candidate domain"
                )
            if policy != "round_robin" and initial_cursor != 0:
                raise QueueFrontendError(
                    "ACPY-TABLE-012: initial cursor requires round_robin"
                )
            if selection_unpack:
                if len(selection_unpack) != count:
                    raise QueueFrontendError(
                        "ACPY-TABLE-012: TableChoice tuple unpack arity "
                        "must equal count"
                    )
                aliases = selection_unpack
            elif count == 1:
                aliases = (name,)
            else:
                aliases = tuple(f"{name}_lane_{lane}" for lane in range(count))
                state.selection_tuple_aliases[name] = aliases
            stable_path = "/".join((*scope_path, name)) if scope_path else name
            binding = SelectionBinding(
                name,
                aliases,
                table_name,
                candidate.name,
                count,
                str(policy),
                key_ordering,
                f"table-selection/{stable_path}",
                initial_cursor,
                key_argument,
                key,
                scope_path,
                current_order,
            )
            state.selections.append(binding)
            for lane, alias in enumerate(aliases):
                state.selection_by_name[alias] = binding
                state.selection_lane_ordinals[alias] = lane
            context.aliases = aliases
            return HANDLED
        view = semantics.parse_view(
            statement.value,
            statement.targets[0].id,
            scope_path,
            current_order,
        )
        if view is not None:
            if view.name in state.by_name or view.name in state.collections:
                raise QueueFrontendError(
                    "ACPY-TABLE-002: EntryView alias requires a fresh name"
                )
            state.entry_views[view.name] = view
            return HANDLED
    if (
        isinstance(statement, ast.Expr)
        and isinstance(statement.value, ast.Call)
        and isinstance(statement.value.func, ast.Attribute)
        and statement.value.func.attr == "release"
        and isinstance(statement.value.func.value, ast.Name)
        and statement.value.func.value.id in state.slot_by_name
    ):
        call = statement.value
        slot_name = call.func.value.id
        if call.args or any(
            keyword.arg is None or keyword.arg != "when" for keyword in call.keywords
        ):
            raise QueueFrontendError(
                "ACPY-SLOT-002: slot.release accepts only when=expression"
            )
        values = [keyword.value for keyword in call.keywords if keyword.arg == "when"]
        if len(values) != 1 or isinstance(values[0], ast.Lambda):
            raise QueueFrontendError(
                "ACPY-SLOT-002: slot.release requires one state expression"
            )
        if any(release.slot == slot_name for release in state.slot_releases):
            raise QueueFrontendError(
                "ACPY-SLOT-002: slot permits exactly one release endpoint"
            )
        state.slot_releases.append(
            SlotReleaseBinding(
                slot_name,
                _constantize_expression(values[0], "", environment.static_values),
                scope_path,
                current_order,
            )
        )
        return HANDLED
    if (
        isinstance(statement, ast.Assign)
        and len(statement.targets) == 1
        and isinstance(statement.targets[0], ast.Name)
        and isinstance(statement.value, ast.Call)
        and isinstance(statement.value.func, ast.Attribute)
        and statement.value.func.attr == "read"
    ):
        call = statement.value
        view = semantics.resolve_view(call.func.value, scope_path, current_order)
        if view is not None:
            if isinstance(view, ProjectedTableViewBinding):
                raise QueueFrontendError(
                    "ACPY-TABLE-003: projected Table view requires a "
                    "complete index before read"
                )
            if isinstance(view, MaskedEntryViewBinding):
                raise QueueFrontendError(
                    "ACPY-TABLE-008: masked Table view does not support read"
                )
            name = statement.targets[0].id
            if (
                name in state.by_name
                or name in state.collections
                or name in state.table_by_name
            ):
                raise QueueFrontendError(
                    "ACPY-TABLE-003: table read output requires a fresh name"
                )
            if len(call.args) > 1 or any(
                keyword.arg is None or keyword.arg not in {"when", "depth", "latency"}
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
                        "ACPY-TABLE-003: Queue-driven read requires a selector lambda"
                    )
                input_name = _queue_reference(state, call.args[0], aliases)
            elif argument is not None:
                raise QueueFrontendError(
                    "ACPY-TABLE-003: state-driven read requires a bound index"
                )
            when_values = [
                keyword.value for keyword in call.keywords if keyword.arg == "when"
            ]
            if len(when_values) > 1:
                raise QueueFrontendError("ACPY-TABLE-003: table read has repeated when")
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
                        "ACPY-TABLE-003: state-driven when is an EntryView expression"
                    )
                when = _constantize_expression(when_node, "", environment.static_values)
            depth = _positive_int(call, "depth", 1)
            latency = _positive_int(call, "latency", 1)
            table = state.table_by_name[view.table]
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
            state.queues.append(queue)
            state.by_name[name] = queue
            state.table_reads.append(
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
            return HANDLED
    if handle_table_write(environment, state, context) is HANDLED:
        return HANDLED
    return UNHANDLED
