from __future__ import annotations

import ast
import copy
import importlib.util
import json
from pathlib import Path

import pytest
from pycircuit import function
from pycircuit.jit import _inline_complexity_cap

LOCK = {
    "release": "0.58.1",
    "encoding_abi": "pto-isa-0.58.1-mode-function-v1",
    "encoding_projection_sha256": "89b872d6eaf0252200bc9349d49b9346e2a69d894cdcc2dcd0fd71911c1e0b8c",
    "content_sha256": "693e8c0734b48598ac35ffe7fe6f2a01037788fba30ebe026895808d23139f2c",
    "source": {
        "repository": "https://github.com/PTO-ISA/pto-spec.git",
        "commit": "c381465b2b8e457e162a4246ee58bb9a2c5b49fd",
        "tree": "463a19db3d6ba70022f18bdbca0d4b2c6ed586e4",
    },
    "catalogs": {
        "command_forms": {
            "sha256": "300a3a57a8728e6c4770da6fff0202b372ec2830edb8dc978dc141d1c26424d0",
            "count": 74,
        },
        "scalar_forms": {
            "sha256": "9f3841d568ffa73fcb43bf4fd365d3c4dba42d27acffa7e273e0f403c0f0c602",
            "count": 474,
        },
        "tile_operations": {
            "sha256": "f163dea8be281fd67173713d373b60f95a9c3c4e558adcdf8034cc213507a1a3",
            "count": 109,
        },
        "extension_encoding_reservations": {
            "sha256": "bdb82b839b98984779d9a1394f6b308f141052ef0b520e5bedb8e87dadd883d4",
            "count": 32,
        },
    },
    "release_manifest": {
        "sha256": "acea87af67301173e6d1c6e04014a8dc6e2f658cd2992ed658ab2589cddc7841"
    },
    "hardware_conformance_profile": {
        "profile_id": "pto-hardware-numeric-0.58.1-ieee-v1",
        "sha256": "170deadacb174c933c287231fb67da1046d7989f84b6852bf353d68a495d1755",
    },
    "numeric_conformance_vectors": {
        "sha256": "59c96cc2f45f8e8f3eebb8230338b21ec3a77a99e8fb5e1c7c7b391819a6aa81"
    },
}

MANIFEST = {
    "version": "0.58.1",
    "cardinality": {
        "command_forms": 74,
        "scalar_forms": 474,
        "tile_operations": 109,
        "extension_encoding_reservations": 32,
    },
}


def _write_authority_fixture(root: Path, *, lock: dict | None = None) -> None:
    isa_root = root / "isa/v0.58"
    isa_root.mkdir(parents=True)
    (isa_root / "pto-spec.lock.json").write_text(
        json.dumps(LOCK if lock is None else lock), encoding="utf-8"
    )
    (isa_root / "release_manifest.json").write_text(
        json.dumps(MANIFEST), encoding="utf-8"
    )
    (isa_root / "linxisa-v0.58.json").write_text(
        json.dumps(
            {
                "version": "0.58.1",
                "instruction_count": 765,
                "instructions": [
                    {
                        "mnemonic": "B.FPATR",
                        "encoding": [
                            {
                                "width_bits": 32,
                                "mask": "0x00007fff",
                                "match": "0x00002023",
                            }
                        ],
                        "pto_source_constraints": [
                            {
                                "field": "PreQuantMode",
                                "operator": "one-of",
                                "values": [
                                    0,
                                    1,
                                    2,
                                    3,
                                    4,
                                    5,
                                    12,
                                    13,
                                    16,
                                    17,
                                    18,
                                    19,
                                    20,
                                    23,
                                    24,
                                    25,
                                    26,
                                    27,
                                    28,
                                    32,
                                    33,
                                    34,
                                    35,
                                    36,
                                    37,
                                    38,
                                    39,
                                ],
                            },
                            {
                                "field": "ReluMode",
                                "operator": "one-of",
                                "values": [0, 1, 2, 3],
                            },
                            {
                                "field": "GroupNCode",
                                "operator": "one-of",
                                "values": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
                            },
                        ],
                    },
                    {
                        "mnemonic": "BSTART.ICALL",
                        "encoding": [
                            {
                                "width_bits": 32,
                                "mask": "0xf83fffff",
                                "match": "0x50166001",
                            }
                        ],
                    },
                    {
                        "mnemonic": "C.BSTART.STD",
                        "encoding": [
                            {"width_bits": 16, "mask": "0xc7ff", "match": "0x0000"}
                        ],
                        "pto_source_constraints": [
                            {
                                "field": "BrType",
                                "operator": "one-of",
                                "values": [1, 5, 7],
                            }
                        ],
                    },
                ],
            }
        ),
        encoding="utf-8",
    )


