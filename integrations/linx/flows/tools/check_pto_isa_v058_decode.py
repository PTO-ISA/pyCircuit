#!/usr/bin/env python3
"""Check Linx pyCircuit example decode tables against PTO ISA 0.58 headers."""

from __future__ import annotations

import ast
import json
import logging
import os
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
DECODE_FILES = (
    ROOT / "integrations/linx/examples/pycircuit/linx_cpu_pyc/decode.py",
    ROOT / "integrations/linx/examples/pycircuit/linxcore_inorder/decode.py",
)
ISA_FILES = (
    ROOT / "integrations/linx/examples/pycircuit/linx_cpu_pyc/isa.py",
    ROOT / "integrations/linx/examples/pycircuit/linxcore_inorder/isa.py",
)

EXPECTED_RELEASE = "0.58.1"
EXPECTED_ENCODING_ABI = "pto-isa-0.58.1-mode-function-v1"
EXPECTED_PROJECTION_SHA256 = (
    "89b872d6eaf0252200bc9349d49b9346e2a69d894cdcc2dcd0fd71911c1e0b8c"
)
EXPECTED_LOCK_FIELDS = {
    "release": EXPECTED_RELEASE,
    "encoding_abi": EXPECTED_ENCODING_ABI,
    "encoding_projection_sha256": EXPECTED_PROJECTION_SHA256,
    "content_sha256": "693e8c0734b48598ac35ffe7fe6f2a01037788fba30ebe026895808d23139f2c",
    "source.repository": "https://github.com/PTO-ISA/pto-spec.git",
    "source.commit": "c381465b2b8e457e162a4246ee58bb9a2c5b49fd",
    "source.tree": "463a19db3d6ba70022f18bdbca0d4b2c6ed586e4",
    "catalogs.command_forms.sha256": "300a3a57a8728e6c4770da6fff0202b372ec2830edb8dc978dc141d1c26424d0",
    "catalogs.command_forms.count": 74,
    "catalogs.scalar_forms.sha256": "9f3841d568ffa73fcb43bf4fd365d3c4dba42d27acffa7e273e0f403c0f0c602",
    "catalogs.scalar_forms.count": 474,
    "catalogs.tile_operations.sha256": "f163dea8be281fd67173713d373b60f95a9c3c4e558adcdf8034cc213507a1a3",
    "catalogs.tile_operations.count": 109,
    "catalogs.extension_encoding_reservations.sha256": "bdb82b839b98984779d9a1394f6b308f141052ef0b520e5bedb8e87dadd883d4",
    "catalogs.extension_encoding_reservations.count": 32,
    "release_manifest.sha256": "acea87af67301173e6d1c6e04014a8dc6e2f658cd2992ed658ab2589cddc7841",
    "hardware_conformance_profile.profile_id": "pto-hardware-numeric-0.58.1-ieee-v1",
    "hardware_conformance_profile.sha256": "170deadacb174c933c287231fb67da1046d7989f84b6852bf353d68a495d1755",
    "numeric_conformance_vectors.sha256": "59c96cc2f45f8e8f3eebb8230338b21ec3a77a99e8fb5e1c7c7b391819a6aa81",
}
EXPECTED_CATALOG_INSTRUCTION_COUNT = 765
EXPECTED_MANIFEST_CARDINALITY = {
    "command_forms": 74,
    "scalar_forms": 474,
    "tile_operations": 109,
    "extension_encoding_reservations": 32,
}
EXPECTED_FORMS = {
    "B.FPATR": (32, 0x00007FFF, 0x00002023),
    "BSTART.ICALL": (32, 0xF83FFFFF, 0x50166001),
    "C.BSTART.STD": (16, 0x0000C7FF, 0x00000000),
}

EXPECTED_FPATR_PREQUANT_MODES = (
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
)
EXPECTED_FPATR_RELU_MODES = (0, 1, 2, 3)
EXPECTED_FPATR_GROUP_N_CODES = (0, 1, 2, 3, 4, 5, 6, 7, 8, 9)
EXPECTED_C_BSTART_STD_BRTYPES = (1, 5, 7)

EXPECTED_TEPL_SELECTORS = (
    0,
    1,
    2,
    3,
    4,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    15,
    16,
    17,
    18,
    19,
    20,
    21,
    22,
    23,
    26,
    27,
    28,
    32,
    33,
    34,
    35,
    36,
    38,
    39,
    40,
    41,
    42,
    43,
    44,
    45,
    58,
    59,
    64,
    65,
    66,
    67,
    68,
    69,
    70,
    71,
    72,
    73,
    74,
    75,
    76,
    77,
    80,
    81,
    82,
    83,
    84,
    85,
    86,
    87,
    88,
    89,
    90,
    91,
    92,
    93,
    96,
    98,
    99,
    100,
    101,
    102,
    103,
    104,
    106,
    107,
    108,
    109,
    110,
    111,
    112,
    113,
    114,
    115,
    116,
)

