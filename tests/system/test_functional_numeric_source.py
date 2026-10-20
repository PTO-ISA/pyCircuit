"""Public numeric providers: explicit closures, native and RTL rational oracle."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytest
from test_functional_float_source import EXAMPLE, ROOT, _helper, _run


@pytest.mark.system
def test_numeric_providers_native_and_rtl(tmp_path: Path) -> None:
    helpers = {
        key: _helper(key)
        for key in (
            "PYCIRCUIT_SOURCE_COMPILER",
            "PYCIRCUIT_LINKER",
            "PYCIRCUIT_EMITTER",
        )
    }
    if not all(helpers.values()):
        pytest.skip("native pyCircuit source-unit helpers are not configured")
    if not shutil.which("iverilog") or not shutil.which("vvp"):
        pytest.skip("Icarus Verilog is not installed")
    env = dict(os.environ, PYTHONPATH=str(ROOT / "python"), **helpers)
    units = tmp_path / "units"
    units.mkdir()
    specs = {
        "fp64_types": (),
        "fp64_round": ("fp64_types",),
        "fp64_sum": ("fp64_types", "fp64_round"),
        "fp64_product": ("fp64_types", "fp64_round"),
        "fp64_divide": ("fp64_types", "fp64_round"),
        "float_compare": (),
        "float_from_integer": ("fp64_types",),
    }
    tops = {
        "fp64_sum": "FP64Sum",
        "fp64_product": "FP64Product",
        "fp64_divide": "FP64Divide",
        "float_compare": "FloatCompare",
        "float_from_integer": "FloatFromInteger",
    }
    for unit, dependencies in specs.items():
        args = [
            "compile",
            "-c",
            str(EXAMPLE / f"{unit}.py"),
            "--source-root",
            str(EXAMPLE),
            "--package-prefix",
            "functional_float",
            "-o",
            str(units / unit),
            "--replace",
        ]
        for dependency in dependencies:
            args.extend(("-I", str(units / dependency)))
        _run(*args, env=env)
        if unit not in tops:
            continue
        linked = tmp_path / f"{unit}.ac"
        _run(
            "link",
            *(str(units / dep) for dep in (*dependencies, unit)),
            "--top",
            f"functional_float.{unit}.{tops[unit]}",
            "-o",
            str(linked),
            "--replace",
            env=env,
        )
        for target in ("cpp", "verilog"):
            _run(
                "emit",
                "--target",
                target,
                "-o",
                str(tmp_path / f"{target}_{unit}"),
                str(linked),
                "--replace",
                env=env,
            )
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    cases = []
    sources = []
    declarations = []
    for index, unit in enumerate(tops):
        # Each port is assigned a typed, known value at the generated boundary.
        pairs = (
            [("lhs", "a"), ("rhs", "b")]
            if index < 3
            else [
                ("lhs", "a"),
                ("rhs", "b"),
                ("wide64", "arg&1"),
                ("operation", "op-10"),
                ("signaling", "(arg&2)!=0"),
            ]
            if unit == "float_compare"
            else [("value", "a"), ("unsigned_input", "b!=0"), ("format", "arg")]
        )
        pairs.append(("request_valid", "1"))
        assignments = "".join(
            f"dut.{port}=decltype(dut.{port})::known(std::decay_t<decltype(dut.{port}.value())>{{{value}}});"
            for port, value in pairs
        )
        output = (
            'v.result.value()<<" "<<v.valid.toBool()<<" "<<v.invalid.toBool()<<" 0 0 0 "<<!v.result.toBool()<<" 0"'
            if unit == "float_compare"
            else 'v.result.value()<<" "<<v.valid.toBool()<<" "<<v.invalid.toBool()<<" "<<v.overflow.toBool()<<" "<<v.underflow.toBool()<<" "<<v.inexact.toBool()<<" "<<v.zero.toBool()<<" "<<v.divide_by_zero.toBool()'
        )
        condition = (
            f"op=={index}"
            if index < 3
            else "op>=10"
            if unit == "float_compare"
            else "op==3"
        )
        cases.append(f"else if({condition})run_{unit}(op,a,b,arg);")
        declarations.append(f"void run_{unit}(unsigned,uint64_t,uint64_t,unsigned);")
        source = tmp_path / f"{unit}_runner.cpp"
        source.write_text(
            f'#include "sources/functional_float/{unit}.hpp"\n'
            "#include <iostream>\n#include <type_traits>\n"
            f"void run_{unit}(unsigned op,uint64_t a,uint64_t b,unsigned arg){{"
            f'functional_float::{unit}::{tops[unit]} dut("numeric");'
            + assignments
            + "dut.Work();if(!dut.result.isFullyKnown())std::exit(2);"
            + "auto v=dut.result.value();std::cout<<std::hex<<"
            + output
            + '<<"\\n";}'
        )
        sources.append(source)
    runner = tmp_path / "runner.cpp"
    runner.write_text(
        "#include <iostream>\n#include <cstdint>\n"
        + "".join(declarations)
        + "\nint main(){"
        "unsigned op,arg;uint64_t a,b;while(std::cin>>std::dec>>op>>std::hex>>a>>b>>std::dec>>arg){"
        "if(false){}" + "".join(cases) + "else return 3;}}\n"
    )
    binary = tmp_path / "runner"
    objects = []
    for source, unit in zip(sources, tops, strict=True):
        obj = source.with_suffix(".o")
        command = [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            "-Wno-narrowing",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / ('cpp_' + unit)}",
            "-c",
            str(source),
            "-o",
            str(obj),
        ]
        build = subprocess.run(command, capture_output=True, text=True)
        assert build.returncode == 0, build.stdout + build.stderr
        objects.append(str(obj))
    subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            str(runner),
            *objects,
            str(runtime / "lib/libpyc6_runtime.a"),
            "-pthread",
            "-o",
            str(binary),
        ],
        check=True,
    )
    here = Path(__file__).parent
    subprocess.run(
        [
            sys.executable,
            str(here / "functional_numeric_oracle.py"),
            str(binary),
            "--out",
            str(tmp_path / "oracle"),
        ],
        check=True,
    )
    subprocess.run(
        [
            sys.executable,
            str(here / "functional_numeric_rtl_oracle.py"),
            str(tmp_path),
            str(tmp_path / "oracle/vectors.json"),
            str(tmp_path / "rtl_oracle"),
        ],
        check=True,
    )


@pytest.mark.system
def test_bit_transform_native_and_rtl(tmp_path: Path) -> None:
    helpers = {
        key: _helper(key)
        for key in (
            "PYCIRCUIT_SOURCE_COMPILER",
            "PYCIRCUIT_LINKER",
            "PYCIRCUIT_EMITTER",
        )
    }
    if not all(helpers.values()) or not shutil.which("iverilog"):
        pytest.skip("native compiler helpers and Icarus Verilog are required")
    env = dict(os.environ, PYTHONPATH=str(ROOT / "python"), **helpers)
    subprocess.run(
        [
            sys.executable,
            str(EXAMPLE / "build_units.py"),
            str(tmp_path),
            "--only",
            "bit_transform",
            "--replace",
        ],
        env=env,
        check=True,
    )
    source = tmp_path / "bit_runner.cpp"
    source.write_text(r"""#include "sources/functional_float/bit_transform.hpp"
