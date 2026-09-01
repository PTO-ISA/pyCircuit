"""Deterministic ACPy construction and canonical ACIR text emission."""

from __future__ import annotations

import json
import re
from dataclasses import dataclass

from ._acpy import (
    AcpyDocument,
    EntityAllocator,
    Property,
    SchemaRef,
    SourceFile,
)
from ._canonical_json import sha256_bytes, utf16_sort_key
from ._diagnostics import SourceSpan
from ._frontend import CapturedProgram
from ._normalize import NormalizedProgram
from ._process import ProcessProgram
from ._resolve import ValueVersion
from ._static_eval import StaticValue


_SYMBOL = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")


@dataclass(frozen=True, slots=True)
class AcirArtifact:
    text: str
    sha256: str
    source_map: tuple[tuple[str, SourceSpan], ...]


def _properties(**values: StaticValue) -> tuple[Property, ...]:
    return tuple(
        Property(name, value)
        for name, value in sorted(values.items(), key=lambda item: utf16_sort_key(item[0]))
    )


def build_verified_acpy(
    captured: CapturedProgram,
    program: NormalizedProgram,
    processes: tuple[ProcessProgram, ...] = (),
) -> AcpyDocument:
    """Build the closed semantic document in dense semantic order."""

    selected = captured.selected_system
    if selected is None:
        raise ValueError("ACPY-VERIFY-001: a selected system is required")
    sites = {site.qualified_name: site for site in captured.source.definitions}
    system_site = sites[selected.qualified_name]
    module_site = sites.get(program.definition)
    if module_site is None:
        raise ValueError("ACPY-VERIFY-001: root module source is missing")

    allocator = EntityAllocator()
    system = allocator.allocate(
        kind="system",
        scope=selected.qualified_name,
        source=system_site.span,
        properties=_properties(root=program.definition),
    )
    module = allocator.allocate(
        kind="module",
        scope=program.definition,
        source=module_site.span,
        parent=system.id,
        properties=_properties(name=program.definition),
    )
    values: dict[str, str] = {}
    for argument in program.arguments:
        entity = allocator.allocate(
            kind="arg",
            scope=program.definition,
            source=module_site.span,
            parent=module.id,
            type=argument.type_key,
            properties=_properties(name=argument.source_name),
        )
        values[argument.name] = entity.id

    for call in program.calls:
        uses = tuple(values[binding.value.name] for binding in call.inputs)
        call_entity = allocator.allocate(
            kind="call",
            scope=program.definition,
            source=call.source,
            parent=module.id,
            uses=uses,
            schema_ref=SchemaRef(call.schema.identity, call.schema.fingerprint),
            properties=_properties(instance_name=call.instance_name),
        )
        for binding in call.results:
            result = allocator.allocate(
                kind="result",
                scope=program.definition,
                source=call.source,
                parent=call_entity.id,
                type=binding.value.type_key,
                uses=(call_entity.id,),
                properties=_properties(name=binding.value.name),
            )
            values[binding.value.name] = result.id

    process_definitions = {
        definition.qualified_name: definition
        for definition in captured.definitions
        if definition.kind == "process"
    }
    for process in processes:
        definition = next(
            (
                item
                for item in process_definitions.values()
                if item.__name__ == process.name
            ),
            None,
        )
        source = sites[definition.qualified_name].span if definition is not None else None
        allocator.allocate(
            kind="process",
            scope=program.definition,
            source=source,
            parent=module.id,
            properties=_properties(name=process.name),
        )

    allocator.allocate(
        kind="return",
        scope=program.definition,
        source=module_site.span,
        parent=module.id,
        uses=tuple(values[value.name] for value in program.returns),
    )
    document = AcpyDocument(
        entry=system.id,
        sources=(SourceFile(captured.source.path, captured.source.sha256),),
        entities=allocator.freeze(),
    )
    errors = document.verify()
    if errors:
        raise ValueError(
            "ACPY-VERIFY-001: " + "; ".join(error.message for error in errors)
        )
    return document


def _symbol(value: str) -> str:
    candidate = value.rsplit(".", 1)[-1]
    if not _SYMBOL.fullmatch(candidate):
        raise ValueError(f"ACPY-VERIFY-001: invalid ACIR symbol {value!r}")
    return candidate


def _ssa(value: ValueVersion) -> str:
    candidate = f"{value.source_name}_{value.version}"
    return re.sub(r"[^A-Za-z0-9_]", "_", candidate)


def _static_attribute(value: StaticValue) -> str:
    if value is None:
        return "unit"
    if type(value) is bool:
        return "true" if value else "false"
    if type(value) is int:
        return str(value)
    if type(value) is str:
        return json.dumps(value, ensure_ascii=False)
    raise ValueError(
        f"ACPY-VERIFY-001: unsupported ACIR static value {type(value).__name__}"
    )


def _static_arguments(values: tuple[tuple[str, StaticValue], ...]) -> str:
    return ", ".join(
        f"{name} = {_static_attribute(value)}" for name, value in values
    )


def _argument_types(program: NormalizedProgram) -> dict[str, str]:
    inferred: dict[str, str] = {}
    for call in program.calls:
        for port, binding in zip(call.schema.ports, call.inputs, strict=True):
            inferred.setdefault(binding.value.name, port.acir_type)
    for argument in program.arguments:
        inferred.setdefault(
            argument.name,
            "i1" if argument.type_key in {"bool", "RuntimeBool"} else "i32",
        )
    return inferred


