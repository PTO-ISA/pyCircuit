"""Public numeric providers: explicit closures, native and RTL rational oracle."""

from __future__ import annotations

import json
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
            else (
                [
                    ("lhs", "a"),
                    ("rhs", "b"),
                    ("wide64", "arg&1"),
                    ("operation", "op-10"),
                    ("signaling", "(arg&2)!=0"),
                ]
                if unit == "float_compare"
                else [("value", "a"), ("unsigned_input", "b!=0"), ("format", "arg")]
            )
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
            else "op>=10" if unit == "float_compare" else "op==3"
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
    source.write_text(
        r"""#include "sources/functional_float/bit_transform.hpp"
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
"""
    )
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


@pytest.mark.system
def test_float_to_integer_native_and_rtl(tmp_path: Path) -> None:
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
            "float_to_integer",
            "--replace",
        ],
        env=env,
        check=True,
    )
    source = tmp_path / "runner.cpp"
    source.write_text(
        "#include \"sources/functional_float/float_to_integer.hpp\"\n#include <iostream>\nint main(){uint64_t a;unsigned w,u,c,r;while(std::cin>>std::hex>>a>>std::dec>>w>>u>>c>>r){\nfunctional_float::float_to_integer::FloatToInteger dut(\"conversion\");\ndut.value=decltype(dut.value)::known(gfsim::Bits<64>{a});dut.wide64=decltype(dut.wide64)::known(gfsim::Bits<1>{w});\ndut.unsigned_output=decltype(dut.unsigned_output)::known(gfsim::Bits<1>{u});dut.width_code=decltype(dut.width_code)::known(gfsim::Bits<2>{c});\ndut.rounding=decltype(dut.rounding)::known(gfsim::Bits<3>{r});dut.request_valid=decltype(dut.request_valid)::known(gfsim::Bits<1>{1});\ndut.Work();if(!dut.result.isFullyKnown())return 2;const auto v=dut.result.value();\nstd::cout<<std::hex<<v.result.value()<<std::dec<<' '<<v.valid.toBool()<<' '<<v.invalid.toBool()<<' '\n<<v.overflow.toBool()<<' '<<v.underflow.toBool()<<' '<<v.inexact.toBool()<<' '<<v.zero.toBool()<<' '<<v.divide_by_zero.toBool()<<'\\n';}}\n"
    )
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    binary = tmp_path / "runner"
    build = subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / 'cpp_float_to_integer'}",
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
            str(Path(__file__).with_name("functional_float_to_integer_oracle.py")),
            str(binary),
            str(tmp_path / "verilog_float_to_integer"),
            str(tmp_path / "oracle"),
        ],
        check=True,
    )


@pytest.mark.system
def test_square_root_native_and_rtl(tmp_path: Path) -> None:
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
            "float_sqrt",
            "--replace",
        ],
        env=env,
        check=True,
    )
    source = tmp_path / "runner.cpp"
    source.write_text(
        "#include \"sources/functional_float/float_sqrt.hpp\"\n#include <iostream>\nint main(){uint64_t a;unsigned w;while(std::cin>>std::hex>>a>>std::dec>>w){\nfunctional_float::float_sqrt::FloatSqrt dut(\"sqrt\");\ndut.value=decltype(dut.value)::known(gfsim::Bits<64>{a});dut.wide64=decltype(dut.wide64)::known(gfsim::Bits<1>{w});\ndut.request_valid=decltype(dut.request_valid)::known(gfsim::Bits<1>{1});dut.Work();\nif(!dut.result.isFullyKnown())return 2;const auto v=dut.result.value();\nstd::cout<<std::hex<<v.result.value()<<std::dec<<' '<<v.valid.toBool()<<' '<<v.invalid.toBool()<<' '\n<<v.overflow.toBool()<<' '<<v.underflow.toBool()<<' '<<v.inexact.toBool()<<' '<<v.zero.toBool()<<' '<<v.divide_by_zero.toBool()<<'\\n';}}\n"
    )
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    binary = tmp_path / "runner"
    build = subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / 'cpp_float_sqrt'}",
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
            str(Path(__file__).with_name("functional_sqrt_oracle.py")),
            str(binary),
            str(tmp_path / "verilog_float_sqrt"),
            str(tmp_path / "oracle"),
        ],
        check=True,
    )