def _load_checker():
    root = Path(__file__).resolve().parents[2]
    checker = root / "contrib/linx/flows/tools/check_pto_isa_v058_decode.py"
    spec = importlib.util.spec_from_file_location("linx_pto_decode_contract", checker)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _set_path(node: dict, path: str, value: object) -> None:
    parts = path.split(".")
    target = node
    for part in parts[:-1]:
        target = target[part]
    target[parts[-1]] = value


@pytest.mark.unit
def test_linx_pto_decode_contract(tmp_path: Path) -> None:
    _write_authority_fixture(tmp_path)
    assert _load_checker().main(linx_root=tmp_path) == 0


@pytest.mark.unit
@pytest.mark.parametrize(
    "path",
    (
        "release",
        "encoding_abi",
        "encoding_projection_sha256",
        "content_sha256",
        "source.repository",
        "source.commit",
        "source.tree",
        "catalogs.command_forms.sha256",
        "catalogs.command_forms.count",
        "catalogs.scalar_forms.sha256",
        "catalogs.scalar_forms.count",
        "catalogs.tile_operations.sha256",
        "catalogs.tile_operations.count",
        "catalogs.extension_encoding_reservations.sha256",
        "catalogs.extension_encoding_reservations.count",
        "release_manifest.sha256",
        "hardware_conformance_profile.profile_id",
        "hardware_conformance_profile.sha256",
        "numeric_conformance_vectors.sha256",
    ),
)
def test_linx_pto_decode_contract_rejects_authority_mutation(
    tmp_path: Path, path: str
) -> None:
    lock = copy.deepcopy(LOCK)
    _set_path(lock, path, "mutated")
    _write_authority_fixture(tmp_path, lock=lock)
    assert any(
        path in error for error in _load_checker().validate_release_authority(tmp_path)
    )


@pytest.mark.unit
def test_linx_pto_decode_contract_rejects_manifest_or_catalog_cardinality_drift(
    tmp_path: Path,
) -> None:
    _write_authority_fixture(tmp_path)
    module = _load_checker()
    catalog_path = tmp_path / "isa/v0.58/linxisa-v0.58.json"
    catalog = json.loads(catalog_path.read_text())
    catalog["instruction_count"] = 764
    catalog_path.write_text(json.dumps(catalog))
    assert any(
        "instruction_count" in e for e in module.validate_release_authority(tmp_path)
    )
    catalog["instruction_count"] = 765
    catalog_path.write_text(json.dumps(catalog))
    manifest_path = tmp_path / "isa/v0.58/release_manifest.json"
    manifest = json.loads(manifest_path.read_text())
    manifest["cardinality"]["command_forms"] = 73
    manifest_path.write_text(json.dumps(manifest))
    assert any(
        "cardinality.command_forms" in e
        for e in module.validate_release_authority(tmp_path)
    )


@pytest.mark.unit
@pytest.mark.parametrize(
    ("word", "expected"),
    (
        (0x00002023, "B.FPATR"),
        (0x18002023, "OP_INVALID"),
        (0x02002023, "OP_INVALID"),
        (0x00502023, "OP_INVALID"),
        (0x50166001, "BSTART.ICALL"),
        (0x00006001, "OP_INVALID"),
        (0x00003000, "OP_INVALID"),
        (0x00000800, "C.BSTART.STD"),
        (0x00002800, "C.BSTART.STD"),
        (0x00003800, "C.BSTART.STD"),
    ),
)
def test_v0581_control_literal_classification(word: int, expected: str) -> None:
    assert _load_checker().classify_v0581_control_word(word) == expected


def _masked_eq_calls(text: str) -> set[tuple[int, int]]:
    calls: set[tuple[int, int]] = set()
    for node in ast.walk(ast.parse(text)):
        if (
            isinstance(node, ast.Call)
            and isinstance(node.func, ast.Name)
            and node.func.id == "masked_eq"
        ):
            values = {}
            for keyword in node.keywords:
                if keyword.arg not in {"mask", "match"}:
                    continue
                try:
                    values[keyword.arg] = ast.literal_eval(keyword.value)
                except (TypeError, ValueError):
                    pass
            if values.keys() == {"mask", "match"}:
                calls.add((values["mask"], values["match"]))
    return calls


