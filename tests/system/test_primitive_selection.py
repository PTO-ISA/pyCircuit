from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
from pathlib import Path

import pytest

pytestmark = pytest.mark.system


def _root() -> Path:
    return Path(__file__).resolve().parents[2]


def _tool(name: str) -> str:
    configured = os.environ.get(name.upper().replace("-", "_"))
    if configured and Path(configured).is_file():
        return configured
    toolchain = os.environ.get("PYC_TOOLCHAIN_ROOT")
    if toolchain:
        candidate = Path(toolchain) / "bin" / name
        if candidate.is_file():
            return str(candidate)
    candidate = _root() / ".pycircuit_out" / "toolchain" / "build" / "bin" / name
    if candidate.is_file():
        return str(candidate)
    found = shutil.which(name)
    if found:
        return found
    pytest.skip(f"system test requires {name}")


def _environment() -> dict[str, str]:
    environment = os.environ.copy()
    environment["PYC_PRIMITIVES_DIR"] = str(_root() / "library" / "verilog")
    return environment


def test_selector_is_catalog_owned_and_fail_closed(tmp_path: Path) -> None:
    root = _root()
    pyc_opt = _tool("pyc-opt")
    fixture = root / "tests" / "mlir" / "pyc" / "priority-primitive.mlir"
    catalog = root / "library" / "verilog" / "rtl_catalog.json"
    selected = tmp_path / "selected.mlir"
    subprocess.run(
        [
            pyc_opt,
            str(fixture),
            f"--pyc-select-rtl-primitives=catalog={catalog}",
            "-o",
            str(selected),
        ],
        cwd=root,
        check=True,
        env=_environment(),
    )
    text = selected.read_text(encoding="utf-8")
    assert text.count("pyc.rtl.comb") == 2
    assert " = pyc.priority_encode " not in text
    assert 'implementation_id = "pyc.bsd.priority_encode.v1"' in text
    assert "basejump" not in text.lower()

    raw = text.replace("pyc.bsd.priority_encode.v1", "untrusted", 1)
    rejected = subprocess.run(
        [
            pyc_opt,
            f"--pyc-select-rtl-primitives=catalog={catalog}",
            "-o",
            os.devnull,
        ],
        cwd=root,
        input=raw,
        text=True,
        capture_output=True,
        check=False,
        env=_environment(),
    )
    assert rejected.returncode != 0
    assert "backend-owned" in rejected.stderr

    pycc = _tool("pycc")
    for arguments in (
        ["--emit=none"],
        ["--emit=cpp", "-o", str(tmp_path / "forbidden.cpp")],
    ):
        rejected = subprocess.run(
            [pycc, str(selected), *arguments],
            cwd=root,
            text=True,
            capture_output=True,
            check=False,
            env=_environment(),
        )
        assert rejected.returncode != 0
        assert "PYC982" in rejected.stderr

    isolated = tmp_path / "catalog"
    isolated.mkdir()
    shutil.copy2(root / "library" / "verilog" / "pyc_priority_encode.v", isolated)
    (isolated / "licenses").mkdir()
    shutil.copy2(
        root / "library" / "verilog" / "licenses" / "BSD-3-Clause.txt",
        isolated / "licenses" / "BSD-3-Clause.txt",
    )
    tampered_catalog = json.loads(catalog.read_text(encoding="utf-8"))
    tampered_catalog["implementations"][0]["sources"][0]["sha256"] = (
        "sha256:" + "0" * 64
    )
    tampered = isolated / "rtl_catalog.json"
    tampered.write_text(json.dumps(tampered_catalog), encoding="utf-8")
    rejected = subprocess.run(
        [
            pyc_opt,
            str(fixture),
            f"--pyc-select-rtl-primitives=catalog={tampered}",
            "-o",
            os.devnull,
        ],
        cwd=root,
        text=True,
        capture_output=True,
        check=False,
        env=_environment(),
    )
    assert rejected.returncode != 0
    assert "digest mismatch" in rejected.stderr

    ambiguous_catalog = json.loads(catalog.read_text(encoding="utf-8"))
    duplicate = dict(ambiguous_catalog["implementations"][0])
    duplicate["implementation_id"] = "pyc.bsd.priority_encode.alternative"
    ambiguous_catalog["implementations"].append(duplicate)
    ambiguous = isolated / "ambiguous_catalog.json"
    ambiguous.write_text(json.dumps(ambiguous_catalog), encoding="utf-8")
    rejected = subprocess.run(
        [
            pyc_opt,
            str(fixture),
            f"--pyc-select-rtl-primitives=catalog={ambiguous}",
            "-o",
            os.devnull,
        ],
        cwd=root,
        text=True,
        capture_output=True,
        check=False,
        env=_environment(),
    )
    assert rejected.returncode != 0
    assert "selection is ambiguous" in rejected.stderr


@pytest.mark.parametrize(
    ("body", "message"),
    [
        (
            '%i, %v = pyc.priority_encode %mask {order = "low"} : i13 -> i3, i1',
            "index result width",
        ),
        (
            '%i, %v = pyc.priority_encode %mask {order = "middle"} : i13 -> i4, i1',
            "order must be",
        ),
    ],
)
def test_priority_semantic_verifier_rejects_malformed_contract(
    body: str, message: str
) -> None:
    pyc_opt = _tool("pyc-opt")
    result_type = "i3" if "-> i3" in body else "i4"
    source = f"""module {{
  func.func @bad(%mask: i13) -> {result_type} {{
    {body}
    return %i : {result_type}
  }}
}}
"""
    rejected = subprocess.run(
        [pyc_opt, "-o", os.devnull],
        input=source,
        text=True,
        capture_output=True,
        check=False,
    )
    assert rejected.returncode != 0
    assert message in rejected.stderr


