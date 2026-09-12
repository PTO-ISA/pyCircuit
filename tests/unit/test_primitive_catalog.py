from __future__ import annotations

import copy
import hashlib
import json
import math
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit


def test_implementation_catalog_is_bsd_and_digest_closed() -> None:
    root = Path(__file__).resolve().parents[2]
    catalog_path = root / "library" / "verilog" / "rtl_catalog.json"
    catalog = json.loads(catalog_path.read_text(encoding="utf-8"))

    assert catalog["schema"] == "pyc-rtl-catalog-v1"
    implementations = catalog["implementations"]
    assert {item["semantic_id"] for item in implementations} == {
        "pyc.count_zeros.v1",
        "pyc.popcount.v1",
        "pyc.priority_encode.v1",
    }
    for implementation in implementations:
        assert implementation["effect_class"] == "comb"
        assert implementation["qualification"]["status"] == "validated"
        assert implementation["license_file"] == "licenses/BSD-3-Clause.txt"
        license_path = catalog_path.parent / implementation["license_file"]
        assert license_path.is_file()
        assert implementation["license_sha256"] == (
            "sha256:" + hashlib.sha256(license_path.read_bytes()).hexdigest()
        )
        assert "basejump" not in implementation["implementation_id"].lower()
        for source in implementation["sources"]:
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
    assert {item["semantic_id"] for item in registry["primitives"]} == {
        "pyc.count_zeros.v1",
        "pyc.popcount.v1",
        "pyc.priority_encode.v1",
    }
    assert "implementation_id" not in encoded
    assert all("module" not in item for item in registry["primitives"])
    assert all(
        item["inputs"][0]["constraints"] == ["1 <= N <= 64"]
        for item in registry["primitives"]
    )
    assert "basejump" not in encoded
    leading = next(
        item
        for item in registry["primitives"]
        if item["semantic_id"] == "pyc.count_zeros.v1"
    )
    assert leading["zero_input"] == {"count": "N"}
    assert leading["parameters"]["direction"]["values"] == [
        "leading",
        "trailing",
    ]


def test_semantic_registry_width_formulas_cover_exact_shared_range() -> None:
    import pycircuit
    from _pycircuit_semantics import (
        is_primitive_input_width,
        primitive_count_width,
        primitive_priority_index_width,
    )

    root = Path(__file__).resolve().parents[2]
    registry = json.loads(
        (root / "schemas/primitives/semantic_registry.json").read_text(encoding="utf-8")
    )
    primitives = {item["semantic_id"]: item for item in registry["primitives"]}

    def ceil_log2(value: int) -> int:
        return 0 if value <= 1 else math.ceil(math.log2(value))

    for width in range(1, 65):
        priority = max(1, ceil_log2(width))
        count = max(1, ceil_log2(width + 1))
        assert primitives["pyc.priority_encode.v1"]["outputs"][0]["type"] == (
            "i(max(1,ceil_log2(N)))"
        )
        assert primitives["pyc.popcount.v1"]["outputs"][0]["type"] == (
            "i(max(1,ceil_log2(N+1)))"
        )
        assert primitives["pyc.count_zeros.v1"]["outputs"][0]["type"] == (
            "i(max(1,ceil_log2(N+1)))"
        )
        assert priority == max(1, (width - 1).bit_length())
        assert count == max(1, width.bit_length())
        assert is_primitive_input_width(width)
        assert primitive_priority_index_width(width) == priority
        assert primitive_count_width(width) == count
        circuit = pycircuit.Circuit(f"primitive_width_{width}")
        value = circuit.input("value", width=width)
        assert circuit.priority_encode(value).index.width == priority
        assert circuit.popcount(value).width == count
        assert circuit.count_leading_zeros(value).width == count

    constraint = primitives["pyc.popcount.v1"]["inputs"][0]["constraints"]
    assert constraint == ["1 <= N <= 64"]
    assert all(not (1 <= width <= 64) for width in range(65, 131))
    for width in range(65, 131):
        assert not is_primitive_input_width(width)
        circuit = pycircuit.Circuit(f"rejected_primitive_width_{width}")
        value = circuit.input("value", width=width)
        with pytest.raises(ValueError, match=r"\[1, 64\]"):
            circuit.priority_encode(value)
        with pytest.raises(ValueError, match=r"\[1, 64\]"):
            circuit.popcount(value)
        with pytest.raises(ValueError, match=r"\[1, 64\]"):
            circuit.count_leading_zeros(value)


def test_semantic_registry_generates_the_compiler_selection_table(tmp_path) -> None:
    root = Path(__file__).resolve().parents[2]
    output = tmp_path / "SemanticPrimitiveRegistry.h"
    subprocess.run(
        [
            sys.executable,
            str(root / "tools/pycircuit/generate-semantic-primitive-registry.py"),
            str(root / "schemas/primitives/semantic_registry.json"),
            str(output),
        ],
        check=True,
    )
    generated = output.read_text(encoding="utf-8")
    assert "SemanticPrimitiveRegistry" in generated
    assert '"pyc.priority_encode.v1", "pyc.priority_encode", "comb", 0' in generated
    assert '"order", "low"' in generated
    assert '"direction", "leading"' in generated
    assert '"count", "value"' in generated
    assert '"count", "N"' in generated
    assert "WidthRule::PriorityIndex" in generated
    assert "WidthRule::Count" in generated
    assert "implementation_id" not in generated
    assert "module" not in generated.lower()