@pytest.mark.system
def test_fp32_remainder_native_and_rtl(tmp_path: Path) -> None:
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
            "fp32_remainder",
            "--replace",
        ],
        env=env,
        check=True,
    )
    source = tmp_path / "runner.cpp"
    source.write_text(
        "#include \"sources/functional_float/fp32_remainder.hpp\"\n#include <iostream>\nint main(){uint32_t a,b;while(std::cin>>std::hex>>a>>b){\nfunctional_float::fp32_remainder::FP32FloorModulo dut(\"modulo\");\ndut.lhs=decltype(dut.lhs)::known(gfsim::Bits<32>{a});dut.rhs=decltype(dut.rhs)::known(gfsim::Bits<32>{b});\ndut.request_valid=decltype(dut.request_valid)::known(gfsim::Bits<1>{1});dut.Work();if(!dut.result.isFullyKnown())return 2;\nconst auto v=dut.result.value();std::cout<<std::hex<<v.result.value()<<std::dec<<' '<<v.valid.toBool()<<' '<<v.invalid.toBool()\n<<' '<<v.overflow.toBool()<<' '<<v.underflow.toBool()<<' '<<v.inexact.toBool()<<' '<<v.zero.toBool()<<'\\n';}}\n"
    )
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    binary = tmp_path / "runner"
    build = subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / 'cpp_fp32_remainder'}",
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
            str(Path(__file__).with_name("functional_floor_modulo_oracle.py")),
            str(binary),
            str(tmp_path / "verilog_fp32_remainder"),
            str(tmp_path / "oracle"),
        ],
        check=True,
    )


@pytest.mark.system
def test_fp32_trunc_modulo_native_and_rtl(tmp_path: Path) -> None:
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
            "fp32_trunc_modulo",
            "--replace",
        ],
        env=env,
        check=True,
    )
    source = tmp_path / "runner.cpp"
    source.write_text(
        "#include \"sources/functional_float/fp32_trunc_modulo.hpp\"\n#include <iostream>\nint main(){uint32_t a,b;while(std::cin>>std::hex>>a>>b){\nfunctional_float::fp32_trunc_modulo::FP32TruncModulo dut(\"modulo\");\ndut.lhs=decltype(dut.lhs)::known(gfsim::Bits<32>{a});dut.rhs=decltype(dut.rhs)::known(gfsim::Bits<32>{b});\ndut.request_valid=decltype(dut.request_valid)::known(gfsim::Bits<1>{1});dut.Work();if(!dut.result.isFullyKnown())return 2;\nconst auto v=dut.result.value();std::cout<<std::hex<<v.result.value()<<std::dec<<' '<<v.valid.toBool()<<' '<<v.invalid.toBool()\n<<' '<<v.overflow.toBool()<<' '<<v.underflow.toBool()<<' '<<v.inexact.toBool()<<' '<<v.zero.toBool()<<'\\n';}}\n"
    )
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    binary = tmp_path / "runner"
    build = subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / 'cpp_fp32_trunc_modulo'}",
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
            str(Path(__file__).with_name("functional_trunc_modulo_oracle.py")),
            str(binary),
            str(tmp_path / "verilog_fp32_trunc_modulo"),
            str(tmp_path / "oracle"),
        ],
        check=True,
    )


@pytest.mark.system
def test_word_division_native_and_rtl(tmp_path: Path) -> None:
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
            "word_division",
            "--replace",
        ],
        env=env,
        check=True,
    )
    source = tmp_path / "runner.cpp"
    source.write_text(
        "#include \"sources/functional_float/word_division.hpp\"\n#include <iostream>\nint main(){uint64_t a,b;unsigned s,w,z,v;while(std::cin>>std::hex>>a>>b>>std::dec>>s>>w>>z>>v){\nfunctional_float::word_division::WordDivision dut(\"quotient\");\ndut.lhs=decltype(dut.lhs)::known(gfsim::Bits<64>{a});dut.rhs=decltype(dut.rhs)::known(gfsim::Bits<64>{b});\ndut.is_signed=decltype(dut.is_signed)::known(gfsim::Bits<1>{s});dut.word32=decltype(dut.word32)::known(gfsim::Bits<1>{w});\ndut.zero_ones=decltype(dut.zero_ones)::known(gfsim::Bits<1>{z});dut.request_valid=decltype(dut.request_valid)::known(gfsim::Bits<1>{v});\ndut.Work();if(!dut.result.isFullyKnown())return 2;const auto out=dut.result.value();\nstd::cout<<std::hex<<out.result.value()<<std::dec<<' '<<out.valid.toBool()<<'\\n';}}\n"
    )
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    binary = tmp_path / "runner"
    build = subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / 'cpp_word_division'}",
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
            str(Path(__file__).with_name("functional_word_division_oracle.py")),
            str(binary),
            str(tmp_path / "verilog_word_division"),
            str(tmp_path / "oracle"),
        ],
        check=True,
    )


