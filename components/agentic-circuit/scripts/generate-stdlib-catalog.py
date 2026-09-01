#!/usr/bin/env python3
"""Generate the frozen ac v0.1 component catalog deterministically."""

import argparse
import hashlib
import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "schemas" / "stdlib"

AVAILABLE = {
    "TraceSource": ("workload", "source", "gfsim/trace.h"),
    "Queue": ("transport", "duplex", "gfsim/queue.h"),
    "Scheduler": ("control", "duplex", "gfsim/components.h"),
    "Compute": ("compute", "duplex", "gfsim/components.h"),
    "Link": ("interconnect", "duplex", "gfsim/components.h"),
    "Memory": ("storage", "request_response", "gfsim/components.h"),
    "Sink": ("workload", "sink", "gfsim/components.h"),
    "ready_valid": ("protocol", "duplex", "gfsim/components.h"),
    "request_response": (
        "protocol",
        "request_response",
        "gfsim/components.h",
    ),
}

UNAVAILABLE = {
    "Arbiter": ("control", "duplex"),
    "Dispatcher": ("control", "duplex"),
    "Scoreboard": ("control", "request_response"),
    "DependencyTracker": ("control", "request_response"),
    "Bus": ("interconnect", "duplex"),
    "Crossbar": ("interconnect", "duplex"),
    "Router": ("interconnect", "duplex"),
    "Switch": ("interconnect", "duplex"),
    "Dma": ("transport", "request_response"),
    "Packetizer": ("transport", "duplex"),
    "Reassembler": ("transport", "duplex"),
    "LoadStore": ("transport", "request_response"),
    "RegisterFile": ("storage", "request_response"),
    "Scratchpad": ("storage", "request_response"),
    "Cache": ("storage", "request_response"),
    "Tlb": ("storage", "request_response"),
    "MemoryController": ("storage", "request_response"),
    "Fork": ("synchronization", "fanout"),
    "Join": ("synchronization", "fanin"),
    "Broadcast": ("synchronization", "fanout"),
    "Barrier": ("synchronization", "fanin"),
    "ProtocolAdapter": ("adaptation", "duplex"),
    "WidthAdapter": ("adaptation", "duplex"),
    "TimeDomainBridge": ("adaptation", "duplex"),
    "AddressTranslator": ("adaptation", "request_response"),
    "MemoryManagement": ("adaptation", "request_response"),
    "TrafficSource": ("workload", "source"),
}


def endpoint(name, direction, role, cardinality=1):
    return {
        "name": name,
        "binding_kind": "endpoint",
        "acir_type": (
            "!ac.endpoint<!ac.interface<ac.Stream<"
            "!ac.packet<ac.Transaction>,ac.ready_valid>>," + role + ">"
        ),
        "direction": direction,
        "role": role,
        "cardinality": cardinality,
        "linearity": "linear",
        "ownership": "borrowed",
        "delegation": "forbidden",
        "result_mapping": None,
    }


def bindings_for(shape):
    if shape == "source":
        return [endpoint("output", "out", "producer")]
    if shape == "sink":
        return [endpoint("input", "in", "consumer")]
    if shape == "request_response":
        return [
            endpoint("request", "in", "responder"),
            endpoint("response", "out", "responder"),
        ]
    if shape == "fanout":
        return [
            endpoint("input", "in", "consumer"),
            endpoint("outputs", "out", "producer", "fanout"),
        ]
    if shape == "fanin":
        return [
            endpoint("inputs", "in", "consumer", "fanin"),
            endpoint("output", "out", "producer"),
        ]
    return [
        endpoint("input", "in", "consumer"),
        endpoint("output", "out", "producer"),
    ]


