#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

import tomllib

ROOT = Path(__file__).resolve().parents[2]
SCHEMA_BY_IDENTITY = {
    "pycircuit-sdk-version-map": "sdk-version-map.schema.json",
    "pycircuit-sdk-platform-manifest": "sdk-manifest.schema.json",
    "pycircuit-sdk-release-index": "release-index.schema.json",
    "agentic-circuit-model-plan": "model-plan.schema.json",
    "agentic-circuit-model-manifest": "model-manifest.schema.json",
    "pycircuit-sdk-lock": "consumer-lock.schema.json",
}
EXAMPLE_BY_IDENTITY = {
    "pycircuit-sdk-platform-manifest": "sdk-platform-manifest.example.json",
    "pycircuit-sdk-release-index": "release-index.example.json",
    "agentic-circuit-model-plan": "model-plan.example.json",
    "agentic-circuit-model-manifest": "model-manifest.example.json",
    "pycircuit-sdk-lock": "consumer-lock.example.json",
}


def fail(message: str) -> None:
    raise ValueError(message)


def load_json(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text())
    if not isinstance(value, dict):
        fail(f"{path}: document must be an object")
    return value


def project_version(path: Path) -> str:
    with path.open("rb") as stream:
        return str(tomllib.load(stream)["project"]["version"])


def require_equal(actual: object, expected: object, field: str) -> None:
    if actual != expected:
        fail(f"{field}: expected {expected!r}, observed {actual!r}")


def require_unique_paths(items: list[dict[str, Any]], field: str) -> None:
    paths = [item["path"] for item in items]
    if len(paths) != len(set(paths)):
        fail(f"{field}: logical paths must be unique")


def validate_schema(document: dict[str, Any]) -> None:
    try:
        from jsonschema.validators import Draft202012Validator
    except ImportError as error:
        raise ValueError(
            "jsonschema is required to validate the SDK contract"
        ) from error
    identity = document.get("schema")
    schema_name = SCHEMA_BY_IDENTITY.get(identity)
    if schema_name is None:
        fail(f"unknown SDK contract schema identity: {identity!r}")
    schema = load_json(ROOT / "schemas/agentic-circuit" / schema_name)
    validator = Draft202012Validator(
        schema, format_checker=Draft202012Validator.FORMAT_CHECKER
    )
    errors = sorted(validator.iter_errors(document), key=lambda item: list(item.path))
    if errors:
        location = ".".join(str(part) for part in errors[0].path) or "<root>"
        fail(f"{identity}.{location}: {errors[0].message}")