@pytest.mark.system
def test_float_convert_native_and_rtl(tmp_path: Path) -> None:
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
            "float_convert",
            "--replace",
        ],
        env=env,
        check=True,
    )
    source = tmp_path / "runner.cpp"
    source.write_text(
        r"""#include "sources/functional_float/float_convert.hpp"
#include <iostream>
int main(){uint64_t value;unsigned source,destination;
while(std::cin>>std::hex>>value>>std::dec>>source>>destination){
functional_float::float_convert::FloatConvert dut("conversion");
dut.value=decltype(dut.value)::known(gfsim::Bits<64>{value});
dut.source_format=decltype(dut.source_format)::known(gfsim::Bits<2>{source});
dut.destination_format=decltype(dut.destination_format)::known(gfsim::Bits<2>{destination});
dut.request_valid=decltype(dut.request_valid)::known(gfsim::Bits<1>{1});
dut.Work();if(!dut.result.isFullyKnown())return 2;const auto v=dut.result.value();
std::cout<<std::hex<<v.result.value()<<std::dec<<' '<<v.valid.toBool()<<' '<<v.invalid.toBool()
<<' '<<v.overflow.toBool()<<' '<<v.underflow.toBool()<<' '<<v.inexact.toBool()<<' '<<v.zero.toBool()
<<' '<<v.divide_by_zero.toBool()<<'\n';}}
"""
    )
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    binary = tmp_path / "runner"
    build = subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / 'cpp_float_convert'}",
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
            str(Path(__file__).with_name("functional_convert_oracle.py")),
            str(binary),
            str(tmp_path / "verilog_float_convert"),
            str(tmp_path / "oracle"),
        ],
        check=True,
    )


@pytest.mark.system
def test_float_fused_native_and_rtl(tmp_path: Path) -> None:
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
            "float_fused",
            "--replace",
        ],
        env=env,
        check=True,
    )
    source = tmp_path / "runner.cpp"
    source.write_text(
        r"""#include "sources/functional_float/float_fused.hpp"
#include <iostream>
int main(){uint64_t a,b,c;unsigned w;while(std::cin>>std::hex>>a>>b>>c>>std::dec>>w){
functional_float::float_fused::FloatFused dut("fused");
dut.lhs=decltype(dut.lhs)::known(gfsim::Bits<64>{a});
dut.rhs=decltype(dut.rhs)::known(gfsim::Bits<64>{b});
dut.addend=decltype(dut.addend)::known(gfsim::Bits<64>{c});
dut.wide64=decltype(dut.wide64)::known(gfsim::Bits<1>{w});
dut.request_valid=decltype(dut.request_valid)::known(gfsim::Bits<1>{1});dut.Work();
if(!dut.result.isFullyKnown())return 2;const auto v=dut.result.value();
std::cout<<std::hex<<v.result.value()<<std::dec<<' '<<v.valid.toBool()<<' '<<v.invalid.toBool()
<<' '<<v.overflow.toBool()<<' '<<v.underflow.toBool()<<' '<<v.inexact.toBool()<<' '<<v.zero.toBool()<<' '<<v.divide_by_zero.toBool()<<'\n';}}
"""
    )
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    binary = tmp_path / "runner"
    build = subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / 'cpp_float_fused'}",
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
            str(Path(__file__).with_name("functional_fused_oracle.py")),
            str(binary),
            str(tmp_path / "verilog_float_fused"),
            str(tmp_path / "oracle"),
        ],
        check=True,
    )


