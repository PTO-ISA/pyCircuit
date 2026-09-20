"""QueueProgram to deterministic raw ACIR lowering."""

from __future__ import annotations

import ast
import copy
import re
from dataclasses import dataclass, replace

from _pycircuit_semantics import (
    ArrayType,
    BitsType,
    BoolType,
    EnumType,
    StructType,
    TupleType,
    ValueType,
)

from .._canonical_json import canonical_mlir_string
from .._static_eval import StaticEnvironment, StaticValue, evaluate_static
from .acir_text import (
    _enum_layout_entry,
    _payload_layout_entry,
    _render_bitfield,
    _render_dense_i64,
    _render_enum,
    _render_interface_display_attributes,
    _render_queue_type,
    _render_table_domain_attributes,
    _render_table_init_value,
    _render_type,
    _table_schema_id,
)
from .errors import QueueFrontendError
from .expressions import _ExpressionEmitter
from .model import (
    BarrierBinding,
    CandidateSetBinding,
    CreditBinding,
    DependencyBinding,
    ExpectBinding,
    FeedbackBinding,
    ForkBinding,
    MaskedTableWriteBinding,
    MemoryRequestBinding,
    MergeBinding,
    ObservationBinding,
    QueueBinding,
    QueueProgram,
    ReorderBinding,
    RouteBinding,
    RuleSlotReleaseBinding,
    RuleStateWriteBinding,
    ScopeBinding,
    SelectBinding,
    SelectionBinding,
    SinkBinding,
    SlotBinding,
    SlotReleaseBinding,
    TableReadBinding,
    TableWriteBinding,
)
from .provenance import DefinitionNdfMetadata, NdfMetadata
from .source import (
    SourceFrame,
    _render_callsite_location,
    _render_fused_source_locations,
    _render_source_frame_location,
    source_frame,
)
from .static_types import (
    _candidate_mask_type,
    _integer_width,
    _is_bool_like,
    _product,
    _table_axis_width,
    _types_compatible,
)
from .syntax import _decorator_name


@dataclass(frozen=True, slots=True)
class _ModuleRenderSpec:
    name: str
    inputs: tuple[tuple[str, ValueType], ...]
    outputs: tuple[tuple[str, ValueType], ...]
    static_arguments: tuple[tuple[str, StaticValue], ...] = ()
    ndf: NdfMetadata = NdfMetadata()
    source_file: str = ""
    source_line: int = 0
    source_column: int = 0
    definition_name: str = ""


def _ndf_attribute_fields(metadata: NdfMetadata) -> tuple[str, ...]:
    fields = []
    if metadata.ids:
        fields.append(
            "ac.ndf_ids = ["
            + ", ".join(canonical_mlir_string(value) for value in metadata.ids)
            + "]"
        )
    if metadata.requires:
        fields.append(
            "ac.ndf_requires = ["
            + ", ".join(canonical_mlir_string(value) for value in metadata.requires)
            + "]"
        )
    return tuple(fields)


def _module_attribute_fields(module: _ModuleRenderSpec) -> tuple[str, ...]:
    fields = [
        "ac.definition_name = "
        + canonical_mlir_string(module.definition_name or module.name)
    ]
    fields.extend(_ndf_attribute_fields(module.ndf))
    if module.source_file:
        fields.extend(
            (
                "ac.source_file = " + canonical_mlir_string(module.source_file),
                f"ac.source_line = {module.source_line} : i64",
                f"ac.source_column = {module.source_column} : i64",
            )
        )
    return tuple(fields)


