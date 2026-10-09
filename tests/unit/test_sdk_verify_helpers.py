"""Current SDK identity and verifier path helpers."""

from __future__ import annotations

import importlib.util
import json
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]
VERIFIER = ROOT / "packaging/sdk/verify_platform_candidate.py"
CONTRACT_CHECKER = ROOT / "packaging/sdk/check_contract.py"
CAPABILITIES = [
    "pycircuit-pythonic-source",
    "pycircuit-source-units",
    "pycircuit-cpp",
    "pycircuit-verilog",
    "pyc6-runtime-v1",
]


@pytest.fixture(scope="module")
def verifier():
    spec = importlib.util.spec_from_file_location("pycircuit_sdk_verifier", VERIFIER)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load the SDK verifier")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@pytest.fixture(scope="module")
def contract_checker():
    spec = importlib.util.spec_from_file_location(
        "pycircuit_sdk_contract_checker", CONTRACT_CHECKER
    )
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load the SDK contract checker")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_sdk_identity_pins_the_current_distribution_and_contract_versions() -> None:
    version_map = json.loads(
        (ROOT / "packaging/sdk/version-map.json").read_text(encoding="utf-8")
    )
    schema = json.loads(
        (ROOT / "schemas/sdk-manifest.schema.json").read_text(encoding="utf-8")
    )

    assert version_map["schema"] == "pycircuit-sdk-version-map"
    assert version_map["compatibility"] == "exact_identity_tuple"
    assert set(version_map["distributions"]) == {"pycircuit-hisi"}
    assert version_map["contracts"] == {
        "sdk_manifest": "1",
        "generator_abi": "2",
        "runtime_abi": "1",
        "consumer_lock": "1",
        "release_index": "1",
    }
    assert schema["properties"]["capabilities"]["const"] == CAPABILITIES
    assert schema["properties"]["distributions"]["$ref"] == "#/$defs/distributions"
    assert schema["$defs"]["distributions"]["required"] == ["pycircuit-hisi"]


@pytest.mark.parametrize("location", ["schema", "document"])
@pytest.mark.parametrize("token", ["fingerprint", "sha256", "checksum", "digest"])
def test_sdk_contract_rejects_content_identity_tokens(
    contract_checker, location: str, token: str
) -> None:
    schema = {"properties": {"source_revision": {"type": "string"}}}
    document = {"source_revision": "a" * 40}
    target = schema if location == "schema" else document
    target[token] = "not a stable content identity"

    with pytest.raises(ValueError, match=f"forbidden content-identity token: {token}"):
        contract_checker.reject_content_identity(schema, document)


def test_sdk_contract_allows_a_hex_source_revision(
    contract_checker,
) -> None:
    schema = {"properties": {"source_revision": {"type": "string"}}}
    document = {"source_revision": "0123456789abcdef" * 2 + "01234567"}

    contract_checker.reject_content_identity(schema, document)


def test_canonical_path_resolves_an_existing_directory(
    verifier, tmp_path: Path
) -> None:
    target = tmp_path / "nested" / "dir"
    target.mkdir(parents=True)

    resolved = verifier.canonical_path(target)

    assert resolved.is_absolute()
    assert resolved.is_dir()
    assert resolved == target.resolve()


def test_installed_console_script_finds_the_single_py_circuit_driver(
    verifier, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    driver = tmp_path / "pycircuit"
    driver.write_text("", encoding="utf-8")

    monkeypatch.setattr(verifier.sys, "platform", "darwin")
    assert verifier.installed_console_script(tmp_path, "pycircuit") == driver
    assert verifier.installed_console_script(tmp_path, "acc.py") is None

    windows_driver = tmp_path / "pycircuit.exe"
    windows_driver.write_text("", encoding="utf-8")
    monkeypatch.setattr(verifier.sys, "platform", "win32")
    assert verifier.installed_console_script(tmp_path, "pycircuit") == windows_driver
    assert verifier.installed_console_script(tmp_path, "missing") is None


def test_run_names_a_command_that_cannot_start(verifier, tmp_path: Path) -> None:
    with pytest.raises(ValueError, match="command could not start"):
        verifier.run(["definitely-not-a-real-binary-xyz"], cwd=tmp_path)
