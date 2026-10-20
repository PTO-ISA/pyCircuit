"""Focused source-unit generation test for the functional floating example."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
EXAMPLE = ROOT / "examples" / "functional_float"


def _helper(name: str) -> str | None:
    value = os.environ.get(name)
    return value if value and Path(value).is_file() else None


def _run(*args: str, env: dict[str, str]) -> None:
    result = subprocess.run(
        [sys.executable, "-m", "pycircuit.cli", *args],
        cwd=ROOT,
        env=env,
        text=True,
        capture_output=True,
        check=False,
    )
    assert result.returncode == 0, result.stdout + result.stderr


@pytest.mark.system
def test_functional_float_source_units_compile_link_and_emit(tmp_path: Path) -> None:
    source_compiler = _helper("PYCIRCUIT_SOURCE_COMPILER")
    linker = _helper("PYCIRCUIT_LINKER")
    emitter = _helper("PYCIRCUIT_EMITTER")
    if not (source_compiler and linker and emitter):
        pytest.skip("native pyCircuit source-unit helpers are not configured")

    env = dict(os.environ)
    env.update(
        PYTHONPATH=str(ROOT / "python"),
        PYCIRCUIT_SOURCE_COMPILER=source_compiler,
        PYCIRCUIT_LINKER=linker,
        PYCIRCUIT_EMITTER=emitter,
    )
    units = tmp_path / "units"
    units.mkdir()
    compile_args = {
        "types": ("fp32_types.py", ()),
        "product": ("fp32_product.py", ("types",)),
        "sum": ("fp32_sum.py", ("types",)),
        "exp": ("fp32_exp.py", ("types", "product", "sum")),
        "divide": ("fp32_divide.py", ("types", "product")),
        "product_sum": (
            "fp32_product_sum.py",
            ("types", "product", "sum"),
        ),
    }
    for unit, (source, interfaces) in compile_args.items():
        args = [
            "compile",
            "-c",
            str(EXAMPLE / source),
            "--source-root",
            str(EXAMPLE),
            "--package-prefix",
            "functional_float",
            "-o",
            str(units / unit),
            "--replace",
        ]
        for interface in interfaces:
            args.extend(("-I", str(units / interface)))
        _run(*args, env=env)

    linked = tmp_path / "linked"
    linked.mkdir()
    _run(
        "link",
        str(units / "types"),
        str(units / "product"),
        str(units / "sum"),
        "--top",
        "functional_float.fp32_product.FP32Product",
        "-o",
        str(linked / "product.ac"),
        "--replace",
        env=env,
    )
    generated = tmp_path / "cpp_product"
    _run(
        "emit",
        "--target",
        "cpp",
        "-o",
        str(generated),
        str(linked / "product.ac"),
        "--replace",
        env=env,
    )
    assert (generated / "pycircuit_support.hpp").is_file()
    assert any(generated.rglob("fp32_product.cpp"))

    for name, top, closure in (
        ("product_sum", "FP32ProductSum", ("types", "product", "sum", "product_sum")),
        ("exp", "FP32Exp", ("types", "product", "sum", "exp")),
        ("divide", "FP32Divide", ("types", "product", "divide")),
    ):
        final = linked / f"{name}.ac"
        _run("link", *(str(units / unit) for unit in closure),
             "--top", f"functional_float.fp32_{name}.{top}",
             "-o", str(final), "--replace", env=env)
        for target in ("cpp", "verilog"):
            _run("emit", "--target", target, "-o", str(tmp_path / f"{target}_{name}"),
                 str(final), "--replace", env=env)

    # Every row independently exposes an exception from the product stage,
    # including a case whose rounded final result is exactly zero (not FMA).
    vectors = [
        (0x3f800001, 0x3f800001, 0xbf800002, 0, 1, 0, 0, 0, 1, 1),
        (0x7f800000, 0, 0, 0x7fc00000, 0, 1, 0, 0, 0, 0),
        (0x7f7fffff, 0x40000000, 0, 0x7f800000, 1, 0, 1, 0, 1, 0),
        (1, 0x3f000000, 0x3f800000, 0x3f800000, 1, 0, 0, 1, 1, 0),
    ]
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    driver = tmp_path / "product_sum.cpp"
    rows = ",\n".join("{" + ",".join(str(x) + "u" for x in row) + "}" for row in vectors)
    driver.write_text('''
#include "sources/functional_float/fp32_product_sum.hpp"
#include <cstdint>
int main() {
    const uint32_t rows[][10] = {ROWS};
    for (const auto& row : rows) {
        functional_float::fp32_product_sum::FP32ProductSum dut("flags");
        dut.lhs = decltype(dut.lhs)::known(gfsim::Bits<32>{row[0]});
        dut.rhs = decltype(dut.rhs)::known(gfsim::Bits<32>{row[1]});
        dut.acc = decltype(dut.acc)::known(gfsim::Bits<32>{row[2]});
        dut.request_valid = decltype(dut.request_valid)::known(gfsim::Bits<1>{1});
        dut.Work();
        if (!dut.result.isFullyKnown()) return 1;
        const auto out = dut.result.value();
        if (out.result.value() != row[3] || out.valid.toBool() != row[4] ||
            out.invalid.toBool() != row[5] || out.overflow.toBool() != row[6] ||
            out.underflow.toBool() != row[7] || out.inexact.toBool() != row[8] ||
            out.zero.toBool() != row[9]) return 2;
    }
}
'''.replace("ROWS", rows), encoding="utf-8")
    binary = tmp_path / "product_sum"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1",
                    f"-I{tmp_path / 'cpp_product_sum'}", f"-I{runtime / 'include'}",
                    str(driver), str(runtime / "lib/libpyc6_runtime.a"),
                    "-pthread", "-o", str(binary)], check=True, capture_output=True)
    subprocess.run([str(binary)], check=True)

    quotient_driver = tmp_path / "quotient.cpp"
    quotient_driver.write_text('''
#include "sources/functional_float/fp32_divide.hpp"
int main() {
    functional_float::fp32_divide::FP32Divide dut("quotient");
    dut.lhs = decltype(dut.lhs)::known(gfsim::Bits<32>{0x3f800000});
    dut.rhs = decltype(dut.rhs)::known(gfsim::Bits<32>{0x80000000});
    dut.request_valid = decltype(dut.request_valid)::known(gfsim::Bits<1>{1});
    dut.Work();
    if (!dut.result.isFullyKnown()) return 1;
    const auto out = dut.result.value();
    return !(out.result.value() == 0xff800000 && out.valid.toBool() &&
             out.divide_by_zero.toBool() && !out.invalid.toBool());
}
''', encoding="utf-8")
    quotient_binary = tmp_path / "quotient"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1",
                    f"-I{tmp_path / 'cpp_divide'}", f"-I{runtime / 'include'}",
                    str(quotient_driver), str(runtime / "lib/libpyc6_runtime.a"),
                    "-pthread", "-o", str(quotient_binary)], check=True, capture_output=True)
    subprocess.run([str(quotient_binary)], check=True)

    exp_driver = tmp_path / "exponential.cpp"
    exp_driver.write_text('''
#include "sources/functional_float/fp32_exp.hpp"
#include <cstdint>
int main() {
    const uint32_t rows[][2] = {{0x3f000000,0x3fd3094c},
        {0x3f800000,0x402df854},{0xbf800000,0x3ebc5ab2}};
    for (const auto& row : rows) {
        functional_float::fp32_exp::FP32Exp dut("exponential");
        dut.value = decltype(dut.value)::known(gfsim::Bits<32>{row[0]});
        dut.request_valid = decltype(dut.request_valid)::known(gfsim::Bits<1>{1});
        dut.Work();
        if (!dut.result.isFullyKnown()) return 1;
        const auto out = dut.result.value();
        if (out.result.value() != row[1] || !out.valid.toBool()) return 2;
    }
}
''', encoding="utf-8")
    exp_binary = tmp_path / "exponential"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1",
                    f"-I{tmp_path / 'cpp_exp'}", f"-I{runtime / 'include'}",
                    str(exp_driver), str(runtime / "lib/libpyc6_runtime.a"),
                    "-pthread", "-o", str(exp_binary)], check=True, capture_output=True)
    subprocess.run([str(exp_binary)], check=True)

    if not (shutil.which("iverilog") and shutil.which("vvp")):
        pytest.fail("Icarus is required to verify the changed composition on RTL")
    rtl = tmp_path / "verilog_product_sum"
    ports = json.loads((rtl / "ports.json").read_text())
    output = ports["outputs"][0]
    fields = {field["path"][0]: field for field in output["fields"]}
    samples = []
    for row in vectors:
        packed = sum(row[index] << fields[name]["low"] for index, name in
                     enumerate(("result", "valid", "invalid", "overflow",
                                "underflow", "inexact", "zero"), start=3))
        samples.append(f"lhs=32'h{row[0]:08x}; rhs=32'h{row[1]:08x}; "
                       f"acc=32'h{row[2]:08x}; #1; "
                       f"if (result !== 38'h{packed:x}) $fatal(1, \"flags\");")
    tb = tmp_path / "tb.sv"
    tb.write_text("module tb; reg [31:0] lhs,rhs,acc; wire [37:0] result; "
                  "pyc_root dut(.lhs(lhs),.rhs(rhs),.acc(acc),.request_valid(1'b1),"
                  ".result(result)); initial begin " + "\n".join(samples) +
                  " $finish; end endmodule\n")
    rtl_binary = tmp_path / "rtl"
    subprocess.run(["iverilog", "-g2012", "-s", "tb", "-o", str(rtl_binary),
                    str(rtl / "design_top.sv"),
                    *(str(path) for path in sorted((rtl / "sources").rglob("*.v"))),
                    str(tb)], check=True, capture_output=True)
    subprocess.run(["vvp", str(rtl_binary)], check=True, capture_output=True)
