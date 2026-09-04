from __future__ import annotations

import hashlib
import json
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit


def test_priority_implementation_catalog_is_bsd_and_digest_closed() -> None:
    root = Path(__file__).resolve().parents[2]
    catalog_path = root / "library" / "verilog" / "rtl_catalog.json"
    catalog = json.loads(catalog_path.read_text(encoding="utf-8"))

    assert catalog["schema"] == "pyc-rtl-catalog-v1"
    implementations = catalog["implementations"]
    assert len(implementations) == 1
    implementation = implementations[0]
    assert implementation["semantic_id"] == "pyc.priority_encode.v1"
    assert implementation["effect_class"] == "comb"
    assert implementation["qualification"]["status"] == "validated"
    assert implementation["license_file"] == "licenses/BSD-3-Clause.txt"
    license_path = catalog_path.parent / implementation["license_file"]
    assert license_path.is_file()
    assert implementation["license_sha256"] == (
        "sha256:" + hashlib.sha256(license_path.read_bytes()).hexdigest()
    )
    assert "basejump" not in implementation["implementation_id"].lower()

    sources = implementation["sources"]
    assert [source["path"] for source in sources] == ["pyc_priority_encode.v"]
    for source in sources:
        assert source["license"] == "BSD-3-Clause"
        path = catalog_path.parent / source["path"]
        digest = "sha256:" + hashlib.sha256(path.read_bytes()).hexdigest()
        assert source["sha256"] == digest


def test_semantic_registry_contains_no_implementation_names() -> None:
    root = Path(__file__).resolve().parents[2]
    registry = json.loads(
        (root / "schemas" / "primitives" / "semantic_registry.json").read_text(
            encoding="utf-8"
        )
    )
    encoded = json.dumps(registry, sort_keys=True).lower()

    assert registry["schema"] == "pyc-semantic-primitive-registry-v1"
    assert registry["primitives"][0]["semantic_id"] == "pyc.priority_encode.v1"
    assert "implementation_id" not in encoded
    assert "module" not in registry["primitives"][0]
    assert "basejump" not in encoded