#include <iostream>
int main(){uint64_t a,b,c;unsigned w,o,l,op;
while(std::cin>>std::hex>>a>>b>>c>>std::dec>>w>>o>>l>>op){
functional_float::bit_transform::BitTransform dut("bits");
dut.lhs=decltype(dut.lhs)::known(gfsim::Bits<64>{a});
dut.rhs=decltype(dut.rhs)::known(gfsim::Bits<64>{b});
dut.alternate=decltype(dut.alternate)::known(gfsim::Bits<64>{c});
dut.width=decltype(dut.width)::known(gfsim::Bits<7>{w});
dut.offset=decltype(dut.offset)::known(gfsim::Bits<6>{o});
dut.last=decltype(dut.last)::known(gfsim::Bits<6>{l});
dut.operation=decltype(dut.operation)::known(gfsim::Bits<4>{op});
dut.request_valid=decltype(dut.request_valid)::known(gfsim::Bits<1>{1});
dut.Work();if(!dut.result.isFullyKnown())return 2;const auto v=dut.result.value();
if(!v.valid.toBool())return 3;std::cout<<std::hex<<v.result.value()<<"\n";}}
""")
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    binary = tmp_path / "bit_runner"
    build = subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / 'cpp_bit_transform'}",
            str(source),
            str(runtime / "lib/libpyc6_runtime.a"),
            "-pthread",
            "-o",
            str(binary),
        ],
        capture_output=True,
        text=True,
    )
    assert build.returncode == 0, build.stdout + build.stderr
    subprocess.run(
        [
            sys.executable,
            str(Path(__file__).with_name("functional_bit_oracle.py")),
            str(binary),
            "--verilog-dir",
            str(tmp_path / "verilog_bit_transform"),
            "--output-dir",
            str(tmp_path / "oracle"),
        ],
        check=True,
    )