def validate_exact(document: dict[str, Any], version_map: dict[str, Any]) -> None:
    identity = str(document["schema"])
    versions = version_map["distributions"]
    contracts = version_map["contracts"]
    product = version_map["product_version"]
    candidate_tag = version_map["candidate_tag"]

    if "product_version" in document:
        require_equal(
            document["product_version"], product, f"{identity}.product_version"
        )
    if "candidate_tag" in document:
        require_equal(
            document["candidate_tag"], candidate_tag, f"{identity}.candidate_tag"
        )
    if "release" in document:
        require_equal(document["release"], candidate_tag, f"{identity}.release")
    if "distributions" in document:
        require_equal(document["distributions"], versions, f"{identity}.distributions")
    if "abi" in document:
        require_equal(document["abi"], contracts, f"{identity}.abi")
    if "capabilities" in document and identity != "agentic-circuit-model-plan":
        require_equal(
            document["capabilities"],
            [
                "cycle-aware-signal",
                "pyc-cpp",
                "pyc-verilog",
                "agentic-model-plan",
                "agentic-model-emit-cpp",
                "gfsim-runtime-v1",
            ],
            f"{identity}.capabilities",
        )

    sdk = document.get("sdk")
    if isinstance(sdk, dict):
        require_equal(
            sdk["product_version"], product, f"{identity}.sdk.product_version"
        )
        sdk_contract_fields = {
            "model_plan_abi": "model_plan",
            "generator_abi": "generator_abi",
            "runtime_abi": "runtime_abi",
        }
        for field, contract_field in sdk_contract_fields.items():
            if field in sdk:
                require_equal(
                    sdk[field], contracts[contract_field], f"{identity}.sdk.{field}"
                )

    wheels = document.get("wheels")
    if isinstance(wheels, dict):
        expected_wheels = (
            {
                "agentic-circuit": versions["agentic-circuit"],
                "pycircuit-hisi-linux-x86_64": versions["pycircuit-hisi"],
                "pycircuit-hisi-macos-arm64": versions["pycircuit-hisi"],
                "pycircuit-semantic-core": versions["pycircuit-semantic-core"],
            }
            if identity == "pycircuit-sdk-release-index"
            else versions
        )
        if identity == "pycircuit-sdk-release-index":
            require_equal(list(wheels), list(expected_wheels), f"{identity}.wheels keys")
        else:
            require_equal(set(wheels), set(expected_wheels), f"{identity}.wheels keys")
        for wheel_id, expected_version in expected_wheels.items():
            wheel = wheels[wheel_id]
            if "version" in wheel:
                require_equal(
                    wheel["version"],
                    expected_version,
                    f"{identity}.wheels.{wheel_id}.version",
                )
            distribution = (
                "pycircuit-hisi" if wheel_id.startswith("pycircuit-hisi-") else wheel_id
            )
            normalized = distribution.replace("-", "_")
            if f"{normalized}-{expected_version}" not in wheel["name"]:
                fail(
                    f"{identity}.wheels.{wheel_id}.name: "
                    f"must contain {normalized}-{expected_version}"
                )
        if identity == "pycircuit-sdk-release-index":
            require_equal(
                wheels["agentic-circuit"]["name"].endswith("-py3-none-any.whl"),
                True,
                f"{identity}.wheels.agentic-circuit universal tag",
            )
            require_equal(
                wheels["pycircuit-semantic-core"]["name"].endswith("-py3-none-any.whl"),
                True,
                f"{identity}.wheels.pycircuit-semantic-core universal tag",
            )
            if not wheels["pycircuit-hisi-linux-x86_64"]["name"].endswith(
                "-py3-none-linux_x86_64.whl"
            ):
                fail(f"{identity}: Linux pycircuit-hisi wheel has the wrong platform tag")
            if re.search(
                r"-py3-none-macosx_[0-9]+_[0-9]+_arm64\.whl$",
                wheels["pycircuit-hisi-macos-arm64"]["name"],
            ) is None:
                fail(f"{identity}: macOS pycircuit-hisi wheel has the wrong platform tag")

    if identity == "pycircuit-sdk-platform-manifest":
        require_unique_paths(document["files"], f"{identity}.files")
        if document["self_path"] in {item["path"] for item in document["files"]}:
            fail(f"{identity}.files: embedded manifest cannot hash itself")
    elif identity == "agentic-circuit-model-plan":
        require_unique_paths(document["inputs"], f"{identity}.inputs")
        require_equal(
            document["outputs"],
            [
                "include/generated/model.h",
                "src/generated/model.cpp",
                "src/generated/queuegraph.cpp",
                "share/generated/source-map.json",
            ],
            f"{identity}.outputs",
        )
        require_equal(
            document["cmake_sources"]["path"],
            "model-sources.cmake",
            f"{identity}.cmake_sources.path",
        )
        require_equal(document["depfile_path"], "model.d", f"{identity}.depfile_path")
    elif identity == "agentic-circuit-model-manifest":
        require_unique_paths(document["generated_files"], f"{identity}.generated_files")
        require_equal(
            [item["path"] for item in document["generated_files"]],
            [
                "include/generated/model.h",
                "src/generated/model.cpp",
                "src/generated/queuegraph.cpp",
                "share/generated/source-map.json",
            ],
            f"{identity}.generated_files",
        )
        require_equal(
            document["cmake_sources"]["path"],
            "model-sources.cmake",
            f"{identity}.cmake_sources.path",
        )
        require_equal(document["depfile_path"], "model.d", f"{identity}.depfile_path")

    release_base = (
        f"https://github.com/PTO-ISA/pyCircuit/releases/download/{candidate_tag}/"
    )

    def require_release_url(artifact: dict[str, Any], field: str) -> None:
        require_equal(artifact["url"], release_base + artifact["name"], f"{field}.url")

    if identity == "pycircuit-sdk-release-index":
        for platform_id, platform in document["platforms"].items():
            require_equal(
                platform["archive"]["name"],
                f"pycircuit-sdk-{product}-{platform_id}.tar.gz",
                f"{identity}.{platform_id}.archive.name",
            )
            require_equal(
                platform["manifest"]["name"],
                f"pycircuit-sdk-{product}-{platform_id}.manifest.json",
                f"{identity}.{platform_id}.manifest.name",
            )
            require_release_url(
                platform["archive"], f"{identity}.{platform_id}.archive"
            )
            require_release_url(
                platform["manifest"], f"{identity}.{platform_id}.manifest"
            )
        for distribution, wheel in document["wheels"].items():
            require_release_url(wheel, f"{identity}.wheels.{distribution}")
        require_release_url(document["licenses"], f"{identity}.licenses")
        require_release_url(document["release_notes"], f"{identity}.release_notes")
    elif identity == "pycircuit-sdk-lock":
        require_equal(
            document["release_index"]["name"],
            f"pycircuit-sdk-{product}-release-index.json",
            f"{identity}.release_index.name",
        )
        require_equal(
            document["release_index"]["url"],
            release_base + document["release_index"]["name"],
            f"{identity}.release_index.url",
        )
        platform_id = document["platform"]["id"]
        require_equal(
            document["platform"]["archive"]["name"],
            f"pycircuit-sdk-{product}-{platform_id}.tar.gz",
            f"{identity}.archive.name",
        )
        require_equal(
            document["platform"]["manifest"]["name"],
            f"pycircuit-sdk-{product}-{platform_id}.manifest.json",
            f"{identity}.manifest.name",
        )
        require_release_url(document["platform"]["archive"], f"{identity}.archive")
        require_release_url(document["platform"]["manifest"], f"{identity}.manifest")
        for distribution, wheel in document["wheels"].items():
            require_release_url(wheel, f"{identity}.wheels.{distribution}")


