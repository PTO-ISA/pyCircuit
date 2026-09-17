"""Typed expression emission for the Queue frontend."""

from __future__ import annotations

import ast
import copy
import re
from collections.abc import Collection, Mapping
from dataclasses import dataclass

from _pycircuit_semantics import (
    ArrayType,
    BitfieldLayout,
    BitsType,
    BoolType,
    ClosedInterval,
    Constant,
    Constraint,
    EnumType,
    RangeType,
    StructType,
    TupleType,
    Unknown,
    ValueType,
    constraint_for_type,
    parse_bitmask_checked,
    primitive_count_width,
    primitive_priority_index_width,
    prove_within,
    transfer_bits,
)

from .acir_text import (
    _render_table_domain_attributes,
    _render_type,
    canonical_mlir_string,
)
from .errors import QueueFrontendError
from .model import (
    CandidateSetBinding,
    InvariantDefinition,
    Payload,
    PureHelperDefinition,
    SelectionBinding,
)
from .source import _render_source_frame_location, source_frame
from .static_types import (
    _constant_integer,
    _epoch_05_integer_width,
    _is_epoch_05_bool_compatible,
    _primitive_integer_width,
    _proven_integer_in,
    _scalar_type_descriptor,
    _table_axis_width,
    _types_equal_in_epoch_05,
)
from .syntax import _decorator_name


def _resolve_invariant_call(
    call: ast.Call,
    invariants: Mapping[str, InvariantDefinition],
    shadowed: Collection[str] = (),
) -> InvariantDefinition | None:
    if not isinstance(call.func, ast.Name) or call.func.id in shadowed:
        return None
    return invariants.get(call.func.id)


@dataclass(frozen=True, slots=True)
class _ExpressionFact:
    value_type: ValueType
    constraint: Constraint


_MAX_ARRAY_COMBINATOR_EXPANSION = 4096


