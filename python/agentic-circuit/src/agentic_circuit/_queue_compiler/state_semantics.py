"""Shared Table and persistent-state semantic helpers."""

from __future__ import annotations

import ast
import copy
from collections.abc import Mapping
from dataclasses import dataclass

from _pycircuit_semantics import (
    ArrayType,
    BitsType,
    BoolType,
    EnumType,
    StructType,
    TupleType,
    ValueType,
)

from .errors import QueueFrontendError
from .model import (
    EntryViewBinding,
    MaskedEntryViewBinding,
    ProjectedTableViewBinding,
    RuleLocalDefinition,
    RuleStateReadDefinition,
    TableBinding,
    VarStateBinding,
)
from .normalize import _constantize_expression
from .parser_context import _ParserEnvironment, _ParserState
from .statement_common import _lambda
from .static_types import _constant_integer, _payload, _product
from .syntax import _decorator_name


@dataclass(slots=True)
class _StateSemantics:
    """Semantic operations shared by State handlers and rule invocation."""

    environment: _ParserEnvironment
    state: _ParserState

    def normalized_write_fields(
        self,
        table: TableBinding,
        value: ast.expr | None,
        patch_fields: tuple[tuple[str, ast.expr], ...],
    ) -> tuple[str, ...]:
        if not isinstance(table.entry_type, StructType):
            return ("$entry",)
        declared = tuple(field.name for field in table.entry_type.fields)
        if value is not None:
            return declared
        requested = {name for name, _ in patch_fields}
        return tuple(name for name in declared if name in requested)

    def proven_field_write_fields(
        self,
        table: TableBinding,
        argument: str,
        value: ast.expr | None,
        write_index: ast.expr | None,
        reads: tuple[RuleStateReadDefinition, ...] = (),
        local_definitions: tuple[RuleLocalDefinition, ...] = (),
        *,
        legacy_read_name: str | None = None,
        legacy_read_index: ast.expr | None = None,
    ) -> tuple[str, ...] | None:
        """Return the canonical footprint of a proven Table field update.

        The proof follows the rule's immutable SSA expressions, including
        local aliases, value selects, and closed pure-helper summaries.  It is
        fail-closed: unrelated or mixed roots remain complete replacements.
        """
        if not isinstance(table.entry_type, StructType) or write_index is None:
            return None
        local_values = {
            local.name: local.value
            for local in local_definitions
            if local.guard is None and not local.guard_negated
        }

        class Substitute(ast.NodeTransformer):
            def __init__(self, replacements: Mapping[str, ast.expr]) -> None:
                self.replacements = replacements

            def visit_Name(self, node: ast.Name) -> ast.expr:  # noqa: N802
                if isinstance(node.ctx, ast.Load) and node.id in self.replacements:
                    return ast.copy_location(
                        copy.deepcopy(self.replacements[node.id]), node
                    )
                return node

        def substitute(
            expression: ast.expr, replacements: Mapping[str, ast.expr]
        ) -> ast.expr:
            result = Substitute(replacements).visit(copy.deepcopy(expression))
            assert isinstance(result, ast.expr)
            return ast.fix_missing_locations(result)

        def resolve_alias(expression: ast.expr, seen: set[str]) -> ast.expr:
            current = expression
            while (
                isinstance(current, ast.Name)
                and current.id in local_values
                and current.id not in seen
            ):
                seen.add(current.id)
                current = local_values[current.id]
            return current

        def same_index(candidate: ast.expr | None) -> bool:
            if candidate is None:
                return False
            left = resolve_alias(candidate, set())
            right = resolve_alias(write_index, set())
            return ast.dump(left, include_attributes=False) == ast.dump(
                right, include_attributes=False
            )

        def analyze(
            expression: ast.expr, active_helpers: frozenset[str] = frozenset()
        ) -> set[str] | None:
            root = resolve_alias(expression, set())
            if (
                isinstance(root, ast.Subscript)
                and isinstance(root.value, ast.Name)
                and root.value.id == argument
                and same_index(root.slice)
            ):
                return set()
            if isinstance(root, ast.Name):
                if any(
                    read.name == root.id
                    and read.argument == argument
                    and same_index(read.index)
                    for read in reads
                ) or (legacy_read_name == root.id and same_index(legacy_read_index)):
                    return set()
                return None
            if (
                isinstance(root, ast.Call)
                and isinstance(root.func, ast.Attribute)
                and root.func.attr == "with_fields"
                and not root.args
                and root.keywords
                and all(keyword.arg is not None for keyword in root.keywords)
            ):
                fields = analyze(root.func.value, active_helpers)
                if fields is None:
                    return None
                return fields | {
                    keyword.arg for keyword in root.keywords if keyword.arg is not None
                }
            if isinstance(root, ast.IfExp):
                left = analyze(root.body, active_helpers)
                right = analyze(root.orelse, active_helpers)
                if left is None or right is None:
                    return None
                return left | right
            if isinstance(root, ast.Call) and isinstance(root.func, ast.Name):
                helper = self.environment.helper_map.get(root.func.id)
                if (
                    helper is None
                    or helper.name in active_helpers
                    or root.keywords
                    or len(root.args) != len(helper.arguments)
                ):
                    return None
                replacements = {
                    name: actual
                    for (name, _), actual in zip(
                        helper.arguments, root.args, strict=True
                    )
                }
                return analyze(
                    substitute(helper.expression, replacements),
                    active_helpers | {helper.name},
                )
            return None

        requested = analyze(value)
        declared = tuple(field.name for field in table.entry_type.fields)
        if not requested or not requested <= set(declared):
            return None
        return tuple(name for name in declared if name in requested)

    def complete_value_fields(self, value_type: ValueType) -> tuple[str, ...]:
        if not isinstance(value_type, StructType):
            return ("$entry",)
        return tuple(field.name for field in value_type.fields)

    def state_value_type(self, owner: VarStateBinding | TableBinding) -> ValueType:
        return (
            owner.value_type if isinstance(owner, VarStateBinding) else owner.entry_type
        )

    def state_owner_kind(self, owner: VarStateBinding | TableBinding) -> str:
        return "var" if isinstance(owner, VarStateBinding) else "table"

    def state_write_fields(
        self, owner: VarStateBinding | TableBinding
    ) -> tuple[str, ...]:
        return self.complete_value_fields(self.state_value_type(owner))

    def table_key_ordering(
        self, table: TableBinding, argument: str, expression: ast.expr
    ) -> str:
        if not (
            isinstance(table.entry_type, StructType)
            and isinstance(expression, ast.Attribute)
            and isinstance(expression.value, ast.Name)
            and expression.value.id == argument
        ):
            return "unsigned"
        for declaration in self.environment.syntax_tree.body:
            if not isinstance(declaration, ast.ClassDef) or (
                declaration.name != table.entry_type.name
            ):
                continue
            for field in declaration.body:
                if (
                    isinstance(field, ast.AnnAssign)
                    and isinstance(field.target, ast.Name)
                    and field.target.id == expression.attr
                ):
                    annotation = _decorator_name(field.annotation).rsplit(".", 1)[-1]
                    return (
                        "signed"
                        if annotation in {"s8", "s16", "s32", "s64"}
                        else "unsigned"
                    )
        return "unsigned"

    def writer_priority_rank(self, policy: ast.expr, diagnostic: str) -> int:
        if (
            not isinstance(policy, ast.Call)
            or _decorator_name(policy.func).rsplit(".", 1)[-1] != "writer_priority"
            or len(policy.args) != 1
            or policy.keywords
        ):
            raise QueueFrontendError(
                f"{diagnostic}: arbitration requires ac.writer_priority(rank)"
            )
        rank = _constant_integer(policy.args[0], self.environment.static_values)
        if rank is None or rank < 0:
            raise QueueFrontendError(
                f"{diagnostic}: writer priority rank must be a non-negative "
                "static integer"
            )
        return rank

    def table_writer_arbitration(
        self, call: ast.Call, diagnostic: str, table: str
    ) -> int | None:
        policies = [
            keyword.value for keyword in call.keywords if keyword.arg == "arbitration"
        ]
        if len(policies) > 1:
            raise QueueFrontendError(f"{diagnostic}: repeated arbitration policy")
        if not policies:
            return None
        policy = policies[0]
        if (
            isinstance(policy, ast.Name)
            and policy.id in self.state.arbitration_descriptors
        ):
            owner = self.state.arbitration_owners.setdefault(policy.id, table)
            if owner != table:
                raise QueueFrontendError(
                    f"{diagnostic}: arbitration descriptor {policy.id!r} cannot "
                    "cross Table owners"
                )
            return self.state.arbitration_descriptors[policy.id]
        return self.writer_priority_rank(policy, diagnostic)

    def reject_overlapping_table_writer(
        self,
        table: str,
        write_fields: tuple[str, ...],
        write_mode: str,
        arbitration_rank: int | None,
    ) -> None:
        requested = set(write_fields)
        for write in (*self.state.table_writes, *self.state.masked_table_writes):
            if write.table != table:
                continue
            if write_mode == "replace" or write.write_mode == "replace":
                if write_mode == write.write_mode == "replace":
                    if arbitration_rank is None or write.arbitration_rank is None:
                        raise QueueFrontendError(
                            "ACPY-TABLE-009: table permits one allocation endpoint "
                            "unless multiple endpoints declare explicit writer "
                            "arbitration"
                        )
                    if arbitration_rank == write.arbitration_rank:
                        raise QueueFrontendError(
                            "ACPY-TABLE-011: writer priority ranks must be unique "
                            "for conflicting endpoints"
                        )
                continue
            overlap = requested.intersection(write.write_fields)
            if overlap:
                if arbitration_rank is not None and write.arbitration_rank is not None:
                    if arbitration_rank == write.arbitration_rank:
                        raise QueueFrontendError(
                            "ACPY-TABLE-011: writer priority ranks must be unique "
                            "for conflicting endpoints"
                        )
                    continue
                field = min(overlap)
                raise QueueFrontendError(
                    "ACPY-TABLE-004: table write field "
                    f"'{field}' has multiple endpoints without explicit arbitration"
                )

    def table_declaration(
        self,
        call: ast.Call,
    ) -> tuple[int, tuple[int, ...], ValueType, tuple[object, ...] | None] | None:
        if not isinstance(call.func, ast.Subscript):
            return None
        if _decorator_name(call.func.value).rsplit(".", 1)[-1] != "table":
            return None
        parameters = call.func.slice
        if not isinstance(parameters, ast.Tuple) or len(parameters.elts) != 2:
            raise QueueFrontendError(
                "ACPY-TABLE-001: table requires ac.table[entries, Entry]"
            )
        shape_node = parameters.elts[0]
        extent_nodes = (
            tuple(shape_node.elts)
            if isinstance(shape_node, ast.Tuple)
            else (shape_node,)
        )
        if not extent_nodes:
            raise QueueFrontendError("ACPY-TABLE-010: Table shape must be non-empty")
        shape_values = tuple(
            _constant_integer(extent, self.environment.static_values)
            for extent in extent_nodes
        )
        if any(extent is None for extent in shape_values):
            raise QueueFrontendError(
                "ACPY-TABLE-010: every Table extent must be a positive static integer"
            )
        shape = tuple(int(extent) for extent in shape_values if extent is not None)
        if any(extent <= 0 for extent in shape):
            raise QueueFrontendError(
                "ACPY-TABLE-010: every Table extent must be positive"
            )
        entries = 1
        for extent in shape:
            if entries > ((1 << 63) - 1) // extent:
                raise QueueFrontendError(
                    "ACPY-TABLE-010: Table flattened size overflows signed i64"
                )
            entries *= extent
        entry_type = _payload(
            parameters.elts[1],
            self.environment.payload_map,
            self.environment.enum_map,
            static_values=self.environment.type_static_values,
        )
        if call.args or any(
            keyword.arg is None or keyword.arg != "init" for keyword in call.keywords
        ):
            raise QueueFrontendError("ACPY-TABLE-001: table accepts only keyword init")
        init_values = [keyword.value for keyword in call.keywords]
        if len(init_values) > 1:
            raise QueueFrontendError("ACPY-TABLE-011: Table init is repeated")
        init_node = init_values[0] if init_values else ast.Constant(0)
        if _constant_integer(init_node, self.environment.static_values) == 0:
            return entries, shape, entry_type, None

        if not isinstance(init_node, ast.Dict):
            raise QueueFrontendError(
                "ACPY-TABLE-011: table init must be exactly zero or a "
                "versioned typed image"
            )
        image_fields: dict[str, ast.expr] = {}
        for key, value in zip(init_node.keys, init_node.values, strict=True):
            if not isinstance(key, ast.Constant) or type(key.value) is not str:
                raise QueueFrontendError(
                    "ACPY-TABLE-011: typed image keys must be static strings"
                )
            if key.value in image_fields:
                raise QueueFrontendError(
                    "ACPY-TABLE-011: typed image field is repeated"
                )
            image_fields[key.value] = value
        if set(image_fields) != {"version", "entry", "values"}:
            raise QueueFrontendError(
                "ACPY-TABLE-011: typed image requires version, entry, and values"
            )
        if (
            _constant_integer(image_fields["version"], self.environment.static_values)
            != 1
        ):
            raise QueueFrontendError(
                "ACPY-TABLE-011: typed image version must be exactly 1"
            )
        image_entry = _payload(
            image_fields["entry"],
            self.environment.payload_map,
            self.environment.enum_map,
            static_values=self.environment.type_static_values,
        )
        if image_entry != entry_type:
            raise QueueFrontendError(
                "ACPY-TABLE-011: typed image Entry type must match the Table"
            )
        values_node = image_fields["values"]
        if not isinstance(values_node, ast.List | ast.Tuple):
            raise QueueFrontendError(
                "ACPY-TABLE-011: typed image values must be a static sequence"
            )
        if len(values_node.elts) != entries:
            raise QueueFrontendError(
                f"ACPY-TABLE-011: typed image requires exactly {entries} entries"
            )

        def canonical_image_value(node: ast.expr, descriptor: ValueType) -> object:
            if isinstance(descriptor, BoolType):
                if isinstance(node, ast.Constant) and type(node.value) is bool:
                    return node.value
            elif isinstance(descriptor, BitsType):
                value = _constant_integer(node, self.environment.static_values)
                if value is not None and 0 <= value < (1 << descriptor.width):
                    return value
                if value is not None:
                    raise QueueFrontendError(
                        f"ACPY-TABLE-011: typed image value does not fit i{descriptor.width}"
                    )
            elif isinstance(descriptor, EnumType):
                if (
                    isinstance(node, ast.Attribute)
                    and _decorator_name(node.value).rsplit(".", 1)[-1]
                    == descriptor.name
                    and node.attr in descriptor.enumerants
                ):
                    return node.attr
            elif isinstance(descriptor, StructType):
                if (
                    isinstance(node, ast.Call)
                    and not node.args
                    and _decorator_name(node.func).rsplit(".", 1)[-1] == descriptor.name
                    and all(keyword.arg is not None for keyword in node.keywords)
                ):
                    fields = {keyword.arg: keyword.value for keyword in node.keywords}
                    if len(fields) == len(node.keywords) and set(fields) == {
                        field.name for field in descriptor.fields
                    }:
                        return {
                            field.name: canonical_image_value(
                                fields[field.name], field.type
                            )
                            for field in descriptor.fields
                        }
            elif isinstance(descriptor, TupleType) and isinstance(
                node, ast.List | ast.Tuple
            ):
                if len(node.elts) == len(descriptor.elements):
                    return [
                        canonical_image_value(value, value_type)
                        for value, value_type in zip(
                            node.elts, descriptor.elements, strict=True
                        )
                    ]
            elif isinstance(descriptor, ArrayType) and isinstance(
                node, ast.List | ast.Tuple
            ):
                if len(node.elts) == descriptor.length:
                    return [
                        canonical_image_value(value, descriptor.element)
                        for value in node.elts
                    ]
            raise QueueFrontendError(
                "ACPY-TABLE-011: typed image values must be closed literals "
                "matching the Entry descriptor"
            )

        canonical_values = [
            canonical_image_value(value, entry_type) for value in values_node.elts
        ]
        return entries, shape, entry_type, tuple(canonical_values)

    def parse_view(
        self,
        node: ast.expr,
        alias: str,
        scope_path: tuple[str, ...],
        current_order: int,
    ) -> EntryViewBinding | MaskedEntryViewBinding | ProjectedTableViewBinding | None:
        if not isinstance(node, ast.Call) or not isinstance(node.func, ast.Attribute):
            return None
        if node.func.attr != "view" or not isinstance(node.func.value, ast.Name):
            return None
        source_name = node.func.value.id
        projected_source = self.state.entry_views.get(source_name)
        if source_name in self.state.table_by_name:
            table_name = source_name
            prefix: tuple[ast.expr, ...] = ()
        elif isinstance(projected_source, ProjectedTableViewBinding):
            table_name = projected_source.table
            prefix = projected_source.prefix
        else:
            return None
        if len(node.args) != 1 or node.keywords:
            raise QueueFrontendError(
                "ACPY-TABLE-002: table.view requires one index or selector lambda"
            )
        selector = node.args[0]
        if (
            isinstance(selector, ast.Name)
            and selector.id in self.state.candidate_by_name
        ):
            candidate = self.state.candidate_by_name[selector.id]
            if candidate.table != table_name:
                raise QueueFrontendError(
                    "ACPY-TABLE-008: CandidateSet belongs to a different Table"
                )
            if isinstance(projected_source, ProjectedTableViewBinding) and (
                candidate.domain_axes != projected_source.domain_axes
                or candidate.domain_shape != projected_source.domain_shape
                or candidate.domain_strides != projected_source.domain_strides
                or candidate.domain_offset != projected_source.domain_offset
            ):
                raise QueueFrontendError(
                    "ACPY-TABLE-008: CandidateSet belongs to a different Table view"
                )
            return MaskedEntryViewBinding(
                alias, table_name, candidate.name, scope_path, current_order
            )
        if (
            isinstance(selector, ast.Attribute)
            and isinstance(selector.value, ast.Name)
            and selector.value.id in self.state.selection_by_name
            and self.state.selection_by_name[selector.value.id].table != table_name
        ):
            raise QueueFrontendError(
                "ACPY-TABLE-007: Selection belongs to a different Table"
            )
        if isinstance(selector, ast.Lambda):
            argument, address = _lambda(self.environment, selector)
        else:
            argument, address = (
                None,
                _constantize_expression(selector, "", self.environment.static_values),
            )
        table = self.state.table_by_name[table_name]
        flattened_choice_index = (
            isinstance(address, ast.Attribute)
            and isinstance(address.value, ast.Name)
            and address.attr == "index"
            and address.value.id in self.state.selection_by_name
            and self.state.selection_by_name[address.value.id].table == table_name
        )
        if flattened_choice_index:
            return EntryViewBinding(
                alias, table_name, argument, address, scope_path, current_order
            )
        local_coordinates = (
            tuple(address.elts) if isinstance(address, ast.Tuple) else (address,)
        )
        coordinates = (*prefix, *local_coordinates)
        if len(coordinates) > len(table.shape):
            raise QueueFrontendError(
                "ACPY-TABLE-010: Table index rank must match shape"
            )
        static_coordinates = tuple(
            _constant_integer(coordinate, self.environment.static_values)
            for coordinate in coordinates
        )
        for axis, (coordinate, extent) in enumerate(
            zip(
                static_coordinates,
                table.shape[: len(static_coordinates)],
                strict=True,
            )
        ):
            if coordinate is not None and not 0 <= coordinate < extent:
                raise QueueFrontendError(
                    f"ACPY-TABLE-010: Table index axis {axis} is out of range"
                )
        if len(coordinates) < len(table.shape):
            if argument is not None or any(
                coordinate is None for coordinate in static_coordinates
            ):
                raise QueueFrontendError(
                    "ACPY-TABLE-010: projected Table view requires static prefix "
                    "coordinates"
                )
            strides = tuple(
                _product(table.shape[axis + 1 :]) for axis in range(len(table.shape))
            )
            prefix_values = tuple(
                int(coordinate)
                for coordinate in static_coordinates
                if coordinate is not None
            )
            fixed = len(prefix_values)
            return ProjectedTableViewBinding(
                alias,
                table_name,
                tuple(coordinates),
                tuple(range(fixed, len(table.shape))),
                table.shape[fixed:],
                strides[fixed:],
                sum(
                    coordinate * stride
                    for coordinate, stride in zip(
                        prefix_values, strides[:fixed], strict=True
                    )
                ),
                scope_path,
                current_order,
            )
        if len(table.shape) == 1:
            address = coordinates[0]
        else:
            address = ast.copy_location(
                ast.Tuple(elts=list(coordinates), ctx=ast.Load()), address
            )
        return EntryViewBinding(
            alias, table_name, argument, address, scope_path, current_order
        )

    def resolve_view(
        self,
        node: ast.expr,
        scope_path: tuple[str, ...],
        current_order: int,
    ) -> EntryViewBinding | MaskedEntryViewBinding | ProjectedTableViewBinding | None:
        if isinstance(node, ast.Name):
            view = self.state.entry_views.get(node.id)
            if view and view.scope == scope_path:
                return view
            return None
        return self.parse_view(node, "", scope_path, current_order)