def test_selector_rejects_width_without_qualified_candidate() -> None:
    pyc_opt = _tool("pyc-opt")
    catalog = _root() / "library" / "verilog" / "rtl_catalog.json"
    source = """module {
  func.func @wide(%mask: i65) -> i7 {
    %i, %v = pyc.priority_encode %mask {order = "low"} : i65 -> i7, i1
    return %i : i7
  }
}
"""
    rejected = subprocess.run(
        [
            pyc_opt,
            f"--pyc-select-rtl-primitives=catalog={catalog}",
            "-o",
            os.devnull,
        ],
        input=source,
        text=True,
        capture_output=True,
        check=False,
    )
    assert rejected.returncode != 0
    assert "no qualified RTL implementation supports width 65" in rejected.stderr


def test_pyc_cpp_and_selected_rtl_agree(tmp_path: Path) -> None:
    root = _root()
    pycc = _tool("pycc")
    fixture = root / "tests" / "mlir" / "pyc" / "priority-primitive.mlir"
    cpp = tmp_path / "priority.cpp"
    verilog = tmp_path / "verilog"
    common = ["--hierarchy-policy=strict", "--inline-policy=off"]
    subprocess.run(
        [pycc, str(fixture), "--emit=cpp", "-o", str(cpp), *common],
        cwd=root,
        check=True,
        env=_environment(),
    )
    subprocess.run(
        [pycc, str(fixture), "--emit=verilog", "--out-dir", str(verilog), *common],
        cwd=root,
        check=True,
        env=_environment(),
    )

    manifest = json.loads((verilog / "manifest.json").read_text(encoding="utf-8"))
    selection = manifest["rtl_selection"]
    assert selection["schema"] == "pyc-rtl-selection-manifest-v1"
    implementations = selection["implementations"]
    assert len(implementations) == 1
    assert {item["semantic_id"] for item in implementations} == {
        "pyc.priority_encode.v1"
    }
    assert {item["sources"][0]["path"] for item in implementations} == {
        "pyc_priority_encode.v"
    }
    source = implementations[0]["sources"][0]
    bundled_source = verilog / source["bundle_path"]
    assert bundled_source.is_file()
    assert source["sha256"] == (
        "sha256:" + hashlib.sha256(bundled_source.read_bytes()).hexdigest()
    )
    bindings = selection["bindings"]
    assert len(bindings) == 2
    assert {item["parameters"]["ORDER_LOW"] for item in bindings} == {0, 1}
    primitives = (verilog / "pyc_primitives.v").read_text(encoding="utf-8")
    assert primitives.count("module pyc_priority_encode") == 1
    assert "basejump" not in primitives.lower()

    cxx = shutil.which("c++")
    if cxx:
        harness = tmp_path / "cpp_harness.cpp"
        harness.write_text(
            f"""#include "{cpp.as_posix()}"
#include <cstdint>
int main() {{
  pyc::gen::priority_top dut;
  dut.mask = pyc::cpp::Wire<13>(0);
  dut.eval();
  if (dut.low_valid.value() || dut.high_valid.value()) return 1;
  dut.mask = pyc::cpp::Wire<13>((std::uint64_t{{1}} << 11) | (std::uint64_t{{1}} << 3));
  dut.eval();
  if (!dut.low_valid.value() || dut.low_index.value() != 3) return 2;
  if (!dut.high_valid.value() || dut.high_index.value() != 11) return 3;
  return 0;
}}
""",
            encoding="utf-8",
        )
        executable = tmp_path / "cpp_harness"
        subprocess.run(
            [
                cxx,
                "-std=c++17",
                f"-I{root / 'library'}",
                str(harness),
                "-o",
                str(executable),
            ],
            cwd=root,
            check=True,
        )
        subprocess.run([str(executable)], cwd=root, check=True)

    verilator = shutil.which("verilator")
    if verilator:
        subprocess.run(
            [
                verilator,
                "--lint-only",
                "--timing",
                "-Wall",
                "-Wno-fatal",
                "--top-module",
                "priority_top",
                str(verilog / "pyc_primitives.v"),
                str(verilog / "priority_top.v"),
            ],
            cwd=root,
            check=True,
        )
    iverilog = shutil.which("iverilog")
    vvp = shutil.which("vvp")
    if iverilog and vvp:
        image = tmp_path / "priority.vvp"
        subprocess.run(
            [
                iverilog,
                "-g2012",
                "-s",
                "priority_top_generated_tb",
                "-o",
                str(image),
                str(verilog / "pyc_primitives.v"),
                str(verilog / "priority_top.v"),
                str(root / "tests" / "verilog" / "priority_top_generated_tb.sv"),
            ],
            cwd=root,
            check=True,
        )
        completed = subprocess.run(
            [vvp, str(image)], cwd=root, text=True, capture_output=True, check=True
        )
        assert "PYC_SELECTED_RTL_PASS" in completed.stdout


def test_installed_catalog_keeps_license_evidence() -> None:
    configured = os.environ.get("PYC_TOOLCHAIN_ROOT")
    install = (
        Path(configured)
        if configured
        else _root() / ".pycircuit_out" / "toolchain" / "install"
    )
    catalog_path = install / "include" / "verilog" / "rtl_catalog.json"
    if not catalog_path.is_file():
        pytest.skip("installed pyCircuit toolchain is unavailable")
    catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    for implementation in catalog["implementations"]:
        license_path = catalog_path.parent / implementation["license_file"]
        assert license_path.is_file()
        digest = "sha256:" + hashlib.sha256(license_path.read_bytes()).hexdigest()
        assert digest == implementation["license_sha256"]