@pytest.mark.unit
def test_decoder_copies_use_identical_v0581_form_ids_and_patterns() -> None:
    root = Path(__file__).resolve().parents[2]
    example_roots = (
        root / "contrib/linx/designs/examples/linx_cpu_pyc",
        root / "contrib/linx/designs/examples/linxcore_inorder",
    )
    isa_texts = [(path / "isa.py").read_text() for path in example_roots]
    decode_texts = [(path / "decode.py").read_text() for path in example_roots]
    for isa_text in isa_texts:
        assert "OP_B_FPATR = 151" in isa_text
        assert "OP_BSTART_STD_ICALL = 152" in isa_text
        assert "PTO_FPATR_PREQUANT_MODES" in isa_text
        assert "PTO_FPATR_RELU_MODES" in isa_text
        assert "PTO_FPATR_GROUP_N_CODES" in isa_text
        assert "PTO_C_BSTART_STD_BRTYPES" in isa_text
    for decode_text in decode_texts:
        assert "fpatr_prequant_valid" in decode_text
        assert "fpatr_relu_valid" in decode_text
        assert "fpatr_group_n_valid" in decode_text
        assert "c_bstart_std_brtype_valid" in decode_text
        assert "decode_icall_contract_window" not in decode_text
        assert "mask=0xF83FFFFF, match=0x50166001" in decode_text
        assert (0x00007FFF, 0x00006001) not in _masked_eq_calls(decode_text)
    for example_root in example_roots:
        pipeline_text = (example_root / "pipeline.py").read_text()
        wb_text = (example_root / "stages/wb_stage.py").read_text()
        assert "icall_tgt: Reg" in pipeline_text
        assert "icall_tgt = state.icall_tgt.out()" in wb_text
        assert "br_target_pc = icall_tgt" in wb_text
        assert "icall_tgt_next = commit_tgt" in wb_text
        assert "ra_write_value = br_base_pc + br_off" in wb_text
    assert isa_texts[0] == isa_texts[1]
    assert decode_texts[0] == decode_texts[1]


@pytest.mark.unit
def test_fused_icall_sequence_snapshots_target_and_keeps_independent_return() -> None:
    root = Path(__file__).resolve().parents[2]
    design = (
        root / "contrib/linx/designs/examples/linx_cpu_pyc/linx_cpu_pyc.py"
    ).read_text()
    testbench = (
        root / "contrib/linx/designs/examples/linx_cpu_pyc/tb_linx_cpu_pyc.cpp"
    ).read_text()
    contract = (
        root / "contrib/linx/designs/examples/linx_cpu_pyc/icall_contract.py"
    ).read_text()
    id_stage = (
        root / "contrib/linx/designs/examples/linx_cpu_pyc/stages/id_stage.py"
    ).read_text()
    assert "from .decode import decode_window" in contract
    assert contract.count("decode_window(m,") == 2
    assert "decode_window(m, window)" in id_stage
    assert "icall_contract = build_icall_contract(" in design
    assert "dut.icall_contract_valid.toBool()" in testbench
    assert "dut.icall_contract_target.value() != 0x8800ull" in testbench
    assert "dut.icall_contract_ra.value() != 0x4002ull" in testbench
    assert "dut.icall_contract_raw_3000_invalid.toBool()" in testbench
    assert not hasattr(_load_checker(), "simulate_fused_icall_sequence")


@pytest.mark.unit
def test_ordinary_function_cannot_override_inline_complexity_cap() -> None:
    @function
    def ordinary_helper(m, value):
        return value

    ordinary_helper.__pycircuit_inline_complexity_cap__ = 100_000
    assert _inline_complexity_cap(ordinary_helper) == 1400


@pytest.mark.unit
def test_production_decoders_do_not_publish_a_complexity_override() -> None:
    root = Path(__file__).resolve().parents[2]
    decoders = (
        root / "contrib/linx/designs/examples/linx_cpu_pyc/decode.py",
        root / "contrib/linx/designs/examples/linxcore_inorder/decode.py",
    )
    for decoder in decoders:
        text = decoder.read_text(encoding="utf-8")
        assert "__pycircuit_inline_complexity_cap__" not in text


@pytest.mark.unit
def test_examples_gate_resolves_decision_rfc_from_pycircuit_root() -> None:
    root = Path(__file__).resolve().parents[2]
    script = (root / "flows/scripts/run_examples.sh").read_text()
    assert 'decision_rfc="${PYC_ROOT_DIR}/docs/rfcs/pyc6-decisions.md"' in script
    assert '--rfc "${decision_rfc}"' in script