@pytest.mark.system
def test_float_fused_format_format_native_and_rtl(tmp_path: Path) -> None:
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
            "float_fused_format",
            "--replace",
        ],
        env=env,
        check=True,
    )
    source = tmp_path / "runner.cpp"
    source.write_text(
        r"""#include "sources/functional_float/float_fused_format.hpp"
#include <iostream>
int main(){uint64_t a,b,c;unsigned w;while(std::cin>>std::hex>>a>>b>>c>>std::dec>>w){
functional_float::float_fused_format::FloatFusedFormat dut("fused");
dut.lhs=decltype(dut.lhs)::known(gfsim::Bits<64>{a});
dut.rhs=decltype(dut.rhs)::known(gfsim::Bits<64>{b});
dut.addend=decltype(dut.addend)::known(gfsim::Bits<64>{c});
dut.format_code=decltype(dut.format_code)::known(gfsim::Bits<2>{w});
dut.request_valid=decltype(dut.request_valid)::known(gfsim::Bits<1>{1});dut.Work();
if(!dut.result.isFullyKnown())return 2;const auto v=dut.result.value();
std::cout<<std::hex<<v.result.value()<<std::dec<<' '<<v.valid.toBool()<<' '<<v.invalid.toBool()
<<' '<<v.overflow.toBool()<<' '<<v.underflow.toBool()<<' '<<v.inexact.toBool()<<' '<<v.zero.toBool()<<' '<<v.divide_by_zero.toBool()<<'\n';}}
"""
    )
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    binary = tmp_path / "runner"
    build = subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / 'cpp_float_fused_format'}",
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
            str(Path(__file__).with_name("functional_fused_format_oracle.py")),
            str(binary),
            str(tmp_path / "verilog_float_fused_format"),
            str(tmp_path / "oracle"),
        ],
        check=True,
    )


@pytest.mark.system
def test_fp32_log_native_and_rtl(tmp_path: Path) -> None:
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
            "fp32_log",
            "--replace",
        ],
        env=env,
        check=True,
    )
    source = tmp_path / "runner.cpp"
    source.write_text(
        r"""#include "sources/functional_float/fp32_log.hpp"
#include <iostream>
int main(){uint32_t value;while(std::cin>>std::hex>>value){
functional_float::fp32_log::FP32Log dut("logarithm");
dut.value=decltype(dut.value)::known(gfsim::Bits<32>{value});
dut.request_valid=decltype(dut.request_valid)::known(gfsim::Bits<1>{1});dut.Work();
if(!dut.result.isFullyKnown())return 2;const auto v=dut.result.value();
std::cout<<std::hex<<v.result.value()<<std::dec<<' '<<v.valid.toBool()<<' '<<v.invalid.toBool()
<<' '<<v.overflow.toBool()<<' '<<v.underflow.toBool()<<' '<<v.inexact.toBool()<<' '<<v.zero.toBool()<<' '<<v.divide_by_zero.toBool()<<'\n';}}
"""
    )
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    binary = tmp_path / "runner"
    build = subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / 'cpp_fp32_log'}",
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
            str(Path(__file__).with_name("functional_log_oracle.py")),
            str(binary),
            str(tmp_path / "verilog_fp32_log"),
            str(tmp_path / "oracle"),
        ],
        check=True,
    )


@pytest.mark.system
def test_fp19_expansion_native_and_rtl(tmp_path: Path) -> None:
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
            "fp19_expansion",
            "--replace",
        ],
        env=env,
        check=True,
    )
    source = tmp_path / "runner.cpp"
    source.write_text(
        r"""#include "sources/functional_float/fp19_expansion.hpp"
#include <iostream>
int main(){unsigned value;while(std::cin>>std::hex>>value){
functional_float::fp19_expansion::FP19FiniteExpand dut("finite19");
dut.value=decltype(dut.value)::known(gfsim::Bits<19>{value});dut.Work();
if(!dut.result.isFullyKnown())return 2;std::cout<<std::hex<<dut.result.value().value()<<'\n';}}
"""
    )
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    binary = tmp_path / "runner"
    build = subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / 'cpp_fp19_expansion'}",
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
            str(Path(__file__).with_name("functional_fp19_oracle.py")),
            str(binary),
            str(tmp_path / "verilog_fp19_expansion"),
            str(tmp_path / "oracle"),
        ],
        check=True,
    )


@pytest.mark.system
def test_fp32_negative_power_native_and_rtl(tmp_path: Path) -> None:
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
            "fp32_negative_power",
            "--replace",
        ],
        env=env,
        check=True,
    )
    source = tmp_path / "runner.cpp"
    source.write_text(
        r"""#include "sources/functional_float/fp32_negative_power.hpp"
#include <iostream>
int main(){unsigned code;while(std::cin>>std::hex>>code){
functional_float::fp32_negative_power::FP32NegativePower dut("negative-power");
dut.code=decltype(dut.code)::known(gfsim::Bits<8>{code});dut.Work();
if(!dut.result.isFullyKnown())return 2;std::cout<<std::hex<<dut.result.value().value()<<'\n';}}
"""
    )
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    binary = tmp_path / "runner"
    build = subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / 'cpp_fp32_negative_power'}",
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
            str(Path(__file__).with_name("functional_negative_power_oracle.py")),
            str(binary),
            str(tmp_path / "verilog_fp32_negative_power"),
            str(tmp_path / "oracle"),
        ],
        check=True,
    )


