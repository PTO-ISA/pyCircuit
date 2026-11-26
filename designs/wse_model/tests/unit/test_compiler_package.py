"""The compiled deployment package and the compile-time concurrency rule."""

from __future__ import annotations

import pytest

from wse_model.compiler import (
    DeploymentPackage,
    build_package,
    check_opcode_domains,
    deployment_artifacts,
)
from wse_model.errors import WseModelError
from wse_model.fixtures import clean_kernel_object, ffn_example
from wse_model.noc.calreg import CalRegImage, CalRegSlot

pytestmark = pytest.mark.unit


def _calreg(*, calendar_version: int = 1, opcodes: tuple[int, ...] = (1,)) -> CalRegImage:
    return CalRegImage(
        slots=tuple(
            CalRegSlot(opcode=opcode, content=bytes([opcode]), arm_lead_cycles=64)
            for opcode in opcodes
        ),
        calendar_version=calendar_version,
    )


def _package(**overrides) -> DeploymentPackage:
    example = ffn_example()
    kwargs = {
        "table": example.table,
        "calreg": _calreg(),
        "kernel_object": clean_kernel_object(),
        "weight_shard_bytes": (2048, 2048),
        "conflict_proofs": {0: "proof-key0", 1: "proof-key1"},
    }
    kwargs.update(overrides)
    return build_package(**kwargs)


# -- artifacts -------------------------------------------------------------


def test_the_five_documented_artifacts() -> None:
    artifacts = deployment_artifacts()
    assert [item["artifact"] for item in artifacts] == [
        "kernel machine code",
        "routeBits static table",
        "CalReg mirror",
        "three version numbers",
        "model weights",
    ]
    assert artifacts[1]["form"] == ".rodata, 64 B aligned"
    assert artifacts[2]["destination"] == "NoC node timeslot registers"


# -- package validity ------------------------------------------------------


def test_a_conforming_ffn_package_validates() -> None:
    package = _package()
    report = package.validate()
    assert report.ok, report.format()
    for check in (
        "per-source-structural-checks",
        "send-receive-conservation",
        "f1-f2-f3-product-checks",
        "artifact-version-agreement",
        "same-opcode-not-concurrent",
        "conflict-proof-present",
        "weight-shard-pages",
        "rodata-alignment",
    ):
        assert check in report.checks_run, check


def test_package_versions_come_from_both_artifacts() -> None:
    package = _package()
    versions = package.versions
    assert versions.topology_version == 1
    assert versions.calendar_version == 1
    assert versions.route_version == 1
    assert package.rodata_bytes == 1280
    assert package.rodata_bytes % 64 == 0


def test_package_description_is_json_serializable() -> None:
    import json

    payload = _package().to_dict()
    assert payload["schema"] == "wse-model/deployment-package/1"
    json.dumps(payload)
    assert payload["call_site_immediates"]
    assert len(payload["artifacts"]) == 5


# -- call-site immediates --------------------------------------------------


def test_call_site_immediates_match_the_published_numbers() -> None:
    immediates = {item.key_id: item for item in _package().call_site_immediates()}
    phase_b = immediates[0]
    assert phase_b.opcode == 1
    assert phase_b.route_version == 1
    assert phase_b.exp_val == 10752
    assert phase_b.capacity == 8 * 1536
    assert phase_b.self_off_stride == 192
    assert phase_b.n_burst == 8

    phase_c = immediates[1]
    assert phase_c.exp_val == 36864
    assert phase_c.capacity == 8 * 6144
    assert phase_c.self_off_stride == 1536


def test_the_key_ref_keeps_key_and_opcode_together() -> None:
    """Invariant O1: the pair is indivisible and the opcode follows the identity."""
    immediate = _package().call_site_immediates()[0]
    ref = immediate.key_ref
    assert ref.key_id == immediate.key_id
    assert ref.opcode_value == immediate.opcode
    assert ref.route_version == immediate.route_version


# -- version agreement -----------------------------------------------------


def test_build_refuses_a_calreg_from_a_different_algorithm_call() -> None:
    example = ffn_example()
    with pytest.raises(WseModelError) as excinfo:
        build_package(table=example.table, calreg=_calreg(calendar_version=2))
    assert "one algorithm call" in str(excinfo.value)


def test_validate_reports_a_calreg_version_mismatch() -> None:
    """Constructed directly to bypass build_package's guard."""
    example = ffn_example()
    package = DeploymentPackage(table=example.table, calreg=_calreg(calendar_version=9))
    report = package.validate()
    assert "V-CALREG-VERSION" in {d.code for d in report.errors}


def test_validate_flags_a_calreg_entry_no_identity_uses() -> None:
    package = _package(calreg=_calreg(opcodes=(1, 2)))
    report = package.validate()
    assert "V-CALREG-ORPHAN" in {d.code for d in report.warnings}


# -- the same-opcode concurrency rule --------------------------------------


def test_ffn_phases_share_opcode_one_and_must_not_be_concurrent() -> None:
    """Calendar §2.3.1: one opcode domain has one time origin and one semaphore."""
    example = ffn_example()
    keys = example.table.ordered_keys()
    assert {definition.opcode for definition in keys} == {1}

    report = example.table.validate()
    assert report.ok  # without a concurrency declaration the table is fine

    concurrent = frozenset({keys[0].key_id, keys[1].key_id})
    checked = check_opcode_domains(example.table, concurrent={concurrent})
    assert "V-OPCODE-CONCURRENT" in {d.code for d in checked.errors}
    assert "redo" in checked.errors[0].message or "redone" in checked.errors[0].message

    package_report = _package().validate(concurrent={concurrent})
    assert "V-OPCODE-CONCURRENT" in {d.code for d in package_report.errors}


def test_seqential_phases_over_one_opcode_are_accepted() -> None:
    """The FFN's two phases are legal because they are separate phases."""
    report = _package().validate(concurrent=())
    assert report.ok, report.format()


def test_a_missing_conflict_proof_is_a_warning_not_a_silent_pass() -> None:
    package = _package(conflict_proofs={})
    report = package.validate()
    codes = {d.code for d in report.warnings}
    assert "V-NO-CONFLICT-PROOF" in codes
    assert report.ok  # a warning, because the algorithm's proof is external


# -- product checks propagate ---------------------------------------------


def test_a_writable_calendar_symbol_fails_the_package() -> None:
    from wse_model.compiler import Section, Symbol, SymbolKind

    broken = clean_kernel_object().with_symbols(
        Symbol("CalendarChannel", SymbolKind.WRITABLE_OBJECT, Section.DATA, size=32)
    )
    report = _package(kernel_object=broken).validate()
    assert "F1" in {d.code for d in report.errors}


def test_unpaged_weight_shards_are_rejected() -> None:
    report = _package(weight_shard_bytes=(2048, 1000)).validate()
    assert "V-WEIGHT-ALIGN" in {d.code for d in report.errors}


def test_a_package_without_a_kernel_object_skips_the_product_checks() -> None:
    package = _package(kernel_object=None)
    report = package.validate()
    assert report.ok
    assert "f1-f2-f3-product-checks" not in report.checks_run
