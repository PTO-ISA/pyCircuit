#!/usr/bin/env python3
"""Validate the pyCircuit semantic primitive registry and emit its C++ table."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any

_ROOT_FIELDS = {"schema", "primitives"}
_PRIMITIVE_FIELDS = {
    "semantic_id",
    "operation",
    "effect_class",
    "parameters",
    "inputs",
    "outputs",
    "latency",
    "dependency_matrix",
    "zero_input",
}
_PARAMETER_FIELDS = {"kind", "values"}
_INPUT_FIELDS = {"name", "type", "constraints"}
_OUTPUT_FIELDS = {"name", "type"}
_WIDTH_RULES = {
    "i1": "FixedOne",
    "i(max(1,ceil_log2(N)))": "PriorityIndex",
    "i(max(1,ceil_log2(N+1)))": "Count",
}
_ENUM_DOMAINS = {
    "order": {"high", "low"},
    "direction": {"leading", "trailing"},
}
_SEMANTIC_ID = re.compile(r"^(pyc\.[a-z][a-z0-9_]*)\.v([1-9][0-9]*)$")
_WIDTH_CONSTRAINT = re.compile(r"^([1-9][0-9]*) <= N <= ([1-9][0-9]*)$")


def _fail(message: str) -> None:
    raise ValueError(f"invalid semantic primitive registry: {message}")


def _exact_fields(value: dict[str, Any], expected: set[str], context: str) -> None:
    if set(value) != expected:
        _fail(f"{context} fields must be exactly {sorted(expected)}")


def _reject_implementation_names(value: Any) -> None:
    if isinstance(value, dict):
        if {"implementation_id", "module"} & set(value):
            _fail("implementation names are forbidden")
        for nested in value.values():
            _reject_implementation_names(nested)
    elif isinstance(value, list):
        for nested in value:
            _reject_implementation_names(nested)


def _validate_primitive(primitive: Any) -> dict[str, Any]:
    if not isinstance(primitive, dict):
        _fail("primitive entries must be objects")
    _exact_fields(primitive, _PRIMITIVE_FIELDS, "primitive")
    semantic_id = primitive["semantic_id"]
    operation = primitive["operation"]
    match = (
        _SEMANTIC_ID.fullmatch(semantic_id) if isinstance(semantic_id, str) else None
    )
    if not match or operation != match.group(1):
        _fail("semantic_id must be a versioned form of operation")
    if primitive["effect_class"] != "comb" or primitive["latency"] != 0:
        _fail(f"{semantic_id} must be zero-latency combinational")

    parameters = primitive["parameters"]
    if not isinstance(parameters, dict):
        _fail(f"{semantic_id} parameters must be an object")
    for name, parameter in parameters.items():
        if not isinstance(name, str) or not isinstance(parameter, dict):
            _fail(f"{semantic_id} parameter entries must be named objects")
        _exact_fields(parameter, _PARAMETER_FIELDS, f"{semantic_id}.{name}")
        values = parameter["values"]
        if (
            parameter["kind"] != "enum"
            or not isinstance(values, list)
            or not values
            or not all(isinstance(value, str) and value for value in values)
            or len(values) != len(set(values))
        ):
            _fail(f"{semantic_id}.{name} must be a nonempty unique enum")
        if name not in _ENUM_DOMAINS or set(values) != _ENUM_DOMAINS[name]:
            _fail(f"{semantic_id}.{name} enum values are not canonical")

    inputs = primitive["inputs"]
    if (
        not isinstance(inputs, list)
        or len(inputs) != 1
        or not isinstance(inputs[0], dict)
    ):
        _fail(f"{semantic_id} must have one input")
    input_contract = inputs[0]
    _exact_fields(input_contract, _INPUT_FIELDS, f"{semantic_id} input")
    constraints = input_contract["constraints"]
    width_match = (
        _WIDTH_CONSTRAINT.fullmatch(constraints[0])
        if isinstance(constraints, list)
        and len(constraints) == 1
        and isinstance(constraints[0], str)
        else None
    )
    if (
        not isinstance(input_contract["name"], str)
        or input_contract["type"] != "iN"
        or not width_match
        or int(width_match.group(1)) > int(width_match.group(2))
    ):
        _fail(f"{semantic_id} input contract is invalid")

    outputs = primitive["outputs"]
    if not isinstance(outputs, list) or not outputs:
        _fail(f"{semantic_id} outputs must be a nonempty array")
    output_names: set[str] = set()
    for output in outputs:
        if not isinstance(output, dict):
            _fail(f"{semantic_id} output entries must be objects")
        _exact_fields(output, _OUTPUT_FIELDS, f"{semantic_id} output")
        name, width_rule = output["name"], output["type"]
        if not isinstance(name, str) or not name or name in output_names:
            _fail(f"{semantic_id} output names must be unique strings")
        if width_rule not in _WIDTH_RULES:
            _fail(f"{semantic_id}.{name} has unknown width formula {width_rule!r}")
        output_names.add(name)

    dependencies = primitive["dependency_matrix"]
    input_names = {input_contract["name"]}
    if not isinstance(dependencies, dict) or set(dependencies) != output_names:
        _fail(f"{semantic_id} dependency_matrix must cover every output")
    for output, sources in dependencies.items():
        if (
            not isinstance(sources, list)
            or not sources
            or not all(source in input_names for source in sources)
            or len(sources) != len(set(sources))
        ):
            _fail(f"{semantic_id}.{output} dependencies are invalid")

    zero_input = primitive["zero_input"]
    if not isinstance(zero_input, dict) or set(zero_input) != output_names:
        _fail(f"{semantic_id} zero_input must cover every output")
    for output, value in zero_input.items():
        if (type(value) is int and value >= 0) or value == "N":
            continue
        _fail(f"{semantic_id}.{output} zero_input is invalid")
    return primitive


def _validate(registry: Any) -> list[dict[str, Any]]:
    if not isinstance(registry, dict):
        _fail("root must be an object")
    _exact_fields(registry, _ROOT_FIELDS, "root")
    if registry["schema"] != "pyc-semantic-primitive-registry-v1":
        _fail("schema must be pyc-semantic-primitive-registry-v1")
    primitives = registry["primitives"]
    if not isinstance(primitives, list) or not primitives:
        _fail("primitives must be a nonempty array")
    _reject_implementation_names(registry)
    validated = [_validate_primitive(primitive) for primitive in primitives]
    semantic_ids = [primitive["semantic_id"] for primitive in validated]
    operations = [primitive["operation"] for primitive in validated]
    if len(semantic_ids) != len(set(semantic_ids)):
        _fail("semantic_id values must be unique")
    if len(operations) != len(set(operations)):
        _fail("operation values must be unique")
    return sorted(validated, key=lambda primitive: primitive["semantic_id"])


def _quoted(value: object) -> str:
    text = value if isinstance(value, str) else str(value)
    return json.dumps(text, ensure_ascii=True)


def _padded(items: list[str], size: int, empty: str) -> str:
    return "{{" + ", ".join(items + [empty] * (size - len(items))) + "}}"


def _emit(primitives: list[dict[str, Any]]) -> str:
    max_parameters = max(
        sum(len(p["values"]) for p in item["parameters"].values())
        for item in primitives
    )
    max_outputs = max(len(item["outputs"]) for item in primitives)
    max_dependencies = max(
        sum(len(value) for value in item["dependency_matrix"].values())
        for item in primitives
    )
    entries: list[str] = []
    for item in primitives:
        constraint = _WIDTH_CONSTRAINT.fullmatch(item["inputs"][0]["constraints"][0])
        assert constraint is not None
        parameter_values = [
            f"{{{_quoted(name)}, {_quoted(value)}}}"
            for name, parameter in sorted(item["parameters"].items())
            for value in parameter["values"]
        ]
        outputs = [
            f"{{{_quoted(output['name'])}, "
            f"WidthRule::{_WIDTH_RULES[output['type']]}}}"
            for output in item["outputs"]
        ]
        dependencies = [
            f"{{{_quoted(output)}, {_quoted(source)}}}"
            for output, sources in sorted(item["dependency_matrix"].items())
            for source in sources
        ]
        zero_inputs = [
            f"{{{_quoted(output)}, {_quoted(value)}}}"
            for output, value in sorted(item["zero_input"].items())
        ]
        record_json = json.dumps(
            item, ensure_ascii=True, separators=(",", ":"), sort_keys=True
        )
        entries.append(
            "    {"
            f"{_quoted(item['semantic_id'])}, {_quoted(item['operation'])}, "
            f"{_quoted(item['effect_class'])}, {item['latency']}, "
            f"{_quoted(item['inputs'][0]['name'])}, "
            f"{constraint.group(1)}, {constraint.group(2)}, "
            f"{_padded(parameter_values, max_parameters, '{}')}, "
            f"{len(parameter_values)}, "
            f"{_padded(outputs, max_outputs, '{}')}, {len(outputs)}, "
            f"{_padded(dependencies, max_dependencies, '{}')}, "
            f"{len(dependencies)}, "
            f"{_padded(zero_inputs, max_outputs, '{}')}, {len(zero_inputs)}, "
            f"{_quoted(record_json)}"
            "},"
        )
    entry_text = "\n".join(entries)
    return f"""// Generated from schemas/primitives/semantic_registry.json. Do not edit.