@pytest.mark.system
@pytest.mark.parametrize("mode", ["decode", "encode", "scale"])
def test_lowp_codec_native_and_rtl(tmp_path: Path, mode: str) -> None:
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
    unit = "e8m0_encode" if mode == "scale" else f"lowp_{mode}"
    subprocess.run(
        [
            sys.executable,
            str(EXAMPLE / "build_units.py"),
            str(tmp_path),
            "--only",
            unit,
            "--replace",
        ],
        env=env,
        check=True,
    )
    fields = [("value", 8 if mode == "decode" else 32)]
    if mode != "scale":
        fields += [("kind", 2)]
    else:
        fields += [("rounding", 3), ("saturating", 1)]
    if mode == "encode":
        fields += [("rounding", 1), ("saturating", 1)]
    fields += [("request_valid", 1)]
    source = tmp_path / "runner.cpp"
    body = f'#include "sources/functional_float/{unit}.hpp"\n#include <iostream>\n'
    body += (
        "int main(){unsigned "
        + ",".join(n for n, _ in fields)
        + ";while(std::cin>>std::hex"
    )
    body += "".join(">>" + n for n, _ in fields) + "){\n"
    top = "E8M0Encode" if mode == "scale" else f"Lowp{mode.capitalize()}"
    body += f'functional_float::{unit}::{top} dut("codec");\n'
    for name, width in fields:
        body += (
            f"dut.{name}=decltype(dut.{name})::known(gfsim::Bits<{width}>{{{name}}});\n"
        )
    body += (
        "dut.Work();if(!dut.result.isFullyKnown())return 2;auto v=dut.result.value();"
    )
    body += "std::cout<<std::hex<<v.result.value()<<std::dec"
    for name in [
        "valid",
        "invalid",
        "overflow",
        "underflow",
        "inexact",
        "zero",
        "divide_by_zero",
    ]:
        body += f"<<' '<<v.{name}.toBool()"
    body += "<<'\\n';}}\n"
    source.write_text(body)
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    binary = tmp_path / "runner"
    result = subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / ('cpp_' + unit)}",
            str(source),
            str(runtime / "lib/libpyc6_runtime.a"),
            "-pthread",
            "-o",
            str(binary),
        ],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    oracle = (
        "functional_e8m0_oracle.py" if mode == "scale" else "functional_lowp_oracle.py"
    )
    subprocess.run(
        [
            sys.executable,
            str(Path(__file__).with_name(oracle)),
            *([] if mode == "scale" else [mode]),
            str(binary),
            str(tmp_path / ("verilog_" + unit)),
            str(tmp_path / "oracle"),
        ],
        check=True,
    )


@pytest.mark.system
def test_float_to_integer_bits_native_and_rtl(tmp_path: Path) -> None:
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
            "float_to_integer_bits",
            "--replace",
        ],
        env=env,
        check=True,
    )
    source = tmp_path / "runner.cpp"
    source.write_text(
        "#include \"sources/functional_float/float_to_integer_bits.hpp\"\n#include <iostream>\nint main(){uint64_t value;unsigned wide,uns,width,round,sat;\nwhile(std::cin>>std::hex>>value>>std::dec>>wide>>uns>>width>>round>>sat){\nfunctional_float::float_to_integer_bits::FloatToIntegerBits dut(\"integer-bits\");\ndut.value=decltype(dut.value)::known(gfsim::Bits<64>{value});\ndut.wide64=decltype(dut.wide64)::known(gfsim::Bits<1>{wide});\ndut.unsigned_output=decltype(dut.unsigned_output)::known(gfsim::Bits<1>{uns});\ndut.width=decltype(dut.width)::known(gfsim::Bits<7>{width});\ndut.rounding=decltype(dut.rounding)::known(gfsim::Bits<3>{round});\ndut.saturating=decltype(dut.saturating)::known(gfsim::Bits<1>{sat});\ndut.request_valid=decltype(dut.request_valid)::known(gfsim::Bits<1>{1});dut.Work();\nif(!dut.result.isFullyKnown())return 2;const auto v=dut.result.value();\nstd::cout<<std::hex<<v.result.value()<<std::dec<<' '<<v.valid.toBool()<<' '<<v.invalid.toBool()<<' '<<v.overflow.toBool()<<' '<<v.underflow.toBool()<<' '<<v.inexact.toBool()<<' '<<v.zero.toBool()<<' '<<v.divide_by_zero.toBool()<<'\\n';}}\n"
    )
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    binary = tmp_path / "runner"
    build = subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / 'cpp_float_to_integer_bits'}",
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
            str(Path(__file__).with_name("functional_integer_bits_oracle.py")),
            str(binary),
            str(tmp_path / "verilog_float_to_integer_bits"),
            str(tmp_path / "oracle"),
        ],
        check=True,
    )


