"""Serial Python to QueueProgram parser."""

from __future__ import annotations

import ast
import copy
from collections.abc import Mapping
from dataclasses import replace

from _pycircuit_semantics import (
    ArrayType,
    BitsType,
    BoolType,
    Constant,
    EnumType,
    RangeType,
    StructType,
    TupleType,
    ValueType,
    prove_within,
)

from .._contract import CONTRACT_EPOCH
from .._diagnostics import SourceSpan
from .._source_map import (
    SourceFrame,
    SourceNodeLocations,
    apply_source_node_locations,
    source_frame,
)
from .._static_eval import (
    MAX_STATIC_EXPANSION,
    StaticEnvironment,
    StaticValue,
    evaluate_static,
)
from .definitions import (
    _extract_conditional_effect_guard,
    _invariant_definitions,
    _is_none_return,
    _lambda_value,
    _pure_helper_definitions,
)
from .errors import QueueFrontendError
from .expressions import (
    _ExpressionEmitter,
)
from .model import (
    BarrierBinding,
    CandidateSetBinding,
    CollectionBinding,
    CreditBinding,
    DependencyBinding,
    EntryViewBinding,
    ExpectBinding,
    FeedbackBinding,
    ForkBinding,
    MaskedEntryViewBinding,
    MaskedTableWriteBinding,
    MemoryBinding,
    MemoryInstanceBinding,
    MemoryRequestBinding,
    MergeBinding,
    ObservationBinding,
    ProjectedTableViewBinding,
    QueueBinding,
    QueueProgram,
    RecursiveQueueHelper,
    ReorderBinding,
    RouteBinding,
    RuleDefinition,
    RuleFindBinding,
    RuleFindDefinition,
    RuleLocalBinding,
    RuleLocalDefinition,
    RuleSlotOwnerBinding,
    RuleSlotReleaseBinding,
    RuleSlotReleaseDefinition,
    RuleStateOwnerBinding,
    RuleStateReadBinding,
    RuleStateReadDefinition,
    RuleStateWriteBinding,
    RuleStateWriteDefinition,
    ScopeBinding,
    SelectBinding,
    SelectedMemoryBinding,
    SelectionBinding,
    SinkBinding,
    SlotBinding,
    SlotReleaseBinding,
    StaticMemoryArrayBinding,
    StaticQueueCollection,
    StaticTypeCheck,
    TableBinding,
    TableReadBinding,
    TableWriteBinding,
    VarStateBinding,
)
from .normalize import (
    _constantize_expression,
    _desugar_nested_rule_captures,
    _normalize_rule_field_assignments,
    _strip_static_assertions,
)
from .source import (
    _normalize_queue_source_path,
)
from .static_types import (
    _bitfields,
    _bounded_annotation_static_checks,
    _constant_integer,
    _contains_declared_range,
    _dependent_static_type_expression,
    _enums,
    _epoch_05_integer_width,
    _is_epoch_05_bool_compatible,
    _module_static_values,
    _nonnegative_int_value,
    _payload,
    _payloads,
    _positive_int_value,
    _product,
    _scalar_annotation_static_check,
    _scalar_reset_init,
    _static_config_bindings_for_checks,
    _static_int_value,
    _static_parameter_aliases,
    _static_type_bindings_for_checks,
    _type_static_values,
    _types_equal_in_epoch_05,
    _validate_static_config_roots,
)
from .syntax import _decorator_name
from .statements import _ParserEnvironment, _ParserState

RULE_LOWERING_PIPELINE = (
    "builtin.module("
    "ac-lower-rules,"
    "ac-inline-pure-helpers,"
    "canonicalize,cse,"
    "ac-verify-rule-closure,"
    "ac-freeze-topology)"
)