#ifndef PYC_GENERATED_SEMANTICPRIMITIVEREGISTRY_H
#define PYC_GENERATED_SEMANTICPRIMITIVEREGISTRY_H

#include <array>
#include <string_view>

namespace pyc::generated {{

enum class WidthRule {{ FixedOne, PriorityIndex, Count }};
struct EnumValue {{ std::string_view parameter; std::string_view value; }};
struct OutputContract {{ std::string_view name; WidthRule widthRule; }};
struct Dependency {{ std::string_view output; std::string_view input; }};
struct ZeroInput {{ std::string_view output; std::string_view value; }};

struct SemanticPrimitiveContract {{
  std::string_view semanticId;
  std::string_view operation;
  std::string_view effectClass;
  unsigned latency;
  std::string_view inputName;
  unsigned minimumInputWidth;
  unsigned maximumInputWidth;
  std::array<EnumValue, {max_parameters}> enumValues;
  unsigned enumValueCount;
  std::array<OutputContract, {max_outputs}> outputs;
  unsigned outputCount;
  std::array<Dependency, {max_dependencies}> dependencies;
  unsigned dependencyCount;
  std::array<ZeroInput, {max_outputs}> zeroInputs;
  unsigned zeroInputCount;
  std::string_view recordJson;
}};

inline constexpr std::array<SemanticPrimitiveContract, {len(primitives)}>
    SemanticPrimitiveRegistry{{{{
{entry_text}
}}}};

constexpr const SemanticPrimitiveContract *findSemanticPrimitive(
    std::string_view semanticId) {{
  for (const auto &contract : SemanticPrimitiveRegistry)
    if (contract.semanticId == semanticId)
      return &contract;
  return nullptr;
}}

constexpr bool supportsInputWidth(const SemanticPrimitiveContract &contract,
                                  unsigned width) {{
  return width >= contract.minimumInputWidth &&
         width <= contract.maximumInputWidth;
}}

constexpr bool enumAllows(const SemanticPrimitiveContract &contract,
                          std::string_view parameter,
                          std::string_view value) {{
  for (unsigned index = 0; index < contract.enumValueCount; ++index)
    if (contract.enumValues[index].parameter == parameter &&
        contract.enumValues[index].value == value)
      return true;
  return false;
}}

constexpr unsigned outputWidth(const SemanticPrimitiveContract &contract,
                               std::string_view output,
                               unsigned inputWidth) {{
  WidthRule rule = WidthRule::FixedOne;
  bool found = false;
  for (unsigned index = 0; index < contract.outputCount; ++index)
    if (contract.outputs[index].name == output) {{
      rule = contract.outputs[index].widthRule;
      found = true;
      break;
    }}
  if (!found)
    return 0;
  if (rule == WidthRule::FixedOne)
    return 1;
  unsigned width = 1;
  if (rule == WidthRule::PriorityIndex) {{
    for (unsigned extent = 2; extent < inputWidth; extent <<= 1)
      ++width;
    return width;
  }}
  for (unsigned representable = 1; representable < inputWidth;
       representable = (representable << 1) | 1)
    ++width;
  return width;
}}

}} // namespace pyc::generated

#endif // PYC_GENERATED_SEMANTICPRIMITIVEREGISTRY_H
"""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("registry", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    try:
        registry = json.loads(args.registry.read_text(encoding="utf-8"))
        text = _emit(_validate(registry))
    except (OSError, UnicodeError, json.JSONDecodeError, ValueError) as exc:
        parser.error(str(exc))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(text, encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