@pytest.mark.system
def test_finite_lowp_cast_native_and_rtl(tmp_path: Path) -> None:
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
            "finite_lowp_cast",
            "--replace",
        ],
        env=env,
        check=True,
    )
    source = tmp_path / "runner.cpp"
    source.write_text(
        "#include \"sources/functional_float/finite_lowp_cast.hpp\"\n#include <iostream>\nint main(){uint64_t value;unsigned kind,rhb;while(std::cin>>std::hex>>value>>std::dec>>kind>>rhb){\nfunctional_float::finite_lowp_cast::FiniteLowpCast dut(\"finite-lowp\");\ndut.value=decltype(dut.value)::known(gfsim::Bits<64>{value});dut.kind=decltype(dut.kind)::known(gfsim::Bits<2>{kind});\ndut.rhb=decltype(dut.rhb)::known(gfsim::Bits<1>{rhb});dut.request_valid=decltype(dut.request_valid)::known(gfsim::Bits<1>{1});dut.Work();\nif(!dut.result.isFullyKnown())return 2;const auto v=dut.result.value();std::cout<<std::hex<<v.result.value()<<std::dec<<' '<<v.valid.toBool()<<' '<<v.invalid.toBool()<<' '<<v.overflow.toBool()<<' '<<v.underflow.toBool()<<' '<<v.inexact.toBool()<<' '<<v.zero.toBool()<<' '<<v.divide_by_zero.toBool()<<'\\n';}}\n"
    )
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    binary = tmp_path / "runner"
    build = subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / 'cpp_finite_lowp_cast'}",
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
            str(Path(__file__).with_name("functional_finite_lowp_oracle.py")),
            str(binary),
            str(tmp_path / "verilog_finite_lowp_cast"),
            str(tmp_path / "oracle"),
        ],
        check=True,
    )


@pytest.mark.system
@pytest.mark.parametrize(
    "unit,top",
    [
        ("float_round_integral", "FloatRoundIntegral"),
        ("packed4_encode", "Packed4Encode"),
        ("packed4_decode", "Packed4Decode"),
    ],
)
def test_integral_and_packed_source_native_and_rtl(
    tmp_path: Path, unit: str, top: str
) -> None:
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
            unit,
            "--replace",
        ],
        env=env,
        check=True,
    )
    ports = json.loads((tmp_path / f"verilog_{unit}" / "ports.json").read_text())
    fields = ports["inputs"]
    reads = "".join(f">>values[{i}]" for i in range(len(fields)))
    assignments = "\n".join(
        f'dut.{p["name"]}=decltype(dut.{p["name"]})::known(gfsim::Bits<{p["width"]}>{{values[{i}]}});'
        for i, p in enumerate(fields)
    )
    source = tmp_path / "runner.cpp"
    source.write_text(
        f"""#include "sources/functional_float/{unit}.hpp"
#include <iostream>
int main(){{uint64_t values[{len(fields)}];while(std::cin>>std::hex{reads}){{
functional_float::{unit}::{top} dut("numeric");
{assignments}
dut.Work();if(!dut.result.isFullyKnown())return 2;const auto v=dut.result.value();
std::cout<<std::hex<<v.result.value()<<std::dec<<' '<<v.valid.toBool()<<' '<<v.invalid.toBool()<<' '<<v.overflow.toBool()<<' '<<v.underflow.toBool()<<' '<<v.inexact.toBool()<<' '<<v.zero.toBool()<<' '<<v.divide_by_zero.toBool()<<'\\n';}}}}
"""
    )
    runtime = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    binary = tmp_path / "runner"
    build = subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++20",
            "-O1",
            f"-I{runtime / 'include'}",
            f"-I{tmp_path / f'cpp_{unit}'}",
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
            str(Path(__file__).with_name("functional_round_packed_oracle.py")),
            unit,
            str(binary),
            str(tmp_path / f"verilog_{unit}"),
            str(tmp_path / "oracle"),
        ],
        check=True,
    )