def parse_queue_program(
    text: str,
    system: str,
    static_arguments: Mapping[str, StaticValue] | None = None,
    specialization_fingerprint: str | None = None,
    *,
    entry_kind: str = "system",
    source_path: str | None = None,
    static_type_namespace: str = "",
    definition_locations: Mapping[str, tuple[str, int, int]] | None = None,
    static_assert_locations: (
        Mapping[str, tuple[tuple[str, int, int], ...]] | None
    ) = None,
    source_node_locations: SourceNodeLocations | None = None,
) -> QueueProgram:
    normalized_source_path = _normalize_queue_source_path(source_path)
    tree = ast.parse(text, filename=normalized_source_path, type_comments=True)
    apply_source_node_locations(
        tree,
        source_node_locations,
        normalized_source_path,
    )
    tree = _desugar_nested_rule_captures(tree, system, entry_kind)
    module_static_values = _module_static_values(tree)
    type_static_values = _type_static_values(tree, static_arguments)
    parameter_aliases = _static_parameter_aliases(tree)
    expression_type_checks: list[StaticTypeCheck] = []
    reachable_expression_owners: set[str] | None = None
    if entry_kind == "module":
        function_nodes = {
            node.name: node
            for node in tree.body
            if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef))
        }
        reachable_expression_owners = set()
        pending_expression_owners = [system]
        while pending_expression_owners:
            name = pending_expression_owners.pop()
            if name in reachable_expression_owners or name not in function_nodes:
                continue
            reachable_expression_owners.add(name)
            for item in ast.walk(function_nodes[name]):
                if (
                    isinstance(item, ast.Name)
                    and isinstance(item.ctx, ast.Load)
                    and item.id in function_nodes
                ):
                    pending_expression_owners.append(item.id)

    class ConcretizeBoundedIntrinsicTargets(ast.NodeTransformer):
        """Resolve dependent range targets in executable expressions only."""

        def __init__(
            self,
            owner: str,
            live_assignments: set[int],
            record_checks: bool,
        ) -> None:
            self.owner = owner
            self.ordinal = 0
            self.live_assignments = live_assignments
            self.record_checks = record_checks

        def visit_Assign(self, node: ast.Assign) -> ast.Assign:
            previous = self.record_checks
            if (
                len(node.targets) == 1
                and isinstance(node.targets[0], ast.Name)
                and id(node) not in self.live_assignments
            ):
                self.record_checks = False
            transformed = self.generic_visit(node)
            self.record_checks = previous
            assert isinstance(transformed, ast.Assign)
            return transformed

        @staticmethod
        def _target_index(call: ast.Call) -> int | None:
            intrinsic = _decorator_name(call.func).rsplit(".", 1)[-1]
            if intrinsic in {"wrap", "saturate", "checked", "refine"}:
                return 1
            if intrinsic == "zero":
                return 0
            if intrinsic == "literal":
                return 1
            return None

        def visit_Call(self, node: ast.Call) -> ast.Call:
            transformed = self.generic_visit(node)
            assert isinstance(transformed, ast.Call)
            target_index = self._target_index(transformed)
            if target_index is None or target_index >= len(transformed.args):
                return transformed
            target = transformed.args[target_index]
            if not isinstance(target, ast.Subscript):
                return transformed
            kind = _decorator_name(target.value).rsplit(".", 1)[-1]
            bounds = (
                (ast.Constant(0), target.slice)
                if kind == "index"
                else (
                    tuple(target.slice.elts)
                    if kind == "range" and isinstance(target.slice, ast.Tuple)
                    else ()
                )
            )
            if len(bounds) != 2:
                return transformed
            lower = _constant_integer(bounds[0], type_static_values)
            upper = _constant_integer(bounds[1], type_static_values)
            if lower is None or upper is None:
                return transformed
            intrinsic = _decorator_name(transformed.func).rsplit(".", 1)[-1]
            if self.record_checks and intrinsic in {
                "wrap",
                "saturate",
                "checked",
                "refine",
            }:
                target_name = (
                    "expression."
                    + static_type_namespace
                    + self.owner
                    + "."
                    + str(self.ordinal)
                )
                self.ordinal += 1
                target_checks: list[str] = []
                concrete = RangeType(lower, upper)
                for suffix, bound in zip(
                    ("range_lower", "range_upper"), bounds, strict=True
                ):
                    dependent = _dependent_static_type_expression(
                        bound,
                        parameter_aliases,
                        type_static_values,
                        binding_namespace=static_type_namespace,
                    )
                    if dependent is None:
                        continue
                    program, result = dependent
                    check_target = f"{target_name}:{suffix}"
                    expression_type_checks.append(
                        StaticTypeCheck(check_target, program, result, concrete)
                    )
                    target_checks.append(check_target)
                if target_checks:
                    setattr(target, "_ac_static_type_target", target_name)
            concrete_bounds = ast.Tuple(
                elts=[ast.Constant(lower), ast.Constant(upper)], ctx=ast.Load()
            )
            target.slice = (
                ast.copy_location(ast.Constant(upper), target.slice)
                if kind == "index"
                else ast.copy_location(concrete_bounds, target.slice)
            )
            return transformed

    for node in tree.body:
        if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            continue
        parameter_names = {argument.arg for argument in node.args.args}
        live_assignments: set[int] = set()

        def loaded_names(candidate: ast.AST | None) -> set[str]:
            if candidate is None:
                return set()
            return {
                item.id
                for item in ast.walk(candidate)
                if isinstance(item, ast.Name) and isinstance(item.ctx, ast.Load)
            }

        def analyze_liveness(
            statements: list[ast.stmt], live_out: set[str]
        ) -> set[str]:
            live = set(live_out)
            for statement in reversed(statements):
                if isinstance(statement, (ast.FunctionDef, ast.AsyncFunctionDef)):
                    analyze_liveness(statement.body, set())
                    continue
                if isinstance(statement, ast.If):
                    true_live = analyze_liveness(statement.body, live)
                    false_live = analyze_liveness(statement.orelse, live)
                    live = true_live | false_live | loaded_names(statement.test)
                    continue
                if (
                    isinstance(statement, ast.Assign)
                    and len(statement.targets) == 1
                    and isinstance(statement.targets[0], ast.Name)
                ):
                    target = statement.targets[0].id
                    if target in live or target in parameter_names:
                        live_assignments.add(id(statement))
                        live.discard(target)
                        live.update(loaded_names(statement.value))
                    continue
                live.update(loaded_names(statement))
            return live

        analyze_liveness(node.body, set())
        target_concretizer = ConcretizeBoundedIntrinsicTargets(
            node.name,
            live_assignments,
            reachable_expression_owners is None
            or node.name in reachable_expression_owners,
        )
        node.body = [
            ast.fix_missing_locations(target_concretizer.visit(statement))
            for statement in node.body
        ]
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
    enums = _enums(tree)
    enum_map = {item.name: item.descriptor for item in enums}
    payloads = _payloads(
        tree,
        enums,
        type_static_values,
        static_type_namespace=static_type_namespace,
        allow_unbound=entry_kind == "module",
    )
    payload_map = {item.name: item for item in payloads}
    bitfields = _bitfields(tree)
    bitfield_map = {binding.name: binding.layout for binding in bitfields}
    invariant_definitions = _invariant_definitions(
        tree, payload_map, bitfield_map, enum_map
    )
    helper_definitions = _pure_helper_definitions(
        tree,
        payload_map,
        enum_map,
        entry=system,
        reachable_only=entry_kind == "module",
    )
    helper_map = {definition.name: definition for definition in helper_definitions}

    class DesugarHelperUnpacking(ast.NodeTransformer):
        next_index = 0

        def visit_Assign(self, node: ast.Assign) -> ast.Assign | list[ast.stmt]:
            transformed = self.generic_visit(node)
            assert isinstance(transformed, ast.Assign)
            if not (
                len(transformed.targets) == 1
                and isinstance(transformed.targets[0], (ast.Tuple, ast.List))
                and isinstance(transformed.value, ast.Call)
                and isinstance(transformed.value.func, ast.Name)
                and transformed.value.func.id in helper_map
                and isinstance(helper_map[transformed.value.func.id].result, TupleType)
            ):
                return transformed
            target = transformed.targets[0]
            helper = helper_map[transformed.value.func.id]
            assert isinstance(target, (ast.Tuple, ast.List))
            assert isinstance(helper.result, TupleType)
            if len(target.elts) != len(helper.result.elements) or not all(
                isinstance(item, ast.Name) for item in target.elts
            ):
                raise QueueFrontendError(
                    "ACPY-HELPER-002: helper tuple unpack arity must match its result"
                )
            temporary = f"__ac_helper_tuple_{self.next_index}"
            self.next_index += 1
            statements: list[ast.stmt] = [
                ast.Assign(
                    targets=[ast.Name(id=temporary, ctx=ast.Store())],
                    value=transformed.value,
                )
            ]
            statements.extend(
                ast.Assign(
                    targets=[copy.deepcopy(item)],
                    value=ast.Subscript(
                        value=ast.Name(id=temporary, ctx=ast.Load()),
                        slice=ast.Constant(index),
                        ctx=ast.Load(),
                    ),
                )
                for index, item in enumerate(target.elts)
            )
            return [ast.copy_location(statement, node) for statement in statements]

    transformed_tree = DesugarHelperUnpacking().visit(tree)
    assert isinstance(transformed_tree, ast.Module)
    tree = ast.fix_missing_locations(transformed_tree)
    rule_definitions: dict[str, RuleDefinition] = {}

    def parse_optional_multi_output_rule(
        node: ast.FunctionDef,
    ) -> RuleDefinition | None:
        annotation = node.returns
        if not (
            isinstance(annotation, ast.Subscript)
            and _decorator_name(annotation.value).rsplit(".", 1)[-1]
            in {"tuple", "Tuple"}
        ):
            return None
        annotation_elements = (
            tuple(annotation.slice.elts)
            if isinstance(annotation.slice, ast.Tuple)
            else (annotation.slice,)
        )
        if len(annotation_elements) < 2:
            return None
        forbidden_runtime_mechanics = {
            "ready",
            "full",
            "pop",
            "push",
            "presence",
            "dummy",
            "reserve",
            "reservation",
            "commit",
        }
        for candidate in ast.walk(node):
            if not isinstance(candidate, ast.Call):
                continue
            spelling = (
                candidate.func.attr
                if isinstance(candidate.func, ast.Attribute)
                else candidate.func.id
                if isinstance(candidate.func, ast.Name)
                else None
            )
            if spelling in forbidden_runtime_mechanics:
                raise QueueFrontendError(
                    "ACPY-RULE-014: ready/full/pop/push/presence/dummy/"
                    "reservation/commit mechanics are compiler-owned"
                )
        parameter_names = tuple(argument.arg for argument in node.args.args)
        if not parameter_names:
            raise QueueFrontendError(
                "ACPY-RULE-014: optional multi-output rules require a payload parameter"
            )
        body = list(node.body)
        if (
            body
            and isinstance(body[0], ast.Expr)
            and isinstance(body[0].value, ast.Constant)
            and isinstance(body[0].value.value, str)
        ):
            body.pop(0)
        reserved_names = {
            candidate.id
            for statement in body
            for candidate in ast.walk(statement)
            if isinstance(candidate, ast.Name)
        }
        body = _normalize_rule_field_assignments(body, reserved_names=reserved_names)
        if not body or not isinstance(body[-1], ast.Return):
            raise QueueFrontendError(
                "ACPY-RULE-014: multi-output rule requires one final fixed tuple return"
            )
        returned = body.pop().value
        if not isinstance(returned, (ast.Tuple, ast.List)):
            raise QueueFrontendError(
                "ACPY-RULE-014: multi-output rule must return a fixed tuple"
            )
        if len(returned.elts) != len(annotation_elements):
            raise QueueFrontendError(
                "ACPY-RULE-014: multi-output return arity must match its annotation"
            )
        if not all(isinstance(element, ast.Name) for element in returned.elts):
            raise QueueFrontendError(
                "ACPY-RULE-014: multi-output return ordinals require local names"
            )
        output_names = tuple(
            element.id for element in returned.elts if isinstance(element, ast.Name)
        )
        if len(set(output_names)) != len(output_names):
            raise QueueFrontendError(
                "ACPY-RULE-014: multi-output return ordinals require unique locals"
            )
        output_types = tuple(
            _payload(
                element,
                payload_map,
                enum_map,
                static_values=type_static_values,
            )
            for element in annotation_elements
        )
        state_references: set[str] = set()
        eligible_state_parameters = frozenset(parameter_names[:-1])
        for statement in body:
            for candidate in ast.walk(statement):
                if isinstance(candidate, ast.Assign):
                    for target in candidate.targets:
                        if (
                            isinstance(target, ast.Name)
                            and target.id in eligible_state_parameters
                        ):
                            state_references.add(target.id)
                        elif (
                            isinstance(target, ast.Subscript)
                            and isinstance(target.value, ast.Name)
                            and target.value.id in eligible_state_parameters
                        ):
                            state_references.add(target.value.id)
                if (
                    isinstance(candidate, ast.Subscript)
                    and isinstance(candidate.value, ast.Name)
                    and candidate.value.id in eligible_state_parameters
                ):
                    state_references.add(candidate.value.id)
        state_count = (
            max(parameter_names.index(name) for name in state_references) + 1
            if state_references
            else 0
        )
        state_parameters = parameter_names[:state_count]
        payload_parameters = parameter_names[state_count:]
        if not payload_parameters:
            raise QueueFrontendError(
                "ACPY-RULE-014: optional multi-output rules require a payload "
                "parameter after persistent state"
            )
        parameter = payload_parameters[-1]
        versions: dict[str, str] = {}
        locals_: list[RuleLocalDefinition] = []
        state_writes: list[RuleStateWriteDefinition] = []
        scalar_state_presence: dict[str, ast.expr] = {}
        unconditionally_initialized: set[str] = set()
        typed: set[str] = set()
        presences: dict[str, ast.expr] = {
            name: ast.Constant(value=False) for name in output_names
        }
        next_version = 0
        next_condition = 0

        class RewriteLoads(ast.NodeTransformer):
            def __init__(self) -> None:
                self.excluded: frozenset[str] = frozenset()

            def visit_Name(self, candidate: ast.Name) -> ast.expr:
                if (
                    isinstance(candidate.ctx, ast.Load)
                    and candidate.id not in self.excluded
                    and candidate.id in versions
                ):
                    return ast.copy_location(
                        ast.Name(id=versions[candidate.id], ctx=ast.Load()), candidate
                    )
                return candidate

            def visit_Lambda(self, candidate: ast.Lambda) -> ast.expr:
                candidate.args.defaults = [
                    self.visit(default) for default in candidate.args.defaults
                ]
                candidate.args.kw_defaults = [
                    self.visit(default) if default is not None else None
                    for default in candidate.args.kw_defaults
                ]
                bound = {
                    argument.arg
                    for argument in (
                        *candidate.args.posonlyargs,
                        *candidate.args.args,
                        *candidate.args.kwonlyargs,
                    )
                }
                if candidate.args.vararg is not None:
                    bound.add(candidate.args.vararg.arg)
                if candidate.args.kwarg is not None:
                    bound.add(candidate.args.kwarg.arg)
                previous = self.excluded
                self.excluded = previous | frozenset(bound)
                candidate.body = self.visit(candidate.body)
                self.excluded = previous
                return candidate

        def rewrite(expression: ast.expr) -> ast.expr:
            rewritten = RewriteLoads().visit(copy.deepcopy(expression))
            assert isinstance(rewritten, ast.expr)
            return ast.fix_missing_locations(rewritten)

        def allocate(name: str) -> tuple[str, str | None]:
            nonlocal next_version
            prior = versions.get(name)
            version = f"__ac_rule_local_{next_version}_{name}"
            next_version += 1
            versions[name] = version
            return version, prior

        def conjunction(
            parent: ast.expr | None, condition: ast.expr, negated: bool
        ) -> ast.expr:
            term: ast.expr = (
                ast.UnaryOp(op=ast.Not(), operand=copy.deepcopy(condition))
                if negated
                else copy.deepcopy(condition)
            )
            if parent is None:
                return ast.fix_missing_locations(term)
            return ast.fix_missing_locations(
                ast.BoolOp(op=ast.And(), values=[copy.deepcopy(parent), term])
            )

        def assign(statement: ast.Assign, guard: ast.expr | None) -> None:
            if len(statement.targets) != 1:
                raise QueueFrontendError(
                    "ACPY-RULE-014: multi-output rule assignments require one target"
                )
            target = statement.targets[0]
            if (
                isinstance(target, ast.Subscript)
                and isinstance(target.value, ast.Name)
                and target.value.id in state_parameters
            ):
                if (
                    isinstance(statement.value, ast.Constant)
                    and statement.value.value is None
                ):
                    raise QueueFrontendError(
                        "ACPY-RULE-014: persistent state cannot be assigned None"
                    )
                state_writes.append(
                    RuleStateWriteDefinition(
                        target.value.id,
                        rewrite(target.slice),
                        rewrite(statement.value),
                        copy.deepcopy(guard),
                        False,
                    )
                )
                return
            if not isinstance(target, ast.Name):
                raise QueueFrontendError(
                    "ACPY-RULE-014: multi-output rule assignments require one local "
                    "or indexed persistent target"
                )
            name = target.id
            if name == parameter:
                raise QueueFrontendError(
                    "ACPY-RULE-014: multi-output rules cannot assign their input"
                )
            is_none = (
                isinstance(statement.value, ast.Constant)
                and statement.value.value is None
            )
            if name in state_parameters:
                if is_none:
                    raise QueueFrontendError(
                        "ACPY-RULE-014: persistent state cannot be assigned None"
                    )
                value = rewrite(statement.value)
                version, prior = allocate(name)
                locals_.append(
                    RuleLocalDefinition(
                        version,
                        value,
                        copy.deepcopy(guard),
                        False,
                        prior or name,
                    )
                )
                previous_presence = scalar_state_presence.get(
                    name, ast.Constant(value=False)
                )
                scalar_state_presence[name] = ast.fix_missing_locations(
                    ast.Constant(value=True)
                    if guard is None
                    else ast.IfExp(
                        test=copy.deepcopy(guard),
                        body=ast.Constant(value=True),
                        orelse=copy.deepcopy(previous_presence),
                    )
                )
                return
            optional_expression: tuple[ast.expr, ast.expr, bool] | None = None
            if isinstance(statement.value, ast.IfExp):
                body_none = (
                    isinstance(statement.value.body, ast.Constant)
                    and statement.value.body.value is None
                )
                else_none = (
                    isinstance(statement.value.orelse, ast.Constant)
                    and statement.value.orelse.value is None
                )
                if body_none != else_none:
                    optional_expression = (
                        statement.value.orelse if body_none else statement.value.body,
                        rewrite(statement.value.test),
                        body_none,
                    )
            if name not in output_names and is_none:
                raise QueueFrontendError(
                    "ACPY-RULE-014: None is only valid as output absence"
                )
            if name in output_names:
                if guard is None:
                    unconditionally_initialized.add(name)
                previous_presence = presences[name]
                assigned_presence: ast.expr = ast.Constant(value=not is_none)
                if optional_expression is not None:
                    _, condition, negated = optional_expression
                    assigned_presence = (
                        ast.UnaryOp(op=ast.Not(), operand=copy.deepcopy(condition))
                        if negated
                        else copy.deepcopy(condition)
                    )
                presences[name] = (
                    assigned_presence
                    if guard is None
                    else ast.IfExp(
                        test=copy.deepcopy(guard),
                        body=assigned_presence,
                        orelse=copy.deepcopy(previous_presence),
                    )
                )
                presences[name] = ast.fix_missing_locations(presences[name])
                if is_none:
                    return
                typed.add(name)
            elif name in output_names:
                raise AssertionError("unreachable")
            if is_none:
                raise AssertionError("unreachable")
            value_guard = guard
            value_expression = statement.value
            if optional_expression is not None:
                value_expression, condition, negated = optional_expression
                value_guard = conjunction(guard, condition, negated)
            value = rewrite(value_expression)
            referenced_optional = set(output_names) & {
                candidate.id
                for candidate in ast.walk(value)
                if isinstance(candidate, ast.Name)
            }
            if referenced_optional:
                raise QueueFrontendError(
                    "ACPY-RULE-014: optional output value cannot escape its ordinal"
                )
            version, prior = allocate(name)
            locals_.append(
                RuleLocalDefinition(
                    version,
                    value,
                    copy.deepcopy(value_guard),
                    False,
                    prior,
                )
            )

        def walk(statements: list[ast.stmt], guard: ast.expr | None = None) -> None:
            nonlocal next_condition
            for statement in statements:
                if isinstance(statement, ast.Assign):
                    assign(statement, guard)
                    continue
                if isinstance(statement, ast.If):
                    condition_name = f"__ac_multi_output_condition_{next_condition}"
                    next_condition += 1
                    condition_assign = ast.Assign(
                        targets=[ast.Name(id=condition_name, ctx=ast.Store())],
                        value=rewrite(statement.test),
                    )
                    assign(ast.fix_missing_locations(condition_assign), guard)
                    condition = ast.Name(id=versions[condition_name], ctx=ast.Load())
                    walk(statement.body, conjunction(guard, condition, False))
                    walk(statement.orelse, conjunction(guard, condition, True))
                    continue
                raise QueueFrontendError(
                    "ACPY-RULE-014: multi-output rule supports only serial local "
                    "assignments and if/elif/else"
                )

        walk(body)
        missing = [
            name for name in output_names if name not in unconditionally_initialized
        ]
        if missing:
            raise QueueFrontendError(
                f"ACPY-RULE-014: output ordinal local {missing[0]!r} is undefined"
            )
        missing_value = [name for name in output_names if name not in typed]
        if missing_value:
            raise QueueFrontendError(
                f"ACPY-RULE-014: output ordinal local {missing_value[0]!r} has no typed value"
            )
        for name, presence in scalar_state_presence.items():
            always_present = (
                isinstance(presence, ast.Constant) and presence.value is True
            )
            state_writes.append(
                RuleStateWriteDefinition(
                    name,
                    None,
                    ast.Name(id=versions[name], ctx=ast.Load()),
                    None if always_present else presence,
                    False,
                )
            )
        expressions = tuple(
            ast.Name(id=versions[name], ctx=ast.Load()) for name in output_names
        )
        return RuleDefinition(
            node.name,
            payload_parameters,
            expressions[0],
            node.lineno,
            node.col_offset + 1,
            locals=tuple(locals_),
            state_arguments=state_parameters,
            state_writes=tuple(state_writes),
            output_types=output_types,
            output_expressions=expressions,
            output_guards=tuple(presences[name] for name in output_names),
        )

    selected_rule_names: set[str] | None = None
    if entry_kind == "module":
        selected_modules = [
            node
            for node in tree.body
            if isinstance(node, ast.FunctionDef)
            and node.name == system
            and any(
                _decorator_name(decorator).rsplit(".", 1)[-1] == "module"
                for decorator in node.decorator_list
            )
        ]
        if len(selected_modules) == 1:
            selected_rule_names = {
                call.func.id
                for call in ast.walk(selected_modules[0])
                if isinstance(call, ast.Call) and isinstance(call.func, ast.Name)
            }

    for node in tree.body:
        if not isinstance(node, ast.FunctionDef) or not any(
            _decorator_name(decorator).rsplit(".", 1)[-1] == "rule"
            for decorator in node.decorator_list
        ):
            continue
        if selected_rule_names is not None and node.name not in selected_rule_names:
            continue
        if node.name in rule_definitions:
            raise QueueFrontendError(
                f"ACPY-RULE-001: rule {node.name!r} is defined more than once"
            )
        if any(isinstance(decorator, ast.Call) for decorator in node.decorator_list):
            raise QueueFrontendError(
                "ACPY-RULE-001: rule decorators do not accept options"
            )
        for candidate in ast.walk(node):
            if (
                isinstance(candidate, ast.Call)
                and _decorator_name(candidate.func).rsplit(".", 1)[-1] == "slot"
            ):
                raise QueueFrontendError(
                    "ACPY-SLOT-003: ac.slot must be declared in module/system "
                    "topology, not inside an @ac.rule"
                )
        if (
            not node.args.args
            or node.args.posonlyargs
            or node.args.kwonlyargs
            or node.args.vararg is not None
            or node.args.kwarg is not None
            or node.args.defaults
            or node.args.kw_defaults
        ):
            raise QueueFrontendError(
                "ACPY-RULE-001: rules require one or more positional parameters"
            )
        multi_output = parse_optional_multi_output_rule(node)
        if multi_output is not None:
            rule_definitions[node.name] = multi_output
            continue
        annotated_single_outputs: tuple[ValueType, ...] = ()
        if not (
            node.returns is None
            or (isinstance(node.returns, ast.Constant) and node.returns.value is None)
            or (isinstance(node.returns, ast.Name) and node.returns.id == "None")
        ):
            annotated_single_outputs = (
                _payload(
                    node.returns,
                    payload_map,
                    enum_map,
                    static_values=type_static_values,
                ),
            )
        body = list(node.body)
        if (
            body
            and isinstance(body[0], ast.Expr)
            and isinstance(body[0].value, ast.Constant)
            and isinstance(body[0].value.value, str)
        ):
            body.pop(0)
        reserved_names = {
            candidate.id
            for statement in body
            for candidate in ast.walk(statement)
            if isinstance(candidate, ast.Name)
        }
        body = _normalize_rule_field_assignments(body, reserved_names=reserved_names)
        if any(
            isinstance(candidate, ast.Name)
            and candidate.id[:18] == "__ac_helper_tuple_"
            for statement in body
            for candidate in ast.walk(statement)
        ):

            class SubstituteHelperLocals(ast.NodeTransformer):
                def __init__(self, values: Mapping[str, ast.expr]) -> None:
                    self.values = values

                def visit_Name(self, candidate: ast.Name) -> ast.expr:
                    if (
                        isinstance(candidate.ctx, ast.Load)
                        and candidate.id in self.values
                    ):
                        return ast.copy_location(
                            copy.deepcopy(self.values[candidate.id]), candidate
                        )
                    return candidate

            def substitute_helper_locals(
                expression: ast.expr, values: Mapping[str, ast.expr]
            ) -> ast.expr:
                result = SubstituteHelperLocals(values).visit(copy.deepcopy(expression))
                assert isinstance(result, ast.expr)
                return ast.fix_missing_locations(result)

            helper_locals: dict[str, ast.expr] = {}
            collapsed = True
            for statement in body[:-1]:
                if (
                    not isinstance(statement, ast.Assign)
                    or len(statement.targets) != 1
                    or not isinstance(statement.targets[0], ast.Name)
                ):
                    collapsed = False
                    break
                helper_locals[statement.targets[0].id] = substitute_helper_locals(
                    statement.value, helper_locals
                )
            if (
                collapsed
                and body
                and isinstance(body[-1], ast.Return)
                and body[-1].value is not None
            ):
                body = [
                    ast.copy_location(
                        ast.Return(
                            value=substitute_helper_locals(
                                body[-1].value, helper_locals
                            )
                        ),
                        body[-1],
                    )
                ]
        if (
            len(body) == 1
            and isinstance(body[0], ast.Return)
            and body[0].value is not None
        ):
            rule_definitions[node.name] = RuleDefinition(
                node.name,
                tuple(argument.arg for argument in node.args.args),
                copy.deepcopy(body[0].value),
                node.lineno,
                node.col_offset + 1,
                output_types=annotated_single_outputs,
            )
            continue

        parameter_names = tuple(argument.arg for argument in node.args.args)
        slot_parameter_names = {
            candidate.value.id
            for candidate in ast.walk(node)
            if isinstance(candidate, ast.Attribute)
            and candidate.attr == "release"
            and isinstance(candidate.value, ast.Name)
            and candidate.value.id in parameter_names
        }
        multi_body = list(body)
        multi_guard: ast.expr | None = None
        multi_effect_guard: ast.expr | None = None
        multi_output_guard: ast.expr | None = None
        multi_return: ast.expr | None = None
        if multi_body and isinstance(multi_body[-1], ast.Return):
            returned = multi_body.pop().value
            if not (
                returned is None
                or (isinstance(returned, ast.Constant) and returned.value is None)
            ):
                multi_return = copy.deepcopy(returned)
        multi_body, multi_effect_guard = _extract_conditional_effect_guard(
            multi_body,
            parameter_names,
            multi_return is not None,
            capture_conditions=True,
        )
        if (
            multi_body
            and isinstance(multi_body[-1], ast.If)
            and not multi_body[-1].orelse
            and len(multi_body[-1].body) == 1
            and isinstance(multi_body[-1].body[0], ast.Return)
            and multi_body[-1].body[0].value is not None
        ):
            optional_output = multi_body.pop()
            assert isinstance(optional_output, ast.If)
            returned = optional_output.body[0]
            assert isinstance(returned, ast.Return)
            multi_return = copy.deepcopy(returned.value)
            multi_output_guard = copy.deepcopy(optional_output.test)
        if (
            multi_body
            and isinstance(multi_body[-1], ast.If)
            and not multi_body[-1].orelse
            and multi_return is None
            and not (
                slot_parameter_names
                and any(
                    isinstance(candidate, ast.Return) and not _is_none_return(candidate)
                    for candidate in multi_body[-1].body
                )
            )
        ):
            if multi_effect_guard is not None:
                raise QueueFrontendError(
                    "ACPY-RULE-010: conditional effects cannot also use a "
                    "blocking rule guard"
                )
            guarded = multi_body.pop()
            if guarded.orelse or not guarded.body:
                raise QueueFrontendError(
                    "ACPY-RULE-007: guarded state rule requires one if body "
                    "without else"
                )
            guarded_body = list(guarded.body)
            if guarded_body and isinstance(guarded_body[-1], ast.Return):
                returned = guarded_body.pop().value
                if not (
                    returned is None
                    or (isinstance(returned, ast.Constant) and returned.value is None)
                ):
                    multi_return = copy.deepcopy(returned)
            multi_guard = copy.deepcopy(guarded.test)
            multi_body.extend(guarded_body)
        guarded_statements: list[tuple[ast.stmt, ast.expr | None, bool]] = []
        slot_releases: list[RuleSlotReleaseDefinition] = []
        absent_output_paths: list[ast.expr] = []
        has_branch_effects = False
        branch_condition_index = 0
        branch_condition_names = {
            node.id
            for statement in multi_body
            for node in ast.walk(statement)
            if isinstance(node, ast.Name)
        } | set(parameter_names)

        def branch_guard(
            path: tuple[tuple[ast.expr, bool], ...],
        ) -> tuple[ast.expr | None, bool]:
            if not path:
                return None, False
            if len(path) == 1:
                condition, negated = path[0]
                return copy.deepcopy(condition), negated
            terms = [
                (
                    ast.UnaryOp(op=ast.Not(), operand=copy.deepcopy(condition))
                    if negated
                    else copy.deepcopy(condition)
                )
                for condition, negated in path
            ]
            return (
                ast.fix_missing_locations(ast.BoolOp(op=ast.And(), values=terms)),
                False,
            )

        def flatten_branch(
            statement: ast.stmt,
            path: tuple[tuple[ast.expr, bool], ...] = (),
        ) -> None:
            nonlocal branch_condition_index, has_branch_effects, multi_return
            nonlocal multi_output_guard
            if isinstance(statement, ast.Return):
                if not path:
                    raise QueueFrontendError(
                        "ACPY-RULE-012: branch returns may only omit one output"
                    )
                guard, negated = branch_guard(path)
                assert guard is not None
                if not _is_none_return(statement):
                    if multi_return is not None:
                        raise QueueFrontendError(
                            "ACPY-RULE-012: rule permits one value-returning branch"
                        )
                    multi_return = copy.deepcopy(statement.value)
                    multi_output_guard = (
                        ast.UnaryOp(op=ast.Not(), operand=guard) if negated else guard
                    )
                    return
                absent_output_paths.append(
                    ast.UnaryOp(op=ast.Not(), operand=guard) if negated else guard
                )
                return
            if isinstance(statement, ast.For):
                if (
                    not isinstance(statement.target, ast.Name)
                    or statement.orelse
                    or statement.type_comment is not None
                ):
                    raise QueueFrontendError(
                        "ACPY-RULE-013: rule for loops require one static name "
                        "target and no else"
                    )
                try:
                    values = evaluate_static(
                        statement.iter, StaticEnvironment(module_static_values)
                    )
                except ValueError as error:
                    raise QueueFrontendError(
                        "ACPY-RULE-013: rule for loop must use a static iterable"
                    ) from error
                if not isinstance(values, tuple):
                    raise QueueFrontendError(
                        "ACPY-RULE-013: rule for loop must use a static iterable"
                    )
                loop_name = statement.target.id

                class SubstituteLoopIndex(ast.NodeTransformer):
                    def __init__(self, value: StaticValue) -> None:
                        self.value = value

                    def visit_Name(self, node: ast.Name) -> ast.expr:
                        if node.id == loop_name and isinstance(node.ctx, ast.Load):
                            return ast.copy_location(
                                ast.Constant(value=self.value), node
                            )
                        return node

                for value in values:
                    if type(value) not in {bool, int}:
                        raise QueueFrontendError(
                            "ACPY-RULE-013: rule for loop values must be bool or int"
                        )
                    substituter = SubstituteLoopIndex(value)
                    for candidate in statement.body:
                        expanded = substituter.visit(copy.deepcopy(candidate))
                        assert isinstance(expanded, ast.stmt)
                        flatten_branch(ast.fix_missing_locations(expanded), path)
                return
            if isinstance(statement, ast.If):
                if multi_effect_guard is not None:
                    raise QueueFrontendError(
                        "ACPY-RULE-011: branch-local effects cannot combine with "
                        "early-return guards"
                    )
                if not statement.body:
                    raise QueueFrontendError(
                        "ACPY-RULE-011: branch-local effects require a non-empty body"
                    )
                has_branch_effects = True
                condition_name = f"__ac_branch_condition_{branch_condition_index}"
                branch_condition_index += 1
                while condition_name in branch_condition_names:
                    condition_name += "_"
                branch_condition_names.add(condition_name)
                guard, negated = branch_guard(path)
                guarded_statements.append(
                    (
                        ast.copy_location(
                            ast.Assign(
                                targets=[ast.Name(id=condition_name, ctx=ast.Store())],
                                value=copy.deepcopy(statement.test),
                            ),
                            statement,
                        ),
                        guard,
                        negated,
                    )
                )
                condition = ast.Name(id=condition_name, ctx=ast.Load())
                for candidate in statement.body:
                    flatten_branch(candidate, (*path, (condition, False)))
                for candidate in statement.orelse:
                    flatten_branch(candidate, (*path, (condition, True)))
                return
            if (
                isinstance(statement, ast.Expr)
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Attribute)
                and statement.value.func.attr == "release"
                and isinstance(statement.value.func.value, ast.Name)
                and statement.value.func.value.id in slot_parameter_names
            ):
                call = statement.value
                if call.args or call.keywords:
                    raise QueueFrontendError(
                        "ACPY-SLOT-004: rule slot.release() takes no arguments; "
                        "use rule control flow for its condition"
                    )
                guard, negated = branch_guard(path)
                slot_releases.append(
                    RuleSlotReleaseDefinition(call.func.value.id, guard, negated)
                )
                has_branch_effects = True
                return
            guard, negated = branch_guard(path)
            guarded_statements.append((statement, guard, negated))

        for statement in multi_body:
            flatten_branch(statement)
        if (
            multi_return is not None
            and multi_output_guard is None
            and absent_output_paths
        ):
            present_terms = [
                ast.UnaryOp(op=ast.Not(), operand=copy.deepcopy(path))
                for path in absent_output_paths
            ]
            multi_output_guard = ast.fix_missing_locations(
                present_terms[0]
                if len(present_terms) == 1
                else ast.BoolOp(op=ast.And(), values=present_terms)
            )
        if slot_releases and multi_guard is None and multi_output_guard is not None:
            multi_guard = copy.deepcopy(multi_output_guard)
        state_reads: list[RuleStateReadDefinition] = []
        state_writes: list[RuleStateWriteDefinition] = []
        rule_locals: list[RuleLocalDefinition] = []
        rule_finds: list[RuleFindDefinition] = []
        local_names: set[str] = set()
        local_versions: dict[str, str] = {}
        partial_local_versions: set[str] = set()
        partial_local_guards: dict[str, tuple[ast.expr, bool]] = {}
        next_local_version = 0

        class RewriteLocalLoads(ast.NodeTransformer):
            def __init__(self, excluded: frozenset[str] = frozenset()) -> None:
                self.excluded = excluded

            def visit_Name(self, candidate: ast.Name) -> ast.expr:
                if (
                    isinstance(candidate.ctx, ast.Load)
                    and candidate.id not in self.excluded
                    and candidate.id in local_versions
                ):
                    return ast.copy_location(
                        ast.Name(id=local_versions[candidate.id], ctx=ast.Load()),
                        candidate,
                    )
                return candidate

            def visit_Lambda(self, candidate: ast.Lambda) -> ast.expr:
                candidate.args.defaults = [
                    self.visit(default) for default in candidate.args.defaults
                ]
                candidate.args.kw_defaults = [
                    self.visit(default) if default is not None else None
                    for default in candidate.args.kw_defaults
                ]
                bound = {
                    argument.arg
                    for argument in (
                        *candidate.args.posonlyargs,
                        *candidate.args.args,
                        *candidate.args.kwonlyargs,
                    )
                }
                if candidate.args.vararg is not None:
                    bound.add(candidate.args.vararg.arg)
                if candidate.args.kwarg is not None:
                    bound.add(candidate.args.kwarg.arg)
                previous = self.excluded
                self.excluded = previous | frozenset(bound)
                candidate.body = self.visit(candidate.body)
                self.excluded = previous
                return candidate

        def rewrite_local_loads(
            expression: ast.expr | None, *, excluded: frozenset[str] = frozenset()
        ) -> ast.expr | None:
            if expression is None:
                return None
            rewritten = RewriteLocalLoads(excluded).visit(copy.deepcopy(expression))
            assert isinstance(rewritten, ast.expr)
            return ast.fix_missing_locations(rewritten)

        def allocate_local_version(name: str) -> tuple[str, str | None]:
            nonlocal next_local_version
            prior = local_versions.get(name)
            while True:
                version = f"__ac_rule_local_{next_local_version}_{name}"
                next_local_version += 1
                if version not in parameter_names:
                    break
            local_versions[name] = version
            return version, prior

        def guards_are_complementary(
            left: tuple[ast.expr, bool], right: tuple[ast.expr, bool]
        ) -> bool:
            return left[1] != right[1] and ast.dump(
                left[0], include_attributes=False
            ) == ast.dump(right[0], include_attributes=False)

        def guard_literals(guard: ast.expr, negated: bool) -> frozenset[str]:
            effective: ast.expr = (
                ast.UnaryOp(op=ast.Not(), operand=copy.deepcopy(guard))
                if negated
                else guard
            )
            terms = (
                effective.values
                if isinstance(effective, ast.BoolOp)
                and isinstance(effective.op, ast.And)
                else (effective,)
            )
            return frozenset(ast.dump(term, include_attributes=False) for term in terms)

        def guard_covers_partial_values(
            guard: ast.expr | None,
            negated: bool,
            referenced: set[str],
        ) -> bool:
            partial = partial_local_versions & referenced
            if not partial:
                return True
            if guard is None:
                return False
            consumer = guard_literals(guard, negated)
            return all(
                guard_literals(*partial_local_guards[name]) <= consumer
                for name in partial
            )

        valid_multi_state = bool(guarded_statements)
        for statement, branch_guard, branch_negated in guarded_statements:
            if not isinstance(statement, ast.Assign) or len(statement.targets) != 1:
                valid_multi_state = False
                break
            target = statement.targets[0]
            if (
                isinstance(target, ast.Name)
                and isinstance(statement.value, ast.Call)
                and _decorator_name(statement.value.func).rsplit(".", 1)[-1] == "find"
            ):
                call = statement.value
                find_argument: str | None = None
                find_row: ast.expr | None = None
                receiver = (
                    call.func.value
                    if isinstance(call.func, ast.Attribute) and call.func.attr == "find"
                    else None
                )
                if isinstance(receiver, ast.Name) and receiver.id in parameter_names:
                    find_argument = receiver.id
                elif (
                    isinstance(receiver, ast.Call)
                    and isinstance(receiver.func, ast.Attribute)
                    and receiver.func.attr == "view"
                    and isinstance(receiver.func.value, ast.Name)
                    and receiver.func.value.id in parameter_names
                    and len(receiver.args) == 1
                    and not receiver.keywords
                ):
                    find_argument = receiver.func.value.id
                    find_row = receiver.args[0]
                elif _decorator_name(call.func) in {"find", "ac.find"}:
                    raise QueueFrontendError(
                        "ACPY-RULE-009: ac.find was removed; use "
                        "table.find(where=..., key=...)"
                    )
                if (
                    call.args
                    or find_argument not in parameter_names
                    or any(
                        keyword.arg not in {"where", "key"} for keyword in call.keywords
                    )
                ):
                    raise QueueFrontendError(
                        "ACPY-RULE-009: table.find requires a Table receiver, "
                        "no positional arguments, and where/key lambdas"
                    )
                where = [
                    keyword.value for keyword in call.keywords if keyword.arg == "where"
                ]
                keys = [
                    keyword.value for keyword in call.keywords if keyword.arg == "key"
                ]
                if len(where) != 1 or len(keys) > 1:
                    raise QueueFrontendError(
                        "ACPY-RULE-009: find requires one where and at most one key"
                    )
                if find_row is not None and keys:
                    raise QueueFrontendError(
                        "ACPY-RULE-009: runtime row find currently supports "
                        "first selection only"
                    )
                if any(
                    not isinstance(callback, ast.Lambda) or len(callback.args.args) != 1
                    for callback in (*where, *keys)
                ):
                    raise QueueFrontendError(
                        "ACPY-RULE-009: table.find where/key must be "
                        "one-argument lambdas"
                    )
                predicate_argument, predicate = _lambda_value(where[0])
                key_argument: str | None = None
                key: ast.expr | None = None
                if keys:
                    key_argument, key = _lambda_value(keys[0])
                if target.id in parameter_names or target.id in local_names:
                    raise QueueFrontendError(
                        "ACPY-RULE-009: find result requires a fresh local name"
                    )
                logical_name = target.id
                local_names.add(logical_name)
                version, _ = allocate_local_version(logical_name)
                rewritten_predicate = rewrite_local_loads(
                    predicate, excluded=frozenset({predicate_argument})
                )
                assert rewritten_predicate is not None
                rewritten_key = rewrite_local_loads(
                    key,
                    excluded=frozenset(() if key_argument is None else (key_argument,)),
                )
                rule_finds.append(
                    RuleFindDefinition(
                        version,
                        find_argument,
                        predicate_argument,
                        rewritten_predicate,
                        key_argument,
                        rewritten_key,
                        rewrite_local_loads(find_row),
                    )
                )
                continue
            if isinstance(target, ast.Name) and target.id in parameter_names:
                rewritten_value = rewrite_local_loads(statement.value)
                rewritten_guard = rewrite_local_loads(branch_guard)
                assert rewritten_value is not None
                prior_version = local_versions.get(target.id, target.id)
                version, _ = allocate_local_version(target.id)
                rule_locals.append(
                    RuleLocalDefinition(
                        version,
                        rewritten_value,
                        rewritten_guard,
                        branch_negated,
                        prior_version,
                    )
                )
                state_writes.append(
                    RuleStateWriteDefinition(
                        target.id,
                        None,
                        ast.Name(id=version, ctx=ast.Load()),
                        rewritten_guard,
                        branch_negated,
                    )
                )
            elif (
                isinstance(target, ast.Subscript)
                and isinstance(target.value, ast.Name)
                and target.value.id in parameter_names
            ):
                rewritten_index = rewrite_local_loads(target.slice)
                rewritten_value = rewrite_local_loads(statement.value)
                rewritten_guard = rewrite_local_loads(branch_guard)
                assert rewritten_index is not None
                assert rewritten_value is not None
                state_writes.append(
                    RuleStateWriteDefinition(
                        target.value.id,
                        rewritten_index,
                        rewritten_value,
                        rewritten_guard,
                        branch_negated,
                    )
                )
            elif isinstance(target, ast.Name):
                if target.id in parameter_names:
                    raise QueueFrontendError(
                        "ACPY-RULE-011: branch locals cannot replace rule parameters"
                    )
                logical_name = target.id
                is_rebind = logical_name in local_names
                prior_version = local_versions.get(logical_name)
                rewritten_value = rewrite_local_loads(statement.value)
                rewritten_guard = rewrite_local_loads(branch_guard)
                assert rewritten_value is not None
                version, allocated_prior = allocate_local_version(logical_name)
                assert allocated_prior == prior_version
                local_names.add(logical_name)
                if branch_guard is not None:
                    if prior_version is None:
                        partial_local_versions.add(version)
                        assert rewritten_guard is not None
                        partial_local_guards[version] = (
                            rewritten_guard,
                            branch_negated,
                        )
                    elif prior_version in partial_local_versions:
                        prior_guard = partial_local_guards.get(prior_version)
                        current_guard = (
                            rewritten_guard,
                            branch_negated,
                        )
                        if prior_guard is None or not guards_are_complementary(
                            prior_guard, current_guard
                        ):
                            partial_local_versions.add(version)
                            partial_local_guards[version] = (
                                prior_guard
                                if prior_guard is not None
                                and guard_literals(*prior_guard)
                                <= guard_literals(*current_guard)
                                else current_guard
                            )
                if (
                    not is_rebind
                    and branch_guard is None
                    and isinstance(statement.value, ast.Subscript)
                    and not any(
                        isinstance(candidate, ast.Name) and candidate.id in local_names
                        for candidate in ast.walk(statement.value.slice)
                    )
                ):
                    source = statement.value
                    if (
                        not isinstance(source.value, ast.Name)
                        or source.value.id not in parameter_names
                    ):
                        valid_multi_state = False
                        break
                    state_reads.append(
                        RuleStateReadDefinition(
                            version,
                            source.value.id,
                            rewrite_local_loads(source.slice),
                        )
                    )
                else:
                    rule_locals.append(
                        RuleLocalDefinition(
                            version,
                            rewritten_value,
                            rewritten_guard,
                            branch_negated,
                            prior_version,
                        )
                    )
            else:
                valid_multi_state = False
                break
        unconditionally_written_scalars = {
            write.argument
            for write in state_writes
            if write.index is None and write.guard is None
        }
        if unconditionally_written_scalars:
            last_scalar_write = {
                write.argument: index
                for index, write in enumerate(state_writes)
                if write.index is None
                and write.argument in unconditionally_written_scalars
            }
            state_writes = [
                (
                    replace(write, guard=None, guard_negated=False)
                    if write.index is None
                    and write.argument in unconditionally_written_scalars
                    and last_scalar_write[write.argument] == index
                    else write
                )
                for index, write in enumerate(state_writes)
                if write.index is not None
                or write.argument not in unconditionally_written_scalars
                or last_scalar_write[write.argument] == index
            ]
        state_names = {
            *(write.argument for write in state_writes),
            *(read.argument for read in state_reads),
            *(find.argument for find in rule_finds),
        }
        state_names.update(slot_parameter_names)
        state_reference_arguments = {
            candidate.value.id
            for local in rule_locals
            for candidate in ast.walk(local.value)
            if isinstance(candidate, ast.Subscript)
            and isinstance(candidate.value, ast.Name)
            and candidate.value.id in parameter_names
        }
        state_names.update(state_reference_arguments)
        rewritten_multi_return = rewrite_local_loads(multi_return)
        # A blocking/effect/output guard selects whether the transaction may
        # begin.  State parameters in that predicate therefore denote the
        # committed snapshot, even when the selected body proposes a new value
        # for the same owner.  Local (non-state) SSA values are still rewritten
        # normally.
        guard_state_names = frozenset(state_names)
        rewritten_multi_guard = rewrite_local_loads(
            multi_guard, excluded=guard_state_names
        )
        rewritten_multi_effect_guard = rewrite_local_loads(
            multi_effect_guard, excluded=guard_state_names
        )
        rewritten_multi_output_guard = rewrite_local_loads(
            multi_output_guard, excluded=guard_state_names
        )
        for local in rule_locals:
            referenced = {
                candidate.id
                for candidate in ast.walk(local.value)
                if isinstance(candidate, ast.Name)
            }
            if not guard_covers_partial_values(
                local.guard, local.guard_negated, referenced
            ):
                raise QueueFrontendError(
                    "ACPY-RULE-011: branch-local value escapes its defining path"
                )
        for write in state_writes:
            expressions = [write.value]
            if write.index is not None:
                expressions.append(write.index)
            referenced = {
                candidate.id
                for expression in expressions
                for candidate in ast.walk(expression)
                if isinstance(candidate, ast.Name)
            }
            if not guard_covers_partial_values(
                write.guard, write.guard_negated, referenced
            ):
                raise QueueFrontendError(
                    "ACPY-RULE-011: branch-local value escapes its defining path"
                )
        if rewritten_multi_return is not None:
            returned_names = {
                candidate.id
                for candidate in ast.walk(rewritten_multi_return)
                if isinstance(candidate, ast.Name)
            }
            if (
                partial_local_versions & returned_names
                and rewritten_multi_output_guard is None
            ):
                raise QueueFrontendError(
                    "ACPY-RULE-011: branch-local value escapes its defining path"
                )
        if rewritten_multi_guard is not None and any(
            write.guard is not None
            and not (
                guard_literals(rewritten_multi_guard, False)
                <= guard_literals(write.guard, write.guard_negated)
            )
            for write in state_writes
        ):
            raise QueueFrontendError(
                "ACPY-RULE-011: nested conditional state effects inside a "
                "blocking guard require explicit CFG implication proof"
            )
        for find in rule_finds:
            for expression in (find.predicate, find.key):
                if expression is None:
                    continue
                for candidate in ast.walk(expression):
                    if (
                        isinstance(candidate, ast.Subscript)
                        and isinstance(candidate.value, ast.Name)
                        and candidate.value.id in parameter_names
                    ):
                        state_names.add(candidate.value.id)
        if (
            rewritten_multi_return is not None
            and rewritten_multi_output_guard is not None
            and not guarded_statements
            and not slot_releases
            and set(parameter_names)
            == {
                candidate.value.id
                for candidate in ast.walk(node)
                if isinstance(candidate, ast.Attribute)
                and candidate.attr in {"valid", "value"}
                and isinstance(candidate.value, ast.Name)
                and candidate.value.id in parameter_names
            }
        ):
            rule_definitions[node.name] = RuleDefinition(
                node.name,
                (),
                rewritten_multi_return,
                node.lineno,
                node.col_offset + 1,
                output_guard=rewritten_multi_output_guard,
                state_arguments=parameter_names,
                slot_arguments=parameter_names,
                output_types=annotated_single_outputs,
            )
            continue
        ordered_state: tuple[str, ...] = ()
        if state_names:
            last_state = max(parameter_names.index(name) for name in state_names)
            ordered_state = parameter_names[: last_state + 1]
        if (
            (valid_multi_state or bool(slot_releases))
            and (
                state_writes
                or state_reads
                or rule_finds
                or slot_releases
                or (multi_return is not None and rule_locals)
            )
            and (
                len(ordered_state) >= 2
                or bool(rule_finds)
                or bool(state_reference_arguments)
                or bool(rule_locals)
                or has_branch_effects
                or rewritten_multi_guard is not None
                or rewritten_multi_output_guard is not None
                or (bool(state_reads) and not state_writes)
                or (bool(state_writes) and multi_return is None)
            )
        ):
            if parameter_names[: len(ordered_state)] != ordered_state:
                raise QueueFrontendError(
                    "ACPY-RULE-008: persistent rule parameters must precede "
                    "payload parameters"
                )
            payload_parameters = parameter_names[len(ordered_state) :]
            if (
                rewritten_multi_output_guard is not None
                and len(payload_parameters) != 1
                and not (
                    not payload_parameters
                    and bool(slot_parameter_names)
                    and set(ordered_state) == slot_parameter_names
                )
            ):
                raise QueueFrontendError(
                    "ACPY-RULE-012: optional output requires exactly one "
                    "payload parameter"
                )
            if (
                rewritten_multi_effect_guard is not None
                and len(payload_parameters) != 1
            ):
                raise QueueFrontendError(
                    "ACPY-RULE-010: conditional-effect early return requires "
                    "exactly one payload parameter"
                )
            rewritten_slot_releases = tuple(
                replace(release, guard=rewrite_local_loads(release.guard))
                for release in slot_releases
            )
            rule_definitions[node.name] = RuleDefinition(
                node.name,
                payload_parameters,
                rewritten_multi_return,
                node.lineno,
                node.col_offset + 1,
                guard=rewritten_multi_guard,
                effect_guard=rewritten_multi_effect_guard,
                output_guard=rewritten_multi_output_guard,
                state_arguments=ordered_state,
                slot_arguments=tuple(
                    name for name in ordered_state if name in slot_parameter_names
                ),
                slot_releases=rewritten_slot_releases,
                state_writes=tuple(state_writes),
                state_reads=tuple(state_reads),
                locals=tuple(rule_locals),
                finds=tuple(rule_finds),
                output_types=annotated_single_outputs,
            )
            continue

        if (
            len(node.args.args) >= 2
            and len(body) == 2
            and isinstance(body[0], ast.Assign)
            and len(body[0].targets) == 1
            and isinstance(body[0].targets[0], ast.Name)
            and body[0].targets[0].id == node.args.args[0].arg
            and isinstance(body[1], ast.Return)
            and body[1].value is not None
        ):
            rule_definitions[node.name] = RuleDefinition(
                node.name,
                tuple(argument.arg for argument in node.args.args[1:]),
                copy.deepcopy(body[1].value),
                node.lineno,
                node.col_offset + 1,
                var_argument=node.args.args[0].arg,
                var_value=copy.deepcopy(body[0].value),
                output_types=annotated_single_outputs,
            )
            continue

        table_argument = node.args.args[0].arg
        payload_arguments = tuple(argument.arg for argument in node.args.args[1:])
        payload_argument = payload_arguments[0] if payload_arguments else None
        return_expression: ast.expr | None = None
        if body and isinstance(body[-1], ast.Return):
            returned = body[-1].value
            if not (isinstance(returned, ast.Constant) and returned.value is None):
                return_expression = copy.deepcopy(returned)
            body = body[:-1]
        effect_guard_expression: ast.expr | None = None
        body, effect_guard_expression = _extract_conditional_effect_guard(
            body,
            tuple(argument.arg for argument in node.args.args),
            return_expression is not None,
        )
        guard_expression: ast.expr | None = None
        if body and isinstance(body[-1], ast.If):
            if effect_guard_expression is not None:
                raise QueueFrontendError(
                    "ACPY-RULE-010: conditional effects cannot also use a "
                    "blocking rule guard"
                )
            guarded = body[-1]
            if guarded.orelse or not guarded.body:
                raise QueueFrontendError(
                    "ACPY-RULE-007: guarded state rule requires one if body "
                    "without else"
                )
            guarded_body = list(guarded.body)
            if guarded_body and isinstance(guarded_body[-1], ast.Return):
                returned = guarded_body[-1].value
                if not (isinstance(returned, ast.Constant) and returned.value is None):
                    return_expression = copy.deepcopy(returned)
                guarded_body = guarded_body[:-1]
            guard_expression = copy.deepcopy(guarded.test)
            body = [*body[:-1], *guarded_body]
        read_statement: ast.Assign | None = None
        if len(body) == 2 and isinstance(body[0], ast.Assign):
            read_statement = body[0]
            body = body[1:]
        if (
            len(body) != 1
            or not isinstance(body[0], ast.Assign)
            or len(body[0].targets) != 1
            or not isinstance(body[0].targets[0], ast.Subscript)
            or not isinstance(body[0].targets[0].value, ast.Name)
            or body[0].targets[0].value.id != table_argument
        ):
            raise QueueFrontendError(
                "ACPY-RULE-002: pure rules require one value-returning path; "
                "stateful rules require one indexed state assignment and an "
                f"optional value return; rule {node.name!r} is unsupported"
            )
        if read_statement is not None and (
            len(read_statement.targets) != 1
            or not isinstance(read_statement.targets[0], ast.Name)
            or read_statement.targets[0].id
            in ({table_argument, payload_argument} - {None})
            or not isinstance(read_statement.value, ast.Subscript)
            or not isinstance(read_statement.value.value, ast.Name)
            or read_statement.value.value.id != table_argument
        ):
            raise QueueFrontendError(
                "ACPY-RULE-002: stateful rule observation must bind one "
                "Entry from the same Table"
            )
        assignment = body[0]
        assert isinstance(assignment.targets[0], ast.Subscript)
        read_name: str | None = None
        read_index: ast.expr | None = None
        if read_statement is not None:
            assert isinstance(read_statement.targets[0], ast.Name)
            assert isinstance(read_statement.value, ast.Subscript)
            read_name = read_statement.targets[0].id
            read_index = copy.deepcopy(read_statement.value.slice)
        if effect_guard_expression is not None and len(payload_arguments) != 1:
            raise QueueFrontendError(
                "ACPY-RULE-010: conditional-effect early return requires "
                "exactly one payload parameter"
            )
        rule_definitions[node.name] = RuleDefinition(
            node.name,
            payload_arguments,
            return_expression,
            node.lineno,
            node.col_offset + 1,
            table_argument,
            copy.deepcopy(assignment.targets[0].slice),
            copy.deepcopy(assignment.value),
            read_name,
            read_index,
            guard=guard_expression,
            effect_guard=effect_guard_expression,
            output_types=annotated_single_outputs,
        )
    if definition_locations:
        rule_definitions = {
            name: (
                replace(
                    definition,
                    source_path=definition_locations[name][0],
                    source_line=definition_locations[name][1],
                    source_column=definition_locations[name][2],
                )
                if name in definition_locations
                else definition
            )
            for name, definition in rule_definitions.items()
        }
    candidates = [
        node
        for node in tree.body
        if isinstance(node, ast.FunctionDef)
        and node.name == system
        and any(
            _decorator_name(d).rsplit(".", 1)[-1] == entry_kind
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
    parameter_sources = {
        parameter.arg: source_frame(parameter) for parameter in parameters
    }
    supplied = dict(static_arguments or {})
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
    static_parameter_names: set[str] = set()
    external_parameters: list[tuple[str, ValueType]] = []
    for parameter in parameters:
        annotation_name = (
            _decorator_name(parameter.annotation.value).rsplit(".", 1)[-1]
            if isinstance(parameter.annotation, ast.Subscript)
            else ""
        )
        if annotation_name != "const":
            if parameter.arg in supplied:
                raise QueueFrontendError(
                    "ACPY-QUEUE-022: supplied static arguments must use ac.const"
                )
            if (
                parameter.arg in positional_defaults
                or parameter.arg in keyword_defaults
            ):
                raise QueueFrontendError(
                    "ACPY-QUEUE-022: external system values cannot have defaults"
                )
            payload = _payload(
                parameter.annotation,
                payload_map,
                enum_map,
                static_values=type_static_values,
            )
            if entry_kind == "system" and _contains_declared_range(payload):
                raise QueueFrontendError(
                    "ACPY-TYPE-009: external bounded input requires an explicit "
                    "checked, wrap, or saturate decoder from bits"
                )
            external_parameters.append((parameter.arg, payload))
            continue
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
    extras = sorted(set(supplied) - static_parameter_names)
    if extras:
        raise QueueFrontendError(
            f"ACPY-QUEUE-001: unknown static argument {extras[0]!r}"
        )
    system_static_values: Mapping[str, StaticValue] = {
        **module_static_values,
        **supplied,
    }
    _strip_static_assertions(
        function,
        system_static_values,
        normalized_source_path,
        definition_locations,
        static_assert_locations,
    )
    def system_result_payloads(
        annotation: ast.expr | None,
    ) -> tuple[ValueType, ...] | None:
        if annotation is None:
            return None
        if (isinstance(annotation, ast.Constant) and annotation.value is None) or (
            isinstance(annotation, ast.Name) and annotation.id == "None"
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
            if not elements:
                raise QueueFrontendError(
                    "ACPY-QUEUE-026: system result tuple cannot be empty"
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

    result_payloads = system_result_payloads(function.returns)
    interface_owner = (
        "module." + static_type_namespace[:-2]
        if static_type_namespace[-2:] == "__"
        else f"{entry_kind}.{system}"
    )
    interface_type_checks: list[StaticTypeCheck] = []
    for parameter in parameters:
        if (
            isinstance(parameter.annotation, ast.Subscript)
            and _decorator_name(parameter.annotation.value).rsplit(".", 1)[-1]
            == "const"
        ):
            continue
        check = _scalar_annotation_static_check(
            f"interface.{interface_owner}.input.{parameter.arg}",
            parameter.annotation,
            parameter_aliases,
            type_static_values,
            binding_namespace=static_type_namespace,
        )
        if check is not None:
            interface_type_checks.append(check)
        interface_type_checks.extend(
            _bounded_annotation_static_checks(
                f"interface.{interface_owner}.input.{parameter.arg}",
                parameter.annotation,
                parameter_aliases,
                type_static_values,
                binding_namespace=static_type_namespace,
            )
        )
    result_annotations = (
        ()
        if function.returns is None
        else (
            tuple(function.returns.slice.elts)
            if isinstance(function.returns, ast.Subscript)
            and _decorator_name(function.returns.value).rsplit(".", 1)[-1]
            in {"tuple", "Tuple"}
            and isinstance(function.returns.slice, ast.Tuple)
            else (function.returns,)
        )
    )
    for index, annotation in enumerate(result_annotations):
        check = _scalar_annotation_static_check(
            f"interface.{interface_owner}.output.{index}",
            annotation,
            parameter_aliases,
            type_static_values,
            binding_namespace=static_type_namespace,
        )
        if check is not None:
            interface_type_checks.append(check)
        interface_type_checks.extend(
            _bounded_annotation_static_checks(
                f"interface.{interface_owner}.output.{index}",
                annotation,
                parameter_aliases,
                type_static_values,
                binding_namespace=static_type_namespace,
            )
        )
    typed_result_payloads: dict[str, ValueType] = {}
    if entry_kind == "module" and result_payloads:
        returned = next(
            (
                statement.value
                for statement in reversed(function.body)
                if isinstance(statement, ast.Return) and statement.value is not None
            ),
            None,
        )
        returned_values = (
            tuple(returned.elts)
            if isinstance(returned, (ast.Tuple, ast.List))
            else (returned,)
            if returned is not None
            else ()
        )
        if len(returned_values) == len(result_payloads) and all(
            isinstance(value, ast.Name) for value in returned_values
        ):
            typed_result_payloads = {
                value.id: payload
                for value, payload in zip(returned_values, result_payloads, strict=True)
                if isinstance(value, ast.Name)
            }

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

    def specialize_rule_call(
        definition: RuleDefinition, call: ast.Call, prefix: int
    ) -> tuple[RuleDefinition, ast.Call]:
        if call.keywords or len(call.args) != prefix + len(definition.arguments):
            return definition, call
        static_values: dict[str, StaticValue] = {}
        runtime_arguments: list[str] = []
        runtime_values: list[ast.expr] = []
        for argument, value in zip(
            definition.arguments, call.args[prefix:], strict=True
        ):
            try:
                static_value = evaluate_static(
                    value, StaticEnvironment(system_static_values)
                )
            except ValueError:
                runtime_arguments.append(argument)
                runtime_values.append(value)
            else:
                static_values[argument] = static_value
        if not static_values and not module_static_values:
            return definition, call

        constant_values = {**module_static_values, **static_values}
        for argument in runtime_arguments:
            constant_values.pop(argument, None)

        def constantize(value: ast.expr | None) -> ast.expr | None:
            if value is None:
                return None
            return _constantize_expression(value, "", constant_values)

        specialized = replace(
            definition,
            arguments=tuple(runtime_arguments),
            expression=constantize(definition.expression),
            table_index=constantize(definition.table_index),
            table_value=constantize(definition.table_value),
            table_read_index=constantize(definition.table_read_index),
            var_value=constantize(definition.var_value),
            guard=constantize(definition.guard),
            effect_guard=constantize(definition.effect_guard),
            output_guard=constantize(definition.output_guard),
            state_writes=tuple(
                replace(
                    write,
                    index=constantize(write.index),
                    value=constantize(write.value),
                    guard=constantize(write.guard),
                )
                for write in definition.state_writes
            ),
            state_reads=tuple(
                replace(read, index=constantize(read.index))
                for read in definition.state_reads
            ),
            locals=tuple(
                replace(
                    local,
                    value=constantize(local.value),
                    guard=constantize(local.guard),
                )
                for local in definition.locals
            ),
            finds=tuple(
                replace(
                    find,
                    predicate=constantize(find.predicate),
                    key=constantize(find.key),
                    row=constantize(find.row),
                )
                for find in definition.finds
            ),
        )
        specialized_call = copy.deepcopy(call)
        specialized_call.args = [*call.args[:prefix], *runtime_values]
        return specialized, specialized_call

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
    parser_environment = _ParserEnvironment(
        system_static_values,
        tuple(payloads),
        result_payloads,
    )
    parser_state = _ParserState()
    queues = parser_state.queues
    effect_rules = parser_state.effect_rules
    scopes = parser_state.scopes
    routes = parser_state.routes
    forks = parser_state.forks
    feedbacks = parser_state.feedbacks
    merges = parser_state.merges
    reorders = parser_state.reorders
    dependencies = parser_state.dependencies
    credits = parser_state.credits
    barriers = parser_state.barriers
    selects = parser_state.selects
    memory_instances = parser_state.memory_instances
    memory_requests = parser_state.memory_requests
    memories = parser_state.memories
    variables = parser_state.variables
    tables = parser_state.tables
    table_reads = parser_state.table_reads
    table_writes = parser_state.table_writes
    masked_table_writes = parser_state.masked_table_writes
    arbitration_descriptors = parser_state.arbitration_descriptors
    arbitration_owners = parser_state.arbitration_owners
    slots = parser_state.slots
    slot_releases = parser_state.slot_releases
    candidates = parser_state.candidates
    selections = parser_state.selections
    table_by_name = parser_state.table_by_name
    variable_by_name = parser_state.variable_by_name
    entry_views = parser_state.entry_views
    slot_by_name = parser_state.slot_by_name
    candidate_by_name = parser_state.candidate_by_name
    selection_by_name = parser_state.selection_by_name
    selection_tuple_aliases = parser_state.selection_tuple_aliases
    selection_lane_ordinals = parser_state.selection_lane_ordinals
    memory_by_name = parser_state.memory_by_name
    memory_arrays = parser_state.memory_arrays
    selected_memories = parser_state.selected_memories
    consumed_selected_memories = parser_state.consumed_selected_memories
    sinks = parser_state.sinks
    observations = parser_state.observations
    expectations = parser_state.expectations
    by_name = parser_state.by_name
    collections = parser_state.collections
    collection_bindings = parser_state.collection_bindings
    statement_sources = parser_state.statement_sources

    for name, payload in external_parameters:
        if name in by_name:
            raise QueueFrontendError(
                "ACPY-QUEUE-026: external system values require unique names"
            )
        binding = QueueBinding(
            name,
            payload,
            1,
            1,
            None,
            scope=(),
            order=parser_state.order,
            provider="boundary",
            source=parameter_sources.get(name),
        )
        queues.append(binding)
        by_name[name] = binding
        parser_state.order += 1

    def call_name(call: ast.Call) -> str:
        return _decorator_name(call.func).rsplit(".", 1)[-1]

    def rewrite_selection_tuple_refs(statement: ast.stmt) -> ast.stmt:
        class StaticSelectionTupleRefs(ast.NodeTransformer):
            def visit_Subscript(self, node: ast.Subscript) -> ast.expr:
                if (
                    isinstance(node.value, ast.Name)
                    and (aliases := selection_tuple_aliases.get(node.value.id))
                    is not None
                ):
                    index = _constant_integer(node.slice, system_static_values)
                    if index is None:
                        raise QueueFrontendError(
                            "ACPY-TABLE-012: TableChoice tuple index must be static"
                        )
                    if not 0 <= index < len(aliases):
                        raise QueueFrontendError(
                            "ACPY-TABLE-012: TableChoice tuple index is out of range"
                        )
                    return ast.copy_location(
                        ast.Name(id=aliases[index], ctx=node.ctx), node
                    )
                return self.generic_visit(node)

            def visit_Name(self, node: ast.Name) -> ast.expr:
                if (
                    isinstance(node.ctx, ast.Load)
                    and node.id in selection_tuple_aliases
                ):
                    raise QueueFrontendError(
                        "ACPY-TABLE-012: TableChoice tuple cannot be iterated, "
                        "stored, or escape its static scope"
                    )
                return node

        return ast.fix_missing_locations(StaticSelectionTupleRefs().visit(statement))

    def normalized_write_fields(
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
        table: TableBinding,
        argument: str,
        value: ast.expr | None,
        write_index: ast.expr | None,
        reads: tuple[RuleStateReadDefinition, ...] = (),
        locals: tuple[RuleLocalDefinition, ...] = (),
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
            for local in locals
            if local.guard is None and not local.guard_negated
        }

        class Substitute(ast.NodeTransformer):
            def __init__(self, replacements: Mapping[str, ast.expr]) -> None:
                self.replacements = replacements

            def visit_Name(self, node: ast.Name) -> ast.expr:
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
                helper = helper_map.get(root.func.id)
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

    def complete_value_fields(value_type: ValueType) -> tuple[str, ...]:
        if not isinstance(value_type, StructType):
            return ("$entry",)
        return tuple(field.name for field in value_type.fields)

    def state_value_type(owner: VarStateBinding | TableBinding) -> ValueType:
        return (
            owner.value_type if isinstance(owner, VarStateBinding) else owner.entry_type
        )

    def state_owner_kind(owner: VarStateBinding | TableBinding) -> str:
        return "var" if isinstance(owner, VarStateBinding) else "table"

    def state_write_fields(owner: VarStateBinding | TableBinding) -> tuple[str, ...]:
        return complete_value_fields(state_value_type(owner))

    def table_key_ordering(
        table: TableBinding, argument: str, expression: ast.expr
    ) -> str:
        if not (
            isinstance(table.entry_type, StructType)
            and isinstance(expression, ast.Attribute)
            and isinstance(expression.value, ast.Name)
            and expression.value.id == argument
        ):
            return "unsigned"
        for declaration in tree.body:
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

    def writer_priority_rank(policy: ast.expr, diagnostic: str) -> int:
        if (
            not isinstance(policy, ast.Call)
            or _decorator_name(policy.func).rsplit(".", 1)[-1] != "writer_priority"
            or len(policy.args) != 1
            or policy.keywords
        ):
            raise QueueFrontendError(
                f"{diagnostic}: arbitration requires ac.writer_priority(rank)"
            )
        rank = _constant_integer(policy.args[0], system_static_values)
        if rank is None or rank < 0:
            raise QueueFrontendError(
                f"{diagnostic}: writer priority rank must be a non-negative "
                "static integer"
            )
        return rank

    def table_writer_arbitration(
        call: ast.Call, diagnostic: str, table: str
    ) -> int | None:
        policies = [
            keyword.value for keyword in call.keywords if keyword.arg == "arbitration"
        ]
        if len(policies) > 1:
            raise QueueFrontendError(f"{diagnostic}: repeated arbitration policy")
        if not policies:
            return None
        policy = policies[0]
        if isinstance(policy, ast.Name) and policy.id in arbitration_descriptors:
            owner = arbitration_owners.setdefault(policy.id, table)
            if owner != table:
                raise QueueFrontendError(
                    f"{diagnostic}: arbitration descriptor {policy.id!r} cannot "
                    "cross Table owners"
                )
            return arbitration_descriptors[policy.id]
        return writer_priority_rank(policy, diagnostic)

    def reject_overlapping_table_writer(
        table: str,
        write_fields: tuple[str, ...],
        write_mode: str,
        arbitration_rank: int | None,
    ) -> None:
        requested = set(write_fields)
        for write in (*table_writes, *masked_table_writes):
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
            _constant_integer(extent, system_static_values) for extent in extent_nodes
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
            payload_map,
            enum_map,
            static_values=type_static_values,
        )
        if call.args or any(
            keyword.arg is None or keyword.arg != "init" for keyword in call.keywords
        ):
            raise QueueFrontendError("ACPY-TABLE-001: table accepts only keyword init")
        init_values = [keyword.value for keyword in call.keywords]
        if len(init_values) > 1:
            raise QueueFrontendError("ACPY-TABLE-011: Table init is repeated")
        init_node = init_values[0] if init_values else ast.Constant(0)
        if _constant_integer(init_node, system_static_values) == 0:
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
        if _constant_integer(image_fields["version"], system_static_values) != 1:
            raise QueueFrontendError(
                "ACPY-TABLE-011: typed image version must be exactly 1"
            )
        image_entry = _payload(
            image_fields["entry"],
            payload_map,
            enum_map,
            static_values=type_static_values,
        )
        if image_entry != entry_type:
            raise QueueFrontendError(
                "ACPY-TABLE-011: typed image Entry type must match the Table"
            )
        values_node = image_fields["values"]
        if not isinstance(values_node, (ast.List, ast.Tuple)):
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
                value = _constant_integer(node, system_static_values)
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
                node, (ast.List, ast.Tuple)
            ):
                if len(node.elts) == len(descriptor.elements):
                    return [
                        canonical_image_value(value, value_type)
                        for value, value_type in zip(
                            node.elts, descriptor.elements, strict=True
                        )
                    ]
            elif isinstance(descriptor, ArrayType) and isinstance(
                node, (ast.List, ast.Tuple)
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
        projected_source = entry_views.get(source_name)
        if source_name in table_by_name:
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
        if isinstance(selector, ast.Name) and selector.id in candidate_by_name:
            candidate = candidate_by_name[selector.id]
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
            and selector.value.id in selection_by_name
            and selection_by_name[selector.value.id].table != table_name
        ):
            raise QueueFrontendError(
                "ACPY-TABLE-007: Selection belongs to a different Table"
            )
        if isinstance(selector, ast.Lambda):
            argument, address = _lambda(selector)
        else:
            argument, address = (
                None,
                _constantize_expression(selector, "", system_static_values),
            )
        table = table_by_name[table_name]
        flattened_choice_index = (
            isinstance(address, ast.Attribute)
            and isinstance(address.value, ast.Name)
            and address.attr == "index"
            and address.value.id in selection_by_name
            and selection_by_name[address.value.id].table == table_name
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
            _constant_integer(coordinate, system_static_values)
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
        node: ast.expr,
        scope_path: tuple[str, ...],
        current_order: int,
    ) -> EntryViewBinding | MaskedEntryViewBinding | ProjectedTableViewBinding | None:
        if isinstance(node, ast.Name):
            view = entry_views.get(node.id)
            if view and view.scope == scope_path:
                return view
            return None
        return parse_view(node, "", scope_path, current_order)

    def lambda_or_constant(node: ast.expr, argument: str, diagnostic: str) -> ast.expr:
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
            (item for item in payloads if item.descriptor == queue.payload), None
        )
        if payload is None or node.value.id != payload.name:
            raise QueueFrontendError(
                "ACPY-QUEUE-024: field descriptor payload does not match Queue"
            )
        if node.attr not in {field.name for field in payload.descriptor.fields}:
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

    def is_queue_reference_syntax(
        node: ast.expr,
        aliases: dict[str, str | StaticQueueCollection],
    ) -> bool:
        return isinstance(node, ast.Subscript) or (
            isinstance(node, ast.Name)
            and (node.id in by_name or node.id in collections or node.id in aliases)
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
        lanes = _positive_int(call, "lanes", 1, static_values)
        if rate > depth:
            raise QueueFrontendError("ACPY-QUEUE-025: Queue rate must not exceed depth")
        if rate > lanes:
            raise QueueFrontendError("ACPY-QUEUE-025: Queue rate must not exceed lanes")
        return QueueBinding(
            name,
            _payload(
                call.args[0],
                payload_map,
                enum_map,
                static_values=type_static_values,
            ),
            depth,
            _positive_int(call, "latency", 1, static_values),
            None,
            scope=scope_path,
            order=current_order,
            rate=rate,
            lanes=lanes,
            source=source_frame(call),
        )

    def memory_instance_binding(
        name: str,
        call: ast.Call,
        scope_path: tuple[str, ...],
        current_order: int,
        static_values: dict[str, int] | None = None,
    ) -> MemoryInstanceBinding:
        if call_name(call) != "memory" or len(call.args) != 1:
            raise QueueFrontendError("ACPY-QUEUE-015: memory requires one data type")
        if any(
            keyword.arg is None or keyword.arg not in {"entries", "init", "latency"}
            for keyword in call.keywords
        ):
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory instance has an unsupported keyword"
            )
        data_type = _payload(
            call.args[0],
            payload_map,
            enum_map,
            static_values=type_static_values,
        )
        if _epoch_05_integer_width(data_type) is None:
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory data type must be an integer"
            )
        entries = _positive_int(call, "entries", 16, static_values)
        init = _nonnegative_int(call, "init", 0, static_values)
        latency = _positive_int(call, "latency", 1, static_values)
        if init != 0:
            raise QueueFrontendError("ACPY-QUEUE-015: memory init must be zero")
        return MemoryInstanceBinding(
            name,
            data_type,
            entries,
            init,
            latency,
            scope_path,
            current_order,
            source_frame(call),
        )

    def memory_request_parameters(
        call: ast.Call,
        incoming: QueueBinding,
        data_type: ValueType,
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
                    f"ACPY-QUEUE-015: memory request requires one {policy} lambda"
                )
            policies[policy] = values[0]
        arguments_and_values = [_lambda(policies[item]) for item in policies]
        if len({argument for argument, _ in arguments_and_values}) != 1:
            raise QueueFrontendError(
                "ACPY-QUEUE-015: memory request lambdas require one argument name"
            )
        result_fields = [
            keyword.value for keyword in call.keywords if keyword.arg == "result_field"
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
                if declaration.descriptor == incoming.payload
            ),
            None,
        )
        result_field = result_fields[0].value
        field_types = dict(payload.field_descriptors) if payload is not None else {}
        if result_field not in field_types:
            raise QueueFrontendError("ACPY-QUEUE-015: memory result_field is unknown")
        if not _types_equal_in_epoch_05(field_types[result_field], data_type):
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
    ) -> None:
        aliases = {} if aliases is None else aliases
        for statement in statements:
            statement = rewrite_selection_tuple_refs(statement)
            if (
                isinstance(statement, ast.Expr)
                and isinstance(statement.value, ast.Constant)
                and isinstance(statement.value.value, str)
            ):
                continue
            current_order = parser_state.order
            parser_state.order += 1
            if (frame := source_frame(statement)) is not None:
                statement_sources[current_order] = frame
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Name)
                and isinstance(statement.value, ast.Call)
                and _decorator_name(statement.value.func).rsplit(".", 1)[-1]
                == "writer_priority"
            ):
                name = statement.targets[0].id
                if name in arbitration_descriptors:
                    raise QueueFrontendError(
                        "ACPY-TABLE-011: arbitration descriptor requires a fresh name"
                    )
                arbitration_descriptors[name] = writer_priority_rank(
                    statement.value, "ACPY-TABLE-011"
                )
                continue
            if (
                isinstance(statement, ast.AnnAssign)
                and isinstance(statement.target, ast.Name)
                and statement.value is not None
            ):
                name = statement.target.id
                if name in variable_by_name or name in by_name:
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
                        payload_map,
                        enum_map,
                        type_static_values,
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
                if isinstance(
                    value_type, (StructType, TupleType, ArrayType, EnumType)
                ) and (type(init) is not int or init != 0):
                    raise QueueFrontendError(
                        "ACPY-VAR-001: persistent struct init must be zero"
                    )
                if not isinstance(
                    value_type, (StructType, TupleType, ArrayType, EnumType)
                ):
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
                variables.append(binding)
                variable_by_name[name] = binding
                continue
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
                or name in slot_by_name
                or name in candidate_by_name
                or name in selection_by_name
                for name in assigned_names
            ):
                raise QueueFrontendError(
                    "ACPY-QUEUE-015: state binding cannot be rebound"
                )
            selection_unpack: tuple[str, ...] = ()
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], (ast.Tuple, ast.List))
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Attribute)
                and statement.value.func.attr == "choose"
            ):
                if not all(
                    isinstance(item, ast.Name) for item in statement.targets[0].elts
                ):
                    raise QueueFrontendError(
                        "ACPY-TABLE-012: TableChoice tuple unpack requires names"
                    )
                selection_unpack = tuple(
                    item.id
                    for item in statement.targets[0].elts
                    if isinstance(item, ast.Name)
                )
                statement.targets[0] = ast.copy_location(
                    ast.Name(id=f"__table_selection_{current_order}", ctx=ast.Store()),
                    statement.targets[0],
                )
            if (
                isinstance(statement, ast.Assign)
                and len(statement.targets) == 1
                and isinstance(statement.targets[0], ast.Name)
                and isinstance(statement.value, ast.Call)
            ):
                call = statement.value
                declaration = table_declaration(statement.value)
                if declaration is not None:
                    name = statement.targets[0].id
                    if (
                        name in by_name
                        or name in collections
                        or name in table_by_name
                        or name in variable_by_name
                    ):
                        raise QueueFrontendError(
                            "ACPY-TABLE-001: table declaration requires a fresh name"
                        )
                    entries, shape, entry_type, init_image = declaration
                    if entry_kind == "module":
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
                        variables.append(variable)
                        variable_by_name[name] = variable
                        continue
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
                    tables.append(binding)
                    table_by_name[name] = binding
                    continue
                if call_name(call) == "slot":
                    if len(call.args) != 1 or call.keywords:
                        raise QueueFrontendError(
                            "ACPY-SLOT-001: ac.slot requires exactly one Queue"
                        )
                    name = statement.targets[0].id
                    if name in by_name or name in collections or name in slot_by_name:
                        raise QueueFrontendError(
                            "ACPY-SLOT-001: slot declaration requires a fresh name"
                        )
                    input_name = queue_reference(call.args[0], aliases)
                    binding = SlotBinding(
                        name,
                        input_name,
                        by_name[input_name].payload,
                        scope_path,
                        current_order,
                    )
                    slots.append(binding)
                    slot_by_name[name] = binding
                    continue
                if (
                    isinstance(call.func, ast.Attribute)
                    and call.func.attr == "match"
                    and isinstance(call.func.value, ast.Name)
                    and (
                        call.func.value.id in table_by_name
                        or isinstance(
                            entry_views.get(call.func.value.id),
                            ProjectedTableViewBinding,
                        )
                    )
                ):
                    name = statement.targets[0].id
                    projected = entry_views.get(call.func.value.id)
                    table_name = (
                        projected.table
                        if isinstance(projected, ProjectedTableViewBinding)
                        else call.func.value.id
                    )
                    table = table_by_name[table_name]
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
                    argument, predicate = _lambda(call.args[0])
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
                    candidates.append(binding)
                    candidate_by_name[name] = binding
                    continue
                if (
                    isinstance(call.func, ast.Attribute)
                    and call.func.attr == "choose"
                    and isinstance(call.func.value, ast.Name)
                    and (
                        call.func.value.id in table_by_name
                        or isinstance(
                            entry_views.get(call.func.value.id),
                            ProjectedTableViewBinding,
                        )
                    )
                ):
                    name = statement.targets[0].id
                    projected = entry_views.get(call.func.value.id)
                    table_name = (
                        projected.table
                        if isinstance(projected, ProjectedTableViewBinding)
                        else call.func.value.id
                    )
                    if len(call.args) != 1 or not isinstance(call.args[0], ast.Name):
                        raise QueueFrontendError(
                            "ACPY-TABLE-007: table.choose requires one CandidateSet"
                        )
                    candidate = candidate_by_name.get(call.args[0].id)
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
                            "ACPY-TABLE-007: CandidateSet belongs to a different "
                            "Table view"
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
                                "ACPY-TABLE-007: first/round_robin policy does not "
                                "accept key"
                            )
                    else:
                        if key_node is None:
                            raise QueueFrontendError(
                                "ACPY-TABLE-007: min/max policy requires key lambda"
                            )
                        key_argument, key = _lambda(key_node)
                        key_ordering = table_key_ordering(
                            table_by_name[table_name], key_argument, key
                        )
                    initial_cursor = _nonnegative_int(call, "initial_cursor", 0)
                    if initial_cursor >= candidate.entries:
                        raise QueueFrontendError(
                            "ACPY-TABLE-012: initial cursor is outside the "
                            "candidate domain"
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
                        aliases = tuple(f"{name}__lane{lane}" for lane in range(count))
                        selection_tuple_aliases[name] = aliases
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
                    selections.append(binding)
                    for lane, alias in enumerate(aliases):
                        selection_by_name[alias] = binding
                        selection_lane_ordinals[alias] = lane
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
                isinstance(statement, ast.Expr)
                and isinstance(statement.value, ast.Call)
                and isinstance(statement.value.func, ast.Attribute)
                and statement.value.func.attr == "release"
                and isinstance(statement.value.func.value, ast.Name)
                and statement.value.func.value.id in slot_by_name
            ):
                call = statement.value
                slot_name = call.func.value.id
                if call.args or any(
                    keyword.arg is None or keyword.arg != "when"
                    for keyword in call.keywords
                ):
                    raise QueueFrontendError(
                        "ACPY-SLOT-002: slot.release accepts only when=expression"
                    )
                values = [
                    keyword.value for keyword in call.keywords if keyword.arg == "when"
                ]
                if len(values) != 1 or isinstance(values[0], ast.Lambda):
                    raise QueueFrontendError(
                        "ACPY-SLOT-002: slot.release requires one state expression"
                    )
                if any(release.slot == slot_name for release in slot_releases):
                    raise QueueFrontendError(
                        "ACPY-SLOT-002: slot permits exactly one release endpoint"
                    )
                slot_releases.append(
                    SlotReleaseBinding(
                        slot_name,
                        _constantize_expression(values[0], "", system_static_values),
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
                and statement.value.func.attr == "read"
            ):
                call = statement.value
                view = resolve_view(call.func.value, scope_path, current_order)
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
                and statement.value.func.attr in {"write", "patch", "allocate"}
            ):
                call = statement.value
                view = resolve_view(call.func.value, scope_path, current_order)
                if view is not None:
                    method = call.func.attr
                    if isinstance(view, ProjectedTableViewBinding):
                        raise QueueFrontendError(
                            "ACPY-TABLE-004: projected Table view requires a "
                            "complete index before write"
                        )
                    arbitration_rank = table_writer_arbitration(
                        call, "ACPY-TABLE-011", view.table
                    )
                    if isinstance(view, MaskedEntryViewBinding):
                        if method == "allocate":
                            raise QueueFrontendError(
                                "ACPY-TABLE-009: allocation requires a scalar view"
                            )
                        if call.args:
                            raise QueueFrontendError(
                                "ACPY-TABLE-008: masked write/patch is state-driven "
                                "and takes no Queue"
                            )
                        enable_values = [
                            keyword.value
                            for keyword in call.keywords
                            if keyword.arg == "enable"
                        ]
                        if len(enable_values) > 1:
                            raise QueueFrontendError(
                                "ACPY-TABLE-008: repeated masked write enable"
                            )
                        enable_node = (
                            enable_values[0] if enable_values else ast.Constant(True)
                        )
                        if isinstance(enable_node, ast.Lambda):
                            raise QueueFrontendError(
                                "ACPY-TABLE-008: masked enable must be an expression"
                            )
                        enable = _constantize_expression(
                            enable_node, "", system_static_values
                        )
                        table = table_by_name[view.table]
                        value: ast.expr | None = None
                        patch_fields: tuple[tuple[str, ast.expr], ...] = ()
                        if method == "write":
                            if any(
                                keyword.arg is None
                                or keyword.arg not in {"value", "enable", "arbitration"}
                                for keyword in call.keywords
                            ):
                                raise QueueFrontendError(
                                    "ACPY-TABLE-008: masked write accepts only "
                                    "value and enable"
                                )
                            values = [
                                keyword.value
                                for keyword in call.keywords
                                if keyword.arg == "value"
                            ]
                            if len(values) != 1:
                                raise QueueFrontendError(
                                    "ACPY-TABLE-008: masked write requires one value"
                                )
                            if isinstance(values[0], ast.Lambda):
                                raise QueueFrontendError(
                                    "ACPY-TABLE-008: masked write value must be a "
                                    "uniform expression, not a lambda"
                                )
                            value = _constantize_expression(
                                values[0], "", system_static_values
                            )
                        else:
                            if not isinstance(table.entry_type, StructType):
                                raise QueueFrontendError(
                                    "ACPY-TABLE-008: masked patch requires a struct "
                                    "Table Entry"
                                )
                            field_types = {
                                field.name: field.type
                                for field in table.entry_type.fields
                            }
                            patches: list[tuple[str, ast.expr]] = []
                            for keyword in call.keywords:
                                if keyword.arg in {"enable", "arbitration"}:
                                    continue
                                if (
                                    keyword.arg is None
                                    or keyword.arg not in field_types
                                ):
                                    raise QueueFrontendError(
                                        "ACPY-TABLE-008: masked patch field is unknown"
                                    )
                                expression = keyword.value
                                if isinstance(expression, ast.Lambda):
                                    old_name, expression = _lambda(expression)

                                    class OldEntryName(ast.NodeTransformer):
                                        def visit_Name(
                                            self, node: ast.Name
                                        ) -> ast.expr:
                                            if node.id == old_name:
                                                return ast.copy_location(
                                                    ast.Name(
                                                        id="__old",
                                                        ctx=node.ctx,
                                                    ),
                                                    node,
                                                )
                                            return node

                                    expression = OldEntryName().visit(
                                        copy.deepcopy(expression)
                                    )
                                else:
                                    expression = _constantize_expression(
                                        expression, "", system_static_values
                                    )
                                patches.append((keyword.arg, expression))
                            if not patches:
                                raise QueueFrontendError(
                                    "ACPY-TABLE-008: masked patch requires at least "
                                    "one field"
                                )
                            if len({name for name, _ in patches}) != len(patches):
                                raise QueueFrontendError(
                                    "ACPY-TABLE-008: masked patch field is repeated"
                                )
                            patch_fields = tuple(patches)
                        write_fields = normalized_write_fields(
                            table, value, patch_fields
                        )
                        reject_overlapping_table_writer(
                            view.table, write_fields, "field", arbitration_rank
                        )
                        masked_table_writes.append(
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
                        continue
                    queue_driven = view.argument is not None
                    if method == "allocate" and queue_driven:
                        raise QueueFrontendError(
                            "ACPY-TABLE-009: allocation must be state-driven"
                        )
                    if len(call.args) != (1 if queue_driven else 0):
                        raise QueueFrontendError(
                            "ACPY-TABLE-004: Queue-driven table write/patch requires "
                            "one Queue; state-driven write/patch takes no Queue"
                        )
                    input_name = (
                        queue_reference(call.args[0], aliases) if queue_driven else None
                    )
                    argument = view.argument
                    enable_values = [
                        keyword.value
                        for keyword in call.keywords
                        if keyword.arg == "enable"
                    ]
                    if len(enable_values) > 1:
                        raise QueueFrontendError(
                            "ACPY-TABLE-004: repeated write enable"
                        )
                    enable_node = (
                        enable_values[0] if enable_values else ast.Constant(True)
                    )
                    if queue_driven:
                        assert argument is not None
                        enable = lambda_or_constant(
                            enable_node,
                            argument,
                            "ACPY-TABLE-004: selector and enable lambdas require "
                            "one argument name",
                        )
                    else:
                        if isinstance(enable_node, ast.Lambda):
                            raise QueueFrontendError(
                                "ACPY-TABLE-004: state-driven enable must be an "
                                "expression, not a lambda"
                            )
                        enable = _constantize_expression(
                            enable_node, "", system_static_values
                        )
                    value: ast.expr | None = None
                    patch_fields: tuple[tuple[str, ast.expr], ...] = ()
                    table = table_by_name[view.table]
                    if method in {"write", "allocate"}:
                        if any(
                            keyword.arg is None
                            or keyword.arg not in {"value", "enable", "arbitration"}
                            for keyword in call.keywords
                        ):
                            raise QueueFrontendError(
                                "ACPY-TABLE-004: write/allocation accepts only "
                                "value and enable"
                            )
                        values = [
                            keyword.value
                            for keyword in call.keywords
                            if keyword.arg == "value"
                        ]
                        if len(values) != 1:
                            raise QueueFrontendError(
                                "ACPY-TABLE-004: write/allocation requires one value"
                            )
                        if queue_driven:
                            assert argument is not None
                            value = lambda_or_constant(
                                values[0],
                                argument,
                                "ACPY-TABLE-004: selector and value lambdas require "
                                "one argument name",
                            )
                        else:
                            if isinstance(values[0], ast.Lambda):
                                raise QueueFrontendError(
                                    "ACPY-TABLE-004: state-driven value must be an "
                                    "expression, not a lambda"
                                )
                            value = _constantize_expression(
                                values[0], "", system_static_values
                            )
                    else:
                        if not isinstance(table.entry_type, StructType):
                            raise QueueFrontendError(
                                "ACPY-TABLE-004: patch requires a struct Table Entry"
                            )
                        field_types = {
                            field.name: field.type for field in table.entry_type.fields
                        }
                        patches: list[tuple[str, ast.expr]] = []
                        for keyword in call.keywords:
                            if keyword.arg in {"enable", "arbitration"}:
                                continue
                            if keyword.arg is None or keyword.arg not in field_types:
                                raise QueueFrontendError(
                                    "ACPY-TABLE-004: patch field is unknown"
                                )
                            patches.append(
                                (
                                    keyword.arg,
                                    (
                                        lambda_or_constant(
                                            keyword.value,
                                            argument or "",
                                            "ACPY-TABLE-004: patch lambdas require "
                                            "one argument name",
                                        )
                                        if queue_driven
                                        else _constantize_expression(
                                            keyword.value, "", system_static_values
                                        )
                                    ),
                                )
                            )
                        if not patches:
                            raise QueueFrontendError(
                                "ACPY-TABLE-004: patch requires at least one field"
                            )
                        if len({name for name, _ in patches}) != len(patches):
                            raise QueueFrontendError(
                                "ACPY-TABLE-004: patch field is repeated"
                            )
                        patch_fields = tuple(patches)
                    write_fields = normalized_write_fields(table, value, patch_fields)
                    write_mode = "replace" if method == "allocate" else "field"
                    reject_overlapping_table_writer(
                        view.table, write_fields, write_mode, arbitration_rank
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
                            write_fields,
                            write_mode,
                            arbitration_rank,
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
                    visit(selected, scope_path, aliases)
                    continue

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
                    payload_map,
                    argument,
                    incoming.payload,
                    enum_types=enum_map,
                    bitfields=bitfield_map,
                ).emit(condition)
                if not _is_epoch_05_bool_compatible(condition_type):
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
                    branch_order = parser_state.order
                    parser_state.order += 1
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

                merge_order = parser_state.order
                parser_state.order += 1
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
                    visit(statement.body, path, aliases)
                    continue
            if isinstance(statement, ast.With) and len(statement.items) == 1:
                item = statement.items[0]
                call = item.context_expr
                if (
                    item.optional_vars is None
                    and isinstance(call, ast.Call)
                    and call_name(call) == "atomic"
                    and not call.args
                    and not call.keywords
                ):
                    raise QueueFrontendError(
                        "ACPY-RULE-005: ac.atomic() was removed in contract epoch "
                        f"{CONTRACT_EPOCH}; express the transaction as @ac.rule"
                    )
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
                if call_name(call) == "array" and len(call.args) == 2:
                    extent = _static_int(call.args[0])
                    argument, generator = _lambda(call.args[1])
                    collection_kinds = {"array", "map", "set", "source", "memory"}
                    if (
                        extent is not None
                        and extent > 0
                        and isinstance(generator, ast.Call)
                        and call_name(generator) not in collection_kinds
                    ):
                        shadows_index = any(
                            (
                                isinstance(candidate, ast.Lambda)
                                and argument
                                in {
                                    item.arg
                                    for item in (
                                        *candidate.args.posonlyargs,
                                        *candidate.args.args,
                                        *candidate.args.kwonlyargs,
                                    )
                                }
                            )
                            or (
                                isinstance(candidate, ast.comprehension)
                                and any(
                                    isinstance(target, ast.Name)
                                    and target.id == argument
                                    for target in ast.walk(candidate.target)
                                )
                            )
                            for candidate in ast.walk(generator)
                        )
                        if shadows_index:
                            raise QueueFrontendError(
                                "ACPY-QUEUE-005: array generator index cannot be "
                                "shadowed in a nested expression"
                            )

                        class CanonicalizeQueueReferences(ast.NodeTransformer):
                            def __init__(self) -> None:
                                self.bound_names: set[str] = set()

                            def visit_Lambda(self, node: ast.Lambda) -> ast.AST:
                                node.args.defaults = [
                                    self.visit(default)
                                    for default in node.args.defaults
                                ]
                                node.args.kw_defaults = [
                                    None if default is None else self.visit(default)
                                    for default in node.args.kw_defaults
                                ]
                                prior = self.bound_names
                                self.bound_names = prior | {
                                    item.arg
                                    for item in (
                                        *node.args.posonlyargs,
                                        *node.args.args,
                                        *node.args.kwonlyargs,
                                    )
                                }
                                node.body = self.visit(node.body)
                                self.bound_names = prior
                                return node

                            def visit_Subscript(self, node: ast.Subscript) -> ast.AST:
                                rewritten = self.generic_visit(node)
                                assert isinstance(rewritten, ast.Subscript)
                                if (
                                    isinstance(rewritten.value, ast.Name)
                                    and rewritten.value.id in self.bound_names
                                ):
                                    return rewritten
                                try:
                                    queue = queue_reference(rewritten, aliases)
                                except QueueFrontendError:
                                    return rewritten
                                return ast.copy_location(
                                    ast.Name(id=queue, ctx=ast.Load()), rewritten
                                )

                        canonicalizer = CanonicalizeQueueReferences()
                        members: list[tuple[int, str]] = []
                        for index in range(extent):
                            member_name = f"{name}__{index}"
                            expanded = _constantize_expression(
                                generator,
                                "",
                                {**system_static_values, argument: index},
                            )
                            expanded = canonicalizer.visit(expanded)
                            assert isinstance(expanded, ast.Call)
                            visit(
                                [
                                    ast.Assign(
                                        targets=[
                                            ast.Name(id=member_name, ctx=ast.Store())
                                        ],
                                        value=expanded,
                                    )
                                ],
                                scope_path,
                                aliases,
                            )
                            if member_name not in by_name:
                                raise QueueFrontendError(
                                    "ACPY-QUEUE-005: array generator must produce "
                                    "one Queue per element"
                                )
                            members.append((index, member_name))
                        collection = StaticQueueCollection("array", tuple(members))
                        signatures = {
                            collection_signature(member) for _, member in members
                        }
                        if len(signatures) != 1:
                            raise QueueFrontendError(
                                "ACPY-QUEUE-005: array-generated Queue elements "
                                "must have one static shape"
                            )
                        collections[name] = collection
                        collection_bindings.append(
                            CollectionBinding(
                                name, collection, scope_path, current_order
                            )
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
                if extent is None or not prove_within(
                    Constant(extent), 0, MAX_STATIC_EXPANSION
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-005: range extent must be a compile-time "
                        f"integer in [0, {MAX_STATIC_EXPANSION}]"
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
                    visit(expanded, scope_path, aliases)
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
                and is_queue_reference_syntax(statement.value.func.value, aliases)
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
                if any(
                    not _types_equal_in_epoch_05(by_name[input_name].payload, payload)
                    for input_name in inputs
                ):
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
                if any(
                    not _types_equal_in_epoch_05(by_name[input_name].payload, payload)
                    for input_name in inputs
                ):
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
                merge_policy = merge_policies[0].value if merge_policies else "priority"
                merge_depth = _positive_int(call, "merge_depth", 1)
                merge_latency = _positive_int(call, "merge_latency", 1)
                response_names = tuple(
                    f"{name}__bank{index}" for index in range(len(array.members))
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
                parser_state.order += 1
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
                and isinstance(statement.targets[0], (ast.Name, ast.Tuple, ast.List))
                and isinstance(statement.value, ast.Call)
                and (
                    isinstance(statement.targets[0], ast.Name)
                    or (
                        call_name(statement.value) in rule_definitions
                        and bool(
                            rule_definitions[call_name(statement.value)].output_types
                        )
                    )
                )
            ):
                target = statement.targets[0]
                call = statement.value
                target_names = (
                    (target.id,)
                    if isinstance(target, ast.Name)
                    else tuple(
                        item.id for item in target.elts if isinstance(item, ast.Name)
                    )
                )
                if not target_names or (
                    not isinstance(target, ast.Name)
                    and len(target_names) != len(target.elts)
                ):
                    raise QueueFrontendError(
                        "ACPY-RULE-014: multi-output rule call requires fixed local unpacking"
                    )
                name = target_names[0]
                if len(target_names) > 1 and (
                    call_name(call) not in rule_definitions
                    or not rule_definitions[call_name(call)].output_types
                ):
                    raise QueueFrontendError(
                        "ACPY-RULE-014: tuple unpacking is reserved for typed "
                        "multi-output rules"
                    )
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
                        )
                        queues.append(binding)
                        by_name[output_name] = binding
                        previous = binding
                    continue
                if any(item in by_name or item in collections for item in target_names):
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
                    if rate > incoming.lanes:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-025: Queue rate must not exceed lanes"
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
                        provider="compute",
                        rate=rate,
                        lanes=incoming.lanes,
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
                    if rate > incoming.lanes:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-025: Queue rate must not exceed lanes"
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
                        provider="pipeline",
                        rate=rate,
                        lanes=incoming.lanes,
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
                    lane_shapes = {
                        (by_name[item].lanes, by_name[item].rate)
                        for item in input_names
                    }
                    if len(lane_shapes) != 1:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-025: merge inputs require matching lanes "
                            "and rate"
                        )
                    lanes, rate = next(iter(lane_shapes))
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
                        rate=rate,
                        lanes=lanes,
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
                    keyword_value(call, "no_dependency")
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
                elif call_name(call) in rule_definitions:
                    definition = rule_definitions[call_name(call)]
                    if definition.output_expressions:
                        if len(target_names) != len(definition.output_types):
                            raise QueueFrontendError(
                                "ACPY-RULE-014: multi-output call unpacking arity "
                                "must match its annotation"
                            )
                    elif len(target_names) != 1:
                        raise QueueFrontendError(
                            "ACPY-RULE-014: single-output rule cannot use tuple unpacking"
                        )
                    if definition.expression is None:
                        raise QueueFrontendError(
                            "ACPY-RULE-006: outputless rule call must be a "
                            "standalone statement"
                        )
                    static_prefix = (
                        len(definition.state_arguments)
                        if definition.state_arguments
                        else (
                            1
                            if definition.var_argument is not None
                            or definition.table_argument is not None
                            else 0
                        )
                    )
                    definition, call = specialize_rule_call(
                        definition, call, static_prefix
                    )
                    while (
                        definition.arguments
                        and len(call.args) > len(definition.state_arguments)
                        and isinstance(
                            call.args[len(definition.state_arguments)], ast.Name
                        )
                        and call.args[len(definition.state_arguments)].id
                        in slot_by_name
                    ):
                        slot_argument = definition.arguments[0]
                        definition = replace(
                            definition,
                            state_arguments=(
                                *definition.state_arguments,
                                slot_argument,
                            ),
                            slot_arguments=(
                                *definition.slot_arguments,
                                slot_argument,
                            ),
                            arguments=definition.arguments[1:],
                        )
                    while (
                        (definition.state_arguments or definition.output_expressions)
                        and definition.arguments
                        and len(call.args) > len(definition.state_arguments)
                        and isinstance(
                            call.args[len(definition.state_arguments)], ast.Name
                        )
                        and (
                            call.args[len(definition.state_arguments)].id
                            in variable_by_name
                            or call.args[len(definition.state_arguments)].id
                            in table_by_name
                        )
                    ):
                        definition = replace(
                            definition,
                            state_arguments=(
                                *definition.state_arguments,
                                definition.arguments[0],
                            ),
                            arguments=definition.arguments[1:],
                        )
                    if definition.output_expressions and len(definition.arguments) != 1:
                        raise QueueFrontendError(
                            "ACPY-RULE-014: optional multi-output rules require "
                            "exactly one payload parameter after persistent state"
                        )
                    table: TableBinding | None = None
                    variable: VarStateBinding | None = None
                    multi_state_writes: tuple[RuleStateWriteBinding, ...] = ()
                    multi_state_reads: tuple[RuleStateReadBinding, ...] = ()
                    multi_state_locals: tuple[RuleLocalBinding, ...] = tuple(
                        RuleLocalBinding(
                            local.name,
                            copy.deepcopy(local.value),
                            copy.deepcopy(local.guard),
                            local.guard_negated,
                            local.prior_name,
                        )
                        for local in definition.locals
                    )
                    multi_state_finds: tuple[RuleFindBinding, ...] = ()
                    multi_state_owners: tuple[RuleStateOwnerBinding, ...] = ()
                    rule_slot_owners: tuple[RuleSlotOwnerBinding, ...] = ()
                    rule_slot_releases: tuple[RuleSlotReleaseBinding, ...] = ()
                    multi_state_result_type: ValueType | None = None
                    if definition.state_arguments:
                        state_count = len(definition.state_arguments)
                        if (
                            len(call.args) != state_count + len(definition.arguments)
                            or call.keywords
                        ):
                            raise QueueFrontendError(
                                "ACPY-RULE-008: multi-state rule invocation "
                                "requires every persistent value followed by "
                                "one Queue per payload parameter"
                            )
                        owners: dict[str, VarStateBinding | TableBinding] = {}
                        slot_owners: dict[str, SlotBinding] = {}
                        for argument, value in zip(
                            definition.state_arguments,
                            call.args[:state_count],
                            strict=True,
                        ):
                            if not isinstance(value, ast.Name):
                                raise QueueFrontendError(
                                    "ACPY-RULE-008: persistent rule parameters "
                                    "must precede payload parameters and bind "
                                    "persistent variables or slots"
                                )
                            if argument in definition.slot_arguments:
                                if value.id not in slot_by_name:
                                    raise QueueFrontendError(
                                        "ACPY-SLOT-004: rule slot parameter must "
                                        "bind a visible ac.slot resource"
                                    )
                                slot_owners[argument] = slot_by_name[value.id]
                            else:
                                owner = variable_by_name.get(
                                    value.id
                                ) or table_by_name.get(value.id)
                                if owner is None:
                                    raise QueueFrontendError(
                                        "ACPY-RULE-008: persistent rule parameters "
                                        "must precede payload parameters and bind "
                                        "persistent variables or Tables"
                                    )
                                owners[argument] = owner
                        bound_slot_names = [
                            owner.name for owner in slot_owners.values()
                        ]
                        if len(set(bound_slot_names)) != len(bound_slot_names):
                            raise QueueFrontendError(
                                "ACPY-SLOT-004: one rule cannot alias the same "
                                "slot through multiple resource parameters"
                            )
                        for owner in slot_owners.values():
                            if owner.scope != scope_path[: len(owner.scope)]:
                                raise QueueFrontendError(
                                    "ACPY-SLOT-004: rule slot parameter crosses "
                                    "an unrelated topology scope"
                                )
                        multi_state_owners = tuple(
                            RuleStateOwnerBinding(
                                owner.name,
                                argument,
                                state_value_type(owner),
                                owner.entries,
                                state_owner_kind(owner),
                                owner.shape,
                            )
                            for argument, owner in owners.items()
                        )
                        rule_slot_owners = tuple(
                            RuleSlotOwnerBinding(owner.name, argument, owner.payload)
                            for argument, owner in slot_owners.items()
                        )
                        rule_slot_releases = tuple(
                            RuleSlotReleaseBinding(
                                slot_owners[release.argument].name,
                                release.argument,
                                copy.deepcopy(release.guard),
                                release.guard_negated,
                            )
                            for release in definition.slot_releases
                        )
                        for find in definition.finds:
                            owner = owners[find.argument]
                            if owner.entries == 1:
                                raise QueueFrontendError(
                                    "ACPY-RULE-009: find requires a persistent "
                                    "list with at least 2 entries"
                                )
                        writes: list[RuleStateWriteBinding] = []
                        for write in definition.state_writes:
                            owner = owners[write.argument]
                            if (write.index is None) != (owner.entries == 1):
                                raise QueueFrontendError(
                                    "ACPY-RULE-008: scalar/list assignment does "
                                    "not match persistent variable shape"
                                )
                            field_fields = (
                                proven_field_write_fields(
                                    owner,
                                    write.argument,
                                    write.value,
                                    write.index,
                                    definition.state_reads,
                                    definition.locals,
                                )
                                if isinstance(owner, TableBinding)
                                else None
                            )
                            writes.append(
                                RuleStateWriteBinding(
                                    owner.name,
                                    write.argument,
                                    state_value_type(owner),
                                    owner.entries,
                                    copy.deepcopy(write.index),
                                    copy.deepcopy(write.value),
                                    copy.deepcopy(write.guard),
                                    write.guard_negated,
                                    state_owner_kind(owner),
                                    owner.shape,
                                    "field" if field_fields is not None else "replace",
                                    field_fields or state_write_fields(owner),
                                )
                            )
                        multi_state_writes = tuple(writes)
                        reads: list[RuleStateReadBinding] = []
                        for read in definition.state_reads:
                            owner = owners[read.argument]
                            if owner.entries == 1:
                                raise QueueFrontendError(
                                    "ACPY-RULE-008: indexed state observation "
                                    "requires a persistent list"
                                )
                            reads.append(
                                RuleStateReadBinding(
                                    read.name,
                                    owner.name,
                                    read.argument,
                                    state_value_type(owner),
                                    owner.entries,
                                    copy.deepcopy(read.index),
                                    state_owner_kind(owner),
                                    owner.shape,
                                )
                            )
                        multi_state_reads = tuple(reads)
                        multi_state_locals = tuple(
                            RuleLocalBinding(
                                local.name,
                                copy.deepcopy(local.value),
                                copy.deepcopy(local.guard),
                                local.guard_negated,
                                local.prior_name,
                            )
                            for local in definition.locals
                        )
                        finds: list[RuleFindBinding] = []
                        for find in definition.finds:
                            owner = owners[find.argument]
                            if owner.entries == 1:
                                raise QueueFrontendError(
                                    "ACPY-RULE-009: find requires a persistent "
                                    "list with at least 2 entries"
                                )
                            if find.row is not None and len(owner.shape) != 2:
                                raise QueueFrontendError(
                                    "ACPY-RULE-009: row view requires a rank-two "
                                    "persistent Table"
                                )
                            finds.append(
                                RuleFindBinding(
                                    find.name,
                                    owner.name,
                                    find.argument,
                                    state_value_type(owner),
                                    owner.entries,
                                    find.predicate_argument,
                                    copy.deepcopy(find.predicate),
                                    find.key_argument,
                                    copy.deepcopy(find.key),
                                    owner.shape,
                                    copy.deepcopy(find.row),
                                    state_owner_kind(owner),
                                )
                            )
                        multi_state_finds = tuple(finds)
                        input_names = tuple(
                            queue_reference(argument, aliases)
                            for argument in call.args[state_count:]
                        )
                        if not input_names:
                            multi_state_result_type = (
                                multi_state_finds[0].value_type
                                if multi_state_finds
                                else (
                                    multi_state_reads[0].value_type
                                    if multi_state_reads
                                    else (
                                        multi_state_writes[0].value_type
                                        if multi_state_writes
                                        else rule_slot_owners[0].payload
                                    )
                                )
                            )
                    elif definition.var_argument is not None:
                        if (
                            len(call.args) != len(definition.arguments) + 1
                            or call.keywords
                            or not isinstance(call.args[0], ast.Name)
                            or call.args[0].id not in variable_by_name
                        ):
                            raise QueueFrontendError(
                                "ACPY-RULE-003: variable rule invocation requires "
                                "one persistent variable followed by one Queue "
                                "per payload parameter"
                            )
                        variable = variable_by_name[call.args[0].id]
                        if variable.entries != 1:
                            raise QueueFrontendError(
                                "ACPY-RULE-003: scalar variable assignment cannot "
                                "target a persistent list"
                            )
                        input_names = tuple(
                            queue_reference(argument, aliases)
                            for argument in call.args[1:]
                        )
                    elif definition.table_argument is None:
                        if len(call.args) != len(definition.arguments) or call.keywords:
                            raise QueueFrontendError(
                                "ACPY-RULE-003: pure rule invocation requires "
                                "one Queue per rule parameter"
                            )
                        input_names = tuple(
                            queue_reference(argument, aliases) for argument in call.args
                        )
                        if len(set(input_names)) != len(input_names):
                            raise QueueFrontendError(
                                "ACPY-RULE-003: each multi-input rule parameter "
                                "requires a distinct Queue"
                            )
                    else:
                        owner_name = (
                            call.args[0].id
                            if call.args and isinstance(call.args[0], ast.Name)
                            else None
                        )
                        if (
                            len(call.args) != len(definition.arguments) + 1
                            or call.keywords
                            or owner_name is None
                            or (
                                owner_name not in table_by_name
                                and owner_name not in variable_by_name
                            )
                        ):
                            raise QueueFrontendError(
                                "ACPY-RULE-003: stateful rule invocation requires "
                                "one indexed persistent value followed by one "
                                "Queue per payload "
                                "parameter"
                            )
                        if owner_name in table_by_name:
                            table = table_by_name[owner_name]
                        else:
                            variable = variable_by_name[owner_name]
                            if variable.entries == 1:
                                raise QueueFrontendError(
                                    "ACPY-RULE-003: indexed state rule requires a "
                                    "persistent list"
                                )
                        input_names = tuple(
                            queue_reference(argument, aliases)
                            for argument in call.args[1:]
                        )
                        if len(set(input_names)) != len(input_names):
                            raise QueueFrontendError(
                                "ACPY-RULE-003: each stateful rule payload "
                                "parameter requires a distinct Queue"
                            )
                    incoming_queues = tuple(by_name[item] for item in input_names)
                    incoming = incoming_queues[0] if incoming_queues else None
                    indexed_variable = variable is not None and variable.entries != 1
                    binding = QueueBinding(
                        name,
                        (
                            definition.output_types[0]
                            if definition.output_types
                            else typed_result_payloads.get(
                                name,
                                (
                                    variable.value_type
                                    if variable is not None
                                    else (
                                        table.entry_type
                                        if table is not None
                                        else (
                                            incoming.payload
                                            if incoming is not None
                                            else multi_state_result_type
                                        )
                                    )
                                ),
                            )
                        ),
                        1,
                        1,
                        None if incoming is None else incoming.name,
                        definition.arguments[0] if definition.arguments else "item",
                        copy.deepcopy(definition.expression),
                        scope_path,
                        current_order,
                        rule_name=definition.name,
                        rule_source_line=definition.source_line,
                        rule_source_column=definition.source_column,
                        rule_source_path=definition.source_path,
                        rule_table=None if table is None else table.name,
                        rule_table_index=(
                            copy.deepcopy(definition.table_index)
                            if table is not None
                            else None
                        ),
                        rule_table_value=(
                            copy.deepcopy(definition.table_value)
                            if table is not None
                            else None
                        ),
                        rule_write_mode=(
                            "field"
                            if table is not None
                            and proven_field_write_fields(
                                table,
                                definition.table_argument or "",
                                definition.table_value,
                                definition.table_index,
                                definition.state_reads,
                                definition.locals,
                                legacy_read_name=definition.table_read_name,
                                legacy_read_index=definition.table_read_index,
                            )
                            is not None
                            else "replace"
                        ),
                        rule_write_fields=(
                            complete_value_fields(variable.value_type)
                            if indexed_variable
                            else (
                                ()
                                if table is None
                                else proven_field_write_fields(
                                    table,
                                    definition.table_argument or "",
                                    definition.table_value,
                                    definition.table_index,
                                    definition.state_reads,
                                    definition.locals,
                                    legacy_read_name=definition.table_read_name,
                                    legacy_read_index=definition.table_read_index,
                                )
                                or normalized_write_fields(
                                    table, definition.table_value, ()
                                )
                            )
                        ),
                        rule_table_read_name=(
                            definition.table_read_name if table is not None else None
                        ),
                        rule_table_read_index=(
                            copy.deepcopy(definition.table_read_index)
                            if table is not None
                            else None
                        ),
                        rule_input_names=input_names,
                        rule_arguments=definition.arguments,
                        rule_payloads=tuple(item.payload for item in incoming_queues),
                        rule_var=None if variable is None else variable.name,
                        rule_var_argument=(
                            definition.table_argument
                            if indexed_variable
                            else definition.var_argument
                        ),
                        rule_var_value=copy.deepcopy(
                            definition.table_value
                            if indexed_variable
                            else definition.var_value
                        ),
                        rule_var_index=(
                            copy.deepcopy(definition.table_index)
                            if indexed_variable
                            else None
                        ),
                        rule_var_read_name=(
                            definition.table_read_name if indexed_variable else None
                        ),
                        rule_var_read_index=(
                            copy.deepcopy(definition.table_read_index)
                            if indexed_variable
                            else None
                        ),
                        rule_guard=copy.deepcopy(definition.guard),
                        rule_effect_guard=copy.deepcopy(definition.effect_guard),
                        rule_output_guard=copy.deepcopy(definition.output_guard),
                        rule_state_writes=multi_state_writes,
                        rule_state_reads=multi_state_reads,
                        rule_locals=multi_state_locals,
                        rule_finds=multi_state_finds,
                        rule_state_owners=multi_state_owners,
                        rule_slot_owners=rule_slot_owners,
                        rule_slot_releases=rule_slot_releases,
                        rule_output_names=(
                            target_names if definition.output_expressions else ()
                        ),
                        rule_output_payloads=(
                            definition.output_types
                            if definition.output_expressions
                            else ()
                        ),
                        rule_output_expressions=tuple(
                            copy.deepcopy(item)
                            for item in definition.output_expressions
                        ),
                        rule_output_guards=tuple(
                            copy.deepcopy(item) for item in definition.output_guards
                        ),
                        source=source_frame(call),
                    )
                elif call_name(call) == "table":
                    raise QueueFrontendError(
                        "ACPY-TABLE-000: ac.table(value, ...) was removed; "
                        "use ac.memory for request/response memory or "
                        "ac.table[entries, Entry](init=0) for state Table"
                    )
                elif (
                    isinstance(call.func, ast.Attribute) and call.func.attr == "firing"
                ):
                    raise QueueFrontendError(
                        "ACPY-RULE-005: Queue.firing() was removed in contract "
                        f"epoch {CONTRACT_EPOCH}; express the transaction as @ac.rule"
                    )
                elif (
                    isinstance(call.func, ast.Attribute)
                    and call.func.attr == "apply"
                    and is_queue_reference_syntax(call.func.value, aliases)
                    and len(call.args) == 1
                ):
                    input_name = queue_reference(call.func.value, aliases)
                    incoming = by_name.get(input_name)
                    if incoming is None:
                        raise QueueFrontendError(
                            f"ACPY-QUEUE-001: input queue {input_name!r} is unbound"
                        )
                    argument, expression = _lambda(call.args[0])
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
                        source=source_frame(call),
                    )
                else:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-001: unsupported queue-producing call "
                        f"{ast.unparse(call)!r}"
                    )
                queues.append(binding)
                if binding.rule_output_payloads:
                    for output_name, output_payload in zip(
                        target_names, binding.rule_output_payloads, strict=True
                    ):
                        by_name[output_name] = replace(
                            binding,
                            name=output_name,
                            payload=output_payload,
                        )
                else:
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
                and call_name(statement.value) in rule_definitions
            ):
                call = statement.value
                definition = rule_definitions[call_name(call)]
                if definition.expression is not None:
                    raise QueueFrontendError(
                        "ACPY-RULE-006: value-returning rule call must be assigned"
                    )
                static_prefix = (
                    len(definition.state_arguments)
                    if definition.state_arguments
                    else (
                        1
                        if definition.var_argument is not None
                        or definition.table_argument is not None
                        else 0
                    )
                )
                definition, call = specialize_rule_call(definition, call, static_prefix)
                while (
                    definition.arguments
                    and len(call.args) > len(definition.state_arguments)
                    and isinstance(call.args[len(definition.state_arguments)], ast.Name)
                    and call.args[len(definition.state_arguments)].id in slot_by_name
                ):
                    slot_argument = definition.arguments[0]
                    definition = replace(
                        definition,
                        state_arguments=(*definition.state_arguments, slot_argument),
                        slot_arguments=(*definition.slot_arguments, slot_argument),
                        arguments=definition.arguments[1:],
                    )
                if definition.state_arguments:
                    state_count = len(definition.state_arguments)
                    if (
                        len(call.args) != state_count + len(definition.arguments)
                        or call.keywords
                    ):
                        raise QueueFrontendError(
                            "ACPY-RULE-008: outputless multi-state rule requires "
                            "every persistent value followed by its payload "
                            "Queues"
                        )
                    owners: dict[str, VarStateBinding | TableBinding] = {}
                    slot_owners: dict[str, SlotBinding] = {}
                    for argument, value in zip(
                        definition.state_arguments,
                        call.args[:state_count],
                        strict=True,
                    ):
                        if not isinstance(value, ast.Name):
                            raise QueueFrontendError(
                                "ACPY-RULE-008: persistent rule parameters "
                                "must precede payload parameters and bind "
                                "persistent variables or slots"
                            )
                        if argument in definition.slot_arguments:
                            if value.id not in slot_by_name:
                                raise QueueFrontendError(
                                    "ACPY-SLOT-004: rule slot parameter must "
                                    "bind a visible ac.slot resource"
                                )
                            slot_owners[argument] = slot_by_name[value.id]
                        else:
                            owner = variable_by_name.get(value.id) or table_by_name.get(
                                value.id
                            )
                            if owner is None:
                                raise QueueFrontendError(
                                    "ACPY-RULE-008: persistent rule parameters "
                                    "must bind persistent variables or Tables"
                                )
                            owners[argument] = owner
                    bound_slot_names = [owner.name for owner in slot_owners.values()]
                    if len(set(bound_slot_names)) != len(bound_slot_names):
                        raise QueueFrontendError(
                            "ACPY-SLOT-004: one rule cannot alias the same slot "
                            "through multiple resource parameters"
                        )
                    for find in definition.finds:
                        owner = owners[find.argument]
                        if owner.entries == 1:
                            raise QueueFrontendError(
                                "ACPY-RULE-009: find requires a persistent "
                                "list with at least 2 entries"
                            )
                    writes: list[RuleStateWriteBinding] = []
                    for write in definition.state_writes:
                        owner = owners[write.argument]
                        if (write.index is None) != (owner.entries == 1):
                            raise QueueFrontendError(
                                "ACPY-RULE-008: scalar/list assignment does not "
                                "match persistent variable shape"
                            )
                        field_fields = (
                            proven_field_write_fields(
                                owner,
                                write.argument,
                                write.value,
                                write.index,
                                definition.state_reads,
                                definition.locals,
                            )
                            if isinstance(owner, TableBinding)
                            else None
                        )
                        writes.append(
                            RuleStateWriteBinding(
                                owner.name,
                                write.argument,
                                state_value_type(owner),
                                owner.entries,
                                copy.deepcopy(write.index),
                                copy.deepcopy(write.value),
                                copy.deepcopy(write.guard),
                                write.guard_negated,
                                state_owner_kind(owner),
                                owner.shape,
                                "field" if field_fields is not None else "replace",
                                field_fields or state_write_fields(owner),
                            )
                        )
                    reads: list[RuleStateReadBinding] = []
                    for read in definition.state_reads:
                        owner = owners[read.argument]
                        reads.append(
                            RuleStateReadBinding(
                                read.name,
                                owner.name,
                                read.argument,
                                state_value_type(owner),
                                owner.entries,
                                copy.deepcopy(read.index),
                                state_owner_kind(owner),
                                owner.shape,
                            )
                        )
                    finds: list[RuleFindBinding] = []
                    for find in definition.finds:
                        owner = owners[find.argument]
                        if owner.entries == 1:
                            raise QueueFrontendError(
                                "ACPY-RULE-009: find requires a persistent "
                                "list with at least 2 entries"
                            )
                        if find.row is not None and len(owner.shape) != 2:
                            raise QueueFrontendError(
                                "ACPY-RULE-009: row view requires a rank-two "
                                "persistent Table"
                            )
                        finds.append(
                            RuleFindBinding(
                                find.name,
                                owner.name,
                                find.argument,
                                state_value_type(owner),
                                owner.entries,
                                find.predicate_argument,
                                copy.deepcopy(find.predicate),
                                find.key_argument,
                                copy.deepcopy(find.key),
                                owner.shape,
                                copy.deepcopy(find.row),
                                state_owner_kind(owner),
                            )
                        )
                    input_names = tuple(
                        queue_reference(argument, aliases)
                        for argument in call.args[state_count:]
                    )
                    incoming_queues = tuple(by_name[item] for item in input_names)
                    effect_rules.append(
                        QueueBinding(
                            f"{definition.name}__effect_{current_order}",
                            (
                                writes[0].value_type
                                if writes
                                else next(iter(slot_owners.values())).payload
                            ),
                            1,
                            1,
                            None if not incoming_queues else incoming_queues[0].name,
                            (
                                definition.arguments[0]
                                if definition.arguments
                                else "item"
                            ),
                            None,
                            scope_path,
                            current_order,
                            rule_name=definition.name,
                            rule_source_line=definition.source_line,
                            rule_source_column=definition.source_column,
                            rule_source_path=definition.source_path,
                            rule_input_names=input_names,
                            rule_arguments=definition.arguments,
                            rule_payloads=tuple(
                                item.payload for item in incoming_queues
                            ),
                            rule_has_output=False,
                            rule_guard=copy.deepcopy(definition.guard),
                            rule_effect_guard=copy.deepcopy(definition.effect_guard),
                            rule_output_guard=copy.deepcopy(definition.output_guard),
                            rule_state_writes=tuple(writes),
                            rule_state_reads=tuple(reads),
                            rule_locals=tuple(
                                RuleLocalBinding(
                                    local.name,
                                    copy.deepcopy(local.value),
                                    copy.deepcopy(local.guard),
                                    local.guard_negated,
                                    local.prior_name,
                                )
                                for local in definition.locals
                            ),
                            rule_finds=tuple(finds),
                            rule_state_owners=tuple(
                                RuleStateOwnerBinding(
                                    owner.name,
                                    argument,
                                    state_value_type(owner),
                                    owner.entries,
                                    state_owner_kind(owner),
                                    owner.shape,
                                )
                                for argument, owner in owners.items()
                            ),
                            rule_slot_owners=tuple(
                                RuleSlotOwnerBinding(
                                    owner.name, argument, owner.payload
                                )
                                for argument, owner in slot_owners.items()
                            ),
                            rule_slot_releases=tuple(
                                RuleSlotReleaseBinding(
                                    slot_owners[release.argument].name,
                                    release.argument,
                                    copy.deepcopy(release.guard),
                                    release.guard_negated,
                                )
                                for release in definition.slot_releases
                            ),
                        )
                    )
                    continue
                if definition.table_argument is None:
                    raise QueueFrontendError(
                        "ACPY-RULE-006: outputless rule must update indexed state"
                    )
                owner_name = (
                    call.args[0].id
                    if call.args and isinstance(call.args[0], ast.Name)
                    else None
                )
                if (
                    len(call.args) != len(definition.arguments) + 1
                    or call.keywords
                    or owner_name is None
                    or (
                        owner_name not in table_by_name
                        and owner_name not in variable_by_name
                    )
                ):
                    raise QueueFrontendError(
                        "ACPY-RULE-006: outputless state rule requires one "
                        "indexed persistent value followed by one Queue per "
                        "payload parameter"
                    )
                table = table_by_name.get(owner_name)
                variable = variable_by_name.get(owner_name)
                if variable is not None and variable.entries == 1:
                    raise QueueFrontendError(
                        "ACPY-RULE-006: indexed state rule requires a persistent list"
                    )
                input_names = tuple(
                    queue_reference(argument, aliases) for argument in call.args[1:]
                )
                if len(set(input_names)) != len(input_names):
                    raise QueueFrontendError(
                        "ACPY-RULE-006: each payload parameter requires a distinct "
                        "Queue"
                    )
                incoming_queues = tuple(by_name[item] for item in input_names)
                incoming = incoming_queues[0]
                value_type = (
                    table.entry_type if table is not None else variable.value_type
                )
                effect_rules.append(
                    QueueBinding(
                        f"{definition.name}__effect_{current_order}",
                        value_type,
                        1,
                        1,
                        incoming.name,
                        definition.arguments[0],
                        None,
                        scope_path,
                        current_order,
                        rule_name=definition.name,
                        rule_source_line=definition.source_line,
                        rule_source_column=definition.source_column,
                        rule_source_path=definition.source_path,
                        rule_table=None if table is None else table.name,
                        rule_table_index=(
                            copy.deepcopy(definition.table_index)
                            if table is not None
                            else None
                        ),
                        rule_table_value=(
                            copy.deepcopy(definition.table_value)
                            if table is not None
                            else None
                        ),
                        rule_write_mode=(
                            "field"
                            if table is not None
                            and proven_field_write_fields(
                                table,
                                definition.table_argument or "",
                                definition.table_value,
                                definition.table_index,
                                definition.state_reads,
                                definition.locals,
                                legacy_read_name=definition.table_read_name,
                                legacy_read_index=definition.table_read_index,
                            )
                            is not None
                            else "replace"
                        ),
                        rule_write_fields=(
                            proven_field_write_fields(
                                table,
                                definition.table_argument or "",
                                definition.table_value,
                                definition.table_index,
                                definition.state_reads,
                                definition.locals,
                                legacy_read_name=definition.table_read_name,
                                legacy_read_index=definition.table_read_index,
                            )
                            or normalized_write_fields(
                                table, definition.table_value, ()
                            )
                            if table is not None
                            else complete_value_fields(value_type)
                        ),
                        rule_table_read_name=(
                            definition.table_read_name if table is not None else None
                        ),
                        rule_table_read_index=(
                            copy.deepcopy(definition.table_read_index)
                            if table is not None
                            else None
                        ),
                        rule_input_names=input_names,
                        rule_arguments=definition.arguments,
                        rule_payloads=tuple(item.payload for item in incoming_queues),
                        rule_var=None if variable is None else variable.name,
                        rule_var_argument=(
                            definition.table_argument if variable is not None else None
                        ),
                        rule_var_value=(
                            copy.deepcopy(definition.table_value)
                            if variable is not None
                            else None
                        ),
                        rule_var_index=(
                            copy.deepcopy(definition.table_index)
                            if variable is not None
                            else None
                        ),
                        rule_var_read_name=(
                            definition.table_read_name if variable is not None else None
                        ),
                        rule_var_read_index=(
                            copy.deepcopy(definition.table_read_index)
                            if variable is not None
                            else None
                        ),
                        rule_has_output=False,
                        rule_guard=copy.deepcopy(definition.guard),
                        rule_effect_guard=copy.deepcopy(definition.effect_guard),
                        rule_output_guard=copy.deepcopy(definition.output_guard),
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
                sinks.append(
                    SinkBinding(
                        name,
                        scope_path,
                        current_order,
                        source_frame(statement),
                    )
                )
                continue
            if isinstance(statement, ast.Return):
                if statement.value is None:
                    if result_payloads not in {None, ()}:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-026: typed system results must be returned"
                        )
                    continue
                values = (
                    tuple(statement.value.elts)
                    if isinstance(statement.value, (ast.Tuple, ast.List))
                    else (statement.value,)
                )
                returned = tuple(queue_reference(value, aliases) for value in values)
                if result_payloads is not None:
                    if len(returned) != len(result_payloads):
                        raise QueueFrontendError(
                            "ACPY-QUEUE-026: system return arity does not match "
                            "its annotation"
                        )
                    for index, (queue_name, expected_payload) in enumerate(
                        zip(returned, result_payloads, strict=True)
                    ):
                        if not _types_equal_in_epoch_05(
                            by_name[queue_name].payload, expected_payload
                        ):
                            raise QueueFrontendError(
                                "ACPY-QUEUE-026: system return "
                                f"{index} payload does not match its annotation"
                            )
                for index, queue_name in enumerate(returned):
                    sinks.append(
                        SinkBinding(
                            queue_name,
                            scope_path,
                            current_order + index,
                            source_frame(statement),
                        )
                    )
                parser_state.order += len(returned) - 1
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
        endpoint_count = (
            sum(read.table == table.name for read in table_reads)
            + sum(write.table == table.name for write in table_writes)
            + sum(write.table == table.name for write in masked_table_writes)
            + sum(candidate.table == table.name for candidate in candidates)
            + sum(queue.rule_table == table.name for queue in (*queues, *effect_rules))
            + sum(
                owner.variable == table.name and owner.owner_kind == "table"
                for queue in (*queues, *effect_rules)
                for owner in queue.rule_state_owners
            )
        )
        if endpoint_count == 0:
            raise QueueFrontendError(
                f"ACPY-TABLE-005: table {table.name!r} requires a read/write endpoint"
            )
    for slot in slots:
        standalone = sum(release.slot == slot.name for release in slot_releases)
        transactional = sum(
            release.slot == slot.name
            for rule_binding in (*queues, *effect_rules)
            for release in rule_binding.rule_slot_releases
        )
        if standalone + transactional == 0:
            raise QueueFrontendError(
                f"ACPY-SLOT-002: slot {slot.name!r} requires one release endpoint"
            )
        if standalone + transactional != 1:
            raise QueueFrontendError(
                f"ACPY-SLOT-002: slot {slot.name!r} permits exactly one release "
                "endpoint or transactional rule owner"
            )
    if not queues or (not sinks and not effect_rules):
        raise QueueFrontendError(
            "ACPY-QUEUE-001: a queue system requires an external value and a "
            "consuming rule or result boundary"
        )
    all_static_checks = (
        *(check for payload in payloads for check in payload.static_type_checks),
        *interface_type_checks,
        *expression_type_checks,
    )
    static_config_bindings = _static_config_bindings_for_checks(
        tree,
        all_static_checks,
        parameter_aliases,
        type_static_values,
        binding_namespace=static_type_namespace,
    )
    _validate_static_config_roots(
        function,
        parameter_aliases,
        {binding.root for binding in static_config_bindings},
        binding_namespace=static_type_namespace,
    )
    return QueueProgram(
        system,
        payloads,
        enums,
        bitfields,
        tuple(invariant_definitions),
        tuple(helper_definitions),
        tuple(queues),
        tuple(effect_rules),
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
        tuple(variables),
        tuple(tables),
        tuple(table_reads),
        tuple(table_writes),
        tuple(masked_table_writes),
        tuple(slots),
        tuple(slot_releases),
        tuple(candidates),
        tuple(selections),
        tuple(collection_bindings),
        tuple(observations),
        tuple(expectations),
        tuple(sinks),
        static_type_bindings=_static_type_bindings_for_checks(
            all_static_checks,
            parameter_aliases,
            type_static_values,
            binding_namespace=static_type_namespace,
        ),
        static_type_checks=tuple((*interface_type_checks, *expression_type_checks)),
        static_config_bindings=static_config_bindings,
        specialization_fingerprint=specialization_fingerprint,
        source_path=normalized_source_path,
        statement_sources=tuple(sorted(statement_sources.items())),
    )