@pytest.mark.parametrize("mutation", ["sideways", "dependency", "zero", "formula"])
def test_semantic_registry_generator_rejects_contract_drift(
    tmp_path: Path, mutation: str
) -> None:
    root = Path(__file__).resolve().parents[2]
    registry = json.loads(
        (root / "schemas/primitives/semantic_registry.json").read_text(encoding="utf-8")
    )
    mutated = copy.deepcopy(registry)
    primitives = {item["semantic_id"]: item for item in mutated["primitives"]}
    if mutation == "sideways":
        primitives["pyc.count_zeros.v1"]["parameters"]["direction"]["values"] = [
            "leading",
            "sideways",
        ]
    elif mutation == "dependency":
        primitives["pyc.popcount.v1"]["dependency_matrix"]["count"] = ["other"]
    elif mutation == "zero":
        primitives["pyc.count_zeros.v1"]["zero_input"] = {"other": "N"}
    else:
        primitives["pyc.popcount.v1"]["outputs"][0]["type"] = "i(N)"
    source = tmp_path / f"{mutation}.json"
    source.write_text(json.dumps(mutated), encoding="utf-8")
    completed = subprocess.run(
        [
            sys.executable,
            str(root / "tools/pycircuit/generate-semantic-primitive-registry.py"),
            str(source),
            str(tmp_path / "out.h"),
        ],
        text=True,
        capture_output=True,
        check=False,
    )
    assert completed.returncode != 0
    assert "invalid semantic primitive registry" in completed.stderr


def test_cpp_primitive_width_helpers_match_registry_for_every_width(
    tmp_path: Path,
) -> None:
    compiler = shutil.which("c++")
    if compiler is None:
        pytest.skip("C++ compiler is required for cross-layer width contract test")
    root = Path(__file__).resolve().parents[2]
    generated = tmp_path / "SemanticPrimitiveRegistry.h"
    subprocess.run(
        [
            sys.executable,
            str(root / "tools/pycircuit/generate-semantic-primitive-registry.py"),
            str(root / "schemas/primitives/semantic_registry.json"),
            str(generated),
        ],
        check=True,
    )
    source = tmp_path / "primitive_width_contract.cpp"
    executable = tmp_path / "primitive_width_contract"
    source.write_text(
        r"""#include "SemanticPrimitiveRegistry.h"
#include "acir/Support/PrimitiveWidths.h"
#include "gfsim/primitive_widths.h"
#include <utility>

constexpr unsigned priority(unsigned width) {
  unsigned result = 1;
  for (unsigned extent = 2; extent < width; extent <<= 1) ++result;
  return result;
}
constexpr unsigned count(unsigned width) {
  unsigned result = 1;
  for (unsigned extent = 1; extent < width; extent = (extent << 1) | 1) ++result;
  return result;
}
template <std::size_t... I>
consteval bool gfsimWidths(std::index_sequence<I...>) {
  return ((gfsim::PriorityIndexWidth<I + 1> == priority(I + 1) &&
           gfsim::CountWidth<I + 1> == count(I + 1)) && ...);
}
static_assert(gfsimWidths(std::make_index_sequence<64>{}));
template <std::size_t... I>
consteval bool gfsimRejectsWide(std::index_sequence<I...>) {
  return ((!gfsim::IsPrimitiveInputWidth<I + 65>) && ...);
}
static_assert(gfsimRejectsWide(std::make_index_sequence<66>{}));

int main() {
  const auto *priorityContract =
      pyc::generated::findSemanticPrimitive("pyc.priority_encode.v1");
  const auto *countContract =
      pyc::generated::findSemanticPrimitive("pyc.popcount.v1");
  if (!priorityContract || !countContract) return 3;
  for (unsigned width = 1; width <= 64; ++width) {
    if (!pyc::generated::supportsInputWidth(*priorityContract, width) ||
        !acir::isPrimitiveInputWidth(width) ||
        pyc::generated::outputWidth(*priorityContract, "index", width) != priority(width) ||
        acir::primitivePriorityIndexWidth(width) != priority(width) ||
        pyc::generated::outputWidth(*countContract, "count", width) != count(width) ||
        acir::primitiveCountWidth(width) != count(width))
      return 1;
  }
  for (unsigned width = 65; width <= 130; ++width)
    if (pyc::generated::supportsInputWidth(*priorityContract, width) ||
        acir::isPrimitiveInputWidth(width))
      return 2;
  return 0;
}
""",
        encoding="utf-8",
    )
    subprocess.run(
        [
            compiler,
            "-std=c++20",
            f"-I{tmp_path}",
            f"-I{root / 'compiler/mlir/include'}",
            f"-I{root / 'compiler/acir/include'}",
            f"-I{root / 'simulator/gfsim/include'}",
            str(source),
            "-o",
            str(executable),
        ],
        check=True,
    )
    subprocess.run([str(executable)], check=True)