def validate_version_map(version_map: dict[str, Any]) -> None:
    validate_schema(version_map)
    require_equal(version_map["schema"], "pycircuit-sdk-version-map", "schema")
    require_equal(version_map["version"], "1", "version")
    require_equal(version_map["compatibility"], "exact_identity_tuple", "compatibility")
    require_equal(
        version_map["candidate_tag"],
        "v" + version_map["product_version"],
        "candidate_tag",
    )
    expected = {
        "pycircuit-hisi": project_version(ROOT / "pyproject.toml"),
        "pycircuit-semantic-core": project_version(
            ROOT / "python/semantic-core/pyproject.toml"
        ),
        "agentic-circuit": project_version(
            ROOT / "python/agentic-circuit/pyproject.toml"
        ),
    }
    require_equal(version_map["distributions"], expected, "distributions")
    expected_platforms = [
        {
            "id": "linux-x86_64",
            "runner": "ubuntu-24.04",
            "architecture": "x86_64",
            "host_triple": "x86_64-linux-gnu",
            "minimum_os": "Ubuntu 24.04",
            "minimum_libc": "glibc 2.39",
            "python": "3.11",
            "cxx_standard": "20",
            "cxx_abi": "libstdc++ CXX11 ABI",
        },
        {
            "id": "macos-arm64",
            "runner": "macos-15",
            "architecture": "arm64",
            "host_triple": "arm64-apple-darwin",
            "minimum_os": "macOS 15",
            "minimum_libc": None,
            "python": "3.11",
            "cxx_standard": "20",
            "cxx_abi": "Apple libc++",
        },
    ]
    require_equal(version_map["platforms"], expected_platforms, "platforms")


def validate_plan_manifest(plan: dict[str, Any], manifest: dict[str, Any]) -> None:
    require_equal(
        [item["path"] for item in manifest["generated_files"]],
        plan["outputs"],
        "model manifest generated_files versus plan outputs",
    )