def static_parameters_for(name, shape):
    type_names = {
        "TraceSource": ["Transaction", "Decoder"],
        "Queue": ["T"],
        "Scheduler": ["T"],
        "Compute": ["Input", "Output", "FunctionalPolicy"],
        "Link": ["T"],
        "Memory": ["T"],
        "Sink": ["T"],
        "ready_valid": ["T"],
        "request_response": ["Req", "Resp"],
    }.get(name, ["Transaction"])
    parameters = [
        {
            "name": type_name,
            "acir_type": f"!ac.static_type<ac.{type_name}>",
            "required": True,
            "default": None,
            "constraint": (
                "ac.FunctionalPolicy"
                if type_name == "FunctionalPolicy"
                else (
                    "gfsim::TraceDecoder<Decoder,Transaction>"
                    if name == "TraceSource" and type_name == "Decoder"
                    else "ac.Packet"
                )
            ),
            "cpp_mapping": "template_argument",
        }
        for type_name in type_names
    ]
    if name in {
        "Queue",
        "Scheduler",
        "Memory",
        "request_response",
        "Scoreboard",
        "DependencyTracker",
        "Dma",
        "LoadStore",
        "RegisterFile",
        "Scratchpad",
        "Cache",
        "Tlb",
        "MemoryController",
    }:
        parameters.append(
            {
                "name": "capacity",
                "acir_type": "index",
                "required": True,
                "default": None,
                "constraint": "value >= 1",
                "cpp_mapping": "constructor_constant",
            }
        )
    if name == "Queue":
        parameters.append(
            {
                "name": "byteCapacity",
                "acir_type": "index",
                "required": False,
                "default": "unbounded",
                "constraint": "value >= 1 or value == unbounded",
                "cpp_mapping": "constructor_constant",
            }
        )
    if shape in {"fanout", "fanin"}:
        parameters.append(
            {
                "name": "ports",
                "acir_type": "index",
                "required": True,
                "default": None,
                "constraint": "value >= 1",
                "cpp_mapping": "constructor_constant",
            }
        )
    return parameters


def resources_for(name, family):
    if name not in {
        "Queue",
        "Scheduler",
        "Memory",
        "Scoreboard",
        "DependencyTracker",
        "Dma",
        "LoadStore",
        "RegisterFile",
        "Scratchpad",
        "Cache",
        "Tlb",
        "MemoryController",
    }:
        return []
    resource_class = "queue" if name in {"Queue", "Scheduler"} else family
    return [
        {
            "name": "capacity",
            "class": resource_class,
            "capacity": "capacity",
            "lanes": 1,
            "issue_width": 1,
            "initiation_interval": 1,
            "latency_policy": "ac.FixedLatency<0>",
            "reservation_owner": "self",
            "transaction_classes": ["ac.Transaction"],
            "statistics": ["accepted_transactions", "queue_occupancy"],
        }
    ]


def protocol_contracts_for(bindings):
    contracts = []
    for binding in bindings:
        contracts.append(
            {
                "protocol": "ac.ready_valid",
                "role": binding["role"],
                "ordering": "fifo",
                "delivery": "exactly_once_on_transfer",
                "correlation": (
                    "required" if binding["name"] in {"request", "response"} else "none"
                ),
                "completion": "transfer",
                "failure": "protocol_violation",
            }
        )
    return contracts


def address_behavior(family):
    address_aware = family in {"storage", "adaptation", "transport"}
    return {
        "consumes": ["ac.system"] if address_aware else [],
        "produces": ["ac.system"] if address_aware else [],
        "ranges": [],
        "translation": "static" if address_aware else "none",
        "routing": "address_range" if address_aware else "none",
        "unmapped": "diagnostic" if address_aware else "not_applicable",
    }


def observations_for(name, has_resources):
    available = {
        "TraceSource": (["accepted_transactions"], ["trace_position"]),
        "Queue": (
            ["accepted_transactions", "completed_transactions"],
            ["queue_occupancy", "queue_occupancy_peak"],
        ),
        "Scheduler": (
            ["accepted_transactions", "completed_transactions"],
            ["queue_occupancy", "queue_occupancy_peak"],
        ),
        "Compute": (["completed_transactions"], []),
        "Link": (["completed_transactions"], []),
        "Memory": (["accepted_transactions"], []),
        "Sink": (["accepted_transactions"], []),
        "ready_valid": (["completed_transactions"], []),
        "request_response": (
            ["completed_transactions"],
            ["active_correlations"],
        ),
    }
    if name in available:
        counters, gauges = available[name]
    else:
        counters = ["accepted_transactions", "stalled_cycles"]
        gauges = ["queue_occupancy"] if has_resources else []
    return {
        "probes": [],
        "counters": counters,
        "gauges": gauges,
        "histograms": [],
    }