class _ExpressionEmitter:
    def __init__(
        self,
        payloads: dict[str, Payload],
        argument: str,
        payload: ValueType,
        *,
        root_name: str = "item",
        root_values: Mapping[str, tuple[str, ValueType]] | None = None,
        prefix: str = "",
        table_views: Mapping[str, tuple[str, ast.expr, ValueType]] | None = None,
        slot_views: Mapping[str, tuple[str, ValueType]] | None = None,
        candidates: Mapping[str, CandidateSetBinding] | None = None,
        selections: Mapping[str, SelectionBinding] | None = None,
        candidate_values: Mapping[str, tuple[str, ValueType]] | None = None,
        selection_values: (
            Mapping[str, tuple[str, ValueType, str, ValueType]] | None
        ) = None,
        find_values: (
            Mapping[
                str,
                tuple[
                    str,
                    ValueType,
                    str,
                    ValueType,
                    str,
                    ValueType,
                    str | None,
                    str,
                ],
            ]
            | None
        ) = None,
        state_views: (
            Mapping[str, tuple[str, str, ValueType, int, tuple[int, ...]]] | None
        ) = None,
        table_domains: (
            Mapping[str, tuple[ValueType, int, tuple[int, ...]]] | None
        ) = None,
        enum_types: Mapping[str, EnumType] | None = None,
        bitfields: Mapping[str, BitfieldLayout] | None = None,
        invariants: Mapping[str, InvariantDefinition] | None = None,
        helpers: Mapping[str, PureHelperDefinition] | None = None,
        inline_pure_helpers: bool = False,
        strict_descriptors: bool = False,
        array_expansion: list[int] | None = None,
    ) -> None:
        self.payloads = payloads
        self.enum_types: dict[str, EnumType] = dict(enum_types or {})

        def collect_enums(descriptor: ValueType) -> None:
            if isinstance(descriptor, EnumType):
                existing = self.enum_types.get(descriptor.name)
                if existing is not None and existing != descriptor:
                    raise QueueFrontendError(
                        "ACPY-TYPE-005: enum identity has conflicting declarations"
                    )
                self.enum_types[descriptor.name] = descriptor
            elif isinstance(descriptor, StructType):
                for field in descriptor.fields:
                    collect_enums(field.type)
            elif isinstance(descriptor, TupleType):
                for element in descriptor.elements:
                    collect_enums(element)
            elif isinstance(descriptor, ArrayType):
                collect_enums(descriptor.element)

        for payload_definition in payloads.values():
            collect_enums(payload_definition.descriptor)
        self.argument = argument
        self.payload = payload
        self.root_name = root_name
        self.root_values = dict(root_values or {})
        self.prefix = prefix
        self.table_views = dict(table_views or {})
        self.slot_views = dict(slot_views or {})
        self.candidates = dict(candidates or {})
        self.selections = dict(selections or {})
        self.candidate_values = dict(candidate_values or {})
        self.selection_values = dict(selection_values or {})
        self.find_values = dict(find_values or {})
        self.state_views = dict(state_views or {})
        self.table_domains = dict(table_domains or {})
        self.bitfields = dict(bitfields or {})
        self.invariants = dict(invariants or {})
        self.helpers = dict(helpers or {})
        self.inline_pure_helpers = inline_pure_helpers
        self.active_inline_helpers: set[str] = set()
        self.strict_descriptors = strict_descriptors
        self.array_expansion = array_expansion if array_expansion is not None else [0]
        self.array_callback_captures: dict[
            tuple[int, str], tuple[str, str, ValueType]
        ] = {}
        self.array_selection_values: dict[
            str, tuple[str, ValueType, str, ValueType]
        ] = {}
        self.lines: list[str] = []
        self.index = 0
        self.priority_values: dict[str, tuple[str, ValueType, str, ValueType]] = {}
        self.onehot_values: dict[
            str,
            tuple[
                str,
                ValueType,
                str,
                ValueType,
                str | None,
                ValueType | None,
                str,
                ValueType,
            ],
        ] = {}
        self.range_checked_values: dict[str, tuple[str, ValueType, str, ValueType]] = {}
        self.enum_checked_values: dict[str, tuple[str, ValueType, str, ValueType]] = {}
        self.onehot_enum_values: dict[
            str,
            tuple[
                str,
                ValueType,
                str,
                ValueType,
                str,
                ValueType,
            ],
        ] = {}
        self.selection_batch_values: dict[
            str, tuple[tuple[str, ValueType, str, ValueType], ...]
        ] = {}
        self.table_view_values: dict[str, tuple[str, ValueType]] = {}
        self.state_read_values: dict[tuple[str, str], tuple[str, ValueType]] = {}
        self.deferred_values: dict[str, ast.expr] = {}
        self.expression_facts: dict[str, _ExpressionFact] = {}

    def _new(self) -> str:
        name = f"{self.prefix}v{self.index}"
        self.index += 1
        return name

    def _remember(
        self,
        name: str,
        value_type: ValueType,
        constraint: Constraint | None = None,
    ) -> tuple[str, ValueType]:
        self.expression_facts[name] = _ExpressionFact(
            value_type,
            (
                self._default_expression_constraint(value_type)
                if constraint is None
                else constraint
            ),
        )
        return name, value_type

    @staticmethod
    def _default_expression_constraint(value_type: ValueType) -> Constraint:
        # A nominal enum type defines its declared members, not a proof that
        # arbitrary ingress bits encode one of those members.  Only explicit
        # constructors and checked decoders install a stronger fact.
        if isinstance(value_type, EnumType):
            return Unknown()
        return constraint_for_type(value_type)

    def constraint_for_result(self, name: str, value_type: ValueType) -> Constraint:
        fact = self.expression_facts.get(name)
        if fact is None:
            return self._default_expression_constraint(value_type)
        if not self._types_match(fact.value_type, value_type):
            raise AssertionError("expression fact type does not match emitted result")
        return fact.constraint

    def _types_match(self, left: ValueType, right: ValueType) -> bool:
        return (
            left == right
            if self.strict_descriptors
            else _types_equal_in_epoch_05(left, right)
        )

    def _runtime_binding_names(self) -> set[str]:
        return {
            self.argument,
            *self.root_values,
            *self.deferred_values,
            *self.table_views,
            *self.slot_views,
            *self.candidates,
            *self.selections,
            *self.candidate_values,
            *self.selection_values,
            *self.find_values,
            *self.state_views,
            *self.table_domains,
        }

    def _unshadowed_enum(self, name: str) -> EnumType | None:
        return (
            None if name in self._runtime_binding_names() else self.enum_types.get(name)
        )

    def _unshadowed_record(self, name: str) -> StructType | None:
        definition = (
            None if name in self._runtime_binding_names() else self.payloads.get(name)
        )
        return None if definition is None else definition.descriptor

    def _explicit_enum_member(
        self, node: ast.expr, expected: EnumType | None = None
    ) -> tuple[EnumType, str] | None:
        if not (isinstance(node, ast.Attribute) and isinstance(node.value, ast.Name)):
            return None
        enumeration = self._unshadowed_enum(node.value.id)
        if (
            enumeration is None
            or (expected is not None and enumeration != expected)
            or node.attr not in enumeration.enumerants
        ):
            return None
        return enumeration, node.attr

    def _reserve_array_expansion(self, count: int) -> None:
        if count <= 0 or self.array_expansion[0] > (
            _MAX_ARRAY_COMBINATOR_EXPANSION - count
        ):
            raise QueueFrontendError(
                "ACPY-TYPE-006: array combinator expansion exceeds 4096 lanes"
            )
        self.array_expansion[0] += count

    def reject_constant_index_outside(
        self,
        name: str,
        value_type: ValueType,
        entries: int,
        diagnostic: str,
    ) -> None:
        """Reject a disproven constant and defer every non-constant to MLIR."""

        fact = self.constraint_for_result(name, value_type)
        if not isinstance(fact, Constant):
            return
        if type(fact.value) is not int or not prove_within(fact, 0, entries - 1):
            raise QueueFrontendError(diagnostic)

    @staticmethod
    def _typed_integer_target(node: ast.expr, intrinsic: str) -> BitsType:
        try:
            target = _scalar_type_descriptor(node)
        except QueueFrontendError as error:
            raise QueueFrontendError(
                f"ACPY-CAST-001: {intrinsic} target must be one concrete ac.uN/ac.bits[N] type"
            ) from error
        if not isinstance(target, BitsType):
            raise QueueFrontendError(
                f"ACPY-CAST-001: {intrinsic} target must be an unsigned bits type"
            )
        return target

    def _emit_typed_integer_intrinsic(
        self, node: ast.Call
    ) -> tuple[str, ValueType] | None:
        intrinsic = _decorator_name(node.func).rsplit(".", 1)[-1]
        if intrinsic not in {"literal", "zero", "zext", "sext", "truncate"}:
            return None
        if node.keywords:
            raise QueueFrontendError(
                f"ACPY-CAST-001: {intrinsic} accepts positional arguments only"
            )
        if intrinsic == "zero":
            if len(node.args) != 1:
                raise QueueFrontendError(
                    "ACPY-CAST-001: zero requires one concrete target type"
                )
            target = _scalar_type_descriptor(node.args[0])
            from _pycircuit_semantics import RangeType

            if not isinstance(target, BitsType | RangeType):
                raise QueueFrontendError(
                    "ACPY-CAST-001: zero target must be bits or bounded range"
                )
            value = 0
            if isinstance(target, RangeType) and target.lower != 0:
                raise QueueFrontendError(
                    "ACPY-RANGE-001: zero is outside the bounded target"
                )
        elif intrinsic == "literal":
            if len(node.args) != 2:
                raise QueueFrontendError(
                    "ACPY-CAST-001: literal requires a value and concrete target type"
                )
            value = _constant_integer(node.args[0])
            target = _scalar_type_descriptor(node.args[1])
            from _pycircuit_semantics import RangeType

            if not isinstance(target, BitsType | RangeType):
                raise QueueFrontendError(
                    "ACPY-CAST-001: literal target must be bits or bounded range"
                )
            if value is None or not _proven_integer_in(
                value,
                target.lower if isinstance(target, RangeType) else 0,
                (
                    target.upper - 1
                    if isinstance(target, RangeType)
                    else (1 << target.width) - 1
                ),
            ):
                raise QueueFrontendError(
                    "ACPY-CAST-001: typed literal must be a nonnegative static integer that fits its target"
                )
        else:
            if len(node.args) != 2:
                raise QueueFrontendError(
                    f"ACPY-CAST-001: {intrinsic} requires a value and concrete target type"
                )
            target = self._typed_integer_target(node.args[1], intrinsic)
            source, source_type = self.emit(node.args[0])
            if not isinstance(source_type, BitsType):
                raise QueueFrontendError(
                    f"ACPY-CAST-001: {intrinsic} source must be an unsigned bits value"
                )
            source_width = source_type.width
            if intrinsic in {"zext", "sext"} and target.width <= source_width:
                raise QueueFrontendError(
                    f"ACPY-CAST-001: {intrinsic} target must be wider than its source"
                )
            if intrinsic == "truncate" and target.width >= source_width:
                raise QueueFrontendError(
                    "ACPY-CAST-001: truncate target must be narrower than its source"
                )
            result = self._new()
            rendered_source = _render_type(source_type)
            rendered_target = _render_type(target)
            if intrinsic == "truncate":
                self.lines.append(
                    f"    %{result} = ac.var.extract %{source} from 0 width {target.width} : "
                    f"!ac.var<{rendered_source}> -> !ac.var<{rendered_target}>"
                )
                constraint = transfer_bits(
                    "and",
                    self.constraint_for_result(source, source_type),
                    Constant((1 << target.width) - 1),
                    width=target.width,
                )
                return self._remember(result, target, constraint)
            extension = target.width - source_width
            if intrinsic == "zext":
                fill = self._new()
                fill_type = BitsType(extension)
                rendered_fill = _render_type(fill_type)
                self.lines.append(
                    f"    %{fill} = ac.var.constant 0 : {rendered_fill} as !ac.var<{rendered_fill}>"
                )
                operands = (fill, source)
                operand_types = (fill_type, source_type)
            else:
                sign = self._new()
                self.lines.append(
                    f"    %{sign} = ac.var.extract %{source} from {source_width - 1} width 1 : "
                    f"!ac.var<{rendered_source}> -> !ac.var<i1>"
                )
                operands = (*((sign,) * extension), source)
                operand_types = (*((BitsType(1),) * extension), source_type)
            self.lines.append(
                f"    %{result} = ac.var.concat "
                + ", ".join(f"%{operand}" for operand in operands)
                + " : "
                + ", ".join(
                    f"!ac.var<{_render_type(operand_type)}>"
                    for operand_type in operand_types
                )
                + f" -> !ac.var<{rendered_target}>"
            )
            constraint = (
                self.constraint_for_result(source, source_type)
                if intrinsic == "zext"
                else constraint_for_type(target)
            )
            return self._remember(result, target, constraint)

        name = self._new()
        from _pycircuit_semantics import RangeType

        rendered = _render_type(target)
        attribute_type = (
            f"i{target.width}" if isinstance(target, RangeType) else rendered
        )
        self.lines.append(
            f"    %{name} = ac.var.constant {value} : {attribute_type} as "
            f"!ac.var<{rendered}>"
        )
        return self._remember(name, target, Constant(value))

    @staticmethod
    def _typed_range_target(node: ast.expr):
        from _pycircuit_semantics import RangeType

        try:
            target = _scalar_type_descriptor(node)
        except QueueFrontendError as error:
            raise QueueFrontendError(
                "ACPY-RANGE-001: range conversion target must be ac.index[N] "
                "or ac.range[lo, hi]"
            ) from error
        if not isinstance(target, RangeType):
            raise QueueFrontendError(
                "ACPY-RANGE-001: range conversion target must be bounded"
            )
        return target

    def _emit_range_intrinsic(self, node: ast.Call) -> tuple[str, ValueType] | None:
        from _pycircuit_semantics import RangeType

        intrinsic = _decorator_name(node.func).rsplit(".", 1)[-1]
        if intrinsic not in {"wrap", "saturate", "refine"}:
            return None
        if node.keywords or len(node.args) != 2:
            raise QueueFrontendError(
                f"ACPY-RANGE-001: {intrinsic} requires value and bounded target"
            )
        source, source_type = self.emit(node.args[0])
        if not isinstance(source_type, BitsType | RangeType):
            raise QueueFrontendError(
                "ACPY-RANGE-001: range conversion source must be unsigned scalar"
            )
        target = self._typed_range_target(node.args[1])
        result = self._new()
        static_target = getattr(node.args[1], "_ac_static_type_target", None)
        attributes = (
            " {ac.static_type_target = " + canonical_mlir_string(static_target) + "}"
            if isinstance(static_target, str)
            else ""
        )
        self.lines.append(
            f"    %{result} = ac.var.range_{intrinsic} %{source}{attributes} : "
            f"!ac.var<{_render_type(source_type)}> -> "
            f"!ac.var<{_render_type(target)}>"
        )
        return self._remember(
            result,
            target,
            ClosedInterval(target.lower, target.upper - 1),
        )

    def _emit_checked_range(
        self, node: ast.Call
    ) -> tuple[str, ValueType, str, ValueType]:
        from _pycircuit_semantics import RangeType

        static_target = getattr(node.args[1], "_ac_static_type_target", None)
        key = ast.dump(node, include_attributes=False) + "|" + str(static_target)
        if cached := self.range_checked_values.get(key):
            return cached
        if (
            _decorator_name(node.func).rsplit(".", 1)[-1] != "checked"
            or node.keywords
            or len(node.args) != 2
        ):
            raise QueueFrontendError(
                "ACPY-RANGE-001: checked requires value and bounded target"
            )
        source, source_type = self.emit(node.args[0])
        if not isinstance(source_type, BitsType | RangeType):
            raise QueueFrontendError(
                "ACPY-RANGE-001: checked source must be unsigned scalar"
            )
        target = self._typed_range_target(node.args[1])
        value = self._new()
        valid = self._new()
        static_target = getattr(node.args[1], "_ac_static_type_target", None)
        attributes = (
            " {ac.static_type_target = " + canonical_mlir_string(static_target) + "}"
            if isinstance(static_target, str)
            else ""
        )
        self.lines.append(
            f"    %{value}, %{valid} = ac.var.range_checked %{source}"
            f"{attributes} : "
            f"!ac.var<{_render_type(source_type)}> -> "
            f"!ac.var<{_render_type(target)}>, !ac.var<i1>"
        )
        result = (value, target, valid, BoolType())
        self.expression_facts[value] = _ExpressionFact(
            target, ClosedInterval(target.lower, target.upper - 1)
        )
        self.expression_facts[valid] = _ExpressionFact(BoolType(), ClosedInterval(0, 1))
        self.range_checked_values[key] = result
        return result

    def _emit_checked_enum(
        self, node: ast.Call
    ) -> tuple[str, ValueType, str, ValueType] | None:
        if (
            _decorator_name(node.func).rsplit(".", 1)[-1] != "checked"
            or len(node.args) < 2
        ):
            return None
        target_name = node.args[1].id if isinstance(node.args[1], ast.Name) else ""
        if (
            target_name in self.enum_types
            and self._unshadowed_enum(target_name) is None
        ):
            raise QueueFrontendError(
                "ACPY-TYPE-005: checked enum target must be an unshadowed enum class"
            )
        target = self._unshadowed_enum(target_name)
        if target is None:
            return None
        if (
            len(node.args) != 2
            or len(node.keywords) != 1
            or node.keywords[0].arg != "fallback"
        ):
            raise QueueFrontendError(
                "ACPY-TYPE-005: checked enum conversion requires fallback=Enum.MEMBER"
            )
        fallback_node = node.keywords[0].value
        if self._explicit_enum_member(fallback_node, target) is None:
            raise QueueFrontendError(
                "ACPY-TYPE-005: checked enum fallback must be an explicit target member"
            )
        key = ast.dump(node, include_attributes=False)
        cached = self.enum_checked_values.get(key)
        if cached is not None:
            return cached
        raw, raw_type = self.emit(node.args[0])
        if (
            not isinstance(raw_type, BitsType)
            or raw_type.width != target.encoding_width
        ):
            raise QueueFrontendError(
                "ACPY-TYPE-005: checked enum input width must exactly match the enum encoding"
            )
        fallback, fallback_type = self.emit(fallback_node)
        if fallback_type != target:
            raise QueueFrontendError(
                "ACPY-TYPE-005: checked enum fallback must match the target enum"
            )
        candidates: list[tuple[str, str]] = []
        rendered_bits = _render_type(raw_type)
        for enumerant, encoding in zip(
            target.enumerants, target.encoding_values, strict=True
        ):
            constant = self._new()
            self.lines.append(
                f"    %{constant} = ac.var.constant {encoding} : {rendered_bits} "
                f"as !ac.var<{rendered_bits}>"
            )
            matched = self._new()
            self.lines.append(
                f'    %{matched} = ac.var.cmp "eq" %{raw}, %{constant} : '
                f"!ac.var<{rendered_bits}> -> !ac.var<i1>"
            )
            value, value_type = self.emit(
                ast.copy_location(
                    ast.Attribute(
                        value=ast.Name(id=target.name, ctx=ast.Load()),
                        attr=enumerant,
                        ctx=ast.Load(),
                    ),
                    node.args[1],
                )
            )
            assert value_type == target
            candidates.append((value, matched))
        while len(candidates) > 1:
            following: list[tuple[str, str]] = []
            for index in range(0, len(candidates), 2):
                if index + 1 == len(candidates):
                    following.append(candidates[index])
                    continue
                left_value, left_valid = candidates[index]
                right_value, right_valid = candidates[index + 1]
                selected = self._emit_typed_select(
                    left_valid, left_value, right_value, target
                )
                valid = self._emit_bool_binary("or", left_valid, right_valid)
                following.append((selected, valid))
            candidates = following
        selected, valid = candidates[0]
        selected = self._emit_typed_select(valid, selected, fallback, target)
        result = (selected, target, valid, BoolType())
        self.enum_checked_values[key] = result
        return result

    def _emit_onehot_enum(
        self, node: ast.Call
    ) -> tuple[str, ValueType, str, ValueType, str, ValueType] | None:
        if _decorator_name(node.func).rsplit(".", 1)[-1] != "onehot_enum":
            return None
        key = ast.dump(node, include_attributes=False)
        cached = self.onehot_enum_values.get(key)
        if cached is not None:
            return cached
        if len(node.args) != 1 or any(
            keyword.arg not in {"members", "empty", "conflict"}
            for keyword in node.keywords
        ):
            raise QueueFrontendError(
                "ACPY-TYPE-005: onehot_enum requires mask, members, empty, and conflict"
            )
        keywords = {
            keyword.arg: keyword.value
            for keyword in node.keywords
            if keyword.arg is not None
        }
        if set(keywords) != {"members", "empty", "conflict"} or len(keywords) != len(
            node.keywords
        ):
            raise QueueFrontendError(
                "ACPY-TYPE-005: onehot_enum requires one members/empty/conflict each"
            )
        members_node = keywords["members"]
        if not isinstance(members_node, (ast.Tuple, ast.List)) or not members_node.elts:
            raise QueueFrontendError(
                "ACPY-TYPE-005: onehot_enum members must be a non-empty static tuple"
            )
        mask, mask_type = self.emit(node.args[0])
        if isinstance(mask_type, BitsType):
            mask_width = mask_type.width
            flags: list[str] = []
            zero_bit = self._new()
            self.lines.append(
                f"    %{zero_bit} = ac.var.constant 0 : i1 as !ac.var<i1>"
            )
            for ordinal in range(mask_width):
                bit = self._new()
                self.lines.append(
                    f"    %{bit} = ac.var.extract %{mask} from {ordinal} width 1 : "
                    f"!ac.var<{_render_type(mask_type)}> -> !ac.var<i1>"
                )
                present = self._new()
                self.lines.append(
                    f'    %{present} = ac.var.cmp "ne" %{bit}, %{zero_bit} : '
                    "!ac.var<i1> -> !ac.var<i1>"
                )
                flags.append(present)
        elif isinstance(mask_type, ArrayType) and mask_type.element == BoolType():
            mask_width = mask_type.length
            self._reserve_array_expansion(mask_width)
            flags = [
                value
                for value, value_type in (
                    self._emit_static_array_element(mask, mask_type, ordinal)
                    for ordinal in range(mask_width)
                )
                if value_type == BoolType()
            ]
            assert len(flags) == mask_width
        else:
            raise QueueFrontendError(
                "ACPY-TYPE-005: onehot_enum mask must be unsigned bits[1..64] "
                "or a fixed bool array"
            )
        if len(members_node.elts) != mask_width:
            raise QueueFrontendError(
                "ACPY-TYPE-005: onehot_enum member count must match mask width"
            )
        enum_type: EnumType | None = None
        members: list[tuple[str, str]] = []
        seen: set[str] = set()
        for ordinal, member_node in enumerate(members_node.elts):
            member = self._explicit_enum_member(member_node)
            if member is None or member[1] in seen:
                raise QueueFrontendError(
                    "ACPY-TYPE-005: onehot_enum members must be unique enum constants"
                )
            value, value_type = self.emit(member_node)
            if not isinstance(value_type, EnumType) or (
                enum_type is not None and value_type != enum_type
            ):
                raise QueueFrontendError(
                    "ACPY-TYPE-005: onehot_enum members must use one exact enum"
                )
            enum_type = value_type
            seen.add(member[1])
            members.append((value, flags[ordinal]))
        assert enum_type is not None

        def explicit_fallback(name: str) -> tuple[str, ValueType]:
            fallback_node = keywords[name]
            if self._explicit_enum_member(fallback_node, enum_type) is None:
                raise QueueFrontendError(
                    f"ACPY-TYPE-005: onehot_enum {name} must be an explicit target member"
                )
            return self.emit(fallback_node)

        empty, empty_type = explicit_fallback("empty")
        conflict_value, conflict_type = explicit_fallback("conflict")
        assert empty_type == enum_type and conflict_type == enum_type

        candidates = members
        while len(candidates) > 1:
            following: list[tuple[str, str]] = []
            for index in range(0, len(candidates), 2):
                if index + 1 == len(candidates):
                    following.append(candidates[index])
                    continue
                left_value, left_present = candidates[index]
                right_value, right_present = candidates[index + 1]
                selected = self._emit_typed_select(
                    left_present, left_value, right_value, enum_type
                )
                present = self._emit_bool_binary("or", left_present, right_present)
                following.append((selected, present))
            candidates = following
        selected, present = candidates[0]
        selected = self._emit_typed_select(present, selected, empty, enum_type)
        if isinstance(mask_type, BitsType):
            count_type: ValueType = BitsType(primitive_count_width(mask_width))
            count = self._new()
            self.lines.append(
                f"    %{count} = ac.var.popcount %{mask} : "
                f"!ac.var<{_render_type(mask_type)}> -> "
                f"!ac.var<{_render_type(count_type)}>"
            )
            count_attribute_type = _render_type(count_type)
        else:
            contribution_type = RangeType(0, 2)
            rendered_contribution = _render_type(contribution_type)
            zero = self._new()
            contribution_one = self._new()
            self.lines.append(
                f"    %{zero} = ac.var.constant 0 : i1 "
                f"as !ac.var<{rendered_contribution}>"
            )
            self.lines.append(
                f"    %{contribution_one} = ac.var.constant 1 : i1 "
                f"as !ac.var<{rendered_contribution}>"
            )
            counts: list[tuple[str, RangeType]] = []
            for flag in flags:
                contribution = self._new()
                self.lines.append(
                    f"    %{contribution} = ac.var.select %{flag}, "
                    f"%{contribution_one}, %{zero} : !ac.var<i1>, "
                    f"!ac.var<{rendered_contribution}> -> "
                    f"!ac.var<{rendered_contribution}>"
                )
                counts.append((contribution, contribution_type))
            while len(counts) > 1:
                following_counts: list[tuple[str, RangeType]] = []
                for index in range(0, len(counts), 2):
                    if index + 1 == len(counts):
                        following_counts.append(counts[index])
                        continue
                    left, left_type = counts[index]
                    right, right_type = counts[index + 1]
                    result_type = RangeType(
                        left_type.lower + right_type.lower,
                        left_type.upper + right_type.upper - 1,
                    )
                    result = self._new()
                    self.lines.append(
                        f"    %{result} = ac.var.range_add %{left}, %{right} : "
                        f"!ac.var<{_render_type(left_type)}>, "
                        f"!ac.var<{_render_type(right_type)}> -> "
                        f"!ac.var<{_render_type(result_type)}>"
                    )
                    following_counts.append((result, result_type))
                counts = following_counts
            count, count_type = counts[0]
            count_attribute_type = f"i{count_type.bit_width()}"
        one = self._new()
        conflict = self._new()
        self.lines.append(
            f"    %{one} = ac.var.constant 1 : {count_attribute_type} "
            f"as !ac.var<{_render_type(count_type)}>"
        )
        if isinstance(count_type, RangeType):
            self.lines.append(
                f'    %{conflict} = ac.var.range_cmp "ugt" %{count}, %{one} : '
                f"!ac.var<{_render_type(count_type)}>, "
                f"!ac.var<{_render_type(count_type)}> -> !ac.var<i1>"
            )
        else:
            self.lines.append(
                f'    %{conflict} = ac.var.cmp "ugt" %{count}, %{one} : '
                f"!ac.var<{_render_type(count_type)}> -> !ac.var<i1>"
            )
        selected = self._emit_typed_select(
            conflict, conflict_value, selected, enum_type
        )
        result = (
            selected,
            enum_type,
            present,
            BoolType(),
            conflict,
            BoolType(),
        )
        self.onehot_enum_values[key] = result
        return result

    def _emit_enum_match(
        self, node: ast.Call, expected: ValueType | None
    ) -> tuple[str, ValueType] | None:
        if _decorator_name(node.func).rsplit(".", 1)[-1] != "match_enum":
            return None
        if (
            len(node.args) != 2
            or len(node.keywords) != 1
            or node.keywords[0].arg != "invalid"
        ):
            raise QueueFrontendError(
                "ACPY-TYPE-005: match_enum requires selector, case dictionary, "
                "and invalid=..."
            )
        selector, selector_type = self.emit(node.args[0])
        if not isinstance(selector_type, EnumType):
            raise QueueFrontendError(
                "ACPY-TYPE-005: match_enum selector must be an enum value"
            )
        cases_node = node.args[1]
        if not isinstance(cases_node, ast.Dict) or any(
            key is None for key in cases_node.keys
        ):
            raise QueueFrontendError(
                "ACPY-TYPE-005: match_enum cases must be a static dictionary "
                "without spread"
            )
        cases: dict[str, ast.expr] = {}
        for key_node, value_node in zip(
            cases_node.keys, cases_node.values, strict=True
        ):
            assert key_node is not None
            case_member = self._explicit_enum_member(key_node, selector_type)
            if case_member is None:
                raise QueueFrontendError(
                    "ACPY-TYPE-005: match_enum case keys must be reachable "
                    "members of the selector enum"
                )
            if case_member[1] in cases:
                raise QueueFrontendError(
                    f"ACPY-TYPE-005: match_enum case {selector_type.name}."
                    f"{case_member[1]} is repeated"
                )
            cases[case_member[1]] = value_node
        missing = [member for member in selector_type.enumerants if member not in cases]
        if missing:
            raise QueueFrontendError(
                "ACPY-TYPE-005: match_enum cases are not exhaustive; missing "
                + ", ".join(f"{selector_type.name}.{member}" for member in missing)
            )
        previous_strict = self.strict_descriptors
        self.strict_descriptors = True
        try:
            invalid, result_type = self.emit(node.keywords[0].value, expected)
            if expected is not None and result_type != expected:
                raise QueueFrontendError(
                    "ACPY-TYPE-005: match_enum cases and invalid value must "
                    "have one exact recursive type"
                )
            values: list[str] = []
            for member in selector_type.enumerants:
                value, value_type = self.emit(cases[member], result_type)
                if value_type != result_type:
                    raise QueueFrontendError(
                        "ACPY-TYPE-005: match_enum cases and invalid value must "
                        "have one exact recursive type"
                    )
                values.append(value)
        finally:
            self.strict_descriptors = previous_strict
        operands = [selector, *values, invalid]
        operand_types = [selector_type, *((result_type,) * (len(values) + 1))]
        result = self._new()
        self.lines.append(
            f"    %{result} = ac.var.enum_match "
            + ", ".join(f"%{operand}" for operand in operands)
            + " cases ["
            + ", ".join(
                canonical_mlir_string(member) for member in selector_type.enumerants
            )
            + "] : "
            + ", ".join(
                f"!ac.var<{_render_type(value_type)}>" for value_type in operand_types
            )
            + f" -> !ac.var<{_render_type(result_type)}>"
        )
        return self._remember(result, result_type)

    def _emit_record_projection(self, node: ast.Call) -> tuple[str, ValueType] | None:
        if not (isinstance(node.func, ast.Attribute) and node.func.attr == "project"):
            return None
        if len(node.args) != 1 or node.keywords:
            raise QueueFrontendError(
                "ACPY-TYPE-006: record project requires one explicit target struct"
            )
        source, source_type = self.emit(node.func.value)
        if not isinstance(source_type, StructType):
            raise QueueFrontendError(
                "ACPY-TYPE-006: record project receiver must be a struct value"
            )
        target_name = node.args[0].id if isinstance(node.args[0], ast.Name) else ""
        target_type = self._unshadowed_record(target_name)
        if target_type is None:
            raise QueueFrontendError(
                "ACPY-TYPE-006: record project target must be an unshadowed "
                "@ac.struct class"
            )
        values: list[str] = []
        value_types: list[ValueType] = []
        for target_field in target_type.fields:
            try:
                source_field = source_type.field(target_field.name)
            except KeyError as error:
                raise QueueFrontendError(
                    "ACPY-TYPE-006: record project source is missing target field "
                    f"{target_field.name!r}"
                ) from error
            if source_field.type != target_field.type:
                raise QueueFrontendError(
                    "ACPY-TYPE-006: record project field "
                    f"{target_field.name!r} requires one exact recursive type"
                )
            value = self._new()
            self.lines.append(
                f"    %{value} = ac.var.get %{source} field "
                f'"{target_field.name}" : '
                f"!ac.var<{_render_type(source_type)}> -> "
                f"!ac.var<{_render_type(target_field.type)}>"
            )
            values.append(value)
            value_types.append(target_field.type)
        result = self._new()
        self.lines.append(
            f"    %{result} = ac.var.record "
            + ", ".join(f"%{value}" for value in values)
            + " : "
            + ", ".join(
                f"!ac.var<{_render_type(value_type)}>" for value_type in value_types
            )
            + f" -> !ac.var<{_render_type(target_type)}>"
        )
        return self._remember(result, target_type)

    def _emit_exact_array_replacement(
        self,
        node: ast.expr,
        expected: ValueType,
        mismatch_message: str = "with_element replacement type must match",
    ) -> tuple[str, ValueType]:
        if isinstance(node, (ast.Tuple, ast.List)):
            if not isinstance(expected, (TupleType, ArrayType)):
                raise QueueFrontendError(f"ACPY-TYPE-006: {mismatch_message}")
            if isinstance(expected, TupleType):
                element_types = expected.elements
                operation = "tuple"
            else:
                element_types = (expected.element,) * expected.length
                operation = "array"
            if len(node.elts) != len(element_types):
                raise QueueFrontendError(
                    "ACPY-TYPE-006: aggregate literal arity must match its type"
                )
            values: list[str] = []
            value_types: list[ValueType] = []
            for element, descriptor in zip(node.elts, element_types, strict=True):
                value, value_type = self._emit_exact_array_replacement(
                    element, descriptor, mismatch_message
                )
                values.append(value)
                value_types.append(value_type)
            name = self._new()
            self.lines.append(
                f"    %{name} = ac.var.{operation} "
                + ", ".join(f"%{value}" for value in values)
                + " : "
                + ", ".join(
                    f"!ac.var<{_render_type(value_type)}>" for value_type in value_types
                )
                + f" -> !ac.var<{_render_type(expected)}>"
            )
            return name, expected

        from _pycircuit_semantics import RangeType

        literal_context = (
            expected
            if isinstance(node, ast.Constant)
            and type(node.value) is int
            and isinstance(expected, (BitsType, RangeType))
            else None
        )
        value, value_type = self.emit(
            node, expected if self.strict_descriptors else literal_context
        )
        if value_type != expected:
            raise QueueFrontendError(f"ACPY-TYPE-006: {mismatch_message}")
        return value, value_type

    def _emit_static_array_element(
        self, aggregate: str, aggregate_type: ArrayType, index: int
    ) -> tuple[str, ValueType]:
        result = self._new()
        self.lines.append(
            f"    %{result} = ac.var.element %{aggregate} at {index} : "
            f"!ac.var<{_render_type(aggregate_type)}> -> "
            f"!ac.var<{_render_type(aggregate_type.element)}>"
        )
        return result, aggregate_type.element

    def _emit_fixed_array(
        self,
        values: list[tuple[str, ValueType]],
        result_type: ArrayType,
    ) -> tuple[str, ValueType]:
        result = self._new()
        self.lines.append(
            f"    %{result} = ac.var.array "
            + ", ".join(f"%{value}" for value, _ in values)
            + " : "
            + ", ".join(
                f"!ac.var<{_render_type(value_type)}>" for _, value_type in values
            )
            + f" -> !ac.var<{_render_type(result_type)}>"
        )
        return result, result_type

    def _emit_callback_result(
        self,
        node: ast.expr,
        expected: ValueType | None,
    ) -> tuple[str, ValueType]:
        if expected is not None:
            if isinstance(node, (ast.Tuple, ast.List)):
                return self._emit_exact_array_replacement(
                    node,
                    expected,
                    "array map callback result type must match",
                )
            result, result_type = self.emit(node, expected)
            if result_type != expected:
                raise QueueFrontendError(
                    "ACPY-TYPE-006: array map callback result type must match"
                )
            return result, result_type
        if isinstance(node, ast.Tuple):
            elements = [self._emit_callback_result(item, None) for item in node.elts]
            if not elements:
                raise QueueFrontendError(
                    "ACPY-TYPE-006: array map callback tuple cannot be empty"
                )
            result_type = TupleType(tuple(value_type for _, value_type in elements))
            result = self._new()
            self.lines.append(
                f"    %{result} = ac.var.tuple "
                + ", ".join(f"%{value}" for value, _ in elements)
                + " : "
                + ", ".join(
                    f"!ac.var<{_render_type(value_type)}>" for _, value_type in elements
                )
                + f" -> !ac.var<{_render_type(result_type)}>"
            )
            return result, result_type
        if isinstance(node, ast.List):
            elements = [self._emit_callback_result(item, None) for item in node.elts]
            if not elements:
                raise QueueFrontendError(
                    "ACPY-TYPE-006: array map callback array cannot be empty"
                )
            element_type = elements[0][1]
            if any(value_type != element_type for _, value_type in elements[1:]):
                raise QueueFrontendError(
                    "ACPY-TYPE-006: array map callback array must be homogeneous"
                )
            return self._emit_fixed_array(
                elements, ArrayType(len(elements), element_type)
            )
        return self.emit(node)

    def _emit_array_callback(
        self,
        callback: ast.expr,
        value: str,
        value_type: ValueType,
        lane: int,
        expected: ValueType | None,
    ) -> tuple[str, ValueType]:
        callback_name: str
        callback_body: ast.expr
        if isinstance(callback, ast.Lambda):
            arguments = callback.args
            if (
                len(arguments.posonlyargs) + len(arguments.args) != 1
                or arguments.vararg is not None
                or arguments.kwarg is not None
                or arguments.kwonlyargs
                or arguments.defaults
                or arguments.kw_defaults
            ):
                raise QueueFrontendError(
                    "ACPY-TYPE-006: array map lambda requires one plain parameter"
                )
            callback_name = (arguments.posonlyargs or arguments.args)[0].arg
            callback_body = callback.body
        elif isinstance(callback, ast.Name) and callback.id in self.helpers:
            helper = self.helpers[callback.id]
            if len(helper.arguments) != 1 or helper.arguments[0][1] != value_type:
                raise QueueFrontendError(
                    "ACPY-TYPE-006: array map helper requires one exact element parameter"
                )
            callback_name = f"__ac_array_map_item_{lane}"
            callback_body = ast.copy_location(
                ast.Call(
                    func=copy.deepcopy(callback),
                    args=[ast.Name(id=callback_name, ctx=ast.Load())],
                    keywords=[],
                ),
                callback,
            )
        else:
            raise QueueFrontendError(
                "ACPY-TYPE-006: array map callback must be a lambda or pure helper"
            )

        callback_captures: dict[str, tuple[str, ValueType]] = {}
        if isinstance(callback, ast.Lambda):
            outer = self
            capture_reserved = {
                candidate.id
                for candidate in ast.walk(callback_body)
                if isinstance(candidate, ast.Name)
            } | {
                candidate.arg
                for candidate in ast.walk(callback_body)
                if isinstance(candidate, ast.arg)
            }
            capture_reserved.update(self.root_values)
            capture_reserved.update(self.deferred_values)
            capture_reserved.update({self.argument, callback_name})

            class MaterializeDeferredCaptures(ast.NodeTransformer):
                def __init__(self) -> None:
                    self.shadowed = {callback_name}
                    self.cache: dict[str, str] = {}

                def materialize(self, node: ast.expr) -> ast.Name:
                    key = ast.dump(node, include_attributes=False)
                    name = self.cache.get(key)
                    if name is None:
                        capture_key = (id(callback), key)
                        cached = outer.array_callback_captures.get(capture_key)
                        if cached is None:
                            value, captured_type = outer.emit(copy.deepcopy(node))
                            ordinal = len(outer.array_callback_captures)
                            name = f"__ac_array_capture_{ordinal}"
                            while name in capture_reserved:
                                ordinal += 1
                                name = f"__ac_array_capture_{ordinal}"
                            cached = (name, value, captured_type)
                            outer.array_callback_captures[capture_key] = cached
                        name, value, captured_type = cached
                        capture_reserved.add(name)
                        self.cache[key] = name
                        callback_captures[name] = (value, captured_type)
                    return ast.copy_location(ast.Name(id=name, ctx=ast.Load()), node)

                def visit_Lambda(self, node: ast.Lambda) -> ast.AST:
                    arguments = {
                        argument.arg
                        for argument in (
                            *node.args.posonlyargs,
                            *node.args.args,
                            *node.args.kwonlyargs,
                        )
                    }
                    if node.args.vararg is not None:
                        arguments.add(node.args.vararg.arg)
                    if node.args.kwarg is not None:
                        arguments.add(node.args.kwarg.arg)
                    previous = self.shadowed
                    self.shadowed = previous | arguments
                    node.body = self.visit(node.body)
                    self.shadowed = previous
                    return node

                def visit_Attribute(self, node: ast.Attribute) -> ast.AST:
                    if (
                        isinstance(node.value, ast.Name)
                        and node.value.id not in self.shadowed
                        and node.value.id in outer.deferred_values
                    ):
                        return self.materialize(node)
                    return self.generic_visit(node)

                def visit_Name(self, node: ast.Name) -> ast.AST:
                    if (
                        isinstance(node.ctx, ast.Load)
                        and node.id not in self.shadowed
                        and node.id in outer.deferred_values
                    ):
                        return self.materialize(node)
                    return node

            transformed = MaterializeDeferredCaptures().visit(
                copy.deepcopy(callback_body)
            )
            assert isinstance(transformed, ast.expr)
            callback_body = transformed

        callback_roots = dict(self.root_values)
        callback_roots.setdefault(self.argument, (self.root_name, self.payload))
        callback_roots[callback_name] = (value, value_type)
        callback_roots.update(callback_captures)
        child = _ExpressionEmitter(
            self.payloads,
            callback_name,
            value_type,
            root_name=value,
            root_values=callback_roots,
            prefix=f"{self.prefix}array_map_{self.index}_{lane}_",
            enum_types=self.enum_types,
            bitfields=self.bitfields,
            invariants=self.invariants,
            helpers=self.helpers,
            strict_descriptors=True,
            array_expansion=self.array_expansion,
        )
        result = child._emit_callback_result(callback_body, expected)
        self.lines.extend(child.lines)
        return result

    def _emit_array_map(self, node: ast.Call) -> tuple[str, ValueType] | None:
        if not (isinstance(node.func, ast.Attribute) and node.func.attr == "map"):
            return None
        if (
            isinstance(node.func.value, ast.Name)
            and node.func.value.id
            in {
                "ac",
                "agentic_circuit",
            }
            and node.func.value.id
            not in {
                self.argument,
                *self.root_values,
                *self.deferred_values,
            }
        ):
            return None
        if node.keywords or len(node.args) != 1:
            raise QueueFrontendError("ACPY-TYPE-006: array map requires one callback")
        aggregate, aggregate_type = self.emit(node.func.value)
        if not isinstance(aggregate_type, ArrayType):
            raise QueueFrontendError(
                "ACPY-TYPE-006: array map receiver must be a value-array"
            )
        self._reserve_array_expansion(aggregate_type.length)
        group = self.index
        self.index += 1
        mapped: list[tuple[str, ValueType]] = []
        element_type: ValueType | None = None
        for lane in range(aggregate_type.length):
            element, source_type = self._emit_static_array_element(
                aggregate, aggregate_type, lane
            )
            value, value_type = self._emit_array_callback(
                node.args[0], element, source_type, group + lane, element_type
            )
            if element_type is None:
                element_type = value_type
            elif value_type != element_type:
                raise QueueFrontendError(
                    "ACPY-TYPE-006: array map callback result type must match"
                )
            mapped.append((value, value_type))
        assert element_type is not None
        return self._emit_fixed_array(
            mapped, ArrayType(aggregate_type.length, element_type)
        )

    def _emit_array_zip(self, node: ast.Call) -> tuple[str, ValueType] | None:
        if not (isinstance(node.func, ast.Attribute) and node.func.attr == "zip"):
            return None
        if node.keywords or not node.args:
            raise QueueFrontendError(
                "ACPY-TYPE-006: array zip requires at least one other array"
            )
        arrays = [self.emit(node.func.value), *(self.emit(arg) for arg in node.args)]
        if not all(isinstance(value_type, ArrayType) for _, value_type in arrays):
            raise QueueFrontendError(
                "ACPY-TYPE-006: array zip operands must be value-arrays"
            )
        array_types = [
            value_type for _, value_type in arrays if isinstance(value_type, ArrayType)
        ]
        extent = array_types[0].length
        if any(value_type.length != extent for value_type in array_types[1:]):
            raise QueueFrontendError(
                "ACPY-TYPE-006: array zip operands must have equal length"
            )
        self._reserve_array_expansion(extent * len(array_types))
        tuple_type = TupleType(tuple(value_type.element for value_type in array_types))
        pairs: list[tuple[str, ValueType]] = []
        for lane in range(extent):
            elements = [
                self._emit_static_array_element(value, value_type, lane)
                for value, value_type in arrays
                if isinstance(value_type, ArrayType)
            ]
            result = self._new()
            self.lines.append(
                f"    %{result} = ac.var.tuple "
                + ", ".join(f"%{value}" for value, _ in elements)
                + " : "
                + ", ".join(
                    f"!ac.var<{_render_type(value_type)}>" for _, value_type in elements
                )
                + f" -> !ac.var<{_render_type(tuple_type)}>"
            )
            pairs.append((result, tuple_type))
        return self._emit_fixed_array(pairs, ArrayType(extent, tuple_type))

    def _emit_balanced_array_combine(
        self,
        values: list[tuple[str, ValueType]],
        kind: str,
    ) -> tuple[str, ValueType]:
        current = list(values)
        while len(current) > 1:
            following: list[tuple[str, ValueType]] = []
            for index in range(0, len(current), 2):
                if index + 1 == len(current):
                    following.append(current[index])
                    continue
                left, left_type = current[index]
                right, right_type = current[index + 1]
                if left_type != right_type:
                    raise QueueFrontendError(
                        "ACPY-TYPE-006: array reduction operands must match"
                    )
                if kind in {"min", "max"}:
                    condition = self._new()
                    predicate = "ule" if kind == "min" else "uge"
                    from _pycircuit_semantics import RangeType

                    rendered_type = _render_type(left_type)
                    comparison_op = (
                        "range_cmp" if isinstance(left_type, RangeType) else "cmp"
                    )
                    operand_types = (
                        f"!ac.var<{rendered_type}>, !ac.var<{rendered_type}>"
                        if comparison_op == "range_cmp"
                        else f"!ac.var<{rendered_type}>"
                    )
                    self.lines.append(
                        f'    %{condition} = ac.var.{comparison_op} "{predicate}" '
                        f"%{left}, %{right} : {operand_types} -> !ac.var<i1>"
                    )
                    result = self._new()
                    self.lines.append(
                        f"    %{result} = ac.var.select %{condition}, %{left}, "
                        f"%{right} : !ac.var<i1>, "
                        f"!ac.var<{_render_type(left_type)}> -> "
                        f"!ac.var<{_render_type(left_type)}>"
                    )
                else:
                    result = self._new()
                    self.lines.append(
                        f"    %{result} = ac.var.{kind} %{left}, %{right} : "
                        f"!ac.var<{_render_type(left_type)}>"
                    )
                following.append((result, left_type))
            current = following
        return current[0]

    def _emit_array_fold(self, node: ast.Call) -> tuple[str, ValueType] | None:
        if not (isinstance(node.func, ast.Attribute) and node.func.attr == "fold"):
            return None
        if (
            node.args
            or len(node.keywords) != 1
            or node.keywords[0].arg != "kind"
            or not isinstance(node.keywords[0].value, ast.Constant)
            or type(node.keywords[0].value.value) is not str
        ):
            raise QueueFrontendError("ACPY-TYPE-006: array fold requires kind='...'")
        aggregate, aggregate_type = self.emit(node.func.value)
        if not isinstance(aggregate_type, ArrayType):
            raise QueueFrontendError(
                "ACPY-TYPE-006: array fold receiver must be a value-array"
            )
        from _pycircuit_semantics import RangeType

        kind = node.keywords[0].value.value
        element_type = aggregate_type.element
        allowed = (
            {"and", "or", "xor"}
            if isinstance(element_type, BoolType)
            else (
                {"min", "max"}
                if isinstance(element_type, RangeType)
                else (
                    {"add", "mul", "and", "or", "xor", "min", "max"}
                    if isinstance(element_type, BitsType)
                    else set()
                )
            )
        )
        if kind not in allowed:
            raise QueueFrontendError(
                "ACPY-TYPE-006: array fold kind is not associative for its element type"
            )
        self._reserve_array_expansion(aggregate_type.length)
        values = [
            self._emit_static_array_element(aggregate, aggregate_type, lane)
            for lane in range(aggregate_type.length)
        ]
        return self._emit_balanced_array_combine(values, kind)

    def _emit_array_bool_reduction(
        self, node: ast.Call
    ) -> tuple[str, ValueType] | None:
        if not (
            isinstance(node.func, ast.Attribute)
            and node.func.attr in {"all", "any", "count"}
        ):
            return None
        method = node.func.attr
        if node.args or node.keywords:
            raise QueueFrontendError(
                f"ACPY-TYPE-006: array {method} takes no arguments"
            )
        aggregate, aggregate_type = self.emit(node.func.value)
        if not isinstance(aggregate_type, ArrayType):
            raise QueueFrontendError(
                f"ACPY-TYPE-006: array {method} receiver must be a value-array"
            )
        if aggregate_type.element != BoolType():
            raise QueueFrontendError(
                f"ACPY-TYPE-006: array {method} requires bool elements"
            )
        self._reserve_array_expansion(aggregate_type.length)
        flags = [
            self._emit_static_array_element(aggregate, aggregate_type, lane)
            for lane in range(aggregate_type.length)
        ]
        if method != "count":
            return self._emit_balanced_array_combine(
                flags, "and" if method == "all" else "or"
            )

        from _pycircuit_semantics import RangeType

        contribution_type = RangeType(0, 2)
        rendered_contribution = _render_type(contribution_type)
        zero = self._new()
        one = self._new()
        self.lines.append(
            f"    %{zero} = ac.var.constant 0 : i1 as !ac.var<{rendered_contribution}>"
        )
        self.lines.append(
            f"    %{one} = ac.var.constant 1 : i1 as !ac.var<{rendered_contribution}>"
        )
        contributions: list[tuple[str, ValueType]] = []
        for flag, _ in flags:
            contribution = self._new()
            self.lines.append(
                f"    %{contribution} = ac.var.select %{flag}, %{one}, %{zero} "
                f": !ac.var<i1>, !ac.var<{rendered_contribution}> -> "
                f"!ac.var<{rendered_contribution}>"
            )
            contributions.append((contribution, contribution_type))
        current = contributions
        while len(current) > 1:
            following: list[tuple[str, ValueType]] = []
            for index in range(0, len(current), 2):
                if index + 1 == len(current):
                    following.append(current[index])
                    continue
                left, left_type = current[index]
                right, right_type = current[index + 1]
                assert isinstance(left_type, RangeType)
                assert isinstance(right_type, RangeType)
                result_type = RangeType(
                    left_type.lower + right_type.lower,
                    left_type.upper + right_type.upper - 1,
                )
                result = self._new()
                self.lines.append(
                    f"    %{result} = ac.var.range_add %{left}, %{right} : "
                    f"!ac.var<{_render_type(left_type)}>, "
                    f"!ac.var<{_render_type(right_type)}> -> "
                    f"!ac.var<{_render_type(result_type)}>"
                )
                following.append((result, result_type))
            current = following
        result, result_type = current[0]
        return self._remember(
            result, result_type, ClosedInterval(0, aggregate_type.length)
        )

    def _emit_bool_not(self, value: str) -> str:
        result = self._new()
        self.lines.append(
            f"    %{result} = ac.var.not %{value} : !ac.var<i1> -> !ac.var<i1>"
        )
        return result

    def _emit_bool_binary(self, kind: str, left: str, right: str) -> str:
        result = self._new()
        self.lines.append(
            f"    %{result} = ac.var.{kind} %{left}, %{right} : !ac.var<i1>"
        )
        return result

    def _emit_typed_select(
        self,
        condition: str,
        true_value: str,
        false_value: str,
        value_type: ValueType,
    ) -> str:
        result = self._new()
        rendered = _render_type(value_type)
        self.lines.append(
            f"    %{result} = ac.var.select %{condition}, %{true_value}, "
            f"%{false_value} : !ac.var<i1>, !ac.var<{rendered}> -> "
            f"!ac.var<{rendered}>"
        )
        return result

    def _emit_array_selection(
        self, node: ast.Call
    ) -> tuple[str, ValueType, str, ValueType] | None:
        if not isinstance(node.func, ast.Attribute) or node.func.attr not in {
            "first",
            "argmin",
        }:
            return None
        method = node.func.attr
        if node.args or any(keyword.arg is None for keyword in node.keywords):
            raise QueueFrontendError(
                f"ACPY-TYPE-006: array {method} requires named callbacks"
            )
        keywords = {
            keyword.arg: keyword.value
            for keyword in node.keywords
            if keyword.arg is not None
        }
        if len(keywords) != len(node.keywords):
            raise QueueFrontendError(
                f"ACPY-TYPE-006: array {method} callback is repeated"
            )
        if method == "first":
            if set(keywords) - {"where"}:
                raise QueueFrontendError(
                    "ACPY-TYPE-006: array first accepts only where=..."
                )
        elif "key" not in keywords or set(keywords) - {"key", "where"}:
            raise QueueFrontendError(
                "ACPY-TYPE-006: array argmin requires key=... and optional where=..."
            )

        aggregate, aggregate_type = self.emit(node.func.value)
        if not isinstance(aggregate_type, ArrayType):
            raise QueueFrontendError(
                f"ACPY-TYPE-006: array {method} receiver must be a value-array"
            )
        callback_count = aggregate_type.length * (
            int("where" in keywords) + int(method == "argmin")
        )
        self._reserve_array_expansion(
            callback_count if callback_count else aggregate_type.length
        )
        true_value: str | None = None
        if "where" not in keywords:
            true_value = self._new()
            self.lines.append(
                f"    %{true_value} = ac.var.constant true as !ac.var<i1>"
            )

        from _pycircuit_semantics import RangeType

        index_type = RangeType(0, aggregate_type.length)
        rendered_index = _render_type(index_type)
        index_width = index_type.bit_width()
        candidates: list[tuple[str, str, str | None, ValueType | None]] = []
        key_type: ValueType | None = None
        for lane in range(aggregate_type.length):
            element, element_type = self._emit_static_array_element(
                aggregate, aggregate_type, lane
            )
            if "where" in keywords:
                valid, valid_type = self._emit_array_callback(
                    keywords["where"],
                    element,
                    element_type,
                    lane,
                    BoolType(),
                )
                if valid_type != BoolType():
                    raise QueueFrontendError(
                        f"ACPY-TYPE-006: array {method} where callback must return bool"
                    )
            else:
                assert true_value is not None
                valid = true_value
            key: str | None = None
            if method == "argmin":
                key, observed_key_type = self._emit_array_callback(
                    keywords["key"],
                    element,
                    element_type,
                    aggregate_type.length + lane,
                    key_type,
                )
                if key_type is None:
                    if not isinstance(observed_key_type, (BitsType, RangeType)):
                        raise QueueFrontendError(
                            "ACPY-TYPE-006: array argmin key must be unsigned bits or range"
                        )
                    key_type = observed_key_type
                elif observed_key_type != key_type:
                    raise QueueFrontendError(
                        "ACPY-TYPE-006: array argmin key types must match"
                    )
            index = self._new()
            self.lines.append(
                f"    %{index} = ac.var.constant {lane} : i{index_width} "
                f"as !ac.var<{rendered_index}>"
            )
            candidates.append((index, valid, key, key_type))

        while len(candidates) > 1:
            following: list[tuple[str, str, str | None, ValueType | None]] = []
            for position in range(0, len(candidates), 2):
                if position + 1 == len(candidates):
                    following.append(candidates[position])
                    continue
                left_index, left_valid, left_key, left_key_type = candidates[position]
                right_index, right_valid, right_key, right_key_type = candidates[
                    position + 1
                ]
                choose_left = left_valid
                selected_key: str | None = None
                if method == "argmin":
                    assert (
                        left_key is not None
                        and right_key is not None
                        and left_key_type is not None
                        and right_key_type == left_key_type
                    )
                    comparison = self._new()
                    comparison_op = (
                        "range_cmp" if isinstance(left_key_type, RangeType) else "cmp"
                    )
                    rendered_key = _render_type(left_key_type)
                    operand_types = (
                        f"!ac.var<{rendered_key}>, !ac.var<{rendered_key}>"
                        if comparison_op == "range_cmp"
                        else f"!ac.var<{rendered_key}>"
                    )
                    self.lines.append(
                        f'    %{comparison} = ac.var.{comparison_op} "ule" '
                        f"%{left_key}, %{right_key} : {operand_types} -> "
                        "!ac.var<i1>"
                    )
                    validity_differs = self._emit_bool_binary(
                        "xor", left_valid, right_valid
                    )
                    choose_left = self._emit_typed_select(
                        validity_differs,
                        left_valid,
                        comparison,
                        BoolType(),
                    )
                    selected_key = self._emit_typed_select(
                        choose_left, left_key, right_key, left_key_type
                    )
                selected_index = self._emit_typed_select(
                    choose_left, left_index, right_index, index_type
                )
                selected_valid = self._emit_bool_binary("or", left_valid, right_valid)
                following.append(
                    (selected_index, selected_valid, selected_key, left_key_type)
                )
            candidates = following

        selected_index, selected_valid, _, _ = candidates[0]
        zero = self._new()
        self.lines.append(
            f"    %{zero} = ac.var.constant 0 : i{index_width} "
            f"as !ac.var<{rendered_index}>"
        )
        selected_index = self._emit_typed_select(
            selected_valid, selected_index, zero, index_type
        )
        return selected_index, index_type, selected_valid, BoolType()

    def _emit_array_scan(self, node: ast.Call) -> tuple[str, ValueType] | None:
        if not (isinstance(node.func, ast.Attribute) and node.func.attr == "scan"):
            return None
        if (
            len(node.args) != 1
            or len(node.keywords) != 1
            or node.keywords[0].arg != "initial"
        ):
            raise QueueFrontendError(
                "ACPY-TYPE-006: array scan requires callback and initial=..."
            )
        aggregate, aggregate_type = self.emit(node.func.value)
        if not isinstance(aggregate_type, ArrayType):
            raise QueueFrontendError(
                "ACPY-TYPE-006: array scan receiver must be a value-array"
            )
        accumulator, accumulator_type = self.emit(node.keywords[0].value)
        callback = node.args[0]

        def lambda_arguments(arguments: ast.arguments) -> tuple[str, str] | None:
            plain = (*arguments.posonlyargs, *arguments.args)
            if (
                len(plain) != 2
                or arguments.vararg is not None
                or arguments.kwarg is not None
                or arguments.kwonlyargs
                or arguments.defaults
                or arguments.kw_defaults
            ):
                return None
            return plain[0].arg, plain[1].arg

        callback_body: ast.expr
        parameter_names: tuple[str, str]
        if isinstance(callback, ast.Lambda):
            parsed = lambda_arguments(callback.args)
            if parsed is None:
                raise QueueFrontendError(
                    "ACPY-TYPE-006: array scan lambda requires two plain parameters"
                )
            parameter_names = parsed
            callback_body = copy.deepcopy(callback.body)
        elif isinstance(callback, ast.Name) and callback.id in self.helpers:
            helper = self.helpers[callback.id]
            if (
                len(helper.arguments) != 2
                or helper.arguments[0][1] != accumulator_type
                or helper.arguments[1][1] != aggregate_type.element
                or helper.result != accumulator_type
            ):
                raise QueueFrontendError(
                    "ACPY-TYPE-006: array scan helper signature must be A, T -> A"
                )
            parameter_names = ("__ac_scan_accumulator", "__ac_scan_element")
            callback_body = ast.copy_location(
                ast.Call(
                    func=copy.deepcopy(callback),
                    args=[
                        ast.Name(id=parameter_names[0], ctx=ast.Load()),
                        ast.Name(id=parameter_names[1], ctx=ast.Load()),
                    ],
                    keywords=[],
                ),
                callback,
            )
        else:
            raise QueueFrontendError(
                "ACPY-TYPE-006: array scan callback must be a lambda or pure helper"
            )

        reserved = {
            candidate.id
            for candidate in ast.walk(callback_body)
            if isinstance(candidate, ast.Name)
        } | {
            candidate.arg
            for candidate in ast.walk(callback_body)
            if isinstance(candidate, ast.arg)
        }
        reserved.update(self.root_values)
        reserved.update(self.deferred_values)
        reserved.update(parameter_names)
        ordinal = self.index
        pair_name = f"__ac_array_scan_pair_{ordinal}"
        while pair_name in reserved:
            ordinal += 1
            pair_name = f"__ac_array_scan_pair_{ordinal}"

        class BindScanParameters(ast.NodeTransformer):
            def __init__(self) -> None:
                self.shadowed: frozenset[str] = frozenset()

            def visit_Lambda(self, candidate: ast.Lambda) -> ast.AST:
                bound = frozenset(
                    argument.arg
                    for argument in (
                        *candidate.args.posonlyargs,
                        *candidate.args.args,
                        *candidate.args.kwonlyargs,
                    )
                )
                previous = self.shadowed
                self.shadowed = previous | bound
                candidate.body = self.visit(candidate.body)
                self.shadowed = previous
                return candidate

            def visit_Name(self, candidate: ast.Name) -> ast.AST:
                if (
                    isinstance(candidate.ctx, ast.Load)
                    and candidate.id in parameter_names
                    and candidate.id not in self.shadowed
                ):
                    index = parameter_names.index(candidate.id)
                    return ast.copy_location(
                        ast.Subscript(
                            value=ast.Name(id=pair_name, ctx=ast.Load()),
                            slice=ast.Constant(index),
                            ctx=ast.Load(),
                        ),
                        candidate,
                    )
                return candidate

        callback_body = BindScanParameters().visit(callback_body)
        assert isinstance(callback_body, ast.expr)
        wrapper = ast.copy_location(
            ast.Lambda(
                args=ast.arguments(
                    posonlyargs=[],
                    args=[ast.arg(arg=pair_name)],
                    kwonlyargs=[],
                    kw_defaults=[],
                    defaults=[],
                ),
                body=callback_body,
            ),
            callback,
        )
        wrapper = ast.fix_missing_locations(wrapper)

        self._reserve_array_expansion(aggregate_type.length)
        outputs: list[tuple[str, ValueType]] = []
        pair_type = TupleType((accumulator_type, aggregate_type.element))
        for lane in range(aggregate_type.length):
            element, element_type = self._emit_static_array_element(
                aggregate, aggregate_type, lane
            )
            pair = self._new()
            self.lines.append(
                f"    %{pair} = ac.var.tuple %{accumulator}, %{element} : "
                f"!ac.var<{_render_type(accumulator_type)}>, "
                f"!ac.var<{_render_type(element_type)}> -> "
                f"!ac.var<{_render_type(pair_type)}>"
            )
            accumulator, observed_type = self._emit_array_callback(
                wrapper,
                pair,
                pair_type,
                lane,
                accumulator_type,
            )
            if observed_type != accumulator_type:
                raise QueueFrontendError(
                    "ACPY-TYPE-006: array scan callback result must match initial"
                )
            outputs.append((accumulator, accumulator_type))
        return self._emit_fixed_array(
            outputs, ArrayType(aggregate_type.length, accumulator_type)
        )

    def _emit_enum_is_one_of(self, node: ast.Call) -> tuple[str, ValueType] | None:
        if not (isinstance(node.func, ast.Attribute) and node.func.attr == "is_one_of"):
            return None
        if node.keywords or not node.args:
            raise QueueFrontendError(
                "ACPY-TYPE-005: enum is_one_of requires one or more members"
            )
        value, value_type = self.emit(node.func.value)
        if not isinstance(value_type, EnumType):
            raise QueueFrontendError(
                "ACPY-TYPE-005: is_one_of receiver must be an enum value"
            )
        seen: set[str] = set()
        matches: list[tuple[str, ValueType]] = []
        for member in node.args:
            explicit_member = self._explicit_enum_member(member, value_type)
            if explicit_member is None:
                raise QueueFrontendError(
                    "ACPY-TYPE-005: is_one_of members must be constants from the same enum"
                )
            if explicit_member[1] in seen:
                raise QueueFrontendError(
                    "ACPY-TYPE-005: is_one_of members must not repeat"
                )
            seen.add(explicit_member[1])
            candidate, candidate_type = self.emit(member)
            if candidate_type != value_type:
                raise QueueFrontendError(
                    "ACPY-TYPE-005: is_one_of members must match the receiver enum"
                )
            matched = self._new()
            self.lines.append(
                f'    %{matched} = ac.var.cmp "eq" %{value}, %{candidate} : '
                f"!ac.var<{_render_type(value_type)}> -> !ac.var<i1>"
            )
            matches.append((matched, BoolType()))
        return self._emit_balanced_array_combine(matches, "or")

    def _emit_array_update(self, node: ast.Call) -> tuple[str, ValueType] | None:
        if not (
            isinstance(node.func, ast.Attribute) and node.func.attr == "with_element"
        ):
            return None
        if node.keywords or len(node.args) != 2:
            raise QueueFrontendError(
                "ACPY-TYPE-006: with_element requires index and replacement"
            )
        aggregate, aggregate_type = self.emit(node.func.value)
        if not isinstance(aggregate_type, ArrayType):
            raise QueueFrontendError(
                "ACPY-TYPE-006: with_element receiver must be a value-array"
            )
        index, index_type = self.emit(node.args[0])
        if _epoch_05_integer_width(index_type) is None:
            raise QueueFrontendError(
                "ACPY-TYPE-006: with_element index must be unsigned"
            )
        self.reject_constant_index_outside(
            index,
            index_type,
            aggregate_type.length,
            "ACPY-TYPE-006: with_element index is out of range",
        )
        replacement, _ = self._emit_exact_array_replacement(
            node.args[1], aggregate_type.element
        )
        result = self._new()
        self.lines.append(
            f"    %{result} = ac.var.with_element %{aggregate} at %{index} "
            f"value %{replacement} : !ac.var<{_render_type(aggregate_type)}>, "
            f"!ac.var<{_render_type(index_type)}>, "
            f"!ac.var<{_render_type(aggregate_type.element)}> -> "
            f"!ac.var<{_render_type(aggregate_type)}>"
        )
        return result, aggregate_type

    def emit_table_index(self, table: str, address: ast.expr) -> tuple[str, ValueType]:
        from _pycircuit_semantics import RangeType

        domain = self.table_domains.get(table)
        if domain is None:
            raise QueueFrontendError("ACPY-TABLE-010: Table domain is unresolved")
        _, entries, shape = domain
        flattened_type = BitsType(max(1, (entries - 1).bit_length()))
        if len(shape) == 1 or not isinstance(address, ast.Tuple):
            index, index_type = self.emit(address, flattened_type)
            bounded = isinstance(index_type, RangeType) and index_type.upper <= entries
            if not bounded and not self._types_match(index_type, flattened_type):
                raise QueueFrontendError(
                    "ACPY-TABLE-010: Table index requires the canonical flattened "
                    f"type {_render_type(flattened_type)}"
                )
            return index, index_type
        if len(address.elts) != len(shape):
            raise QueueFrontendError(
                "ACPY-TABLE-010: Table index rank must match shape"
            )
        coordinates: list[str] = []
        coordinate_types: list[ValueType] = []
        for axis, (coordinate, extent) in enumerate(
            zip(address.elts, shape, strict=True)
        ):
            expected_type = BitsType(_table_axis_width(extent))
            value, value_type = self.emit(coordinate, expected_type)
            bounded = isinstance(value_type, RangeType) and value_type.upper <= extent
            if not bounded and not self._types_match(value_type, expected_type):
                raise QueueFrontendError(
                    f"ACPY-TABLE-010: Table index axis {axis} requires "
                    f"{_render_type(expected_type)}"
                )
            coordinates.append(value)
            coordinate_types.append(value_type)
        flattened = self._new()
        self.lines.append(
            f"    %{flattened} = ac.table.index @{table} ["
            + ", ".join(f"%{coordinate}" for coordinate in coordinates)
            + "] : "
            + ", ".join(
                f"!ac.var<{_render_type(value_type)}>"
                for value_type in coordinate_types
            )
            + f" -> !ac.var<{_render_type(flattened_type)}>"
        )
        return self._remember(flattened, flattened_type)

    def _coerce_bool_to_expected_bits(
        self, value: str, value_type: ValueType, expected: ValueType | None
    ) -> tuple[str, ValueType]:
        if self.strict_descriptors or not (
            _is_epoch_05_bool_compatible(value_type)
            and isinstance(expected, BitsType)
            and expected.width > 1
        ):
            return value, value_type
        zero = self._new()
        one = self._new()
        result = self._new()
        rendered = _render_type(expected)
        self.lines.append(
            f"    %{zero} = ac.var.constant 0 : {rendered} as !ac.var<{rendered}>"
        )
        self.lines.append(
            f"    %{one} = ac.var.constant 1 : {rendered} as !ac.var<{rendered}>"
        )
        self.lines.append(
            f"    %{result} = ac.var.select %{value}, %{one}, %{zero} : "
            f"!ac.var<i1>, !ac.var<{rendered}> -> !ac.var<{rendered}>"
        )
        return self._remember(result, expected, ClosedInterval(0, 1))

    def _bitfield_view(
        self, node: ast.expr
    ) -> tuple[str, BitfieldLayout, ast.expr] | None:
        if not isinstance(node, ast.Call) or len(node.args) != 1 or node.keywords:
            return None
        schema_name: str | None = None
        if isinstance(node.func, ast.Name):
            schema_name = node.func.id
        elif (
            isinstance(node.func, ast.Attribute)
            and node.func.attr == "view"
            and isinstance(node.func.value, ast.Name)
        ):
            schema_name = node.func.value.id
        layout = self.bitfields.get(schema_name or "")
        if layout is None or schema_name is None:
            return None
        return schema_name, layout, node.args[0]

    def _emit_bitfield_field(
        self,
        schema_name: str,
        layout: BitfieldLayout,
        base: str,
        base_type: ValueType,
        field_name: str,
    ) -> tuple[str, ValueType]:
        try:
            msb, lsb = layout.field(field_name)
        except KeyError as exc:
            raise QueueFrontendError(f"ACPY-BITFIELD-002: {exc.args[0]}") from exc
        if not self._types_match(base_type, BitsType(layout.width)):
            raise QueueFrontendError(
                "ACPY-BITFIELD-002: bitfield value width does not match its schema"
            )
        width = msb - lsb + 1
        result_type = BitsType(width)
        name = self._new()
        self.lines.append(
            f"    %{name} = ac.var.extract %{base} from {lsb} width {width} "
            f"{{ac.bitfield_field = {canonical_mlir_string(field_name)}, "
            f"ac.bitfield_fingerprint = {canonical_mlir_string(layout.fingerprint)}, "
            f"ac.bitfield_schema = @types::@{schema_name}}} : "
            f"!ac.var<{_render_type(base_type)}> -> "
            f"!ac.var<{_render_type(result_type)}>"
        )
        return name, result_type

    def emit(
        self, node: ast.expr, expected: ValueType | None = None
    ) -> tuple[str, ValueType]:
        first_line = len(self.lines)
        result = self._emit_node(node, expected)
        self._attach_source_locations(first_line, node)
        return result

    @staticmethod
    def _brace_delta(line: str) -> int:
        opened = 0
        quoted = False
        escaped = False
        for character in line:
            if escaped:
                escaped = False
                continue
            if character == "\\" and quoted:
                escaped = True
                continue
            if character == '"':
                quoted = not quoted
                continue
            if quoted:
                continue
            if character == "{":
                opened += 1
            elif character == "}":
                opened -= 1
        return opened

    def _attach_source_locations(self, first_line: int, node: ast.AST) -> None:
        frame = source_frame(node)
        if frame is None or first_line >= len(self.lines):
            return
        location = _render_source_frame_location(frame)
        cursor = first_line
        while cursor < len(self.lines):
            line = self.lines[cursor]
            if re.match(r"^\s*(?:%[^=]+\s*=\s*)?(?:ac\.|func\.)", line) is None:
                cursor += 1
                continue
            end = cursor
            balance = self._brace_delta(line)
            while balance > 0 and end + 1 < len(self.lines):
                end += 1
                balance += self._brace_delta(self.lines[end])
            if " loc(" not in self.lines[end]:
                self.lines[end] += location
            cursor = end + 1

    def _emit_node(
        self, node: ast.expr, expected: ValueType | None = None
    ) -> tuple[str, ValueType]:
        if isinstance(node, ast.Call):
            record_projection = self._emit_record_projection(node)
            if record_projection is not None:
                return record_projection
            enum_match = self._emit_enum_match(node, expected)
            if enum_match is not None:
                return enum_match
            array_selection = self._emit_array_selection(node)
            if array_selection is not None:
                index, index_type, valid, valid_type = array_selection
                key = ast.dump(node, include_attributes=False)
                self.array_selection_values[key] = array_selection
                result_type = TupleType((index_type, valid_type))
                result = self._new()
                self.lines.append(
                    f"    %{result} = ac.var.tuple %{index}, %{valid} : "
                    f"!ac.var<{_render_type(index_type)}>, !ac.var<i1> -> "
                    f"!ac.var<{_render_type(result_type)}>"
                )
                return result, result_type
            scanned_array = self._emit_array_scan(node)
            if scanned_array is not None:
                return scanned_array
            enum_membership = self._emit_enum_is_one_of(node)
            if enum_membership is not None:
                return enum_membership
            mapped_array = self._emit_array_map(node)
            if mapped_array is not None:
                return mapped_array
            zipped_array = self._emit_array_zip(node)
            if zipped_array is not None:
                return zipped_array
            folded_array = self._emit_array_fold(node)
            if folded_array is not None:
                return folded_array
            reduced_bool_array = self._emit_array_bool_reduction(node)
            if reduced_bool_array is not None:
                return reduced_bool_array
            updated_array = self._emit_array_update(node)
            if updated_array is not None:
                return updated_array
            typed_integer = self._emit_typed_integer_intrinsic(node)
            if typed_integer is not None:
                return typed_integer
            bounded = self._emit_range_intrinsic(node)
            if bounded is not None:
                return bounded
            if _decorator_name(node.func).rsplit(".", 1)[-1] == "checked":
                raise QueueFrontendError(
                    "ACPY-TYPE-005: checked result requires .value or .valid"
                )
            if _decorator_name(node.func).rsplit(".", 1)[-1] == "onehot_enum":
                raise QueueFrontendError(
                    "ACPY-TYPE-005: onehot_enum result requires "
                    ".value, .present, or .conflict"
                )
            lexical_bindings = {
                self.argument,
                *self.root_values,
                *self.deferred_values,
                *self.table_views,
                *self.slot_views,
                *self.candidates,
                *self.selections,
                *self.candidate_values,
                *self.selection_values,
                *self.find_values,
                *self.state_views,
                *self.table_domains,
            }
            helper = (
                self.helpers.get(node.func.id)
                if isinstance(node.func, ast.Name)
                and node.func.id not in lexical_bindings
                else None
            )
            if helper is not None:
                if node.keywords or len(node.args) != len(helper.arguments):
                    raise QueueFrontendError(
                        f"ACPY-HELPER-002: helper {helper.name!r} call arity does not match"
                    )
                operands: list[str] = []
                operand_types: list[ValueType] = []
                for argument, (_, parameter_type) in zip(
                    node.args, helper.arguments, strict=True
                ):
                    operand, operand_type = self.emit(argument, parameter_type)
                    if not self._types_match(operand_type, parameter_type):
                        raise QueueFrontendError(
                            f"ACPY-HELPER-002: helper {helper.name!r} argument type does not match"
                        )
                    operands.append(operand)
                    operand_types.append(operand_type)
                if self.inline_pure_helpers:
                    if helper.name in self.active_inline_helpers:
                        raise QueueFrontendError(
                            f"ACPY-HELPER-002: helper {helper.name!r} is recursive"
                        )
                    saved_roots = {
                        name: self.root_values.get(name) for name, _ in helper.arguments
                    }
                    self.root_values.update(
                        {
                            name: (operand, operand_type)
                            for (name, _), operand, operand_type in zip(
                                helper.arguments,
                                operands,
                                operand_types,
                                strict=True,
                            )
                        }
                    )
                    self.active_inline_helpers.add(helper.name)
                    try:
                        result, result_type = self.emit(
                            helper.expression, helper.result
                        )
                    finally:
                        self.active_inline_helpers.remove(helper.name)
                        for name, previous in saved_roots.items():
                            if previous is None:
                                self.root_values.pop(name, None)
                            else:
                                self.root_values[name] = previous
                    if not self._types_match(result_type, helper.result):
                        raise QueueFrontendError(
                            f"ACPY-HELPER-002: helper {helper.name!r} "
                            "result type does not match"
                        )
                    return self._remember(result, helper.result)
                result = self._new()
                self.lines.append(
                    f"    %{result} = func.call @{helper.name}("
                    + ", ".join(f"%{operand}" for operand in operands)
                    + ") : ("
                    + ", ".join(
                        f"!ac.var<{_render_type(value_type)}>"
                        for value_type in operand_types
                    )
                    + f") -> !ac.var<{_render_type(helper.result)}>"
                )
                return self._remember(result, helper.result)
            invariant = _resolve_invariant_call(node, self.invariants, lexical_bindings)
            if invariant is not None:
                if len(node.args) != 1 or node.keywords:
                    raise QueueFrontendError(
                        f"ACPY-INVARIANT-003: invariant "
                        f"{invariant.qualified_name} requires exactly one payload"
                    )
                operand, operand_type = self.emit(node.args[0], invariant.payload)
                if not self._types_match(operand_type, invariant.payload):
                    raise QueueFrontendError(
                        f"ACPY-INVARIANT-003: invariant "
                        f"{invariant.qualified_name} requires payload "
                        f"{invariant.payload.name}, got {_render_type(operand_type)}"
                    )
                predicate_prefix = f"{self.prefix}invariant{self.index}_"
                predicate_argument = f"{predicate_prefix}value"
                predicate_emitter = _ExpressionEmitter(
                    self.payloads,
                    invariant.argument,
                    invariant.payload,
                    root_name=predicate_argument,
                    prefix=predicate_prefix,
                    enum_types=self.enum_types,
                    bitfields=self.bitfields,
                    invariants=self.invariants,
                    helpers=self.helpers,
                )
                predicate, predicate_type = predicate_emitter.emit(
                    invariant.expression, BoolType()
                )
                if not _is_epoch_05_bool_compatible(predicate_type):
                    raise QueueFrontendError(
                        f"ACPY-INVARIANT-002: invariant "
                        f"{invariant.qualified_name} predicate must produce bool"
                    )
                result = self._new()
                rendered_payload = _render_type(invariant.payload)
                self.lines.append(
                    f"    %{result} = ac.var.invariant %{operand} name "
                    f"{canonical_mlir_string(invariant.qualified_name)} {{"
                )
                self.lines.append(
                    f"    ^predicate(%{predicate_argument}: "
                    f"!ac.var<{rendered_payload}>):"
                )
                self.lines.extend(predicate_emitter.lines)
                self.lines.append(
                    f"      ac.var.invariant.yield %{predicate} : !ac.var<i1>"
                )
                self.lines.append(
                    f"    }} : !ac.var<{rendered_payload}> -> !ac.var<i1>"
                )
                return self._remember(result, BoolType())
        if isinstance(node, ast.IfExp):
            condition, condition_type = self.emit(node.test, BoolType())
            if not _is_epoch_05_bool_compatible(condition_type):
                raise QueueFrontendError(
                    "ACPY-QUEUE-003: conditional expression requires bool"
                )
            true_value, true_type = self.emit(node.body, expected)
            false_value, false_type = self.emit(node.orelse, expected or true_type)
            if not self._types_match(true_type, false_type):
                raise QueueFrontendError(
                    "ACPY-QUEUE-003: conditional expression branches must "
                    "have one exact type"
                )
            name = self._new()
            self.lines.append(
                f"    %{name} = ac.var.select %{condition}, %{true_value}, "
                f"%{false_value} : !ac.var<i1>, "
                f"!ac.var<{_render_type(true_type)}> -> "
                f"!ac.var<{_render_type(true_type)}>"
            )
            return name, true_type
        if (
            isinstance(node, ast.Attribute)
            and isinstance(node.value, ast.Name)
            and self._unshadowed_enum(node.value.id) is not None
        ):
            enumeration = self._unshadowed_enum(node.value.id)
            assert enumeration is not None
            if node.attr not in enumeration.enumerants:
                raise QueueFrontendError(
                    f"ACPY-TYPE-005: unknown enumerant {node.value.id}.{node.attr}"
                )
            name = self._new()
            self.lines.append(
                f"    %{name} = ac.var.enum @types::@{enumeration.name} "
                f"{canonical_mlir_string(node.attr)} : "
                f"!ac.var<{_render_type(enumeration)}>"
            )
            return self._remember(name, enumeration, Constant(node.attr))
        if isinstance(node, (ast.Tuple, ast.List)):
            aggregate = expected
            if not isinstance(aggregate, (TupleType, ArrayType)):
                raise QueueFrontendError(
                    "ACPY-TYPE-006: aggregate literal requires a tuple or value-array context"
                )
            if isinstance(aggregate, TupleType):
                element_types = aggregate.elements
                operation = "tuple"
            elif isinstance(aggregate, ArrayType):
                if len(node.elts) != aggregate.length:
                    raise QueueFrontendError(
                        "ACPY-TYPE-006: aggregate literal arity must match its type"
                    )
                element_types = (aggregate.element,) * aggregate.length
                operation = "array"
            else:
                raise AssertionError("unreachable aggregate descriptor")
            if len(node.elts) != len(element_types):
                raise QueueFrontendError(
                    "ACPY-TYPE-006: aggregate literal arity must match its type"
                )
            values: list[str] = []
            value_types: list[ValueType] = []
            for element, descriptor in zip(node.elts, element_types, strict=True):
                value, value_type = self.emit(element, descriptor)
                if not self._types_match(value_type, descriptor):
                    raise QueueFrontendError(
                        "ACPY-TYPE-006: aggregate element type mismatch"
                    )
                values.append(value)
                value_types.append(value_type)
            name = self._new()
            self.lines.append(
                f"    %{name} = ac.var.{operation} "
                + ", ".join(f"%{value}" for value in values)
                + " : "
                + ", ".join(
                    f"!ac.var<{_render_type(value_type)}>" for value_type in value_types
                )
                + f" -> !ac.var<{_render_type(aggregate)}>"
            )
            return name, aggregate
        if isinstance(node, ast.Subscript):
            view = self._bitfield_view(node.value)
            if view is not None:
                schema_name, layout, base_node = view
                keys = (
                    tuple(node.slice.elts)
                    if isinstance(node.slice, ast.Tuple)
                    else (node.slice,)
                )
                if not keys or not all(
                    isinstance(key, ast.Constant) and type(key.value) is str
                    for key in keys
                ):
                    raise QueueFrontendError(
                        "ACPY-BITFIELD-002: bitfield selection requires static field names"
                    )
                base, base_type = self.emit(base_node)
                selected = [
                    self._emit_bitfield_field(
                        schema_name, layout, base, base_type, key.value
                    )
                    for key in keys
                    if isinstance(key, ast.Constant) and type(key.value) is str
                ]
                if len(selected) == 1:
                    return selected[0]
                result_width = sum(value_type.bit_width() for _, value_type in selected)
                if result_width > 64:
                    raise QueueFrontendError(
                        "ACPY-BITFIELD-002: selected bitfield width must be in [1, 64]"
                    )
                name = self._new()
                field_names = [
                    key.value
                    for key in keys
                    if isinstance(key, ast.Constant) and type(key.value) is str
                ]
                self.lines.append(
                    f"    %{name} = ac.var.concat "
                    + ", ".join(f"%{value}" for value, _ in selected)
                    + " {ac.bitfield_fields = "
                    + "["
                    + ", ".join(canonical_mlir_string(item) for item in field_names)
                    + "]"
                    + ", ac.bitfield_fingerprint = "
                    + canonical_mlir_string(layout.fingerprint)
                    + ", ac.bitfield_schema = @types::@"
                    + schema_name
                    + "} : "
                    + ", ".join(
                        f"!ac.var<{_render_type(value_type)}>"
                        for _, value_type in selected
                    )
                    + f" -> !ac.var<i{result_width}>"
                )
                return name, BitsType(result_width)
        if isinstance(node, ast.Attribute) and node.attr in {"value", "valid"}:
            checked_call = (
                node.value
                if isinstance(node.value, ast.Call)
                else (
                    self.deferred_values.get(node.value.id)
                    if isinstance(node.value, ast.Name)
                    else None
                )
            )
            if (
                isinstance(checked_call, ast.Call)
                and _decorator_name(checked_call.func).rsplit(".", 1)[-1] == "checked"
            ):
                enum_result = self._emit_checked_enum(checked_call)
                if enum_result is None:
                    value, value_type, valid, valid_type = self._emit_checked_range(
                        checked_call
                    )
                else:
                    value, value_type, valid, valid_type = enum_result
                return (
                    (value, value_type) if node.attr == "value" else (valid, valid_type)
                )
        if isinstance(node, ast.Attribute) and node.attr in {
            "value",
            "present",
            "conflict",
        }:
            onehot_call = (
                node.value
                if isinstance(node.value, ast.Call)
                else (
                    self.deferred_values.get(node.value.id)
                    if isinstance(node.value, ast.Name)
                    else None
                )
            )
            if (
                isinstance(onehot_call, ast.Call)
                and _decorator_name(onehot_call.func).rsplit(".", 1)[-1]
                == "onehot_enum"
            ):
                emitted = self._emit_onehot_enum(onehot_call)
                assert emitted is not None
                value, value_type, present, present_type, conflict, conflict_type = (
                    emitted
                )
                if node.attr == "value":
                    return value, value_type
                if node.attr == "present":
                    return present, present_type
                return conflict, conflict_type
        if isinstance(node, ast.Attribute):
            view = self._bitfield_view(node.value)
            if view is not None:
                schema_name, layout, base_node = view
                base, base_type = self.emit(base_node)
                return self._emit_bitfield_field(
                    schema_name, layout, base, base_type, node.attr
                )
        if (
            isinstance(node, ast.Call)
            and isinstance(node.func, ast.Attribute)
            and node.func.attr == "update"
            and isinstance(node.func.value, ast.Name)
            and node.func.value.id in self.bitfields
        ):
            if len(node.args) != 1 or any(
                keyword.arg is None for keyword in node.keywords
            ):
                raise QueueFrontendError(
                    "ACPY-BITFIELD-003: bitfield update requires one value and named fields"
                )
            schema_name = node.func.value.id
            layout = self.bitfields[schema_name]
            base, base_type = self.emit(node.args[0])
            if not self._types_match(base_type, BitsType(layout.width)):
                raise QueueFrontendError(
                    "ACPY-BITFIELD-003: bitfield value width does not match its schema"
                )
            values = {
                keyword.arg: keyword.value
                for keyword in node.keywords
                if keyword.arg is not None
            }
            try:
                writes = layout.checked_writes(values)
            except (ValueError, KeyError) as exc:
                message = exc.args[0] if exc.args else str(exc)
                raise QueueFrontendError(f"ACPY-BITFIELD-003: {message}") from exc
            current = base
            for lsb, msb, field_name in writes:
                width = msb - lsb + 1
                field_type = BitsType(width)
                value, value_type = self.emit(values[field_name], field_type)
                if not self._types_match(value_type, field_type):
                    raise QueueFrontendError(
                        f"ACPY-BITFIELD-003: field {field_name!r} requires i{width}"
                    )
                name = self._new()
                self.lines.append(
                    f"    %{name} = ac.var.insert %{current}, %{value} at {lsb} "
                    f"{{ac.bitfield_field = {canonical_mlir_string(field_name)}, "
                    f"ac.bitfield_fingerprint = {canonical_mlir_string(layout.fingerprint)}, "
                    f"ac.bitfield_schema = @types::@{schema_name}}} : "
                    f"!ac.var<{_render_type(base_type)}>, "
                    f"!ac.var<{_render_type(value_type)}> -> "
                    f"!ac.var<{_render_type(base_type)}>"
                )
                current = name
            return current, base_type
        if (
            isinstance(node, ast.Subscript)
            and isinstance(node.value, ast.Name)
            and node.value.id in self.state_views
        ):
            owner_kind, variable, value_type, entries, _ = self.state_views[
                node.value.id
            ]
            cache_key = (
                variable,
                ast.dump(node.slice, include_attributes=False),
            )
            if cached := self.state_read_values.get(cache_key):
                return cached
            if owner_kind == "table":
                index, index_type = self.emit_table_index(variable, node.slice)
            else:
                index, index_type = self.emit(node.slice)
            index_width = _epoch_05_integer_width(index_type)
            if index_width is None:
                raise QueueFrontendError(
                    "ACPY-RULE-009: persistent find capture index must be integer"
                )
            self.reject_constant_index_outside(
                index,
                index_type,
                entries,
                "ACPY-RULE-009: persistent find capture index is out of range",
            )
            name = self._new()
            operation = "ac.var.read_element" if owner_kind == "var" else "ac.table.get"
            self.lines.append(
                f"    %{name} = {operation} @{variable}[%{index}] : "
                f"!ac.var<{_render_type(index_type)}> -> "
                f"!ac.var<{_render_type(value_type)}>"
            )
            self.state_read_values[cache_key] = (name, value_type)
            return self.state_read_values[cache_key]
        if isinstance(node, ast.Subscript):
            value, value_type = self.emit(node.value)
            if isinstance(value_type, (TupleType, ArrayType)):
                aggregate = value_type
                index = _constant_integer(node.slice)
                if index is None and isinstance(aggregate, TupleType):
                    raise QueueFrontendError(
                        "ACPY-TYPE-006: tuple index must be a static integer"
                    )
                if index is None and isinstance(aggregate, ArrayType):
                    dynamic_index, dynamic_type = self.emit(node.slice)
                    if _epoch_05_integer_width(dynamic_type) is None:
                        raise QueueFrontendError(
                            "ACPY-TYPE-006: value-array index must be unsigned"
                        )
                    self.reject_constant_index_outside(
                        dynamic_index,
                        dynamic_type,
                        aggregate.length,
                        "ACPY-TYPE-006: value-array index is out of range",
                    )
                    name = self._new()
                    self.lines.append(
                        f"    %{name} = ac.var.dynamic_element %{value} at "
                        f"%{dynamic_index} : !ac.var<{_render_type(value_type)}>, "
                        f"!ac.var<{_render_type(dynamic_type)}> -> "
                        f"!ac.var<{_render_type(aggregate.element)}>"
                    )
                    return name, aggregate.element
                assert index is not None
                if isinstance(aggregate, TupleType):
                    if not _proven_integer_in(index, 0, len(aggregate.elements) - 1):
                        raise QueueFrontendError(
                            "ACPY-TYPE-006: tuple index is out of range"
                        )
                    result_type = aggregate.elements[index]
                elif isinstance(aggregate, ArrayType):
                    if not _proven_integer_in(index, 0, aggregate.length - 1):
                        raise QueueFrontendError(
                            "ACPY-TYPE-006: value-array index is out of range"
                        )
                    result_type = aggregate.element
                else:
                    raise AssertionError("unreachable aggregate descriptor")
                name = self._new()
                self.lines.append(
                    f"    %{name} = ac.var.element %{value} at {index} : "
                    f"!ac.var<{_render_type(value_type)}> -> "
                    f"!ac.var<{_render_type(result_type)}>"
                )
                return name, result_type
            source_width = _epoch_05_integer_width(value_type)
            if source_width is None:
                raise QueueFrontendError(
                    "ACPY-BITS-001: bit extraction requires a bits value"
                )
            if isinstance(node.slice, ast.Slice):
                if node.slice.step is not None:
                    raise QueueFrontendError(
                        "ACPY-BITS-001: bit slice step is not supported"
                    )
                lower = node.slice.lower
                upper = node.slice.upper
                lsb = _constant_integer(lower) if lower is not None else None
                end = _constant_integer(upper) if upper is not None else None
                if lsb is None or end is None:
                    raise QueueFrontendError(
                        "ACPY-BITS-001: bit slice bounds must be static integers"
                    )
            elif (index := _constant_integer(node.slice)) is not None:
                lsb = index
                end = lsb + 1
            else:
                raise QueueFrontendError(
                    "ACPY-BITS-001: bit index must be a static integer"
                )
            if (
                end <= lsb
                or not _proven_integer_in(lsb, 0, source_width - 1)
                or not _proven_integer_in(end, 1, source_width)
            ):
                raise QueueFrontendError(
                    "ACPY-BITS-001: bit slice is empty or out of range"
                )
            result_width = end - lsb
            result_type = BitsType(result_width)
            name = self._new()
            self.lines.append(
                f"    %{name} = ac.var.extract %{value} from {lsb} width "
                f"{result_width} : !ac.var<{_render_type(value_type)}> -> "
                f"!ac.var<{_render_type(result_type)}>"
            )
            return name, result_type
        if isinstance(node, ast.Name) and node.id in self.deferred_values:
            return self.emit(self.deferred_values[node.id], expected)
        if isinstance(node, ast.Name) and node.id in self.root_values:
            return self.root_values[node.id]
        if isinstance(node, ast.Name) and node.id == self.argument:
            return self.root_name, self.payload
        if isinstance(node, ast.Name) and node.id in self.candidate_values:
            return self.candidate_values[node.id]
        if isinstance(node, ast.Name) and node.id in self.candidates:
            candidate = self.candidates[node.id]
            domain = self.table_domains.get(candidate.table)
            if domain is None:
                raise QueueFrontendError(
                    "ACPY-TABLE-008: CandidateSet domain is unresolved"
                )
            entry_type, _, _ = domain
            mask_width = candidate.entries
            predicate_emitter = _ExpressionEmitter(
                self.payloads,
                candidate.argument,
                entry_type,
                root_name="entry",
                prefix=f"{self.prefix}m{self.index}_",
                slot_views=self.slot_views,
                enum_types=self.enum_types,
                bitfields=self.bitfields,
                invariants=self.invariants,
                helpers=self.helpers,
            )
            predicate, predicate_type = predicate_emitter.emit(
                candidate.predicate, BoolType()
            )
            if not _is_epoch_05_bool_compatible(predicate_type):
                raise QueueFrontendError(
                    "ACPY-TABLE-006: match predicate must lower to i1"
                )
            mask = self._new()
            self.lines.append(
                f"    %{mask} = ac.table.match @{candidate.table} predicate {{"
            )
            self.lines.append(
                f"    ^predicate(%entry: !ac.var<{_render_type(entry_type)}>):"
            )
            self.lines.extend(predicate_emitter.lines)
            self.lines.append(f"      ac.table.match.yield %{predicate} : !ac.var<i1>")
            self.lines.append(
                "    } "
                + _render_table_domain_attributes(candidate)
                + " "
                + f"-> !ac.var<i{mask_width}>"
            )
            return mask, BitsType(mask_width)
        if (
            isinstance(node, ast.Attribute)
            and isinstance(node.value, ast.Name)
            and node.value.id in self.find_values
            and node.attr in {"index", "valid", "value"}
        ):
            (
                index,
                index_type,
                valid,
                valid_type,
                variable,
                value_type,
                value,
                owner_kind,
            ) = self.find_values[node.value.id]
            if node.attr == "index":
                return index, index_type
            if node.attr == "valid":
                return valid, valid_type
            if value is None:
                value = self._new()
                operation = (
                    "ac.var.read_element" if owner_kind == "var" else "ac.table.get"
                )
                self.lines.append(
                    f"    %{value} = {operation} @{variable}[%{index}] : "
                    f"!ac.var<{_render_type(index_type)}> -> "
                    f"!ac.var<{_render_type(value_type)}>"
                )
                self.find_values[node.value.id] = (
                    index,
                    index_type,
                    valid,
                    valid_type,
                    variable,
                    value_type,
                    value,
                    owner_kind,
                )
            return value, value_type
        if isinstance(node, ast.Name) and node.id in self.table_views:
            if node.id in self.table_view_values:
                return self.table_view_values[node.id]
            table, address, entry_type = self.table_views[node.id]
            index, index_type = self.emit_table_index(table, address)
            name = self._new()
            self.lines.append(
                f"    %{name} = ac.table.get @{table} [%{index}] : "
                f"!ac.var<{_render_type(index_type)}> -> "
                f"!ac.var<{_render_type(entry_type)}>"
            )
            self.table_view_values[node.id] = (name, entry_type)
            return self.table_view_values[node.id]
        if (
            isinstance(node, ast.Attribute)
            and isinstance(node.value, ast.Name)
            and node.value.id in self.selection_values
            and node.attr in {"index", "valid"}
        ):
            index, index_type, valid, valid_type = self.selection_values[node.value.id]
            return (index, index_type) if node.attr == "index" else (valid, valid_type)
        if (
            isinstance(node, ast.Attribute)
            and isinstance(node.value, ast.Name)
            and node.value.id in self.slot_views
            and node.attr in {"valid", "value"}
        ):
            slot, payload = self.slot_views[node.value.id]
            valid = self._new()
            value = self._new()
            self.lines.append(
                f"    %{valid}, %{value} = ac.slot.get @{slot} : "
                f"!ac.var<i1>, !ac.var<{_render_type(payload)}>"
            )
            return (valid, BoolType()) if node.attr == "valid" else (value, payload)
        if (
            isinstance(node, ast.Attribute)
            and isinstance(node.value, ast.Name)
            and node.value.id in self.selections
            and node.attr in {"index", "valid"}
        ):
            selection = self.selections[node.value.id]
            lane = selection.aliases.index(node.value.id)
            if cached := self.selection_batch_values.get(selection.name):
                index, index_type, valid, valid_type = cached[lane]
                return (
                    (index, index_type) if node.attr == "index" else (valid, valid_type)
                )
            candidate = self.candidates[selection.candidates]
            domain = self.table_domains.get(selection.table)
            if domain is None:
                raise QueueFrontendError(
                    "ACPY-TABLE-007: selection domain is unresolved"
                )
            entry_type, table_entries, _ = domain
            mask_width = candidate.entries
            predicate_emitter = _ExpressionEmitter(
                self.payloads,
                candidate.argument,
                entry_type,
                root_name="entry",
                prefix=f"{self.prefix}m{self.index}_",
                slot_views=self.slot_views,
                enum_types=self.enum_types,
                bitfields=self.bitfields,
                invariants=self.invariants,
                helpers=self.helpers,
            )
            predicate, predicate_type = predicate_emitter.emit(
                candidate.predicate, BoolType()
            )
            if not _is_epoch_05_bool_compatible(predicate_type):
                raise QueueFrontendError(
                    "ACPY-TABLE-006: match predicate must lower to i1"
                )
            mask = self._new()
            self.lines.append(
                f"    %{mask} = ac.table.match @{selection.table} predicate {{"
            )
            self.lines.append(
                f"    ^predicate(%entry: !ac.var<{_render_type(entry_type)}>):"
            )
            self.lines.extend(predicate_emitter.lines)
            self.lines.append(f"      ac.table.match.yield %{predicate} : !ac.var<i1>")
            self.lines.append(
                "    } "
                + _render_table_domain_attributes(candidate)
                + " "
                + f"-> !ac.var<i{mask_width}>"
            )
            indices = [self._new() for _ in range(selection.count)]
            valids = [self._new() for _ in range(selection.count)]
            index_width = max(1, (table_entries - 1).bit_length())
            if selection.policy in {"first", "round_robin"}:
                key_region = "{}"
            else:
                assert selection.argument is not None and selection.key is not None
                key_emitter = _ExpressionEmitter(
                    self.payloads,
                    selection.argument,
                    entry_type,
                    root_name="entry",
                    prefix=f"{self.prefix}k{self.index}_",
                    enum_types=self.enum_types,
                    bitfields=self.bitfields,
                    invariants=self.invariants,
                )
                key, key_type = key_emitter.emit(selection.key)
                if _epoch_05_integer_width(key_type) is None:
                    raise QueueFrontendError(
                        "ACPY-TABLE-007: choose key must lower to an integer"
                    )
                key_lines = [
                    "{",
                    f"    ^key(%entry: !ac.var<{_render_type(entry_type)}>):",
                    *key_emitter.lines,
                    f"      ac.table.choose.yield %{key} : "
                    f"!ac.var<{_render_type(key_type)}>",
                    "    }",
                ]
                key_region = "\n".join(key_lines)
            lhs = ", ".join(f"%{value}" for value in (*indices, *valids))
            result_types = ", ".join(
                [f"!ac.var<i{index_width}>"] * selection.count
                + ["!ac.var<i1>"] * selection.count
            )
            key_order = (
                ""
                if selection.key_ordering is None
                else " key_order #ac<table_key_ordering " + selection.key_ordering + ">"
            )
            cursor = (
                ""
                if selection.initial_cursor == 0
                else f" initial_cursor {selection.initial_cursor}"
            )
            self.lines.append(
                f"    {lhs} = ac.table.choose @{selection.table} %{mask} : "
                f"!ac.var<i{mask_width}> count {selection.count} policy "
                f"#ac<table_selection_policy {selection.policy}>{key_order} "
                f"stable_id {canonical_mlir_string(selection.stable_id)}{cursor} "
                f"key {key_region} -> {result_types}"
            )
            batch = tuple(
                (index, BitsType(index_width), valid, BoolType())
                for index, valid in zip(indices, valids, strict=True)
            )
            self.selection_batch_values[selection.name] = batch
            index, index_type, valid, valid_type = batch[lane]
            return (index, index_type) if node.attr == "index" else (valid, valid_type)
        if isinstance(node, ast.Constant) and type(node.value) in {int, bool}:
            from _pycircuit_semantics import RangeType

            typ = (
                BoolType()
                if self.strict_descriptors and type(node.value) is bool
                else (
                    expected
                    if self.strict_descriptors
                    and type(node.value) is int
                    and isinstance(expected, (BitsType, RangeType))
                    else (
                        expected
                        if not self.strict_descriptors and expected is not None
                        else BoolType()
                        if type(node.value) is bool
                        else BitsType(64)
                    )
                )
            )
            if isinstance(typ, RangeType) and (
                type(node.value) is not int or not typ.lower <= node.value < typ.upper
            ):
                raise QueueFrontendError(
                    "ACPY-TYPE-009: range constant is outside its declared bounds"
                )
            name = self._new()
            value = (
                "true"
                if node.value is True
                else "false"
                if node.value is False
                else str(node.value)
            )
            attribute_type = (
                f"i{typ.width}" if isinstance(typ, RangeType) else _render_type(typ)
            )
            attribute = (
                value if type(node.value) is bool else f"{value} : {attribute_type}"
            )
            self.lines.append(
                f"    %{name} = ac.var.constant {attribute} as "
                f"!ac.var<{_render_type(typ)}>"
            )
            return self._remember(name, typ, Constant(node.value))
        if isinstance(node, ast.Attribute) and node.attr in {"index", "valid"}:
            if isinstance(node.value, ast.Name):
                captured = self.root_values.get(node.value.id)
                if captured is not None:
                    aggregate, aggregate_type = captured
                    from _pycircuit_semantics import RangeType

                    if (
                        isinstance(aggregate_type, TupleType)
                        and len(aggregate_type.elements) == 2
                        and isinstance(aggregate_type.elements[0], RangeType)
                        and aggregate_type.elements[1] == BoolType()
                    ):
                        index = 0 if node.attr == "index" else 1
                        result_type = aggregate_type.elements[index]
                        result = self._new()
                        self.lines.append(
                            f"    %{result} = ac.var.element %{aggregate} at {index} : "
                            f"!ac.var<{_render_type(aggregate_type)}> -> "
                            f"!ac.var<{_render_type(result_type)}>"
                        )
                        return result, result_type
            selection_call = (
                node.value
                if isinstance(node.value, ast.Call)
                else (
                    self.deferred_values.get(node.value.id)
                    if isinstance(node.value, ast.Name)
                    else None
                )
            )
            if (
                isinstance(selection_call, ast.Call)
                and isinstance(selection_call.func, ast.Attribute)
                and selection_call.func.attr in {"first", "argmin"}
            ):
                key = ast.dump(selection_call, include_attributes=False)
                selection = self.array_selection_values.get(key)
                if selection is None:
                    selection = self._emit_array_selection(selection_call)
                    assert selection is not None
                    self.array_selection_values[key] = selection
                index, index_type, valid, valid_type = selection
                return (
                    (index, index_type) if node.attr == "index" else (valid, valid_type)
                )
        if (
            isinstance(node, ast.Attribute)
            and node.attr in {"index", "valid"}
            and isinstance(node.value, ast.Call)
            and _decorator_name(node.value.func).rsplit(".", 1)[-1] == "priority_encode"
        ):
            call = node.value
            if len(call.args) != 1 or any(
                keyword.arg != "order" for keyword in call.keywords
            ):
                raise QueueFrontendError(
                    "ACPY-VAR-003",
                    "priority_encode requires one value and optional order",
                )
            order = "low"
            if call.keywords:
                raw_order = call.keywords[0].value
                if (
                    not isinstance(raw_order, ast.Constant)
                    or type(raw_order.value) is not str
                ):
                    raise QueueFrontendError(
                        "ACPY-VAR-003", "priority_encode order must be static"
                    )
                order = raw_order.value.strip().lower()
            if order not in {"low", "high"}:
                raise QueueFrontendError(
                    "ACPY-VAR-003", "priority_encode order must be low or high"
                )
            key = ast.dump(call, include_attributes=False)
            cached = self.priority_values.get(key)
            if cached is None:
                value, value_type = self.emit(call.args[0])
                width = _primitive_integer_width("priority_encode", value_type)
                index_type = BitsType(primitive_priority_index_width(width))
                index = self._new()
                valid = self._new()
                self.lines.append(
                    f"    %{index}, %{valid} = ac.var.priority_encode %{value} "
                    f'order "{order}" : !ac.var<{_render_type(value_type)}> -> '
                    f"!ac.var<{_render_type(index_type)}>, !ac.var<i1>"
                )
                cached = (index, index_type, valid, BoolType())
                self.priority_values[key] = cached
            index, index_type, valid, valid_type = cached
            return (
                (index, index_type)
                if node.attr == "index"
                else (
                    valid,
                    valid_type,
                )
            )
        if (
            isinstance(node, ast.Attribute)
            and node.attr in {"index", "valid", "conflict"}
            and isinstance(node.value, ast.Call)
            and _decorator_name(node.value.func).rsplit(".", 1)[-1] == "onehot_encode"
        ):
            call = node.value
            if len(call.args) != 1 or any(
                keyword.arg != "order" for keyword in call.keywords
            ):
                raise QueueFrontendError(
                    "ACPY-VAR-003",
                    "onehot_encode requires one value and optional order",
                )
            order = "low"
            if call.keywords:
                raw_order = call.keywords[0].value
                if (
                    not isinstance(raw_order, ast.Constant)
                    or type(raw_order.value) is not str
                ):
                    raise QueueFrontendError(
                        "ACPY-VAR-003", "onehot_encode order must be static"
                    )
                order = raw_order.value.strip().lower()
            if order not in {"low", "high"}:
                raise QueueFrontendError(
                    "ACPY-VAR-003", "onehot_encode order must be low or high"
                )
            key = ast.dump(call, include_attributes=False)
            cached = self.onehot_values.get(key)
            if cached is None:
                value, value_type = self.emit(call.args[0])
                width = _primitive_integer_width("onehot_encode", value_type)
                index_type = BitsType(primitive_priority_index_width(width))
                index = self._new()
                valid = self._new()
                self.lines.append(
                    f"    %{index}, %{valid} = ac.var.priority_encode %{value} "
                    f'order "{order}" : !ac.var<{_render_type(value_type)}> -> '
                    f"!ac.var<{_render_type(index_type)}>, !ac.var<i1>"
                )
                cached = (
                    index,
                    index_type,
                    valid,
                    BoolType(),
                    None,
                    None,
                    value,
                    value_type,
                )
                self.onehot_values[key] = cached
            (
                index,
                index_type,
                valid,
                valid_type,
                conflict,
                conflict_type,
                value,
                value_type,
            ) = cached
            if node.attr == "conflict" and conflict is None:
                width = _primitive_integer_width("onehot_encode", value_type)
                count_type = BitsType(primitive_count_width(width))
                count = self._new()
                one = self._new()
                conflict = self._new()
                self.lines.append(
                    f"    %{count} = ac.var.popcount %{value} : "
                    f"!ac.var<{_render_type(value_type)}> -> "
                    f"!ac.var<{_render_type(count_type)}>"
                )
                self.lines.append(
                    f"    %{one} = ac.var.constant 1 : {_render_type(count_type)} "
                    f"as !ac.var<{_render_type(count_type)}>"
                )
                self.lines.append(
                    f'    %{conflict} = ac.var.cmp "ugt" %{count}, %{one} : '
                    f"!ac.var<{_render_type(count_type)}> -> !ac.var<i1>"
                )
                conflict_type = BoolType()
                self.onehot_values[key] = (
                    index,
                    index_type,
                    valid,
                    valid_type,
                    conflict,
                    conflict_type,
                    value,
                    value_type,
                )
            if node.attr == "index":
                return index, index_type
            if node.attr == "valid":
                return valid, valid_type
            assert conflict is not None and conflict_type is not None
            return conflict, conflict_type
        if isinstance(node, ast.Attribute):
            record, record_type = self.emit(node.value)
            if not isinstance(record_type, StructType):
                raise QueueFrontendError(f"ACPY-QUEUE-003: unknown field {node.attr!r}")
            try:
                field_type = record_type.field(node.attr).type
            except KeyError as exc:
                raise QueueFrontendError(
                    f"ACPY-QUEUE-003: unknown field {node.attr!r}"
                ) from exc
            rendered_record_type = _render_type(record_type)
            rendered_field_type = _render_type(field_type)
            name = self._new()
            self.lines.append(
                f'    %{name} = ac.var.get %{record} field "{node.attr}" : '
                f"!ac.var<{rendered_record_type}> -> "
                f"!ac.var<{rendered_field_type}>"
            )
            return name, field_type
        if isinstance(node, ast.BinOp) and isinstance(
            node.op,
            (
                ast.Add,
                ast.Sub,
                ast.Mult,
                ast.FloorDiv,
                ast.Mod,
                ast.BitAnd,
                ast.BitOr,
                ast.BitXor,
                ast.LShift,
                ast.RShift,
            ),
        ):
            left, left_type = self.emit(node.left, expected)
            left, left_type = self._coerce_bool_to_expected_bits(
                left, left_type, expected
            )
            from _pycircuit_semantics import RangeType

            if isinstance(left_type, RangeType):
                if not isinstance(node.op, (ast.Add, ast.Sub)):
                    raise QueueFrontendError(
                        "ACPY-RANGE-002: bounded values support only + and -; "
                        "use an explicit bits view for modular arithmetic"
                    )
                right_expected = None
                if (
                    isinstance(node.right, ast.Constant)
                    and type(node.right.value) is int
                    and 0 <= node.right.value < (1 << 64)
                ):
                    right_expected = RangeType(node.right.value, node.right.value + 1)
                right, right_type = self.emit(node.right, right_expected)
                if not isinstance(right_type, RangeType):
                    raise QueueFrontendError(
                        "ACPY-RANGE-002: bounded arithmetic operands must be ranges"
                    )
                if isinstance(node.op, ast.Add):
                    lower = left_type.lower + right_type.lower
                    upper_inclusive = left_type.upper - 1 + right_type.upper - 1
                    if upper_inclusive >= (1 << 64):
                        raise QueueFrontendError(
                            "ACPY-RANGE-002: bounded addition exceeds u64"
                        )
                    operation = "range_add"
                else:
                    if left_type.lower < right_type.upper - 1:
                        raise QueueFrontendError(
                            "ACPY-RANGE-002: bounded subtraction may be negative"
                        )
                    lower = left_type.lower - (right_type.upper - 1)
                    upper_inclusive = left_type.upper - 1 - right_type.lower
                    operation = "range_sub"
                result_type = RangeType(lower, upper_inclusive + 1)
                name = self._new()
                self.lines.append(
                    f"    %{name} = ac.var.{operation} %{left}, %{right} : "
                    f"!ac.var<{_render_type(left_type)}>, "
                    f"!ac.var<{_render_type(right_type)}> -> "
                    f"!ac.var<{_render_type(result_type)}>"
                )
                return self._remember(
                    name,
                    result_type,
                    ClosedInterval(result_type.lower, result_type.upper - 1),
                )
            right, right_type = self.emit(node.right, left_type)
            right, right_type = self._coerce_bool_to_expected_bits(
                right, right_type, left_type
            )
            if not self._types_match(left_type, right_type):
                raise QueueFrontendError("ACPY-QUEUE-003: binary operands must match")
            if isinstance(left_type, EnumType):
                raise QueueFrontendError(
                    "ACPY-TYPE-005: enum values support only equality comparison"
                )
            opcode = {
                ast.Add: "add",
                ast.Sub: "sub",
                ast.Mult: "mul",
                ast.FloorDiv: "udiv",
                ast.Mod: "urem",
                ast.BitAnd: "and",
                ast.BitOr: "or",
                ast.BitXor: "xor",
                ast.LShift: "shl",
                ast.RShift: "shr",
            }[type(node.op)]
            name = self._new()
            self.lines.append(
                f"    %{name} = ac.var.{opcode} %{left}, %{right} : "
                f"!ac.var<{_render_type(left_type)}>"
            )
            width = _epoch_05_integer_width(left_type)
            constraint = (
                transfer_bits(
                    opcode,
                    self.constraint_for_result(left, left_type),
                    self.constraint_for_result(right, right_type),
                    width=width,
                )
                if width is not None
                else Unknown()
            )
            return self._remember(name, left_type, constraint)
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.Invert):
            value, value_type = self.emit(node.operand)
            if _epoch_05_integer_width(value_type) is None:
                raise QueueFrontendError(
                    "ACPY-QUEUE-003: bitwise not requires an integer payload"
                )
            name = self._new()
            self.lines.append(
                f"    %{name} = ac.var.not %{value} : "
                f"!ac.var<{_render_type(value_type)}> -> "
                f"!ac.var<{_render_type(value_type)}>"
            )
            return name, value_type
        if isinstance(node, ast.BoolOp) and isinstance(node.op, (ast.And, ast.Or)):
            operator = "and" if isinstance(node.op, ast.And) else "or"
            if len(node.values) < 2:
                raise QueueFrontendError(
                    f"ACPY-QUEUE-003: boolean {operator} requires two operands"
                )
            current, current_type = self.emit(node.values[0], BoolType())
            if not _is_epoch_05_bool_compatible(current_type):
                raise QueueFrontendError("ACPY-QUEUE-003: boolean operands must be i1")
            for operand in node.values[1:]:
                value, value_type = self.emit(operand, BoolType())
                if not _is_epoch_05_bool_compatible(value_type):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-003: boolean operands must be i1"
                    )
                name = self._new()
                self.lines.append(
                    f"    %{name} = ac.var."
                    f"{'mul' if operator == 'and' else 'or'} "
                    f"%{current}, %{value} : !ac.var<i1>"
                )
                current = name
            return current, BoolType()
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.Not):
            value, value_type = self.emit(node.operand, BoolType())
            if not _is_epoch_05_bool_compatible(value_type):
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
            return name, BoolType()
        if (
            isinstance(node, ast.Compare)
            and len(node.ops) == len(node.comparators) == 1
        ):
            from _pycircuit_semantics import RangeType

            comparator = node.comparators[0]
            if isinstance(node.left, ast.Constant) and type(node.left.value) is int:
                right, right_type = self.emit(comparator)
                expected_left = right_type
                if isinstance(right_type, RangeType):
                    if not 0 <= node.left.value < (1 << 64):
                        raise QueueFrontendError(
                            "ACPY-RANGE-002: bounded comparison literal must be "
                            "in the unsigned u64 domain"
                        )
                    expected_left = RangeType(node.left.value, node.left.value + 1)
                left, left_type = self.emit(node.left, expected_left)
            elif (
                isinstance(node.left, ast.Name) and node.left.id in self.deferred_values
            ):
                right, right_type = self.emit(comparator)
                left, left_type = self.emit(node.left, right_type)
            else:
                left, left_type = self.emit(node.left)
                expected_right = left_type
                if (
                    isinstance(left_type, RangeType)
                    and isinstance(comparator, ast.Constant)
                    and type(comparator.value) is int
                ):
                    if not 0 <= comparator.value < (1 << 64):
                        raise QueueFrontendError(
                            "ACPY-RANGE-002: bounded comparison literal must be "
                            "in the unsigned u64 domain"
                        )
                    expected_right = RangeType(comparator.value, comparator.value + 1)
                right, right_type = self.emit(comparator, expected_right)

            if isinstance(left_type, RangeType) and isinstance(right_type, RangeType):
                predicates = {
                    ast.Eq: "eq",
                    ast.NotEq: "ne",
                    ast.Lt: "ult",
                    ast.LtE: "ule",
                    ast.Gt: "ugt",
                    ast.GtE: "uge",
                }
                predicate = predicates.get(type(node.ops[0]))
                if predicate is None:
                    raise QueueFrontendError(
                        "ACPY-RANGE-002: unsupported bounded comparison"
                    )
                name = self._new()
                self.lines.append(
                    f'    %{name} = ac.var.range_cmp "{predicate}" '
                    f"%{left}, %{right} : !ac.var<{_render_type(left_type)}>, "
                    f"!ac.var<{_render_type(right_type)}> -> !ac.var<i1>"
                )
                return name, BoolType()
            if not self._types_match(left_type, right_type):
                raise QueueFrontendError(
                    "ACPY-QUEUE-003: comparison operands must match for "
                    f"{ast.unparse(node)!r} "
                    f"({_render_type(left_type)} vs {_render_type(right_type)})"
                )
            predicates = {
                ast.Eq: "eq",
                ast.NotEq: "ne",
                ast.Lt: "ult",
                ast.LtE: "ule",
                ast.Gt: "ugt",
                ast.GtE: "uge",
            }
            predicate = predicates.get(type(node.ops[0]))
            if predicate is None:
                raise QueueFrontendError("ACPY-QUEUE-003: unsupported comparison")
            if isinstance(left_type, EnumType) and predicate not in {"eq", "ne"}:
                raise QueueFrontendError(
                    "ACPY-TYPE-005: enum values support only equality comparison"
                )
            if isinstance(
                left_type, (StructType, TupleType, ArrayType)
            ) and predicate not in {
                "eq",
                "ne",
            }:
                raise QueueFrontendError(
                    "ACPY-TYPE-007: aggregate values support only equality comparison"
                )
            name = self._new()
            self.lines.append(
                f'    %{name} = ac.var.cmp "{predicate}" %{left}, %{right} : '
                f"!ac.var<{_render_type(left_type)}> -> !ac.var<i1>"
            )
            return name, BoolType()
        if (
            isinstance(node, ast.Call)
            and _decorator_name(node.func).rsplit(".", 1)[-1] == "matches"
        ):
            if len(node.args) != 2 or node.keywords:
                raise QueueFrontendError(
                    "ACPY-BITS-004: matches requires two positional arguments"
                )
            if not (
                isinstance(node.args[1], ast.Constant)
                and type(node.args[1].value) is str
            ):
                raise QueueFrontendError(
                    "ACPY-BITS-004: matches pattern must be a compile-time str"
                )
            value, value_type = self.emit(node.args[0])
            if not isinstance(value_type, BitsType):
                raise QueueFrontendError("ACPY-BITS-004: matches requires a bits value")
            try:
                mask, expected_value = parse_bitmask_checked(
                    node.args[1].value,
                    width=value_type.width,
                    extended=False,
                )
            except (TypeError, ValueError) as error:
                raise QueueFrontendError(f"ACPY-BITS-004: {error}") from error
            name = self._new()
            self.lines.append(
                f"    %{name} = ac.var.matches %{value} mask {mask} "
                f"value {expected_value} : "
                f"!ac.var<{_render_type(value_type)}> -> !ac.var<i1>"
            )
            return self._remember(name, BoolType())
        if (
            isinstance(node, ast.Call)
            and _decorator_name(node.func).rsplit(".", 1)[-1] == "concat"
        ):
            if not node.args or node.keywords:
                raise QueueFrontendError(
                    "ACPY-BITS-002: concat requires one or more positional values"
                )
            operands: list[str] = []
            operand_types: list[ValueType] = []
            result_width = 0
            for argument in node.args:
                operand, operand_type = self.emit(argument)
                operand_width = _epoch_05_integer_width(operand_type)
                if operand_width is None:
                    raise QueueFrontendError(
                        "ACPY-BITS-002: concat operands must be bits values"
                    )
                operands.append(operand)
                operand_types.append(operand_type)
                result_width += operand_width
            if result_width > 64:
                raise QueueFrontendError(
                    "ACPY-BITS-002: concat result width must be in [1, 64]"
                )
            name = self._new()
            self.lines.append(
                f"    %{name} = ac.var.concat "
                + ", ".join(f"%{operand}" for operand in operands)
                + " : "
                + ", ".join(f"!ac.var<{_render_type(typ)}>" for typ in operand_types)
                + f" -> !ac.var<i{result_width}>"
            )
            return name, BitsType(result_width)
        if (
            isinstance(node, ast.Call)
            and _decorator_name(node.func).rsplit(".", 1)[-1] == "insert"
        ):
            lsb_values = [
                keyword.value for keyword in node.keywords if keyword.arg == "lsb"
            ]
            if (
                len(node.args) != 2
                or len(lsb_values) != 1
                or len(node.keywords) != 1
                or _constant_integer(lsb_values[0]) is None
            ):
                raise QueueFrontendError(
                    "ACPY-BITS-003: insert requires value, field, and static lsb"
                )
            base, base_type = self.emit(node.args[0])
            field, field_type = self.emit(node.args[1])
            base_width = _epoch_05_integer_width(base_type)
            field_width = _epoch_05_integer_width(field_type)
            if base_width is None or field_width is None:
                raise QueueFrontendError(
                    "ACPY-BITS-003: insert operands must be bits values"
                )
            lsb = _constant_integer(lsb_values[0])
            assert lsb is not None
            if field_width > base_width or not _proven_integer_in(
                lsb, 0, base_width - field_width
            ):
                raise QueueFrontendError(
                    "ACPY-BITS-003: inserted field is out of range"
                )
            name = self._new()
            self.lines.append(
                f"    %{name} = ac.var.insert %{base}, %{field} at {lsb} : "
                f"!ac.var<{_render_type(base_type)}>, "
                f"!ac.var<{_render_type(field_type)}> -> "
                f"!ac.var<{_render_type(base_type)}>"
            )
            return name, base_type
        if (
            isinstance(node, ast.Call)
            and _decorator_name(node.func).rsplit(".", 1)[-1] == "popcount"
        ):
            if len(node.args) != 1 or node.keywords:
                raise QueueFrontendError(
                    "ACPY-VAR-003",
                    "popcount requires exactly one positional operand",
                )
            value, value_type = self.emit(node.args[0])
            width = _primitive_integer_width("popcount", value_type)
            result_width = primitive_count_width(width)
            name = self._new()
            self.lines.append(
                f"    %{name} = ac.var.popcount %{value} : "
                f"!ac.var<{_render_type(value_type)}> -> !ac.var<i{result_width}>"
            )
            return name, BitsType(result_width)
        if isinstance(node, ast.Call) and _decorator_name(node.func).rsplit(".", 1)[
            -1
        ] in {"count_leading_zeros", "count_trailing_zeros"}:
            operation = _decorator_name(node.func).rsplit(".", 1)[-1]
            if len(node.args) != 1 or node.keywords:
                raise QueueFrontendError(
                    "ACPY-VAR-003",
                    f"{operation} requires exactly one positional operand",
                )
            value, value_type = self.emit(node.args[0])
            width = _primitive_integer_width(operation, value_type)
            result_width = primitive_count_width(width)
            name = self._new()
            direction = "trailing" if operation == "count_trailing_zeros" else "leading"
            self.lines.append(
                f'    %{name} = ac.var.count_zeros %{value} direction "{direction}" : '
                f"!ac.var<{_render_type(value_type)}> -> !ac.var<i{result_width}>"
            )
            return name, BitsType(result_width)
        if (
            isinstance(node, ast.Call)
            and isinstance(node.func, ast.Name)
            and node.func.id in self.payloads
        ):
            if node.args:
                raise QueueFrontendError(
                    "ACPY-TYPE-006: record construction requires named fields"
                )
            record_type = self.payloads[node.func.id].descriptor
            expected_names = tuple(field.name for field in record_type.fields)
            expected_fields = {field.name: field.type for field in record_type.fields}
            provided: dict[str, tuple[str, ValueType]] = {}

            def add_field(
                name: str,
                value: str,
                value_type: ValueType,
                *,
                spread: bool = False,
            ) -> None:
                expected_type = expected_fields.get(name)
                if expected_type is None:
                    raise QueueFrontendError(
                        f"ACPY-TYPE-006: record spread provides unknown field {name!r}"
                    )
                if name in provided:
                    raise QueueFrontendError(
                        f"ACPY-TYPE-006: record field {name!r} is provided more than once"
                    )
                if (
                    value_type != expected_type
                    if spread
                    else not self._types_match(value_type, expected_type)
                ):
                    raise QueueFrontendError(
                        f"ACPY-TYPE-006: record field {name!r} type mismatch"
                    )
                provided[name] = (value, value_type)

            for keyword in node.keywords:
                if keyword.arg is not None:
                    expected_type = expected_fields.get(keyword.arg)
                    if expected_type is None:
                        raise QueueFrontendError(
                            "ACPY-TYPE-006: record construction provides unknown "
                            f"field {keyword.arg!r}"
                        )
                    value, value_type = self.emit(keyword.value, expected_type)
                    add_field(keyword.arg, value, value_type)
                    continue
                source, source_type = self.emit(keyword.value)
                if not isinstance(source_type, StructType):
                    raise QueueFrontendError(
                        "ACPY-TYPE-006: record spread requires a struct value"
                    )
                for source_field in source_type.fields:
                    field_value = self._new()
                    self.lines.append(
                        f"    %{field_value} = ac.var.get %{source} field "
                        f'"{source_field.name}" : '
                        f"!ac.var<{_render_type(source_type)}> -> "
                        f"!ac.var<{_render_type(source_field.type)}>"
                        + _render_source_frame_location(source_frame(keyword.value))
                    )
                    add_field(
                        source_field.name,
                        field_value,
                        source_field.type,
                        spread=True,
                    )

            if set(provided) != set(expected_names):
                raise QueueFrontendError(
                    "ACPY-TYPE-006: record construction must initialize every "
                    f"declared field exactly once for {node.func.id!r}; "
                    f"expected {expected_names!r}, got {tuple(provided)!r}"
                )
            operands: list[str] = []
            operand_types: list[ValueType] = []
            for field in record_type.fields:
                value, value_type = provided[field.name]
                operands.append(value)
                operand_types.append(value_type)
            name = self._new()
            self.lines.append(
                f"    %{name} = ac.var.record "
                + ", ".join(f"%{value}" for value in operands)
                + " : "
                + ", ".join(
                    f"!ac.var<{_render_type(value_type)}>"
                    for value_type in operand_types
                )
                + f" -> !ac.var<{_render_type(record_type)}>"
            )
            return name, record_type
        if (
            isinstance(node, ast.Call)
            and isinstance(node.func, ast.Attribute)
            and node.func.attr == "with_fields"
            and not node.args
        ):
            record, record_type = self.emit(node.func.value)
            if not isinstance(record_type, StructType):
                raise QueueFrontendError(
                    "ACPY-QUEUE-003: field update requires a struct value"
                )
            field_types = {field.name: field.type for field in record_type.fields}
            updates: dict[str, tuple[str, ValueType]] = {}

            def add_update(
                name: str,
                value: str,
                value_type: ValueType,
                *,
                spread: bool = False,
            ) -> None:
                field_type = field_types.get(name)
                if field_type is None:
                    raise QueueFrontendError(f"ACPY-QUEUE-003: unknown field {name!r}")
                if name in updates:
                    raise QueueFrontendError(
                        f"ACPY-QUEUE-003: field {name!r} is updated more than once"
                    )
                if (
                    value_type != field_type
                    if spread
                    else not self._types_match(value_type, field_type)
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-003: field update type mismatch"
                    )
                updates[name] = (value, value_type)

            for keyword in node.keywords:
                if keyword.arg is not None:
                    field_type = field_types.get(keyword.arg)
                    if field_type is None:
                        raise QueueFrontendError(
                            f"ACPY-QUEUE-003: unknown field {keyword.arg!r}"
                        )
                    value, value_type = self.emit(keyword.value, field_type)
                    add_update(keyword.arg, value, value_type)
                    continue
                source, source_type = self.emit(keyword.value)
                if not isinstance(source_type, StructType):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-003: field spread requires a struct value"
                    )
                for source_field in source_type.fields:
                    field_value = self._new()
                    self.lines.append(
                        f"    %{field_value} = ac.var.get %{source} field "
                        f'"{source_field.name}" : '
                        f"!ac.var<{_render_type(source_type)}> -> "
                        f"!ac.var<{_render_type(source_field.type)}>"
                        + _render_source_frame_location(source_frame(keyword.value))
                    )
                    add_update(
                        source_field.name,
                        field_value,
                        source_field.type,
                        spread=True,
                    )

            current = record
            for field in record_type.fields:
                if field.name not in updates:
                    continue
                value, value_type = updates[field.name]
                name = self._new()
                self.lines.append(
                    f"    %{name} = ac.var.with %{current}, %{value} field "
                    f'"{field.name}" : !ac.var<{_render_type(record_type)}>, '
                    f"!ac.var<{_render_type(field.type)}> -> "
                    f"!ac.var<{_render_type(record_type)}>"
                )
                current = name
            return current, record_type
        raise QueueFrontendError(
            "ACPY-QUEUE-003: unsupported lambda or rule expression "
            f"{ast.unparse(node)!r}"
        )