def validate_header() -> None:
    path = ROOT / "simulator/gfsim/include/gfsim/model_api.h"
    text = path.read_text()
    required = (
        "#define AGENTIC_MODEL_ABI_V1 1u",
        "typedef struct AgenticModelApiV1",
        "agentic_model_query_v1(void)",
        "sizeof(AgenticModelApiV1) == 80",
        "sizeof(AgenticModelBufferV1) == 16",
        "sizeof(AgenticModelStepResultV1) == 24",
    )
    for marker in required:
        if marker not in text:
            fail(f"model ABI header is missing {marker!r}")
    if re.search(r"\bac_model_v1_(?:create|destroy|reset|step)\b", text):
        fail("model ABI header exposes a direct operation symbol")


def adversarial_checks(version_map: dict[str, Any]) -> None:
    import copy

    for field, value in (
        ("runner", "ubuntu-latest"),
        ("minimum_libc", "glibc 1"),
        ("architecture", "arm64"),
    ):
        candidate = copy.deepcopy(version_map)
        candidate["platforms"][0][field] = value
        try:
            validate_version_map(candidate)
        except ValueError:
            continue
        fail(f"adversarial contract was accepted: version map {field}")

    examples = {
        identity: load_json(ROOT / "packaging/sdk/examples" / name)
        for identity, name in EXAMPLE_BY_IDENTITY.items()
    }

    negatives: list[tuple[str, dict[str, Any]]] = []
    lock = examples["pycircuit-sdk-lock"]
    wrong_wheel = copy.deepcopy(lock)
    wrong_wheel["wheels"]["agentic-circuit"]["version"] = "9.9.9"
    negatives.append(("wrong wheel version", wrong_wheel))
    missing_wheel = copy.deepcopy(lock)
    del missing_wheel["wheels"]["pycircuit-hisi"]
    negatives.append(("missing wheel", missing_wheel))
    empty_caps = copy.deepcopy(lock)
    empty_caps["capabilities"] = []
    negatives.append(("empty capabilities", empty_caps))

    sdk = examples["pycircuit-sdk-platform-manifest"]
    mixed_platform = copy.deepcopy(sdk)
    mixed_platform["platform"]["architecture"] = "arm64"
    negatives.append(("mixed platform", mixed_platform))
    missing_tool = copy.deepcopy(sdk)
    missing_tool["files"] = [
        item for item in missing_tool["files"] if item["path"] != "bin/acir-opt"
    ]
    negatives.append(("missing SDK tool", missing_tool))
    unhashed_bundle = copy.deepcopy(sdk)
    unhashed_bundle["runtime_dependencies"] = [
        {"name": "libexample", "kind": "bundled", "version": "1", "sha256": None}
    ]
    negatives.append(("unhashed bundled dependency", unhashed_bundle))
    self_hash = copy.deepcopy(sdk)
    self_hash["files"].append(
        {
            "path": self_hash["self_path"],
            "kind": "metadata",
            "sha256": "sha256:" + "0" * 64,
            "size": 1,
        }
    )
    negatives.append(("embedded manifest self hash", self_hash))
    duplicate_sdk_path = copy.deepcopy(sdk)
    duplicate_sdk_path["files"].append(copy.deepcopy(duplicate_sdk_path["files"][0]))
    duplicate_sdk_path["files"][-1]["sha256"] = "sha256:" + "1" * 64
    negatives.append(("duplicate SDK path", duplicate_sdk_path))

    plan = examples["agentic-circuit-model-plan"]
    windows_path = copy.deepcopy(plan)
    windows_path["inputs"][0]["path"] = "C:\\producer\\model.py"
    negatives.append(("Windows absolute path", windows_path))
    no_queuegraph = copy.deepcopy(plan)
    del no_queuegraph["queuegraph"]
    negatives.append(("missing QueueGraph", no_queuegraph))
    duplicate_input = copy.deepcopy(plan)
    duplicate_input["inputs"].append(copy.deepcopy(duplicate_input["inputs"][0]))
    duplicate_input["inputs"][-1]["role"] = "import"
    negatives.append(("duplicate input path", duplicate_input))

    manifest = examples["agentic-circuit-model-manifest"]
    direct_symbol = copy.deepcopy(manifest)
    direct_symbol["exports"]["create"] = "ac_model_v1_create"
    negatives.append(("direct ABI symbol", direct_symbol))
    depfile_hash = copy.deepcopy(manifest)
    depfile_hash["depfile"] = {"path": "model.d", "sha256": "sha256:" + "0" * 64}
    negatives.append(("canonical depfile hash", depfile_hash))
    duplicate_generated = copy.deepcopy(manifest)
    duplicate_generated["generated_files"].append(
        copy.deepcopy(duplicate_generated["generated_files"][0])
    )
    negatives.append(("duplicate generated path", duplicate_generated))

    release_index = examples["pycircuit-sdk-release-index"]
    null_url = copy.deepcopy(release_index)
    null_url["platforms"]["linux-x86_64"]["archive"]["url"] = None
    negatives.append(("null final URL", null_url))
    wrong_asset_name = copy.deepcopy(release_index)
    wrong_asset_name["platforms"]["linux-x86_64"]["archive"]["name"] = (
        "pycircuit-sdk-9.9.9-linux-x86_64.tar.gz"
    )
    wrong_asset_name["platforms"]["linux-x86_64"]["archive"]["url"] = (
        "https://github.com/PTO-ISA/pyCircuit/releases/download/v6.0.0/"
        "pycircuit-sdk-9.9.9-linux-x86_64.tar.gz"
    )
    negatives.append(("wrong product version in asset name", wrong_asset_name))
    wrong_url_tag = copy.deepcopy(release_index)
    wrong_url_tag["platforms"]["linux-x86_64"]["archive"]["url"] = wrong_url_tag[
        "platforms"
    ]["linux-x86_64"]["archive"]["url"].replace("/v6.0.0/", "/v9.9.9/")
    negatives.append(("wrong release tag in URL", wrong_url_tag))
    missing_platform_wheel = copy.deepcopy(release_index)
    del missing_platform_wheel["wheels"]["pycircuit-hisi-macos-arm64"]
    negatives.append(("missing platform wheel", missing_platform_wheel))
    universal_with_platform_tag = copy.deepcopy(release_index)
    universal_with_platform_tag["wheels"]["agentic-circuit"]["name"] = (
        "agentic_circuit-0.1.0-py3-none-linux_x86_64.whl"
    )
    universal_with_platform_tag["wheels"]["agentic-circuit"]["url"] = (
        "https://github.com/PTO-ISA/pyCircuit/releases/download/v6.0.0/"
        "agentic_circuit-0.1.0-py3-none-linux_x86_64.whl"
    )
    negatives.append(("non-universal Python wheel", universal_with_platform_tag))

    for name, document in negatives:
        try:
            validate_schema(document)
            validate_exact(document, version_map)
        except ValueError:
            continue
        fail(f"adversarial contract was accepted: {name}")

    mismatched_manifest = copy.deepcopy(manifest)
    mismatched_manifest["generated_files"].pop()
    try:
        validate_plan_manifest(plan, mismatched_manifest)
    except ValueError:
        pass
    else:
        fail("adversarial contract was accepted: plan/manifest output mismatch")


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate pyCircuit SDK v1 contracts")
    parser.add_argument("--document", action="append", default=[])
    args = parser.parse_args()
    try:
        version_map = load_json(ROOT / "packaging/sdk/version-map.json")
        validate_version_map(version_map)
        validate_header()
        for identity, name in EXAMPLE_BY_IDENTITY.items():
            document = load_json(ROOT / "packaging/sdk/examples" / name)
            require_equal(document.get("schema"), identity, f"example {name}")
            validate_schema(document)
            validate_exact(document, version_map)
        plan = load_json(ROOT / "packaging/sdk/examples/model-plan.example.json")
        manifest = load_json(
            ROOT / "packaging/sdk/examples/model-manifest.example.json"
        )
        validate_plan_manifest(plan, manifest)
        adversarial_checks(version_map)
        provided: dict[str, dict[str, Any]] = {}
        for raw in args.document:
            document = load_json(Path(raw))
            validate_schema(document)
            validate_exact(document, version_map)
            provided[str(document["schema"])] = document
        if {
            "agentic-circuit-model-plan",
            "agentic-circuit-model-manifest",
        }.issubset(provided):
            validate_plan_manifest(
                provided["agentic-circuit-model-plan"],
                provided["agentic-circuit-model-manifest"],
            )
    except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    print(
        "SDK contract: OK (6 schemas, 6 documents, exact version/platform/ABI "
        "tuple, path/set closure, adversarial negatives)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