def component_record(name, family, shape, header):
    bindings = bindings_for(shape)
    resources = resources_for(name, family)
    observation = observations_for(name, bool(resources))
    for resource in resources:
        resource["statistics"] = observation["counters"] + observation["gauges"]
    sources = [f"{binding['name']}.transfer" for binding in bindings]
    inputs = [binding["name"] for binding in bindings if binding["direction"] == "in"]
    symbol_names = {
        "ready_valid": "ReadyValid",
        "request_response": "RequestResponse",
    }
    record = {
        "schema_kind": "agentic-circuit-component",
        "schema_version": "0.1",
        "contract_epoch": "0.4",
        "canonical_name": f"ac.{name}",
        "family": family,
        "provider_namespace": "ac",
        "stability": "provisional",
        "cpp_binding": {
            "header": header,
            "symbol": f"gfsim::{symbol_names.get(name, name)}",
            "language": "c++20",
            "concept": "gfsim::Component",
            "toolchain_target": "ac-gfsim-cxx20-v0.1",
            "functional_policy": "optional" if name == "Compute" else "none",
        },
        "static_parameters": static_parameters_for(name, shape),
        "bindings": bindings,
        "results": [],
        "resources": resources,
        "address_behavior": address_behavior(family),
        "protocol_contracts": protocol_contracts_for(bindings),
        "effect": {
            "kind": "stateful",
            "requirements": ["input offers remain immutable until committed transfer"]
            if inputs
            else ["trace or generated transactions are schema-valid"],
            "guarantees": ["accepted transactions commit exactly once in stable order"],
            "observable_effects": observation["counters"] + observation["gauges"],
            "failure_behavior": "terminate_with_diagnostic",
        },
        "activation": {
            "sources": sources,
            "predicate": "an input is transferable or an output can accept",
            "wakeup_contract": sources,
            "quiescence": (
                "all inputs are closed and all committed internal state is empty"
                if inputs
                else "source is exhausted and no output offer is pending"
            ),
        },
        "observation": observation,
    }
    canonical = json.dumps(
        record, ensure_ascii=False, sort_keys=True, separators=(",", ":")
    ).encode()
    record["schema_fingerprint"] = "sha256:" + hashlib.sha256(canonical).hexdigest()
    return record


def rendered_files():
    records = {}
    catalog_entries = []
    definitions = {
        **AVAILABLE,
        **{
            name: (family, shape, "gfsim/components.h")
            for name, (family, shape) in UNAVAILABLE.items()
        },
    }
    for name in sorted(definitions):
        family, shape, header = definitions[name]
        record = component_record(name, family, shape, header)
        path = OUTPUT / f"{name}.json"
        records[path] = json.dumps(record, indent=2, ensure_ascii=False) + "\n"
        catalog_entries.append(
            {
                "canonical_name": record["canonical_name"],
                "availability": (
                    "available" if name in AVAILABLE else "declared_unavailable"
                ),
                "schema_path": f"schemas/stdlib/{name}.json",
                "schema_fingerprint": record["schema_fingerprint"],
            }
        )
    catalog_entries.sort(key=lambda entry: entry["canonical_name"])
    catalog = {
        "catalog": "ac",
        "version": "0.1",
        "contract_epoch": "0.4",
        "entries": catalog_entries,
    }
    records[OUTPUT / "catalog.json"] = (
        json.dumps(catalog, indent=2, ensure_ascii=False) + "\n"
    )
    return records


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    expected = rendered_files()
    if args.check:
        failures = []
        for path, content in expected.items():
            if not path.is_file() or path.read_text() != content:
                failures.append(path.relative_to(ROOT))
        extras = set(OUTPUT.glob("*.json")) - set(expected)
        failures.extend(sorted(path.relative_to(ROOT) for path in extras))
        if failures:
            for path in failures:
                print(f"error: generated catalog is stale: {path}", file=sys.stderr)
            return 1
        print("standard-library catalog generation: OK (36 component schemas)")
        return 0

    OUTPUT.mkdir(parents=True, exist_ok=True)
    for path, content in expected.items():
        path.write_text(content)
    print("generated 36 standard-library component schemas and catalog")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