EXPECTED_TLSU_HEADER_MATCHES = (
    0x00011181,
    0x00111181,
    0x00211181,
    0x00311181,
    0x00411181,
    0x00511181,
    0x00611181,
    0x00711181,
    0x00811181,
    0x00D11181,
)

EXPECTED_CUBE_HEADER_MATCHES = (
    0x00031181,
    0x00131181,
    0x00231181,
    0x00431181,
    0x00531181,
    0x00631181,
    0x01031181,
    0x01131181,
    0x01231181,
    0x01431181,
    0x01531181,
    0x01631181,
)

REQUIRED_PATTERNS = (
    ("BSTART.TEPL", "mask=0x000FFFFF, match=0x00019181"),
    ("B.IOS", "mask=0xF00871FF, match=0x00001013"),
    ("B.IOT two-source", "mask=0x0000707F, match=0x00004013"),
    ("B.IOT one-source", "mask=0xFC00707F, match=0x00005013"),
    ("B.IOT destination-only", "mask=0xFFF0707F, match=0x00006013"),
    ("B.FPATR", "mask=0x00007FFF, match=0x00002023"),
    ("BSTART.ICALL", "mask=0xF83FFFFF, match=0x50166001"),
)

FORBIDDEN_PATTERNS = (
    "mask=0x060FFFFF, match=0x00011181",
    "mask=0x060FFFFF, match=0x00031181",
    "mask=0x0000607F, match=0x00004013",
    "mask=0xC03FFFFF, match=0x00006013",
    "PTO_TEPL_SELECTORS_V0571",
    "match=0x00006001",
    "mask=32767, match=24577",
)

FORBIDDEN_REGEXES = (
    re.compile(r"\bBSTART\.TMA\b"),
    re.compile(r"\bBSTART\.CUBE\b"),
)

RETIRED_TILE_OPS = (
    "TPRELU",
    "TAXPY",
    "TDEINTERLEAVE",
    "TINTERLEAVE",
    "TGATHERB",
    "TRESHAPE",
    "TALLOC",
    "TFREE",
    "TPUSH",
    "TPOP",
    "TPARTARGMAX",
    "TPARTARGMIN",
    "ACCCVT",
)


def _integer(value: object) -> int:
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        return int(value, 0)
    raise TypeError(f"not an integer encoding value: {value!r}")


def _catalog_forms(node: object) -> list[tuple[str, int, int, int]]:
    forms: list[tuple[str, int, int, int]] = []
    if isinstance(node, list):
        for item in node:
            forms.extend(_catalog_forms(item))
        return forms
    if not isinstance(node, dict):
        return forms

    mnemonic = node.get("mnemonic")
    encoding = node.get("encoding")
    parts: object = []
    if isinstance(encoding, list):
        parts = encoding
    elif isinstance(encoding, dict):
        parts = encoding.get("parts", [])
    if isinstance(mnemonic, str) and isinstance(parts, list):
        for part in parts:
            if not isinstance(part, dict):
                continue
            if not {"width_bits", "mask", "match"} <= part.keys():
                continue
            forms.append(
                (
                    mnemonic,
                    _integer(part["width_bits"]),
                    _integer(part["mask"]),
                    _integer(part["match"]),
                )
            )

    for value in node.values():
        forms.extend(_catalog_forms(value))
    return forms


def _catalog_selectors(catalog: object, mnemonic: str) -> dict[str, tuple[int, ...]]:
    if not isinstance(catalog, dict):
        return {}
    instructions = catalog.get("instructions")
    if not isinstance(instructions, list):
        return {}
    for instruction in instructions:
        if not isinstance(instruction, dict) or instruction.get("mnemonic") != mnemonic:
            continue
        constraints = instruction.get("pto_source_constraints")
        if not isinstance(constraints, list):
            return {}
        selectors: dict[str, tuple[int, ...]] = {}
        for constraint in constraints:
            if (
                not isinstance(constraint, dict)
                or constraint.get("operator") != "one-of"
            ):
                continue
            field = constraint.get("field")
            values = constraint.get("values")
            if isinstance(field, str) and isinstance(values, list):
                selectors[field] = tuple(values)
        return selectors
    return {}


def _nested_value(node: object, path: str) -> object:
    value = node
    for part in path.split("."):
        if not isinstance(value, dict) or part not in value:
            return None
        value = value[part]
    return value