def _component_declarations(program: NormalizedProgram) -> list[str]:
    schemas = {call.schema.identity: call.schema for call in program.calls}
    symbols: set[str] = set()
    lines: list[str] = []
    for identity in sorted(schemas, key=utf16_sort_key):
        schema = schemas[identity]
        symbol = _symbol(identity)
        if symbol in symbols:
            raise ValueError("ACPY-CALL-006: component symbol collision")
        symbols.add(symbol)
        arguments = ", ".join(
            f"%{port.name} : {port.acir_type}" for port in schema.ports
        )
        result_types = tuple(result.acir_type for result in schema.results)
        result_signature = (
            ""
            if not result_types
            else " -> " + (
                result_types[0]
                if len(result_types) == 1
                else "(" + ", ".join(result_types) + ")"
            )
        )
        calls = [call for call in program.calls if call.schema.identity == identity]
        schema_parameters = calls[0].static_arguments if calls else ()
        parameter_text = _static_arguments(schema_parameters)
        if schema.external_binding is not None:
            lines.append(
                f"  ac.module.extern @{symbol} : ({', '.join(port.acir_type for port in schema.ports)})"
                f" -> {('()' if not result_types else result_signature.removeprefix(' -> '))} "
                f"parameters {{{parameter_text}}} implementation "
                f"{{registry = \"cpp\", name = {json.dumps(schema.external_binding)}}}"
            )
            continue
        lines.append(
            f"  ac.module @{symbol}({arguments}){result_signature} parameters {{}} graph {{"
        )
        returned: list[str] = []
        for result in schema.results:
            if result.source_binding is None:
                raise ValueError(
                    f"ACPY-VERIFY-001: {identity} result {result.name!r} has no structural source"
                )
            returned.append(f"%{result.source_binding}")
        if returned:
            lines.append(
                "    ac.return "
                + ", ".join(returned)
                + " : "
                + ", ".join(result_types)
            )
        else:
            lines.append("    ac.return")
        lines.append("  }")
    return lines


def _emit_process(process: ProcessProgram, kind: str) -> list[str]:
    if process.captures:
        raise ValueError("ACPY-VERIFY-001: captured process lowering is not closed yet")
    if len(process.blocks) != 1:
        raise ValueError("ACPY-VERIFY-001: multi-block process requires CFG lowering")
    block = process.blocks[0]
    if block.actions or block.edge.kind != "suspend" or block.edge.operation != "yield_sim":
        raise ValueError("ACPY-VERIFY-001: unsupported process operation shape")
    return [
        f"    ac.process @{_symbol(process.name)} kind {json.dumps(kind)} {{",
        "      ac.yield_sim",
        "    }",
    ]


def lower_to_acir(
    program: NormalizedProgram,
    document: AcpyDocument,
    *,
    system_name: str,
    processes: tuple[tuple[ProcessProgram, str], ...] = (),
) -> AcirArtifact:
    errors = document.verify()
    if errors:
        raise ValueError("ACPY-VERIFY-001: lowering requires verified ACPy")
    types = _argument_types(program)
    root = _symbol(program.definition)
    lines = ['module attributes {ac.contract_epoch = "0.4"} {']
    workload = next(
        (process.name for process, kind in processes if kind == "workload"), None
    )
    system = f"  ac.system @{_symbol(system_name)} root @{root} as \"root\" tick 0 \"cycle\""
    if workload is not None:
        system += f" workload @{root}::@{_symbol(workload)}"
    system += (
        " seed {kind = \"fixed\", value = 0 : i64} instrumentation [] "
        "results {id = \"default\", format = \"json\"} selected true"
    )
    lines.append(system)
    declarations = _component_declarations(program)
    if declarations:
        lines.extend(declarations)

    arguments = ", ".join(
        f"%{argument.source_name} : {types[argument.name]}"
        for argument in program.arguments
    )
    return_types = tuple(value.type_key for value in program.returns)
    result_signature = (
        ""
        if not return_types
        else " -> "
        + (return_types[0] if len(return_types) == 1 else "(" + ", ".join(return_types) + ")")
    )
    lines.append(
        f"  ac.module @{root}({arguments}){result_signature} parameters {{}} graph {{"
    )
    source_map: list[tuple[str, SourceSpan]] = []
    names = {argument.name: f"%{argument.source_name}" for argument in program.arguments}
    for call in program.calls:
        operands = ", ".join(names[binding.value.name] for binding in call.inputs)
        operand_types = ", ".join(
            port.acir_type for port in call.schema.ports
        )
        results = tuple(binding.value for binding in call.results)
        if len(results) > 1:
            raise ValueError("ACPY-VERIFY-001: multi-result instance emission is not closed")
        prefix = ""
        if results:
            name = f"%{_ssa(results[0])}"
            names[results[0].name] = name
            prefix = name + " = "
        result_types = ", ".join(result.acir_type for result in call.schema.results)
        arrow = result_types if len(call.schema.results) == 1 else f"({result_types})"
        lines.append(
            f"    {prefix}ac.instance @{_symbol(call.instance_name)} of @{_symbol(call.schema.identity)}({operands}) "
            f"static {{{_static_arguments(call.static_arguments)}}} id {json.dumps(call.instance_name)} "
            f"path {json.dumps(call.instance_name)} : ({operand_types}) -> {arrow}"
        )
        source_map.append((f"@{root}::@{call.instance_name}", call.source))

    for process, kind in processes:
        lines.extend(_emit_process(process, kind))
    if program.returns:
        operands = ", ".join(names[value.name] for value in program.returns)
        lines.append(f"    ac.return {operands} : {', '.join(return_types)}")
    else:
        lines.append("    ac.return")
    lines.extend(("  }", "}"))
    text = "\n".join(lines) + "\n"
    return AcirArtifact(
        text=text,
        sha256=sha256_bytes(text.encode("utf-8")),
        source_map=tuple(source_map),
    )