def lower_queue_program(
    program: QueueProgram,
    *,
    module: _ModuleRenderSpec | None = None,
    include_helpers: bool = True,
    helper_names_to_emit: frozenset[str] | None = None,
    definition_locations: dict[str, tuple[str, int, int]] | None = None,
    definition_ndf: DefinitionNdfMetadata | None = None,
) -> str:
    definition_ndf = definition_ndf or {}

    def add_display_name(
        emitted_lines: list[str], result: str, logical_name: str
    ) -> None:
        """Attach presentation-only metadata to the op defining ``result``."""

        display_name = re.sub(r"^compiler_rule_local_[0-9]+_", "", logical_name)
        for index in range(len(emitted_lines) - 1, -1, -1):
            line = emitted_lines[index]
            operation, separator, _ = line.partition("=")
            defined_results = re.findall(r"%([A-Za-z0-9_.$-]+)", operation)
            if not separator or result not in defined_results:
                continue
            if (
                "= ac.var." not in line and "= ac.table.get " not in line
            ) or " : " not in line:
                return
            if "{ac.query =" in line:
                return
            attribute = "ac.display_name = " + canonical_mlir_string(display_name)
            if "= ac.var.constant " in line:
                prefix, separator, suffix = line.partition(" as ")
                if not separator:
                    return
                emitted_lines[index] = prefix + f" {{{attribute}}}" + separator + suffix
                return
            prefix, type_separator, suffix = line.rpartition(" : ")
            existing = re.search(r" \{([^{}]*)\}$", prefix)
            if existing is None:
                prefix += f" {{{attribute}}}"
            else:
                contents = existing.group(1).strip()
                if re.search(r"(?:^|, )ac\.display_name\s*=", contents):
                    return
                merged = attribute if not contents else f"{contents}, {attribute}"
                prefix = prefix[: existing.start()] + f" {{{merged}}}"
            emitted_lines[index] = prefix + type_separator + suffix
            return

    def table_writer_identity(
        write: TableWriteBinding | MaskedTableWriteBinding,
    ) -> str:
        kind = "masked_write" if isinstance(write, MaskedTableWriteBinding) else "write"
        owner = "_".join((*write.scope, write.table))
        return f"{owner}_{kind}_{write.arbitration_rank}"

    module_inputs = set() if module is None else {name for name, _ in module.inputs}
    module_outputs = set() if module is None else {name for name, _ in module.outputs}
    initial_mapping: dict[str, str] = {}
    if module is None:
        system_fields = list(
            _ndf_attribute_fields(definition_ndf.get(program.system, NdfMetadata()))
        )
        if program.system_source is not None:
            system_fields.extend(
                (
                    "ac.source_file = "
                    + canonical_mlir_string(program.system_source.file),
                    f"ac.source_line = {program.system_source.line} : i64",
                    f"ac.source_column = {program.system_source.column} : i64",
                )
            )
        system_metadata_text = "".join(f", {field}" for field in system_fields)
        lines = [
            "module attributes {"
            f'ac.model_kind = "queue_graph", '
            f'ac.queue_graph_domain = "cycle", '
            f'ac.system = "{program.system}"{system_metadata_text}'
            "} {"
        ]
        content_indent = "  "
    else:
        argument_types = ", ".join(
            f"%input_{index}: !ac.queue<{_render_type(payload)}>"
            for index, (_, payload) in enumerate(module.inputs)
        )
        result_types = ", ".join(
            f"!ac.queue<{_render_type(payload)}>" for _, payload in module.outputs
        )
        result_signature = (
            ""
            if not module.outputs
            else " -> "
            + (result_types if len(module.outputs) == 1 else f"({result_types})")
        )
        scope_results = [
            f"module_result_{index}" for index in range(len(module.outputs))
        ]
        scope_lhs = (
            ""
            if not scope_results
            else ", ".join(f"%{name}" for name in scope_results) + " = "
        )
        scope_operands = ", ".join(
            f"%input_{index}" for index in range(len(module.inputs))
        )
        scope_arguments = ", ".join(
            f"%borrowed_{index}: !ac.queue<{_render_type(payload)}>"
            for index, (_, payload) in enumerate(module.inputs)
        )
        source_owner = module.source_file or "generated/module.py"
        owner = (
            "#ac.source_owner<"
            + canonical_mlir_string(source_owner)
            + ", "
            + canonical_mlir_string(source_owner)
            + ">"
        )
        provenance = (
            "#ac.source_provenance<"
            + canonical_mlir_string(source_owner)
            + f", {max(module.source_line, 1)}, {max(module.source_column, 1)}, "
            + f"{max(module.source_line, 1)}, {max(module.source_column, 1)}>"
        )
        empty_arguments = "#ac.static_arguments<[]>"
        interface_ports: list[str] = []
        for name, payload in module.inputs:
            queue_type = f"!ac.queue<{_render_type(payload)}>"
            interface_ports.append(
                "#ac.interface_port<"
                f"{canonical_mlir_string(name)}, \"input\", "
                f"#ac.type_expr<#ac.type_expr_concrete<{queue_type}>>, "
                f"{provenance}>"
            )
        for name, payload in module.outputs:
            queue_type = f"!ac.queue<{_render_type(payload)}>"
            interface_ports.append(
                "#ac.interface_port<"
                f"{canonical_mlir_string(name)}, \"output\", "
                f"#ac.type_expr<#ac.type_expr_concrete<{queue_type}>>, "
                f"{provenance}>"
            )
        schema = (
            "#ac.module_family_schema<#ac.static_parameters<[]>, "
            "#ac.static_cases<[#ac.static_arguments<[]>]>, "
            "#ac.module_interface<[" + ", ".join(interface_ports) + "]>, "
            f"{owner}, []>"
        )
        physical_inputs = ", ".join(
            f"!ac.queue<{_render_type(payload)}>" for _, payload in module.inputs
        )
        function_type = f"({physical_inputs}) -> {result_signature[4:] if result_signature else '()'}"
        lines = [
            f"  ac.module @{module.name} source {owner} schema {schema} {{",
            f"    ac.module.case arguments {empty_arguments} type {function_type} "
            f"{_render_interface_display_attributes(tuple(name for name, _ in module.inputs), tuple(name for name, _ in module.outputs), _module_attribute_fields(module)).removeprefix(' attributes')} "
            f"source {provenance} graph {{",
            f"    ^bb0({argument_types}):" if argument_types else "    ^bb0:",
            f"    {scope_lhs}ac.scope @body({scope_operands}) {{",
            f"    ^bb0({scope_arguments}):" if scope_arguments else "    ^bb0:",
        ]
        initial_mapping = {
            name: f"borrowed_{index}" for index, (name, _) in enumerate(module.inputs)
        }
        content_indent = "      "
    payloads = {item.name: item for item in program.payloads}
    enum_types = {item.name: item.descriptor for item in program.enums}
    helpers = {item.name: item for item in program.helpers}
    invariants = {
        definition.function_name: definition for definition in program.invariants
    }
    bitfields = {item.name: item.layout for item in program.bitfields}
    if (program.payloads or program.enums or program.bitfields) and module is None:
        lines.append("  ac.type_scope @types {")
        for enumeration in program.enums:
            rendered = _render_enum(enumeration, "    ")
            location = (definition_locations or {}).get(enumeration.name)
            if location is not None:
                rendered += (
                    " {ac.source_file = "
                    + canonical_mlir_string(location[0])
                    + "}"
                )
            lines.append(rendered)
        for payload in program.payloads:
            fields = ", ".join(
                f'{{name = "{name}", type = {typ}}}' for name, typ in payload.fields
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
        for bitfield in program.bitfields:
            lines.append(_render_bitfield(bitfield, "    "))
        layouts = [
            *(_enum_layout_entry(enumeration) for enumeration in program.enums),
            *(_payload_layout_entry(payload) for payload in program.payloads),
        ]
        if layouts:
            lines.append(
                "  } {dlti.dl_spec = #dlti.dl_spec<" + ", ".join(layouts) + ">}"
            )
        else:
            lines.append("  }")
    helper_start = len(lines)
    for helper in program.helpers if include_helpers else ():
        if (
            helper_names_to_emit is not None
            and helper.name not in helper_names_to_emit
        ):
            continue
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
            payloads,
            "",
            helper.result,
            root_values={
                name: (name, value_type) for name, value_type in helper.arguments
            },
            enum_types=enum_types,
            bitfields=bitfields,
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
    if module is not None and len(lines) != helper_start:
        helper_lines = lines[helper_start:]
        del lines[helper_start:]
        lines[0:0] = helper_lines
    for instance in sorted(
        program.memory_instances,
        key=lambda value: (value.scope, value.order, value.name),
    ):
        owner = "/" + "/".join(instance.scope) if instance.scope else "/"
        stable_id = (
            "/".join((*instance.scope, instance.name))
            if instance.scope
            else instance.name
        )
        if module is not None:
            raise QueueFrontendError(
                "ACPY-MODULE-005: module-local memory instances are not implemented"
            )
        lines.append(
            f"{content_indent}ac.memory.instance @{instance.name} "
            f"data {_render_type(instance.data_type)} "
            f"entries {instance.entries} init {instance.init} "
            f'latency {instance.latency} owner "{owner}" '
            f'stable_id "memory/{stable_id}"'
            + _render_source_frame_location(instance.source)
        )
    for variable in sorted(
        program.variables, key=lambda value: (value.scope, value.order, value.name)
    ):
        owner_scope = variable.scope if module is None else ("body", *variable.scope)
        owner = "/" + "/".join(owner_scope) if owner_scope else "/"
        stable_id = (
            "/".join((*owner_scope, variable.name)) if owner_scope else variable.name
        )
        init = (
            "true"
            if variable.init is True
            else (
                "false"
                if variable.init is False
                else f"{variable.init} : "
                + (
                    "i64"
                    if isinstance(
                        variable.value_type,
                        (StructType, TupleType, ArrayType, EnumType),
                    )
                    else (
                        f"i{variable.value_type.width}"
                        if type(variable.value_type).__name__ == "RangeType"
                        else _render_type(variable.value_type)
                    )
                )
            )
        )
        lines.append(
            f"{content_indent}ac.var.decl @{variable.name} "
            f"type {_render_type(variable.value_type)} "
            f'init {init} owner "{owner}" stable_id "var/{stable_id}"'
            + (
                " shape [" + ", ".join(str(value) for value in variable.shape) + "]"
                if variable.shape
                else ""
            )
            + _render_source_frame_location(variable.source)
        )
    for table in sorted(
        program.tables, key=lambda value: (value.scope, value.order, value.name)
    ):
        owner_scope = table.scope if module is None else ("body", *table.scope)
        owner = "/" + "/".join(owner_scope) if owner_scope else "/"
        stable_id = "/".join((*owner_scope, table.name)) if owner_scope else table.name
        attributes = ""
        if len(table.shape) > 1 or table.init_image is not None:
            typed_attributes = [
                "shape = " + _render_dense_i64(table.shape),
                "axis_widths = "
                + _render_dense_i64(
                    tuple(_table_axis_width(extent) for extent in table.shape)
                ),
                'layout = "row_major"',
                "layout_version = 1 : i64",
                "schema_id = "
                + canonical_mlir_string(
                    _table_schema_id(table.entry_type, table.shape)
                ),
            ]
            if table.init_image is not None:
                typed_attributes.extend(
                    (
                        "init_version = 1 : i64",
                        "init_image = ["
                        + ", ".join(
                            _render_table_init_value(value, table.entry_type)
                            for value in table.init_image
                        )
                        + "]",
                    )
                )
            attributes = " {" + ", ".join(typed_attributes) + "}"
        lines.append(
            f"{content_indent}ac.table @{table.name} "
            f"entry {_render_type(table.entry_type)} "
            f'entries {table.entries} init 0 owner "{owner}" '
            f'stable_id "table/{stable_id}"'
            + attributes
            + _render_source_frame_location(table.source)
        )
    by_name = {item.name: item for item in program.queues}
    for item in program.queues:
        for output_name, output_payload in zip(
            item.rule_output_names,
            item.rule_output_payloads,
            strict=True,
        ):
            by_name[output_name] = replace(
                item,
                name=output_name,
                payload=output_payload,
            )
    memory_ordinals: dict[tuple[str, str], int] = {}
    requests_by_instance: dict[str, list[MemoryRequestBinding]] = {}
    for request in program.memory_requests:
        requests_by_instance.setdefault(request.instance, []).append(request)
    for instance, requests in requests_by_instance.items():
        for ordinal, request in enumerate(
            sorted(
                requests,
                key=lambda value: (value.scope, value.order, value.output_name),
            )
        ):
            memory_ordinals[(instance, request.output_name)] = ordinal

    def name_array(names: list[str] | tuple[str, ...]) -> str:
        return "[" + ", ".join(f'"{name}"' for name in names) + "]"

    consumers: dict[str, list[tuple[QueueBinding, int]]] = {}
    for queue in (*program.queues, *program.effect_rules):
        input_names = (
            queue.rule_input_names
            if queue.rule_input_names
            else (() if queue.input_name is None else (queue.input_name,))
        )
        for input_index, input_name in enumerate(input_names):
            consumers.setdefault(input_name, []).append((queue, input_index))
    fanouts: dict[
        str, tuple[tuple[str, ...], tuple[tuple[QueueBinding, int], ...]]
    ] = {}

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
            common_scope([consumer.scope for consumer, _ in group]),
            tuple(group),
        )
    payload_by_queue = {name: queue.payload for name, queue in by_name.items()}
    slot_views = {slot.name: (slot.name, slot.payload) for slot in program.slots}
    candidate_views = {candidate.name: candidate for candidate in program.candidates}
    selection_views = {selection.name: selection for selection in program.selections}
    table_domains = {
        table.name: (table.entry_type, table.entries, table.shape)
        for table in program.tables
    }
    variable_domains = {
        variable.name: (variable.value_type, variable.entries)
        for variable in program.variables
    }
    rule_writer_counts: dict[str, int] = {}
    for rule in (*program.queues, *program.effect_rules):
        owners = {
            owner
            for owner in (
                rule.rule_table,
                rule.rule_var,
                *(write.variable for write in rule.rule_state_writes),
            )
            if owner is not None
        }
        for owner in owners:
            rule_writer_counts[owner] = rule_writer_counts.get(owner, 0) + 1

    def rule_writer_arbitration(rule: QueueBinding, owner: str) -> str:
        if rule_writer_counts.get(owner, 0) < 2:
            return ""
        return f" {{ac.arbitration = #ac.writer_priority<{rule.order}>}}"

    materialized_candidates: dict[str, tuple[str, ValueType]] = {}
    materialized_selections: dict[str, tuple[str, ValueType, str, ValueType]] = {}
    queue_scope = {name: queue.scope for name, queue in by_name.items()}
    effective_input: dict[tuple[str, int], str] = {}
    for source_name, (fanout_scope, group) in fanouts.items():
        for index, (consumer, input_index) in enumerate(group):
            synthetic = f"{source_name}_fanout_{index}"
            effective_input[(consumer.name, input_index)] = synthetic
            payload_by_queue[synthetic] = by_name[source_name].payload
            queue_scope[synthetic] = fanout_scope

    uses: dict[str, list[tuple[str, ...]]] = {name: [] for name in payload_by_queue}
    for queue in (*program.queues, *program.effect_rules):
        input_names = (
            queue.rule_input_names
            if queue.rule_input_names
            else (() if queue.input_name is None else (queue.input_name,))
        )
        for input_index, input_name in enumerate(input_names):
            selected = effective_input.get((queue.name, input_index), input_name)
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
        if write.input_name is not None:
            uses[write.input_name].append(write.scope)
    for slot in program.slots:
        uses[slot.input_name].append(slot.scope)

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

    def queue_attributes(
        name: str,
        rates: tuple[int, ...],
        output_names: tuple[str, ...] = (),
        source: tuple[str, int, int] | None = None,
        ndf: NdfMetadata = NdfMetadata(),
    ) -> str:
        attributes = [f'ac.name = "{name}"']
        attributes.extend(_ndf_attribute_fields(ndf))
        if output_names:
            attributes.append(
                "ac.output_names = ["
                + ", ".join(canonical_mlir_string(output) for output in output_names)
                + "]"
            )
        if any(rate != 1 for rate in rates):
            attributes.append(
                "ac.output_rates = array<i64: "
                + ", ".join(str(rate) for rate in rates)
                + ">"
            )
        if source is not None:
            source_file, source_line, source_column = source
            attributes.extend(
                (
                    "ac.source_file = " + canonical_mlir_string(source_file),
                    f"ac.source_line = {source_line} : i64",
                    f"ac.source_column = {source_column} : i64",
                )
            )
        return "{" + ", ".join(attributes) + "}"

    def emit_queue(
        queue: QueueBinding,
        output_ssa: str | None,
        mapping: dict[str, str],
        indent: str,
    ) -> None:
        if queue.input_name is None and queue.rule_name is None:
            assert output_ssa is not None
            lines.append(
                f"{indent}%{output_ssa} = ac.source depth {queue.depth} "
                f"latency {queue.latency} "
                f"{queue_attributes(queue.name, (queue.rate,))} : "
                + _render_queue_type(queue.payload, lanes=queue.lanes, rate=queue.rate)
                + _render_source_frame_location(queue.source)
            )
            mapping[queue.name] = output_ssa
            return
        assert queue.argument is not None
        if queue.rule_name is None:
            assert queue.expression is not None
        if queue.rule_name is not None:
            rule_input_names = queue.rule_input_names
            rule_arguments = queue.rule_arguments
            rule_payloads = queue.rule_payloads
            selected_input_names = tuple(
                effective_input.get((queue.name, index), name)
                for index, name in enumerate(rule_input_names)
            )
            input_ssas = tuple(mapping[name] for name in selected_input_names)
            root_names = tuple(
                "item" if len(rule_arguments) == 1 else f"item{index}"
                for index in range(len(rule_arguments))
            )
            root_values = {
                argument: (root_name, payload)
                for argument, root_name, payload in zip(
                    rule_arguments, root_names, rule_payloads, strict=True
                )
            }
            rule_table_views: dict[str, tuple[str, ast.expr, ValueType]] = {}
            if queue.rule_table_read_name is not None:
                assert queue.rule_table is not None
                assert queue.rule_table_read_index is not None
                entry_type, _, _ = table_domains[queue.rule_table]
                rule_table_views[queue.rule_table_read_name] = (
                    queue.rule_table,
                    queue.rule_table_read_index,
                    entry_type,
                )
            emitter = _ExpressionEmitter(
                payloads,
                queue.argument,
                queue.payload,
                root_name="item",
                root_values=root_values,
                table_views=rule_table_views,
                table_domains=table_domains,
                state_views={
                    owner.argument: (
                        owner.owner_kind,
                        owner.variable,
                        owner.value_type,
                        owner.entries,
                        owner.shape,
                    )
                    for owner in queue.rule_state_owners
                },
                slot_views={
                    owner.argument: (owner.slot, owner.payload)
                    for owner in queue.rule_slot_owners
                },
                enum_types=enum_types,
                bitfields=bitfields,
                invariants=invariants,
                helpers=helpers,
                inline_pure_helpers=module is not None,
            )
            rule_expressions: list[ast.expr] = []
            if queue.expression is not None:
                rule_expressions.append(queue.expression)
            if queue.rule_guard is not None:
                rule_expressions.append(queue.rule_guard)
            if queue.rule_effect_guard is not None:
                rule_expressions.append(queue.rule_effect_guard)
            if queue.rule_output_guard is not None:
                rule_expressions.append(queue.rule_output_guard)
            rule_expressions.extend(queue.rule_output_expressions)
            rule_expressions.extend(queue.rule_output_guards)
            rule_expressions.extend(local.value for local in queue.rule_locals)
            rule_expressions.extend(
                local.guard for local in queue.rule_locals if local.guard is not None
            )
            for find in queue.rule_finds:
                rule_expressions.append(find.predicate)
                if find.key is not None:
                    rule_expressions.append(find.key)
                if find.row is not None:
                    rule_expressions.append(find.row)
            rule_expressions.extend(
                read.index for read in queue.rule_state_reads if read.index is not None
            )
            for write in queue.rule_state_writes:
                if write.guard is not None:
                    rule_expressions.append(write.guard)
                if write.index is not None:
                    rule_expressions.append(write.index)
                rule_expressions.append(write.value)
            rule_expressions.extend(
                release.guard
                for release in queue.rule_slot_releases
                if release.guard is not None
            )
            referenced_names = {
                node.id
                for expression in rule_expressions
                for node in ast.walk(expression)
                if isinstance(node, ast.Name)
            }
            referenced_names.update(
                local.prior_name
                for local in queue.rule_locals
                if local.prior_name is not None
            )
            for state_owner in queue.rule_state_owners:
                if (
                    state_owner.entries != 1
                    or state_owner.argument in emitter.root_values
                    or state_owner.argument not in referenced_names
                ):
                    continue
                state_read = emitter._new()
                if state_owner.owner_kind != "var":
                    raise QueueFrontendError(
                        "ACPY-RULE-008: Table state requires an indexed access"
                    )
                emitter.lines.append(
                    f"    %{state_read} = ac.var.read @{state_owner.variable} : "
                    f"!ac.var<{_render_type(state_owner.value_type)}>"
                )
                emitter.root_values[state_owner.argument] = (
                    state_read,
                    state_owner.value_type,
                )
            for state_read_binding in queue.rule_state_reads:
                assert state_read_binding.index is not None
                if state_read_binding.owner_kind == "table":
                    read_index, read_index_type = emitter.emit_table_index(
                        state_read_binding.variable, state_read_binding.index
                    )
                else:
                    read_index, read_index_type = emitter.emit(state_read_binding.index)
                read_index_width = _integer_width(read_index_type)
                if read_index_width is None:
                    raise QueueFrontendError(
                        "ACPY-RULE-008: persistent list read index must be an "
                        "exact-width integer"
                    )
                emitter.reject_constant_index_outside(
                    read_index,
                    read_index_type,
                    state_read_binding.entries,
                    "ACPY-RULE-008: persistent list read index is out of range",
                )
                state_read = emitter._new()
                read_operation = (
                    "ac.var.read_element"
                    if state_read_binding.owner_kind == "var"
                    else "ac.table.get"
                )
                emitter.lines.append(
                    f"    %{state_read} = {read_operation} "
                    f"@{state_read_binding.variable}[%{read_index}] : "
                    f"!ac.var<{_render_type(read_index_type)}> -> "
                    f"!ac.var<{_render_type(state_read_binding.value_type)}>"
                )
                add_display_name(emitter.lines, state_read, state_read_binding.name)
                emitter.root_values[state_read_binding.name] = (
                    state_read,
                    state_read_binding.value_type,
                )
            find_local_values = {
                local.name: copy.deepcopy(local.value)
                for local in queue.rule_locals
                if local.guard is None
            }
            guarded_find_locals = {
                local.name for local in queue.rule_locals if local.guard is not None
            }
            for find in queue.rule_finds:
                captured_names = {
                    candidate.id
                    for expression in (find.predicate, find.key, find.row)
                    if expression is not None
                    for candidate in ast.walk(expression)
                    if isinstance(candidate, ast.Name)
                }
                guarded_captures = guarded_find_locals & captured_names
                if guarded_captures:
                    raise QueueFrontendError(
                        "ACPY-RULE-009: find cannot capture branch-local values: "
                        + ", ".join(sorted(guarded_captures))
                    )
                index_width = max(1, (find.entries - 1).bit_length())
                domain_entries = (
                    find.shape[-1] if find.row is not None else find.entries
                )
                mask_type = _candidate_mask_type(domain_entries)
                row_value: str | None = None
                row_type: ValueType | None = None
                table_domain_base: str | None = None
                table_domain_base_type: ValueType | None = None
                if find.row is not None:
                    row_type = BitsType(_table_axis_width(find.shape[0]))
                    previous_deferred = dict(emitter.deferred_values)
                    emitter.deferred_values.update(find_local_values)
                    try:
                        row_value, actual_row_type = emitter.emit(find.row, row_type)
                    finally:
                        emitter.deferred_values = previous_deferred
                    if not _types_compatible(actual_row_type, row_type):
                        raise QueueFrontendError(
                            "ACPY-RULE-009: row view index requires the canonical "
                            "first-axis width"
                        )
                    emitter.reject_constant_index_outside(
                        row_value,
                        actual_row_type,
                        find.shape[0],
                        "ACPY-RULE-009: row view index is out of range",
                    )
                    if find.owner_kind == "table":
                        zero_type = BitsType(_table_axis_width(find.shape[1]))
                        zero_value, actual_zero_type = emitter.emit(
                            ast.Constant(0), zero_type
                        )
                        table_domain_base_type = BitsType(index_width)
                        table_domain_base = emitter._new()
                        emitter.lines.append(
                            f"    %{table_domain_base} = ac.table.index "
                            f"@{find.variable} [%{row_value}, %{zero_value}] : "
                            f"!ac.var<{_render_type(row_type)}>, "
                            f"!ac.var<{_render_type(actual_zero_type)}> -> "
                            f"!ac.var<{_render_type(table_domain_base_type)}>"
                        )
                        emitter._remember(table_domain_base, table_domain_base_type)
                predicate_emitter = _ExpressionEmitter(
                    payloads,
                    find.predicate_argument,
                    find.value_type,
                    root_name="entry",
                    root_values=emitter.root_values,
                    prefix=f"find{emitter.index}_predicate_",
                    state_views=emitter.state_views,
                    table_domains=emitter.table_domains,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    invariants=invariants,
                    helpers=helpers,
                    inline_pure_helpers=True,
                )
                predicate_emitter.deferred_values.update(find_local_values)
                predicate, predicate_type = predicate_emitter.emit(
                    find.predicate, BoolType()
                )
                if not _is_bool_like(predicate_type):
                    raise QueueFrontendError(
                        "ACPY-RULE-009: find where predicate must lower to bool"
                    )
                mask = emitter._new()
                match_operation = (
                    "ac.var.match" if find.owner_kind == "var" else "ac.table.match"
                )
                if find.owner_kind == "var":
                    domain_prefix = (
                        f" row %{row_value} : !ac.var<{_render_type(row_type)}>"
                        if row_value is not None and row_type is not None
                        else ""
                    )
                else:
                    domain_prefix = (
                        f" base %{table_domain_base} : "
                        f"!ac.var<{_render_type(table_domain_base_type)}>"
                        if table_domain_base is not None
                        and table_domain_base_type is not None
                        else ""
                    )
                emitter.lines.append(
                    f"    %{mask} = {match_operation} @{find.variable}"
                    + domain_prefix
                    + " predicate {"
                )
                emitter.lines.append(
                    f"    ^predicate(%entry: !ac.var<{_render_type(find.value_type)}>):"
                )
                emitter.lines.extend(predicate_emitter.lines)
                emitter.lines.append(
                    f"      {match_operation}.yield %{predicate} : !ac.var<i1>"
                    + _render_source_frame_location(source_frame(find.predicate))
                )
                if find.owner_kind == "table":
                    domain_axes = (
                        (1,) if find.row is not None else tuple(range(len(find.shape)))
                    )
                    domain_shape = (
                        (find.shape[1],) if find.row is not None else find.shape
                    )
                    domain_strides = (
                        (1,)
                        if find.row is not None
                        else tuple(
                            _product(find.shape[axis + 1 :])
                            for axis in range(len(find.shape))
                        )
                    )
                    domain_attributes = " " + _render_table_domain_attributes(
                        CandidateSetBinding(
                            find.name,
                            find.variable,
                            domain_entries,
                            domain_axes,
                            domain_shape,
                            domain_strides,
                            0,
                            find.predicate_argument,
                            find.predicate,
                            (),
                            0,
                        )
                    )
                else:
                    domain_attributes = ""
                emitter.lines.append(
                    f"    }}{domain_attributes} -> !ac.var<{_render_type(mask_type)}>"
                    + _render_source_frame_location(
                        source_frame(find.row or find.predicate)
                    )
                )
                selected_index = emitter._new()
                selected_valid = emitter._new()
                if find.key is None:
                    if find.owner_kind == "var":
                        choice_contract = (
                            'policy "first" key {} '
                            f"{{ac.query = {canonical_mlir_string(find.name)}}}"
                        )
                    else:
                        choice_contract = (
                            "policy #ac<table_selection_policy first> "
                            f"stable_id {canonical_mlir_string(find.name)} key {{}}"
                        )
                    emitter.lines.append(
                        f"    %{selected_index}, %{selected_valid} = "
                        f"{'ac.var.choose' if find.owner_kind == 'var' else 'ac.table.choose'} "
                        f"@{find.variable} %{mask} : "
                        f"!ac.var<{_render_type(mask_type)}> count 1 "
                        f"{choice_contract} -> "
                        f"!ac.var<i{index_width}>, !ac.var<i1>"
                        + _render_source_frame_location(source_frame(find.predicate))
                    )
                else:
                    assert find.key_argument is not None
                    key_emitter = _ExpressionEmitter(
                        payloads,
                        find.key_argument,
                        find.value_type,
                        root_name="entry",
                        root_values=emitter.root_values,
                        prefix=f"find{emitter.index}_key_",
                        state_views=emitter.state_views,
                        table_domains=emitter.table_domains,
                        enum_types=enum_types,
                        bitfields=bitfields,
                        invariants=invariants,
                        helpers=helpers,
                    )
                    key_emitter.deferred_values.update(find_local_values)
                    key, key_type = key_emitter.emit(find.key)
                    if _integer_width(key_type) is None:
                        raise QueueFrontendError(
                            "ACPY-RULE-009: find key must lower to an integer"
                        )
                    choice_operation = (
                        "ac.var.choose"
                        if find.owner_kind == "var"
                        else "ac.table.choose"
                    )
                    choice_policy = (
                        'policy "min"'
                        if find.owner_kind == "var"
                        else "policy #ac<table_selection_policy min> "
                        "key_order #ac<table_key_ordering unsigned> "
                        f"stable_id {canonical_mlir_string(find.name)}"
                    )
                    emitter.lines.append(
                        f"    %{selected_index}, %{selected_valid} = "
                        f"{choice_operation} @{find.variable} %{mask} : "
                        f"!ac.var<{_render_type(mask_type)}> count 1 "
                        f"{choice_policy} key {{"
                    )
                    emitter.lines.append(
                        f"    ^key(%entry: !ac.var<{_render_type(find.value_type)}>):"
                    )
                    emitter.lines.extend(key_emitter.lines)
                    emitter.lines.append(
                        f"      {choice_operation}.yield %{key} : "
                        f"!ac.var<{_render_type(key_type)}>"
                    )
                    suffix = (
                        f" {{ac.query = {canonical_mlir_string(find.name)}}}"
                        if find.owner_kind == "var"
                        else ""
                    )
                    emitter.lines.append(
                        f"    }}{suffix} -> "
                        f"!ac.var<i{index_width}>, !ac.var<i1>"
                        + _render_source_frame_location(
                            source_frame(find.key or find.predicate)
                        )
                    )
                emitter.find_values[find.name] = (
                    selected_index,
                    BitsType(index_width),
                    selected_valid,
                    BoolType(),
                    find.variable,
                    find.value_type,
                    None,
                    find.owner_kind,
                )
            for local in queue.rule_locals:
                previous_deferred = (
                    None
                    if local.prior_name is None
                    else emitter.deferred_values.get(local.prior_name)
                )
                if local.guard is not None and previous_deferred is not None:
                    guard_expression: ast.expr = copy.deepcopy(local.guard)
                    if local.guard_negated:
                        guard_expression = ast.UnaryOp(
                            op=ast.Not(), operand=guard_expression
                        )
                    emitter.deferred_values[local.name] = ast.fix_missing_locations(
                        ast.IfExp(
                            test=guard_expression,
                            body=copy.deepcopy(local.value),
                            orelse=copy.deepcopy(previous_deferred),
                        )
                    )
                    continue
                local_static: StaticValue | None = None
                if local.guard is None:
                    try:
                        local_static = evaluate_static(
                            local.value,
                            StaticEnvironment(
                                {
                                    name: value.value
                                    for name, value in emitter.deferred_values.items()
                                    if isinstance(value, ast.Constant)
                                }
                            ),
                        )
                    except ValueError:
                        pass
                if type(local_static) in {bool, int}:
                    emitter.root_values.pop(local.name, None)
                    emitter.deferred_values[local.name] = ast.Constant(
                        value=local_static
                    )
                    continue
                if (
                    local.guard is None
                    and isinstance(local.value, ast.Call)
                    and _decorator_name(local.value.func).rsplit(".", 1)[-1]
                    in {"checked", "onehot_enum"}
                ):
                    emitter.root_values.pop(local.name, None)
                    emitter.deferred_values[local.name] = copy.deepcopy(local.value)
                    continue
                local_value, local_type = emitter.emit(local.value)
                previous = (
                    None
                    if local.prior_name is None
                    else emitter.root_values.get(local.prior_name)
                )
                if previous is not None:
                    _, previous_type = previous
                    if not _types_compatible(local_type, previous_type):
                        raise QueueFrontendError(
                            "ACPY-RULE-011: local reassignments must preserve "
                            "one exact type"
                        )
                emitter.deferred_values.pop(local.name, None)
                if local.guard is not None:
                    guard_expression: ast.expr = local.guard
                    if local.guard_negated:
                        guard_expression = ast.UnaryOp(
                            op=ast.Not(), operand=copy.deepcopy(local.guard)
                        )
                    guard, guard_type = emitter.emit(guard_expression, BoolType())
                    if not _is_bool_like(guard_type):
                        raise QueueFrontendError(
                            "ACPY-RULE-011: branch-local assignment guard "
                            "must lower to bool"
                        )
                    if previous is not None:
                        previous_value, previous_type = previous
                        selected = emitter._new()
                        emitter.lines.append(
                            f"    %{selected} = ac.var.select %{guard}, "
                            f"%{local_value}, %{previous_value} : !ac.var<i1>, "
                            f"!ac.var<{_render_type(local_type)}> -> "
                            f"!ac.var<{_render_type(local_type)}>"
                        )
                        local_value = selected
                add_display_name(emitter.lines, local_value, local.name)
                emitter.root_values[local.name] = (local_value, local_type)
            if queue.rule_var is not None:
                assert queue.rule_var_argument is not None
                if queue.rule_var_index is None:
                    state_read = emitter._new()
                    emitter.lines.append(
                        f"    %{state_read} = ac.var.read @{queue.rule_var} : "
                        f"!ac.var<{_render_type(queue.payload)}>"
                    )
                    emitter.root_values[queue.rule_var_argument] = (
                        state_read,
                        queue.payload,
                    )
                elif queue.rule_var_read_name is not None:
                    assert queue.rule_var_read_index is not None
                    read_index, read_index_type = emitter.emit(
                        queue.rule_var_read_index
                    )
                    if _integer_width(read_index_type) is None:
                        raise QueueFrontendError(
                            "ACPY-RULE-004: persistent list index must be an "
                            "exact-width integer"
                        )
                    state_read = emitter._new()
                    emitter.lines.append(
                        f"    %{state_read} = ac.var.read_element "
                        f"@{queue.rule_var}[%{read_index}] : "
                        f"!ac.var<{_render_type(read_index_type)}> -> "
                        f"!ac.var<{_render_type(queue.payload)}>"
                    )
                    add_display_name(
                        emitter.lines, state_read, queue.rule_var_read_name
                    )
                    emitter.root_values[queue.rule_var_read_name] = (
                        state_read,
                        queue.payload,
                    )
            guard_result: str | None = None
            if queue.rule_guard is not None:
                guard_result, guard_type = emitter.emit(queue.rule_guard, BoolType())
                if not _is_bool_like(guard_type):
                    raise QueueFrontendError(
                        "ACPY-RULE-007: rule condition must lower to bool"
                    )
            effect_guard_result: str | None = None
            if queue.rule_effect_guard is not None:
                effect_guard_result, effect_guard_type = emitter.emit(
                    queue.rule_effect_guard, BoolType()
                )
                if not _is_bool_like(effect_guard_type):
                    raise QueueFrontendError(
                        "ACPY-RULE-010: conditional effect must lower to bool"
                    )
            output_guard_result: str | None = None
            if queue.rule_output_guard is not None:
                output_guard_result, output_guard_type = emitter.emit(
                    queue.rule_output_guard, BoolType()
                )
                if not _is_bool_like(output_guard_type):
                    raise QueueFrontendError(
                        "ACPY-RULE-012: optional output condition must lower to bool"
                    )
            multi_output_guard_results: list[str] = []
            for output_guard in queue.rule_output_guards:
                presence, presence_type = emitter.emit(output_guard, BoolType())
                if not _is_bool_like(presence_type):
                    raise QueueFrontendError(
                        "ACPY-RULE-014: output presence must lower to bool"
                    )
                multi_output_guard_results.append(presence)
            condition_result = guard_result
            if effect_guard_result is not None:
                condition_result = emitter._new()
                emitter.lines.append(
                    f"    %{condition_result} = ac.var.constant true as !ac.var<i1>"
                )
            elif condition_result is None and (
                output_guard_result is not None
                or any(write.guard is not None for write in queue.rule_state_writes)
                or multi_output_guard_results
            ):
                if (
                    output_guard_result is not None
                    and not queue.rule_input_names
                    and queue.rule_slot_owners
                    and not queue.rule_state_writes
                ):
                    condition_result = output_guard_result
                else:
                    condition_result = emitter._new()
                    emitter.lines.append(
                        f"    %{condition_result} = ac.var.constant true as !ac.var<i1>"
                    )
            index_result: str | None = None
            index_type: ValueType | None = None
            write_result: str | None = None
            if queue.rule_table is not None:
                assert queue.rule_table_index is not None
                assert queue.rule_table_value is not None
                index_result, index_type = emitter.emit_table_index(
                    queue.rule_table, queue.rule_table_index
                )
                index_width = _integer_width(index_type)
                if index_width is None:
                    raise QueueFrontendError(
                        "ACPY-RULE-004: stateful rule Table index must be an "
                        "exact-width integer"
                    )
                _, entries, _ = table_domains[queue.rule_table]
                emitter.reject_constant_index_outside(
                    index_result,
                    index_type,
                    entries,
                    "ACPY-RULE-004: stateful rule Table index is out of range",
                )
                write_result, write_type = emitter.emit(queue.rule_table_value)
                if not _types_compatible(write_type, queue.payload):
                    raise QueueFrontendError(
                        "ACPY-RULE-004: stateful rule assignment must write "
                        "one complete Table Entry"
                    )
            var_write_result: str | None = None
            var_index_result: str | None = None
            var_index_type: ValueType | None = None
            if queue.rule_var is not None:
                assert queue.rule_var_argument is not None
                assert queue.rule_var_value is not None
                if queue.rule_var_index is not None:
                    var_index_result, var_index_type = emitter.emit(
                        queue.rule_var_index
                    )
                    var_index_width = _integer_width(var_index_type)
                    if var_index_width is None:
                        raise QueueFrontendError(
                            "ACPY-RULE-004: persistent list index must be an "
                            "exact-width integer"
                        )
                    _, entries = variable_domains[queue.rule_var]
                    emitter.reject_constant_index_outside(
                        var_index_result,
                        var_index_type,
                        entries,
                        "ACPY-RULE-004: persistent list index is out of range",
                    )
                variable_type, _ = variable_domains[queue.rule_var]
                var_write_result, var_write_type = emitter.emit(queue.rule_var_value)
                if not _types_compatible(var_write_type, variable_type):
                    raise QueueFrontendError(
                        "ACPY-RULE-004: persistent variable assignment must "
                        "preserve its declared type"
                    )
                emitter.root_values[queue.rule_var_argument] = (
                    var_write_result,
                    variable_type,
                )
            multi_state_results: list[
                tuple[
                    RuleStateWriteBinding,
                    ValueType | None,
                    str | None,
                    str,
                    str | None,
                ]
            ] = []
            branch_guard_results: dict[tuple[str, bool], str] = {}

            def emit_effect_guard(
                guarded: RuleStateWriteBinding | RuleSlotReleaseBinding,
            ) -> str | None:
                if guarded.guard is None:
                    return None
                guard_key = ast.dump(guarded.guard, include_attributes=False)
                cache_key = (guard_key, guarded.guard_negated)
                cached = branch_guard_results.get(cache_key)
                if cached is not None:
                    return cached
                base_key = (guard_key, False)
                base_result = branch_guard_results.get(base_key)
                if base_result is None:
                    base_result, base_type = emitter.emit(guarded.guard, BoolType())
                    if not _is_bool_like(base_type):
                        raise QueueFrontendError(
                            "ACPY-RULE-011: branch condition must lower to bool"
                        )
                    branch_guard_results[base_key] = base_result
                if not guarded.guard_negated:
                    return base_result
                false_value = emitter._new()
                emitter.lines.append(
                    f"    %{false_value} = ac.var.constant false as !ac.var<i1>"
                )
                result = emitter._new()
                emitter.lines.append(
                    f'    %{result} = ac.var.cmp "eq" %{base_result}, '
                    f"%{false_value} : !ac.var<i1> -> !ac.var<i1>"
                )
                branch_guard_results[cache_key] = result
                return result

            def emit_state_guard(state_write: RuleStateWriteBinding) -> str | None:
                return emit_effect_guard(state_write)

            def emit_state_value(state_write: RuleStateWriteBinding) -> str:
                value, value_type = emitter.emit(
                    state_write.value, state_write.value_type
                )
                if not _types_compatible(value_type, state_write.value_type):
                    raise QueueFrontendError(
                        "ACPY-RULE-008: persistent state assignment must "
                        "preserve its declared type"
                    )
                return value

            def emit_state_index(
                state_write: RuleStateWriteBinding,
            ) -> tuple[str | None, ValueType | None]:
                if state_write.index is None:
                    return None, None
                expected_index_type = BitsType(
                    max(1, (state_write.entries - 1).bit_length())
                )
                if state_write.owner_kind == "table":
                    index, index_type = emitter.emit_table_index(
                        state_write.variable, state_write.index
                    )
                else:
                    index, index_type = emitter.emit(
                        state_write.index, expected_index_type
                    )
                index_width = _integer_width(index_type)
                if index_width is None:
                    raise QueueFrontendError(
                        "ACPY-RULE-008: persistent list index must be an "
                        "exact-width integer"
                    )
                emitter.reject_constant_index_outside(
                    index,
                    index_type,
                    state_write.entries,
                    "ACPY-RULE-008: persistent list index is out of range",
                )
                return index, index_type

            writes_by_owner: dict[str, list[RuleStateWriteBinding]] = {}
            for state_write in queue.rule_state_writes:
                writes_by_owner.setdefault(state_write.variable, []).append(state_write)
            writes_by_variable: dict[
                tuple[str, str, str, tuple[str, ...]],
                list[RuleStateWriteBinding],
            ] = {}
            for variable, owner_writes in writes_by_owner.items():
                complementary_pair = (
                    len(owner_writes) == 2
                    and all(write.guard is not None for write in owner_writes)
                    and {write.guard_negated for write in owner_writes} == {False, True}
                    and ast.dump(owner_writes[0].guard, include_attributes=False)
                    == ast.dump(owner_writes[1].guard, include_attributes=False)
                    and owner_writes[0].mode == owner_writes[1].mode
                    and owner_writes[0].write_fields == owner_writes[1].write_fields
                )
                if complementary_pair:
                    writes_by_variable[
                        (
                            variable,
                            "<complementary>",
                            owner_writes[0].mode,
                            owner_writes[0].write_fields,
                        )
                    ] = owner_writes
                    continue
                for state_write in owner_writes:
                    index_identity = (
                        "<scalar>"
                        if state_write.index is None
                        else ast.dump(state_write.index, include_attributes=False)
                    )
                    writes_by_variable.setdefault(
                        (
                            variable,
                            index_identity,
                            state_write.mode,
                            state_write.write_fields,
                        ),
                        [],
                    ).append(state_write)
            for owner_writes in writes_by_variable.values():
                complementary_pair = (
                    len(owner_writes) == 2
                    and all(write.guard is not None for write in owner_writes)
                    and {write.guard_negated for write in owner_writes} == {False, True}
                    and ast.dump(owner_writes[0].guard, include_attributes=False)
                    == ast.dump(owner_writes[1].guard, include_attributes=False)
                )
                if complementary_pair:
                    true_write = next(
                        write for write in owner_writes if not write.guard_negated
                    )
                    false_write = next(
                        write for write in owner_writes if write.guard_negated
                    )
                    condition = emit_state_guard(true_write)
                    assert condition is not None
                    true_index, true_index_type = emit_state_index(true_write)
                    false_index, false_index_type = emit_state_index(false_write)
                    if not _types_compatible(true_index_type, false_index_type):
                        raise QueueFrontendError(
                            "ACPY-RULE-011: same-owner branch indices must have "
                            "one exact type"
                        )
                    true_value = emit_state_value(true_write)
                    false_value = emit_state_value(false_write)
                    selected_value = emitter._new()
                    emitter.lines.append(
                        f"    %{selected_value} = ac.var.select %{condition}, "
                        f"%{true_value}, %{false_value} : !ac.var<i1>, "
                        f"!ac.var<{_render_type(true_write.value_type)}> -> "
                        f"!ac.var<{_render_type(true_write.value_type)}>"
                    )
                    selected_index: str | None = None
                    if true_index is not None:
                        assert false_index is not None
                        assert true_index_type is not None
                        selected_index = emitter._new()
                        emitter.lines.append(
                            f"    %{selected_index} = ac.var.select %{condition}, "
                            f"%{true_index}, %{false_index} : !ac.var<i1>, "
                            f"!ac.var<{_render_type(true_index_type)}> -> "
                            f"!ac.var<{_render_type(true_index_type)}>"
                        )
                    multi_state_results.append(
                        (
                            true_write,
                            selected_index,
                            true_index_type,
                            selected_value,
                            None,
                        )
                    )
                    emitter.root_values[true_write.argument] = (
                        selected_value,
                        true_write.value_type,
                    )
                    continue
                if len(owner_writes) > 1:
                    if any(write.guard is None for write in owner_writes):
                        raise QueueFrontendError(
                            "ACPY-RULE-011: repeated same-owner proposals require "
                            "path predicates"
                        )
                    rendered = [
                        (
                            write,
                            emit_state_guard(write),
                            *emit_state_index(write),
                            emit_state_value(write),
                        )
                        for write in owner_writes
                    ]
                    guards = [guard for _, guard, _, _, _ in rendered]
                    assert all(guard is not None for guard in guards)
                    combined_guard = guards[0]
                    assert combined_guard is not None
                    for guard in guards[1:]:
                        assert guard is not None
                        joined = emitter._new()
                        emitter.lines.append(
                            f"    %{joined} = ac.var.or %{combined_guard}, "
                            f"%{guard} : !ac.var<i1>"
                        )
                        combined_guard = joined
                    (
                        selected_write,
                        _,
                        selected_index,
                        selected_index_type,
                        selected_value,
                    ) = rendered[0]
                    for (
                        candidate,
                        guard,
                        candidate_index,
                        candidate_index_type,
                        candidate_value,
                    ) in rendered[1:]:
                        assert guard is not None
                        if not _types_compatible(
                            selected_index_type, candidate_index_type
                        ):
                            raise QueueFrontendError(
                                "ACPY-RULE-011: same-owner branch indices must "
                                "have one exact type"
                            )
                        joined_value = emitter._new()
                        emitter.lines.append(
                            f"    %{joined_value} = ac.var.select %{guard}, "
                            f"%{candidate_value}, %{selected_value} : "
                            f"!ac.var<i1>, "
                            f"!ac.var<{_render_type(candidate.value_type)}> -> "
                            f"!ac.var<{_render_type(candidate.value_type)}>"
                        )
                        selected_value = joined_value
                        if candidate_index is not None:
                            if selected_index is None or candidate_index_type is None:
                                raise QueueFrontendError(
                                    "ACPY-RULE-011: same-owner branch index shape "
                                    "must match"
                                )
                            joined_index = emitter._new()
                            emitter.lines.append(
                                f"    %{joined_index} = ac.var.select %{guard}, "
                                f"%{candidate_index}, %{selected_index} : "
                                f"!ac.var<i1>, "
                                f"!ac.var<{_render_type(candidate_index_type)}> -> "
                                f"!ac.var<{_render_type(candidate_index_type)}>"
                            )
                            selected_index = joined_index
                    multi_state_results.append(
                        (
                            selected_write,
                            selected_index,
                            selected_index_type,
                            selected_value,
                            combined_guard,
                        )
                    )
                    continue
                state_write = owner_writes[0]
                state_guard_result = emit_state_guard(state_write)
                state_index, state_index_type = emit_state_index(state_write)
                state_value = emit_state_value(state_write)
                multi_state_results.append(
                    (
                        state_write,
                        state_index,
                        state_index_type,
                        state_value,
                        state_guard_result,
                    )
                )
                if state_write.index is None:
                    emitter.root_values[state_write.argument] = (
                        state_value,
                        state_write.value_type,
                    )
            slot_release_results: list[tuple[RuleSlotReleaseBinding, str]] = []
            for release in queue.rule_slot_releases:
                release_guard = emit_effect_guard(release)
                if release_guard is None:
                    release_guard = emitter._new()
                    emitter.lines.append(
                        f"    %{release_guard} = ac.var.constant true as !ac.var<i1>"
                    )
                slot_release_results.append((release, release_guard))
            result: str | None = None
            multi_output_results: list[str] = []
            if queue.rule_has_output:
                if queue.rule_output_expressions:
                    for ordinal, (expression, payload) in enumerate(
                        zip(
                            queue.rule_output_expressions,
                            queue.rule_output_payloads,
                            strict=True,
                        )
                    ):
                        output_value, output_type = emitter.emit(expression, payload)
                        if not _types_compatible(output_type, payload):
                            raise QueueFrontendError(
                                "ACPY-RULE-014: rule output ordinal "
                                f"{ordinal} does not match its annotated type"
                            )
                        multi_output_results.append(output_value)
                    result = multi_output_results[0]
                else:
                    assert queue.expression is not None
                    result, result_type = emitter.emit(queue.expression)
                    if not _types_compatible(result_type, queue.payload):
                        raise QueueFrontendError(
                            "ACPY-RULE-004: rule result must preserve Queue payload type"
                        )
                assert output_ssa is not None
            output_ssas = (
                tuple(
                    name if not queue.scope else f"{name}_local"
                    for name in queue.rule_output_names
                )
                if queue.rule_output_names
                else (() if output_ssa is None else (output_ssa,))
            )
            stable_rule_path = (
                (*queue.scope, queue.name)
                if module is None
                else (module.name, *queue.scope, queue.name)
            )
            lines.append(
                (
                    f"{indent}" + ", ".join(f"%{name}" for name in output_ssas) + " = "
                    if output_ssas
                    else indent
                )
                + "ac.rule "
                + ", ".join(f"%{value}" for value in input_ssas)
                + " "
                + (
                    "depths ["
                    + ", ".join(str(queue.depth) for _ in output_ssas)
                    + "] latencies ["
                    + ", ".join(str(queue.latency) for _ in output_ssas)
                    + "] "
                    if queue.rule_has_output
                    else "depths [] latencies [] "
                )
                + f"name {canonical_mlir_string(queue.rule_name)} "
                f"stable_id {canonical_mlir_string('/'.join(stable_rule_path))} "
                f'domain "cycle" type exact {{'
            )
            block_arguments = ", ".join(
                f"%{root_name}: !ac.var<{_render_type(payload)}>"
                for root_name, payload in zip(root_names, rule_payloads, strict=True)
            )
            lines.append(f"{indent}^rule({block_arguments}):")
            lines.extend(indent + line[2:] for line in emitter.lines)
            if condition_result is not None:
                lines.append(
                    f"{indent}  ac.rule.condition %{condition_result} : !ac.var<i1>"
                )
            effect_presence = (
                f" when %{effect_guard_result} : !ac.var<i1>"
                if effect_guard_result is not None
                else (
                    f" when %{condition_result} : !ac.var<i1>"
                    if output_guard_result is not None or multi_output_guard_results
                    else ""
                )
            )
            if queue.rule_var is not None:
                assert var_write_result is not None
                variable_type, _ = variable_domains[queue.rule_var]
                if queue.rule_var_index is None:
                    lines.append(
                        f"{indent}  ac.var.assign @{queue.rule_var} = "
                        f"%{var_write_result}{effect_presence} "
                        f"{rule_writer_arbitration(queue, queue.rule_var)} : "
                        f"!ac.var<{_render_type(variable_type)}>"
                    )
                else:
                    assert var_index_result is not None
                    assert var_index_type is not None
                    lines.append(
                        f"{indent}  ac.var.assign_element @{queue.rule_var}"
                        f"[%{var_index_result}] = %{var_write_result}"
                        f"{effect_presence} "
                        f"{rule_writer_arbitration(queue, queue.rule_var)} : "
                        f"!ac.var<{_render_type(var_index_type)}>, "
                        f"!ac.var<{_render_type(variable_type)}>"
                    )
            for (
                state_write,
                state_index,
                state_index_type,
                state_value,
                state_guard_result,
            ) in multi_state_results:
                state_effect_presence = (
                    effect_presence
                    if state_guard_result is None
                    else f" when %{state_guard_result} : !ac.var<i1>"
                )
                if state_index is None:
                    if state_write.owner_kind != "var":
                        raise QueueFrontendError(
                            "ACPY-RULE-008: Table state requires an indexed proposal"
                        )
                    lines.append(
                        f"{indent}  ac.var.assign @{state_write.variable} = "
                        f"%{state_value}{state_effect_presence} "
                        f"{rule_writer_arbitration(queue, state_write.variable)} : "
                        f"!ac.var<{_render_type(state_write.value_type)}>"
                    )
                else:
                    assert state_index_type is not None
                    if state_write.owner_kind == "var":
                        lines.append(
                            f"{indent}  ac.var.assign_element "
                            f"@{state_write.variable}[%{state_index}] = "
                            f"%{state_value}{state_effect_presence} "
                            f"{rule_writer_arbitration(queue, state_write.variable)} : "
                            f"!ac.var<{_render_type(state_index_type)}>, "
                            f"!ac.var<{_render_type(state_write.value_type)}>"
                        )
                    else:
                        fields = (
                            "["
                            + ", ".join(
                                canonical_mlir_string(item)
                                for item in state_write.write_fields
                            )
                            + "]"
                        )
                        lines.append(
                            f"{indent}  ac.table.propose "
                            f"@{state_write.variable}[%{state_index}] = "
                            f"%{state_value}{state_effect_presence} "
                            f'mode "{state_write.mode}" '
                            f"write_fields {fields} "
                            f"{rule_writer_arbitration(queue, state_write.variable)} : "
                            f"!ac.var<{_render_type(state_index_type)}>, "
                            f"!ac.var<{_render_type(state_write.value_type)}>"
                        )
            if queue.rule_table is not None:
                assert index_result is not None
                assert index_type is not None
                assert write_result is not None
                fields = (
                    "["
                    + ", ".join(
                        canonical_mlir_string(item) for item in queue.rule_write_fields
                    )
                    + "]"
                )
                lines.append(
                    f"{indent}  ac.table.propose @{queue.rule_table} "
                    f"[%{index_result}] = %{write_result}{effect_presence} "
                    f'mode "{queue.rule_write_mode}" '
                    f"write_fields {fields} "
                    f"{rule_writer_arbitration(queue, queue.rule_table)} : "
                    f"!ac.var<{_render_type(index_type)}>, "
                    f"!ac.var<{_render_type(queue.payload)}>"
                )
            for release, release_guard in slot_release_results:
                lines.append(
                    f"{indent}  ac.slot.propose_release @{release.slot} when "
                    f"%{release_guard} : !ac.var<i1>"
                )
            if queue.rule_has_output:
                assert result is not None
                if multi_output_results:
                    ready_values: list[str] = []
                    for ordinal, (output_value, payload, presence) in enumerate(
                        zip(
                            multi_output_results,
                            queue.rule_output_payloads,
                            multi_output_guard_results,
                            strict=True,
                        )
                    ):
                        ready = f"rule_ready{ordinal}"
                        ready_values.append(ready)
                        lines.append(
                            f"{indent}  %{ready} = ac.marker.obligation "
                            f"%{output_value} state pending resolver handshake "
                            f"origin {canonical_mlir_string(queue.rule_name + ':return[' + str(ordinal) + ']')} "
                            f'path "true" : !ac.var<{_render_type(payload)}> '
                        )
                        lines.append(
                            f"{indent}  ac.rule.output %{output_value} when "
                            f"%{presence} ordinal {ordinal} : "
                            f"!ac.var<{_render_type(payload)}>, !ac.var<i1>"
                        )
                    lines.append(
                        f"{indent}  ac.rule.return "
                        + ", ".join(f"%{ready}" for ready in ready_values)
                        + " : "
                        + ", ".join(
                            f"!ac.var<{_render_type(payload)}>"
                            for payload in queue.rule_output_payloads
                        )
                    )
                else:
                    lines.append(
                        f"{indent}  %rule_ready = ac.marker.obligation %{result} "
                        f"state pending resolver handshake origin "
                        f'{canonical_mlir_string(queue.rule_name + ":return")} path "true" : '
                        f"!ac.var<{_render_type(queue.payload)}>"
                    )
                if not multi_output_results and (
                    output_guard_result is not None
                    or any(write.guard is not None for write in queue.rule_state_writes)
                ):
                    presence = output_guard_result or condition_result
                    assert presence is not None
                    lines.append(
                        f"{indent}  ac.rule.output %{result} when "
                        f"%{presence} ordinal 0 : "
                        f"!ac.var<{_render_type(queue.payload)}>, !ac.var<i1>"
                    )
                if not multi_output_results:
                    lines.append(
                        f"{indent}  ac.rule.return %rule_ready : "
                        f"!ac.var<{_render_type(queue.payload)}>"
                    )
            else:
                lines.append(f"{indent}  ac.rule.return")
            definition_source = SourceFrame(
                queue.rule_source_path or program.source_path,
                queue.rule_source_line or 1,
                queue.rule_source_column or 1,
                queue.rule_source_line or 1,
                queue.rule_source_column or 1,
            )
            lines.append(
                f"{indent}}} "
                f"{queue_attributes(queue.name, (queue.rate,), queue.rule_output_names, (queue.rule_source_path or program.source_path, queue.rule_source_line or 1, queue.rule_source_column or 1), definition_ndf.get(queue.rule_name, NdfMetadata()))} : "
                f"("
                + ", ".join(
                    f"!ac.queue<{_render_type(payload)}>" for payload in rule_payloads
                )
                + ") -> "
                + (
                    (
                        "("
                        + ", ".join(
                            f"!ac.queue<{_render_type(payload)}>"
                            for payload in queue.rule_output_payloads
                        )
                        + ") "
                        if queue.rule_output_payloads
                        else f"!ac.queue<{_render_type(queue.payload)}> "
                    )
                    if queue.rule_has_output
                    else "() "
                )
                + _render_callsite_location(definition_source, queue.source)
            )
            if output_ssas:
                for name, ssa in zip(
                    queue.rule_output_names or (queue.name,), output_ssas, strict=True
                ):
                    mapping[name] = ssa
            return
        assert output_ssa is not None
        assert queue.input_name is not None
        input_name = effective_input.get((queue.name, 0), queue.input_name)
        input_ssa = mapping[input_name]
        emitter = _ExpressionEmitter(
            payloads,
            queue.argument,
            queue.payload,
            enum_types=enum_types,
            bitfields=bitfields,
            helpers=helpers,
            inline_pure_helpers=module is not None,
        )
        result, result_type = emitter.emit(queue.expression)
        if not _types_compatible(result_type, queue.payload):
            raise QueueFrontendError(
                "ACPY-QUEUE-003: lambda result must preserve Queue payload type"
            )
        lines.append(
            f"{indent}%{output_ssa} = ac.transform %{input_ssa} "
            f"depths [{queue.depth}] latencies [{queue.latency}] {{"
        )
        lines.append(
            f"{indent}^transform(%item: !ac.var<{_render_type(queue.payload)}>):"
        )
        lines.extend(indent + line[2:] for line in emitter.lines)
        lines.append(
            f"{indent}  ac.transform.yield %{result} : "
            f"!ac.var<{_render_type(queue.payload)}>"
        )
        lines.append(
            f"{indent}}} {queue_attributes(queue.name, (queue.rate,))} : "
            "("
            + _render_queue_type(
                by_name[queue.input_name].payload,
                lanes=by_name[queue.input_name].lanes,
                rate=by_name[queue.input_name].rate,
            )
            + ") -> "
            + _render_queue_type(queue.payload, lanes=queue.lanes, rate=queue.rate)
            + _render_source_frame_location(queue.source)
        )
        mapping[queue.name] = output_ssa

    def render_items(
        path: tuple[str, ...], mapping: dict[str, str], indent: str
    ) -> None:
        source_by_order = dict(program.statement_sources)

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
            and queue.name not in module_inputs
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
        )
        events.extend(
            (rule.order, "effect_rule", rule)
            for rule in program.effect_rules
            if rule.scope == path
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
            (candidate.order, "table_match", candidate)
            for candidate in program.candidates
            if candidate.scope == path
        )
        events.extend(
            (selection.order, "table_choose", selection)
            for selection in program.selections
            if selection.scope == path
        )
        events.extend(
            (write.order, "table_write", write)
            for write in program.table_writes
            if write.scope == path
        )
        events.extend(
            (write.order, "masked_table_write", write)
            for write in program.masked_table_writes
            if write.scope == path
        )
        events.extend(
            (slot.order, "slot", slot) for slot in program.slots if slot.scope == path
        )
        events.extend(
            (release.order, "slot_release", release)
            for release in program.slot_releases
            if release.scope == path
        )
        events.extend(
            (
                min(visible_order(consumer) for consumer, _ in group) - 0.4,
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
            if sink_binding.scope == path and sink_binding.queue not in module_outputs
        )
        for event_order, kind, item in sorted(events, key=lambda event: event[0]):
            first_line = len(lines)
            if kind in {"queue", "effect_rule"}:
                queue = item
                assert isinstance(queue, QueueBinding)
                output = (
                    (queue.name if not path else f"{queue.name}_local")
                    if queue.rule_has_output
                    else None
                )
                emit_queue(queue, output, mapping, indent)
            elif kind == "table_match":
                candidate = item
                assert isinstance(candidate, CandidateSetBinding)
                table = next(
                    value for value in program.tables if value.name == candidate.table
                )
                emitter = _ExpressionEmitter(
                    payloads,
                    candidate.argument,
                    table.entry_type,
                    root_name="entry",
                    prefix=f"match_{candidate.order}_",
                    slot_views=slot_views,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    helpers=helpers,
                )
                predicate, predicate_type = emitter.emit(
                    candidate.predicate, BoolType()
                )
                if not _is_bool_like(predicate_type):
                    raise QueueFrontendError(
                        "ACPY-TABLE-006: match predicate must lower to i1"
                    )
                result = f"table_match_{candidate.order}"
                lines.append(
                    f"{indent}%{result} = ac.table.match @{candidate.table} "
                    "predicate {"
                )
                lines.append(
                    f"{indent}^predicate(%entry: "
                    f"!ac.var<{_render_type(table.entry_type)}>):"
                )
                lines.extend(indent + line[2:] for line in emitter.lines)
                lines.append(
                    f"{indent}  ac.table.match.yield %{predicate} : !ac.var<i1>"
                )
                lines.append(
                    f"{indent}}} "
                    + _render_table_domain_attributes(candidate)
                    + f" -> !ac.var<i{candidate.entries}>"
                )
                materialized_candidates[candidate.name] = (
                    result,
                    BitsType(candidate.entries),
                )
            elif kind == "table_choose":
                selection = item
                assert isinstance(selection, SelectionBinding)
                table = next(
                    value for value in program.tables if value.name == selection.table
                )
                mask, mask_type = materialized_candidates[selection.candidates]
                indices = [
                    f"table_choose_{selection.order}_index_{lane}"
                    for lane in range(selection.count)
                ]
                valids = [
                    f"table_choose_{selection.order}_valid_{lane}"
                    for lane in range(selection.count)
                ]
                index_type = BitsType(max(1, (table.entries - 1).bit_length()))
                if selection.policy in {"first", "round_robin"}:
                    key_region = "{}"
                else:
                    assert selection.argument is not None and selection.key is not None
                    emitter = _ExpressionEmitter(
                        payloads,
                        selection.argument,
                        table.entry_type,
                        root_name="entry",
                        prefix=f"choose_{selection.order}_",
                        enum_types=enum_types,
                        bitfields=bitfields,
                        helpers=helpers,
                    )
                    key, key_type = emitter.emit(selection.key)
                    if _integer_width(key_type) is None:
                        raise QueueFrontendError(
                            "ACPY-TABLE-007: choose key must lower to an integer"
                        )
                    key_lines = ["{"]
                    key_lines.append(
                        f"{indent}^key(%entry: "
                        f"!ac.var<{_render_type(table.entry_type)}>):"
                    )
                    key_lines.extend(indent + line[2:] for line in emitter.lines)
                    key_lines.append(
                        f"{indent}  ac.table.choose.yield %{key} : "
                        f"!ac.var<{_render_type(key_type)}>"
                    )
                    key_lines.append(f"{indent}}}")
                    key_region = "\n".join(key_lines)
                lhs = ", ".join(f"%{name}" for name in (*indices, *valids))
                result_types = ", ".join(
                    [f"!ac.var<{_render_type(index_type)}>"] * selection.count
                    + ["!ac.var<i1>"] * selection.count
                )
                key_order = (
                    ""
                    if selection.key_ordering is None
                    else " key_order #ac<table_key_ordering "
                    + selection.key_ordering
                    + ">"
                )
                cursor = (
                    ""
                    if selection.initial_cursor == 0
                    else f" initial_cursor {selection.initial_cursor}"
                )
                lines.append(
                    f"{indent}{lhs} = ac.table.choose @{selection.table} "
                    f"%{mask} : !ac.var<{_render_type(mask_type)}> "
                    f"count {selection.count} policy "
                    f"#ac<table_selection_policy {selection.policy}>{key_order} "
                    f"stable_id {canonical_mlir_string(selection.stable_id)}{cursor} "
                    f"key {key_region} -> {result_types}"
                )
                for alias, index, valid in zip(
                    selection.aliases, indices, valids, strict=True
                ):
                    materialized_selections[alias] = (
                        index,
                        index_type,
                        valid,
                        BoolType(),
                    )
            elif kind == "scope":
                scope = item
                assert isinstance(scope, ScopeBinding)
                render_scope(scope, mapping, indent)
            elif kind == "broadcast":
                source = item
                assert isinstance(source, str)
                _, group = fanouts[source]
                outputs = [f"{source}_fanout_{index}" for index in range(len(group))]
                lhs = ", ".join(f"%{name}" for name in outputs)
                depths = ", ".join("1" for _ in outputs)
                payload = payload_by_queue[source]
                output_types = ", ".join(
                    f"!ac.queue<{_render_type(payload)}>" for _ in outputs
                )
                lines.append(
                    f"{indent}{lhs} = ac.broadcast %{mapping[source]} depths "
                    f"[{depths}] latencies [{depths}] "
                    f"{{ac.output_names = {name_array(outputs)}}} : "
                    f"!ac.queue<{_render_type(payload)}> -> "
                    f"({output_types})"
                )
                for (consumer, input_index), output in zip(group, outputs, strict=True):
                    mapping[effective_input[(consumer.name, input_index)]] = output
            elif kind == "barrier":
                barrier = item
                assert isinstance(barrier, BarrierBinding)
                output_names = [
                    name if not path else f"{name}_local" for name in barrier.outputs
                ]
                lhs = ", ".join(f"%{name}" for name in output_names)
                operands = ", ".join(
                    f"%{mapping[input_name]}" for input_name in barrier.inputs
                )
                depths = ", ".join(str(barrier.depth) for _ in output_names)
                latencies = ", ".join(str(barrier.latency) for _ in output_names)
                input_types = ", ".join(
                    f"!ac.queue<{_render_type(by_name[input_name].payload)}>"
                    for input_name in barrier.inputs
                )
                output_types = ", ".join(
                    f"!ac.queue<{_render_type(by_name[input_name].payload)}>"
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
                emitter = _ExpressionEmitter(
                    payloads,
                    select.argument,
                    control.payload,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    helpers=helpers,
                )
                selector, selector_type = emitter.emit(select.selector)
                if _integer_width(selector_type) is None:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-018: select key must lower to an integer"
                    )
                output = select.output if not path else f"{select.output}_local"
                operands = ", ".join(
                    f"%{mapping[name]}" for name in (select.control, *select.inputs)
                )
                input_types = ", ".join(
                    f"!ac.queue<{_render_type(by_name[name].payload)}>"
                    for name in (select.control, *select.inputs)
                )
                lines.append(
                    f"{indent}%{output} = ac.select {operands} "
                    f"depth {select.depth} latency {select.latency} key {{"
                )
                lines.append(
                    f"{indent}^key(%item: !ac.var<{_render_type(control.payload)}>):"
                )
                lines.extend(indent + line[2:] for line in emitter.lines)
                lines.append(
                    f"{indent}  ac.select.yield %{selector} : "
                    f"!ac.var<{_render_type(selector_type)}>"
                )
                lines.append(
                    f'{indent}}} {{ac.name = "{select.output}"}} : '
                    f"({input_types}) -> "
                    f"!ac.queue<{_render_type(by_name[select.output].payload)}>"
                )
                mapping[select.output] = output
            elif kind == "route":
                route = item
                assert isinstance(route, RouteBinding)
                incoming = by_name[route.input_name]
                emitter = _ExpressionEmitter(
                    payloads,
                    route.argument,
                    incoming.payload,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    helpers=helpers,
                )
                selector, selector_type = emitter.emit(route.selector)
                if route.boolean_selector and not _is_bool_like(
                    selector_type
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-011: runtime if condition must lower to bool"
                    )
                if not route.boolean_selector and not isinstance(
                    selector_type, BitsType
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-006: route key must lower to an integer"
                    )
                output_names = [
                    name if not path else f"{name}_local" for name in route.outputs
                ]
                lhs = ", ".join(f"%{name}" for name in output_names)
                depths = ", ".join(str(route.depth) for _ in output_names)
                latencies = ", ".join(str(route.latency) for _ in output_names)
                output_types = ", ".join(
                    f"!ac.queue<{_render_type(incoming.payload)}>" for _ in output_names
                )
                lines.append(
                    f"{indent}{lhs} = ac.route %{mapping[route.input_name]} "
                    f"depths [{depths}] latencies [{latencies}] {{"
                )
                lines.append(
                    f"{indent}^selector(%item: "
                    f"!ac.var<{_render_type(incoming.payload)}>):"
                )
                lines.extend(indent + line[2:] for line in emitter.lines)
                lines.append(
                    f"{indent}  ac.route.yield %{selector} : "
                    f"!ac.var<{_render_type(selector_type)}>"
                )
                lines.append(
                    f"{indent}}} "
                    f"{{ac.output_names = {name_array(route.outputs)}}} : "
                    f"!ac.queue<{_render_type(incoming.payload)}> -> ({output_types})"
                )
                for name, output in zip(route.outputs, output_names, strict=True):
                    mapping[name] = output
            elif kind == "fork":
                fork = item
                assert isinstance(fork, ForkBinding)
                incoming = by_name[fork.input_name]
                output_names = [
                    name if not path else f"{name}_local" for name in fork.outputs
                ]
                lhs = ", ".join(f"%{name}" for name in output_names)
                depths = ", ".join(str(fork.depth) for _ in output_names)
                latencies = ", ".join(str(fork.latency) for _ in output_names)
                output_types = ", ".join(
                    f"!ac.queue<{_render_type(incoming.payload)}>" for _ in output_names
                )
                lines.append(
                    f"{indent}{lhs} = ac.fork %{mapping[fork.input_name]} "
                    f"depths [{depths}] latencies [{latencies}] "
                    f"{{ac.output_names = {name_array(fork.outputs)}}} : "
                    f"!ac.queue<{_render_type(incoming.payload)}> -> ({output_types})"
                )
                for name, output in zip(fork.outputs, output_names, strict=True):
                    mapping[name] = output
            elif kind == "feedback":
                feedback = item
                assert isinstance(feedback, FeedbackBinding)
                incoming = by_name[feedback.input_name]
                emitter = _ExpressionEmitter(
                    payloads,
                    feedback.argument,
                    incoming.payload,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    helpers=helpers,
                )
                condition, condition_type = emitter.emit(feedback.condition)
                update, update_type = emitter.emit(feedback.update)
                if not _is_bool_like(
                    condition_type
                ) or not _types_compatible(update_type, incoming.payload):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-007: while condition must be bool and update "
                        "must preserve Queue payload"
                    )
                output = (
                    feedback.output_name
                    if not path
                    else f"{feedback.output_name}_local"
                )
                lines.append(
                    f"{indent}%{output} = ac.feedback %{mapping[feedback.input_name]} "
                    f"depth {feedback.depth} latency {feedback.latency} "
                    f"max_iterations {feedback.max_iterations} {{"
                )
                lines.append(
                    f"{indent}^body(%item: !ac.var<{_render_type(incoming.payload)}>):"
                )
                lines.extend(indent + line[2:] for line in emitter.lines)
                lines.append(
                    f"{indent}  ac.feedback.yield %{update} continue %{condition} : "
                    f"!ac.var<{_render_type(incoming.payload)}>, !ac.var<i1>"
                )
                lines.append(
                    f'{indent}}} {{ac.name = "{feedback.output_name}"}} : '
                    f"!ac.queue<{_render_type(incoming.payload)}> -> "
                    f"!ac.queue<{_render_type(incoming.payload)}>"
                )
                mapping[feedback.output_name] = output
            elif kind == "reorder":
                reorder = item
                assert isinstance(reorder, ReorderBinding)
                incoming = by_name[reorder.input_name]
                emitter = _ExpressionEmitter(
                    payloads,
                    reorder.argument,
                    incoming.payload,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    helpers=helpers,
                )
                key, key_type = emitter.emit(reorder.key)
                if _integer_width(key_type) is None:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-013: reorder key must lower to an integer"
                    )
                output = (
                    reorder.output_name if not path else f"{reorder.output_name}_local"
                )
                lines.append(
                    f"{indent}%{output} = ac.reorder "
                    f"%{mapping[reorder.input_name]} capacity {reorder.capacity} "
                    f"start {reorder.start} depth {reorder.depth} "
                    f"latency {reorder.latency} {{"
                )
                lines.append(
                    f"{indent}^key(%item: !ac.var<{_render_type(incoming.payload)}>):"
                )
                lines.extend(indent + line[2:] for line in emitter.lines)
                lines.append(
                    f"{indent}  ac.reorder.yield %{key} : "
                    f"!ac.var<{_render_type(key_type)}>"
                )
                lines.append(
                    f'{indent}}} {{ac.name = "{reorder.output_name}"}} : '
                    f"!ac.queue<{_render_type(incoming.payload)}> -> "
                    f"!ac.queue<{_render_type(incoming.payload)}>"
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
                emitted: list[tuple[str, ValueType, list[str]]] = []
                for _policy_name, expression in policies:
                    emitter = _ExpressionEmitter(
                        payloads,
                        dependency.argument,
                        incoming.payload,
                        enum_types=enum_types,
                        bitfields=bitfields,
                        helpers=helpers,
                    )
                    value, value_type = emitter.emit(expression)
                    if _integer_width(value_type) is None:
                        raise QueueFrontendError(
                            "ACPY-QUEUE-014: dependency policies must lower to integers"
                        )
                    emitted.append((value, value_type, emitter.lines))
                if not _types_compatible(emitted[0][1], emitted[1][1]):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-014: key and waits_for types must match"
                    )
                key_width = _integer_width(emitted[0][1])
                if dependency.provider == "schedule" and (
                    not isinstance(emitted[0][1], BitsType)
                    or not isinstance(emitted[1][1], BitsType)
                    or emitted[0][1] != emitted[1][1]
                    or key_width is None
                    or key_width > 16
                    or dependency.no_dependency != (1 << key_width) - 1
                ):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-014: schedule requires exact matching unsigned "
                        "key/waits_for bits no wider than 16 and an all-ones "
                        "no_dependency sentinel"
                    )
                output = (
                    dependency.output_name
                    if not path
                    else f"{dependency.output_name}_local"
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
                        f"{indent}^{policy_name}(%item: "
                        f"!ac.var<{_render_type(incoming.payload)}>):"
                    )
                    value, value_type, policy_lines = emitted[index]
                    lines.extend(indent + line[2:] for line in policy_lines)
                    lines.append(
                        f"{indent}  ac.dependency.yield %{value} : "
                        f"!ac.var<{_render_type(value_type)}>"
                    )
                provider = (
                    ', ac.schedule_provider = "v2"'
                    if dependency.provider == "schedule"
                    else ""
                )
                lines.append(
                    f'{indent}}} {{ac.name = "{dependency.output_name}"{provider}}} : '
                    f"!ac.queue<{_render_type(incoming.payload)}> -> "
                    f"!ac.queue<{_render_type(incoming.payload)}>"
                )
                mapping[dependency.output_name] = output
            elif kind == "credit":
                credit = item
                assert isinstance(credit, CreditBinding)
                incoming = by_name[credit.input_name]
                emitter = _ExpressionEmitter(
                    payloads,
                    credit.argument,
                    incoming.payload,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    helpers=helpers,
                )
                cost, cost_type = emitter.emit(credit.cost)
                if _integer_width(cost_type) is None:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-016: credit cost must lower to an integer"
                    )
                output = (
                    credit.output_name if not path else f"{credit.output_name}_local"
                )
                lines.append(
                    f"{indent}%{output} = ac.credit "
                    f"%{mapping[credit.input_name]} credits {credit.credits} "
                    f"depth {credit.depth} latency {credit.latency} cost {{"
                )
                lines.append(
                    f"{indent}^cost(%item: !ac.var<{_render_type(incoming.payload)}>):"
                )
                lines.extend(indent + line[2:] for line in emitter.lines)
                lines.append(
                    f"{indent}  ac.credit.yield %{cost} : "
                    f"!ac.var<{_render_type(cost_type)}>"
                )
                lines.append(
                    f'{indent}}} {{ac.name = "{credit.output_name}"}} : '
                    f"!ac.queue<{_render_type(incoming.payload)}> -> "
                    f"!ac.queue<{_render_type(incoming.payload)}>"
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
                emitted: list[tuple[str, ValueType, list[str]]] = []
                for _policy_name, expression in policies:
                    emitter = _ExpressionEmitter(
                        payloads,
                        memory.argument,
                        incoming.payload,
                        enum_types=enum_types,
                        bitfields=bitfields,
                        helpers=helpers,
                    )
                    value, value_type = emitter.emit(expression)
                    emitted.append((value, value_type, emitter.lines))
                if _integer_width(emitted[0][1]) is None:
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: memory address must lower to an integer"
                    )
                if not _is_bool_like(emitted[1][1]):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: memory write must lower to bool"
                    )
                if not _types_compatible(emitted[2][1], instance.data_type):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-015: memory data must match result_field"
                    )
                output = (
                    memory.output_name if not path else f"{memory.output_name}_local"
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
                        f"{indent}^{policy_name}(%item: "
                        f"!ac.var<{_render_type(incoming.payload)}>):"
                    )
                    value, value_type, policy_lines = emitted[index]
                    lines.extend(indent + line[2:] for line in policy_lines)
                    lines.append(
                        f"{indent}  ac.memory.yield %{value} : "
                        f"!ac.var<{_render_type(value_type)}>"
                    )
                lines.append(
                    f'{indent}}} {{ac.endpoint_path = "'
                    f'{"/" + "/".join((*memory.scope, memory.output_name))}", '
                    f'ac.name = "{memory.output_name}"}} : '
                    f"!ac.queue<{_render_type(incoming.payload)}> -> "
                    f"!ac.queue<{_render_type(incoming.payload)}>"
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
                    payloads,
                    argument,
                    input_payload,
                    slot_views=slot_views,
                    candidates=candidate_views,
                    selections=selection_views,
                    candidate_values=materialized_candidates,
                    selection_values=materialized_selections,
                    table_domains=table_domains,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    helpers=helpers,
                )
                address, address_type = address_emitter.emit_table_index(
                    read.table, read.address
                )
                when_emitter = _ExpressionEmitter(
                    payloads,
                    argument,
                    input_payload,
                    table_views=table_views,
                    slot_views=slot_views,
                    candidates=candidate_views,
                    selections=selection_views,
                    candidate_values=materialized_candidates,
                    selection_values=materialized_selections,
                    table_domains=table_domains,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    helpers=helpers,
                )
                condition, condition_type = when_emitter.emit(read.when, BoolType())
                if not isinstance(
                    address_type, BitsType
                ) or not _is_bool_like(condition_type):
                    raise QueueFrontendError(
                        "ACPY-TABLE-003: read address/when type mismatch"
                    )
                output = read.output_name if not path else f"{read.output_name}_local"
                operand = (
                    ""
                    if read.input_name is None
                    else f", %{mapping[read.input_name]} : "
                    f"!ac.queue<{_render_type(input_payload)}> "
                )
                lines.append(
                    f"{indent}%{output} = ac.table.read @{read.table}{operand}"
                    f" depth {read.depth} latency {read.latency} address {{"
                )
                block_argument = (
                    ""
                    if read.input_name is None
                    else f"(%item: !ac.var<{_render_type(input_payload)}>)"
                )
                lines.append(f"{indent}^address{block_argument}:")
                lines.extend(indent + line[2:] for line in address_emitter.lines)
                lines.append(
                    f"{indent}  ac.table.yield %{address} : "
                    f"!ac.var<{_render_type(address_type)}>"
                )
                lines.append(f"{indent}}} when {{")
                lines.append(f"{indent}^when{block_argument}:")
                lines.extend(indent + line[2:] for line in when_emitter.lines)
                lines.append(f"{indent}  ac.table.yield %{condition} : !ac.var<i1>")
                lines.append(
                    f'{indent}}} {{ac.endpoint_path = "'
                    f'{"/" + "/".join((*read.scope, read.output_name))}", '
                    f'ac.name = "{read.output_name}"}} -> '
                    f"!ac.queue<{_render_type(table.entry_type)}>"
                )
                mapping[read.output_name] = output
            elif kind == "table_write":
                write = item
                assert isinstance(write, TableWriteBinding)
                table = next(
                    value for value in program.tables if value.name == write.table
                )
                input_payload = (
                    table.entry_type
                    if write.input_name is None
                    else by_name[write.input_name].payload
                )
                argument = write.argument or ""
                address_emitter = _ExpressionEmitter(
                    payloads,
                    argument,
                    input_payload,
                    slot_views=slot_views,
                    candidates=candidate_views,
                    selections=selection_views,
                    candidate_values=materialized_candidates,
                    selection_values=materialized_selections,
                    table_domains=table_domains,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    helpers=helpers,
                )
                address, address_type = address_emitter.emit_table_index(
                    write.table, write.address
                )
                enable_emitter = _ExpressionEmitter(
                    payloads,
                    argument,
                    input_payload,
                    slot_views=slot_views,
                    candidates=candidate_views,
                    selections=selection_views,
                    candidate_values=materialized_candidates,
                    selection_values=materialized_selections,
                    table_domains=table_domains,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    helpers=helpers,
                )
                enabled, enable_type = enable_emitter.emit(write.enable, BoolType())
                value_emitter = _ExpressionEmitter(
                    payloads,
                    argument,
                    input_payload,
                    table_views={
                        "compiler_old": (write.table, write.address, table.entry_type)
                    },
                    slot_views=slot_views,
                    candidates=candidate_views,
                    selections=selection_views,
                    candidate_values=materialized_candidates,
                    selection_values=materialized_selections,
                    table_domains=table_domains,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    helpers=helpers,
                )
                if write.value is not None:
                    value, value_type = value_emitter.emit(
                        write.value, table.entry_type
                    )
                else:
                    patch_call = ast.Call(
                        func=ast.Attribute(
                            value=ast.Name(id="compiler_old", ctx=ast.Load()),
                            attr="with_fields",
                            ctx=ast.Load(),
                        ),
                        args=[],
                        keywords=[
                            ast.keyword(arg=name, value=expression)
                            for name, expression in write.patch_fields
                        ],
                    )
                    value, value_type = value_emitter.emit(patch_call, table.entry_type)
                if (
                    _integer_width(address_type) is None
                    or not _is_bool_like(enable_type)
                    or not _types_compatible(value_type, table.entry_type)
                ):
                    raise QueueFrontendError(
                        "ACPY-TABLE-004: write address/enable/value type mismatch"
                    )
                lines.append(
                    f"{indent}ac.table.write @{write.table}"
                    + (
                        ""
                        if write.input_name is None
                        else f", %{mapping[write.input_name]} : "
                        f"!ac.queue<{_render_type(input_payload)}>"
                    )
                    + f' mode "{write.write_mode}" write_fields ['
                    + ", ".join(f'"{field}"' for field in write.write_fields)
                    + "] address {"
                )
                block_argument = (
                    ""
                    if write.input_name is None
                    else f"(%item: !ac.var<{_render_type(input_payload)}>)"
                )
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
                        f"!ac.var<{_render_type(policy_type)}>"
                    )
                endpoint_base = (
                    f"{write.table}_allocate"
                    if write.write_mode == "replace"
                    else f"{write.table}_write"
                )
                peer_writes = sum(
                    candidate.table == write.table
                    and candidate.write_mode == write.write_mode
                    for candidate in program.table_writes
                )
                endpoint_id = table_writer_identity(write)
                endpoint_name = endpoint_base + (
                    "" if peer_writes == 1 else f"_{endpoint_id[:24]}"
                )
                arbitration = (
                    ""
                    if write.arbitration_rank is None
                    else f", ac.arbitration = #ac.writer_priority<{write.arbitration_rank}>"
                )
                lines.append(
                    f'{indent}}} {{ac.endpoint_path = "'
                    f'{"/" + "/".join((*write.scope, endpoint_name))}", '
                    f'ac.name = "{endpoint_name}", '
                    f'ac.endpoint_id = "table-writer/{endpoint_id}"{arbitration}}}'
                )
            elif kind == "masked_table_write":
                write = item
                assert isinstance(write, MaskedTableWriteBinding)
                table = next(
                    value for value in program.tables if value.name == write.table
                )
                candidate = candidate_views[write.candidates]
                mask_emitter = _ExpressionEmitter(
                    payloads,
                    "",
                    table.entry_type,
                    prefix=f"mask_{write.order}_",
                    slot_views=slot_views,
                    candidates=candidate_views,
                    candidate_values=materialized_candidates,
                    selection_values=materialized_selections,
                    table_domains=table_domains,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    helpers=helpers,
                )
                mask, mask_type = mask_emitter.emit(
                    ast.Name(id=write.candidates, ctx=ast.Load())
                )
                enable_emitter = _ExpressionEmitter(
                    payloads,
                    "",
                    table.entry_type,
                    prefix="enable_",
                    slot_views=slot_views,
                    candidates=candidate_views,
                    selections=selection_views,
                    candidate_values=materialized_candidates,
                    selection_values=materialized_selections,
                    table_domains=table_domains,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    helpers=helpers,
                )
                enabled, enable_type = enable_emitter.emit(write.enable, BoolType())
                value_emitter = _ExpressionEmitter(
                    payloads,
                    "compiler_old",
                    table.entry_type,
                    root_name="old",
                    prefix="value_",
                    slot_views=slot_views,
                    candidates=candidate_views,
                    selections=selection_views,
                    candidate_values=materialized_candidates,
                    selection_values=materialized_selections,
                    table_domains=table_domains,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    helpers=helpers,
                )
                if write.value is not None:
                    value, value_type = value_emitter.emit(
                        write.value, table.entry_type
                    )
                else:
                    patch_call = ast.Call(
                        func=ast.Attribute(
                            value=ast.Name(id="compiler_old", ctx=ast.Load()),
                            attr="with_fields",
                            ctx=ast.Load(),
                        ),
                        args=[],
                        keywords=[
                            ast.keyword(arg=name, value=expression)
                            for name, expression in write.patch_fields
                        ],
                    )
                    value, value_type = value_emitter.emit(patch_call, table.entry_type)
                if (
                    mask_type != BitsType(candidate.entries)
                    or not _is_bool_like(enable_type)
                    or not _types_compatible(value_type, table.entry_type)
                ):
                    raise QueueFrontendError(
                        "ACPY-TABLE-008: masked write mask/enable/value type mismatch"
                    )
                lines.extend(indent + line[2:] for line in mask_emitter.lines)
                lines.append(
                    f"{indent}ac.table.masked_write @{write.table} %{mask} : "
                    f'!ac.var<{_render_type(mask_type)}> mode "field" write_fields ['
                    + ", ".join(f'"{field}"' for field in write.write_fields)
                    + "] enable {"
                )
                lines.append(f"{indent}^enable:")
                lines.extend(indent + line[2:] for line in enable_emitter.lines)
                lines.append(f"{indent}  ac.table.yield %{enabled} : !ac.var<i1>")
                lines.append(f"{indent}}} value {{")
                lines.append(
                    f"{indent}^value(%old: !ac.var<{_render_type(table.entry_type)}>):"
                )
                lines.extend(indent + line[2:] for line in value_emitter.lines)
                lines.append(
                    f"{indent}  ac.table.yield %{value} : "
                    f"!ac.var<{_render_type(value_type)}>"
                )
                peer_writes = sum(
                    candidate.table == write.table
                    for candidate in program.masked_table_writes
                )
                endpoint_id = table_writer_identity(write)
                endpoint_name = f"{write.table}_masked_write" + (
                    "" if peer_writes == 1 else f"_{endpoint_id[:24]}"
                )
                arbitration = (
                    ""
                    if write.arbitration_rank is None
                    else f", ac.arbitration = #ac.writer_priority<{write.arbitration_rank}>"
                )
                lines.append(
                    f'{indent}}} {{ac.endpoint_path = "'
                    f'{"/" + "/".join((*write.scope, endpoint_name))}", '
                    f'ac.name = "{endpoint_name}", '
                    f'ac.endpoint_id = "table-writer/{endpoint_id}"{arbitration}}}'
                )
            elif kind == "slot":
                slot = item
                assert isinstance(slot, SlotBinding)
                owner = "/" + "/".join(slot.scope) if slot.scope else "/"
                stable_id = "slot/" + (
                    "/".join((*slot.scope, slot.name)) if slot.scope else slot.name
                )
                lines.append(
                    f"{indent}ac.slot @{slot.name}, %{mapping[slot.input_name]} "
                    f'owner "{owner}" stable_id "{stable_id}" : '
                    f"!ac.queue<{_render_type(slot.payload)}>"
                )
            elif kind == "slot_release":
                release = item
                assert isinstance(release, SlotReleaseBinding)
                slot = next(
                    value for value in program.slots if value.name == release.slot
                )
                emitter = _ExpressionEmitter(
                    payloads,
                    "",
                    slot.payload,
                    slot_views=slot_views,
                    candidates=candidate_views,
                    selections=selection_views,
                    candidate_values=materialized_candidates,
                    selection_values=materialized_selections,
                    table_domains=table_domains,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    helpers=helpers,
                )
                condition, condition_type = emitter.emit(release.when, BoolType())
                if not _is_bool_like(condition_type):
                    raise QueueFrontendError(
                        "ACPY-SLOT-002: slot release condition must lower to i1"
                    )
                lines.append(f"{indent}ac.slot.release @{release.slot} when {{")
                lines.append(f"{indent}^when:")
                lines.extend(indent + line[2:] for line in emitter.lines)
                lines.append(f"{indent}  ac.slot.yield %{condition} : !ac.var<i1>")
                endpoint_name = f"{release.slot}_release"
                lines.append(
                    f'{indent}}} {{ac.endpoint_path = "'
                    f'{"/" + "/".join((*release.scope, endpoint_name))}", '
                    f'ac.name = "{endpoint_name}"}}'
                )
            elif kind == "merge":
                merge = item
                assert isinstance(merge, MergeBinding)
                output = merge.output if not path else f"{merge.output}_local"
                operands = ", ".join(f"%{mapping[name]}" for name in merge.inputs)
                input_types = ", ".join(
                    _render_queue_type(
                        by_name[name].payload,
                        lanes=by_name[name].lanes,
                        rate=by_name[name].rate,
                    )
                    for name in merge.inputs
                )
                output_queue = by_name[merge.output]
                lines.append(
                    f'{indent}%{output} = ac.merge {operands} policy "{merge.policy}" '
                    f"depth {merge.depth} latency {merge.latency} "
                    f'{{ac.name = "{merge.output}"}} : '
                    f"({input_types}) -> "
                    + _render_queue_type(
                        output_queue.payload,
                        lanes=output_queue.lanes,
                        rate=output_queue.rate,
                    )
                )
                mapping[merge.output] = output
            elif kind == "expect":
                expectation = item
                assert isinstance(expectation, ExpectBinding)
                queue = by_name[expectation.queue]
                emitter = _ExpressionEmitter(
                    payloads,
                    expectation.argument,
                    queue.payload,
                    enum_types=enum_types,
                    bitfields=bitfields,
                    helpers=helpers,
                )
                condition, condition_type = emitter.emit(expectation.predicate)
                if not _is_bool_like(condition_type):
                    raise QueueFrontendError(
                        "ACPY-QUEUE-021: expect predicate must lower to bool"
                    )
                lines.append(
                    f"{indent}ac.expect %{mapping[expectation.queue]} message "
                    f"{canonical_mlir_string(expectation.message)} {{"
                )
                lines.append(
                    f"{indent}^predicate(%item: "
                    f"!ac.var<{_render_type(queue.payload)}>):"
                )
                lines.extend(indent + line[2:] for line in emitter.lines)
                lines.append(f"{indent}  ac.expect.yield %{condition} : !ac.var<i1>")
                lines.append(
                    f'{indent}}} {{ac.name = "expect_{expectation.order}"}} : '
                    + _render_queue_type(
                        queue.payload, lanes=queue.lanes, rate=queue.rate
                    )
                )
            elif kind == "observe":
                observation = item
                assert isinstance(observation, ObservationBinding)
                queue = by_name[observation.queue]
                lines.append(
                    f"{indent}ac.observe %{mapping[observation.queue]} name "
                    f'"{observation.name}" : '
                    + _render_queue_type(
                        queue.payload, lanes=queue.lanes, rate=queue.rate
                    )
                )
            else:
                sink_binding = item
                assert isinstance(sink_binding, SinkBinding)
                queue = by_name[sink_binding.queue]
                lines.append(
                    f"{indent}ac.sink %{mapping[sink_binding.queue]} "
                    f'{{ac.name = "sink_{sink_binding.order}"}} : '
                    + _render_queue_type(
                        queue.payload, lanes=queue.lanes, rate=queue.rate
                    )
                    + _render_source_frame_location(sink_binding.source)
                )
            if len(lines) > first_line and " loc(" not in lines[-1]:
                if kind == "broadcast":
                    _, group = fanouts[item]
                    source_location = _render_fused_source_locations(
                        (
                            by_name[item].source,
                            *(
                                source_by_order.get(consumer.order)
                                for consumer, _ in group
                            ),
                        )
                    )
                else:
                    source_location = _render_source_frame_location(
                        source_by_order.get(getattr(item, "order", int(event_order)))
                    )
                lines[-1] += source_location

    def render_scope(
        scope: ScopeBinding, parent_mapping: dict[str, str], indent: str
    ) -> None:
        inputs, outputs = scope_io(scope.path)
        result_names = [
            name if len(scope.path) == 1 else f"{name}_inner" for name in outputs
        ]
        lhs = (
            ""
            if not result_names
            else ", ".join(f"%{name}" for name in result_names) + " = "
        )
        operands = ", ".join(f"%{parent_mapping[name]}" for name in inputs)
        input_types = ", ".join(
            f"!ac.queue<{_render_type(payload_by_queue[name])}>" for name in inputs
        )
        output_types = ", ".join(
            f"!ac.queue<{_render_type(payload_by_queue[name])}>" for name in outputs
        )
        lines.append(f"{indent}{lhs}ac.scope @{scope.name}({operands}) {{")
        local_mapping = dict(parent_mapping)
        if inputs:
            args = ", ".join(
                f"%{name}_in: !ac.queue<{_render_type(payload_by_queue[name])}>"
                for name in inputs
            )
            lines.append(f"{indent}^body({args}):")
            for name in inputs:
                local_mapping[name] = f"{name}_in"
        else:
            lines.append(f"{indent}^body:")
        render_items(scope.path, local_mapping, indent + "  ")
        yielded = ", ".join(f"%{local_mapping[name]}" for name in outputs)
        yield_types = ", ".join(
            f"!ac.queue<{_render_type(payload_by_queue[name])}>" for name in outputs
        )
        lines.append(
            f"{indent}  ac.scope.yield"
            + (f" {yielded} : {yield_types}" if outputs else "")
        )
        result_signature = output_types if len(outputs) == 1 else f"({output_types})"
        lines.append(f"{indent}}} : ({input_types}) -> {result_signature}")
        for name, result in zip(outputs, result_names, strict=True):
            parent_mapping[name] = result

    render_items((), initial_mapping, content_indent)
    if module is None:
        lines.append("}")
    else:
        yielded = ", ".join(f"%{initial_mapping[name]}" for name, _ in module.outputs)
        yield_types = ", ".join(
            f"!ac.queue<{_render_type(payload)}>" for _, payload in module.outputs
        )
        lines.append(
            "      ac.scope.yield"
            + (f" {yielded} : {yield_types}" if module.outputs else "")
        )
        input_types = ", ".join(
            f"!ac.queue<{_render_type(payload)}>" for _, payload in module.inputs
        )
        output_types = ", ".join(
            f"!ac.queue<{_render_type(payload)}>" for _, payload in module.outputs
        )
        output_signature = (
            "()"
            if not module.outputs
            else output_types
            if len(module.outputs) == 1
            else f"({output_types})"
        )
        lines.append(f"    }} : ({input_types}) -> {output_signature}")
        returned = ", ".join(
            f"%module_result_{index}" for index in range(len(module.outputs))
        )
        lines.append(
            "    ac.return"
            + (f" {returned} : {output_types}" if module.outputs else "")
        )
        lines.append("    }")
        lines.append("  }")
    return "\n".join(lines) + "\n"