def classify_v0581_control_word(word: int) -> str:
    """Classify the promoted control forms, rejecting all reserved literals."""
    if (word & 0xF83FFFFF) == 0x50166001:
        return "BSTART.ICALL"
    if (word & 0x00007FFF) == 0x00002023:
        prequant = (word >> 26) & 0x3F
        relu = (word >> 23) & 0x7
        group_n = (word >> 19) & 0xF
        if (
            prequant in EXPECTED_FPATR_PREQUANT_MODES
            and relu in EXPECTED_FPATR_RELU_MODES
            and group_n in EXPECTED_FPATR_GROUP_N_CODES
        ):
            return "B.FPATR"
    if (word & 0xC7FF) == 0 and ((word >> 11) & 0x7) in EXPECTED_C_BSTART_STD_BRTYPES:
        return "C.BSTART.STD"
    return "OP_INVALID"


def _masked_eq_forms(text: str) -> set[tuple[int, int]]:
    forms: set[tuple[int, int]] = set()
    for node in ast.walk(ast.parse(text)):
        if not (
            isinstance(node, ast.Call)
            and isinstance(node.func, ast.Name)
            and node.func.id == "masked_eq"
        ):
            continue
        values: dict[str, int] = {}
        for keyword in node.keywords:
            if keyword.arg in {"mask", "match"}:
                try:
                    values[keyword.arg] = ast.literal_eval(keyword.value)
                except (TypeError, ValueError):
                    pass
        if values.keys() == {"mask", "match"}:
            forms.add((values["mask"], values["match"]))
    return forms


def validate_release_authority(linx_root: Path) -> list[str]:
    errors: list[str] = []
    lock_path = linx_root / "isa/v0.58/pto-spec.lock.json"
    catalog_path = linx_root / "isa/v0.58/linxisa-v0.58.json"
    manifest_path = linx_root / "isa/v0.58/release_manifest.json"
    try:
        lock = json.loads(lock_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return [f"{lock_path}: cannot read exact root PTO lock: {exc}"]
    try:
        catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return [f"{catalog_path}: cannot read exact root LinxISA catalog: {exc}"]
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return [f"{manifest_path}: cannot read exact root release manifest: {exc}"]

    for path, expected in EXPECTED_LOCK_FIELDS.items():
        actual = _nested_value(lock, path)
        if actual != expected:
            errors.append(f"{lock_path}: {path} expected {expected!r}, got {actual!r}")
    if catalog.get("version") != EXPECTED_RELEASE:
        errors.append(
            f"{catalog_path}: expected catalog version {EXPECTED_RELEASE}, "
            f"got {catalog.get('version')!r}"
        )
    if catalog.get("instruction_count") != EXPECTED_CATALOG_INSTRUCTION_COUNT:
        errors.append(
            f"{catalog_path}: instruction_count expected "
            f"{EXPECTED_CATALOG_INSTRUCTION_COUNT}, got {catalog.get('instruction_count')!r}"
        )
    if manifest.get("version") != EXPECTED_RELEASE:
        errors.append(
            f"{manifest_path}: version expected {EXPECTED_RELEASE!r}, "
            f"got {manifest.get('version')!r}"
        )
    for name, expected in EXPECTED_MANIFEST_CARDINALITY.items():
        path = f"cardinality.{name}"
        actual = _nested_value(manifest, path)
        if actual != expected:
            errors.append(
                f"{manifest_path}: {path} expected {expected!r}, got {actual!r}"
            )

    forms = set(_catalog_forms(catalog))
    for mnemonic, (width, mask, match) in EXPECTED_FORMS.items():
        expected = (mnemonic, width, mask, match)
        if expected not in forms:
            errors.append(
                f"{catalog_path}: missing {mnemonic} exact {width}-bit form "
                f"mask=0x{mask:08x} match=0x{match:08x}"
            )
    if any(
        mnemonic == "BSTART.ICALL" and width == 32 and match == 0x00006001
        for mnemonic, width, _mask, match in forms
    ):
        errors.append(
            f"{catalog_path}: retired BSTART.ICALL 0x00006001 form remains active"
        )
    selectors = _catalog_selectors(catalog, "B.FPATR")
    expected_selectors = {
        "PreQuantMode": EXPECTED_FPATR_PREQUANT_MODES,
        "ReluMode": EXPECTED_FPATR_RELU_MODES,
        "GroupNCode": EXPECTED_FPATR_GROUP_N_CODES,
    }
    for field, expected in expected_selectors.items():
        if selectors.get(field) != expected:
            errors.append(
                f"{catalog_path}: B.FPATR {field} selector set differs from exact 0.58.1 catalog"
            )
    c_bstart_selectors = _catalog_selectors(catalog, "C.BSTART.STD")
    if c_bstart_selectors.get("BrType") != EXPECTED_C_BSTART_STD_BRTYPES:
        errors.append(
            f"{catalog_path}: C.BSTART.STD BrType selector set differs from exact 0.58.1 catalog"
        )
    return errors


def find_linx_root() -> Path | None:
    candidates: list[Path] = []
    if os.environ.get("LINX_ROOT"):
        candidates.append(Path(os.environ["LINX_ROOT"]).resolve())
    for start in (Path.cwd().resolve(), Path(__file__).resolve()):
        candidates.extend((start, *start.parents))
    for candidate in candidates:
        if (candidate / "isa/v0.58/pto-spec.lock.json").is_file() and (
            candidate / "isa/v0.58/linxisa-v0.58.json"
        ).is_file():
            return candidate
    return None


def main(*, linx_root: Path | None = None) -> int:
    errors: list[str] = []

    if linx_root is None:
        linx_root = find_linx_root()
    if linx_root is None:
        errors.append(
            "cannot locate LinxISA superproject root; set LINX_ROOT to the checkout containing isa/v0.58"
        )
    else:
        errors.extend(validate_release_authority(linx_root))

    for decode_file in DECODE_FILES:
        text = decode_file.read_text(encoding="utf-8")
        for label, pattern in REQUIRED_PATTERNS:
            if pattern not in text:
                errors.append(
                    f"{decode_file}: missing {label} exact decode pattern {pattern}"
                )
        for pattern in FORBIDDEN_PATTERNS:
            if pattern in text:
                errors.append(
                    f"{decode_file}: stale 0.56-era generic decode marker remains: {pattern}"
                )
        for regex in FORBIDDEN_REGEXES:
            if regex.search(text):
                errors.append(
                    f"{decode_file}: stale 0.56-era generic decode mnemonic remains: {regex.pattern}"
                )
        if "tepl_selector_valid" not in text or "PTO_TEPL_SELECTORS" not in text:
            errors.append(
                f"{decode_file}: TEPL family decode is not gated by the exact accepted selector set"
            )
        if (
            "PTO_TLSU_HEADER_MATCHES" not in text
            or "PTO_CUBE_HEADER_MATCHES" not in text
        ):
            errors.append(
                f"{decode_file}: TLSU/CUBE header decode is not gated by the exact named-form sets"
            )
        if (0x00007FFF, 0x00006001) in _masked_eq_forms(text):
            errors.append(
                f"{decode_file}: retired numeric BSTART.ICALL 0x00006001 decode remains"
            )
        if not all(
            marker in text
            for marker in (
                "fpatr_prequant_valid",
                "fpatr_relu_valid",
                "fpatr_group_n_valid",
            )
        ):
            errors.append(
                f"{decode_file}: B.FPATR is not gated by all exact selector sets"
            )
        if "c_bstart_std_brtype_valid" not in text:
            errors.append(
                f"{decode_file}: C.BSTART.STD is not gated by the exact BrType selector set"
            )

    expected_constants = {
        "PTO_TEPL_SELECTORS": EXPECTED_TEPL_SELECTORS,
        "PTO_TLSU_HEADER_MATCHES": EXPECTED_TLSU_HEADER_MATCHES,
        "PTO_CUBE_HEADER_MATCHES": EXPECTED_CUBE_HEADER_MATCHES,
        "PTO_FPATR_PREQUANT_MODES": EXPECTED_FPATR_PREQUANT_MODES,
        "PTO_FPATR_RELU_MODES": EXPECTED_FPATR_RELU_MODES,
        "PTO_FPATR_GROUP_N_CODES": EXPECTED_FPATR_GROUP_N_CODES,
        "PTO_C_BSTART_STD_BRTYPES": EXPECTED_C_BSTART_STD_BRTYPES,
    }
    for isa_file in ISA_FILES:
        tree = ast.parse(isa_file.read_text(encoding="utf-8"), filename=str(isa_file))
        actual_constants: dict[str, tuple[int, ...]] = {}
        for node in tree.body:
            if (
                isinstance(node, ast.Assign)
                and len(node.targets) == 1
                and isinstance(node.targets[0], ast.Name)
                and node.targets[0].id in expected_constants
            ):
                actual_constants[node.targets[0].id] = tuple(
                    ast.literal_eval(node.value)
                )
        for name, expected in expected_constants.items():
            if actual_constants.get(name) != expected:
                errors.append(
                    f"{isa_file}: {name} differs from the exact PTO ISA 0.58.1 map"
                )

    search_roots = (
        ROOT / "integrations/linx/examples/pycircuit/linx_cpu_pyc",
        ROOT / "integrations/linx/examples/pycircuit/linxcore_inorder",
    )
    for search_root in search_roots:
        for path in search_root.rglob("*.py"):
            text = path.read_text(encoding="utf-8")
            for opname in RETIRED_TILE_OPS:
                if opname in text:
                    errors.append(
                        f"{path}: retired PTO ISA 0.58 tile op remains: {opname}"
                    )

    if errors:
        for error in errors:
            logging.error(error)
        return 1

    logging.info("pyCircuit PTO ISA 0.58.1 decode guard: ok")
    return 0


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    raise SystemExit(main())
