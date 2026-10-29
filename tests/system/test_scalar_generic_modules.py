"""Scalar fixed-Bits Modules specialize through one source-unit contract.

The tests keep static width binding separate from hardware data.  They exercise
root binding, same-source and imported calls, publication protection, and one
known/four-state execution oracle on both generated backends.
"""

from __future__ import annotations

import json
import os
from pathlib import Path

import pytest
from test_source_system_execution import (
    _bundle,
    _cli,
    _compile,
    _install,
    _ok,
    _records,
    _run,
)

pytestmark = pytest.mark.system


GENERIC = """\
import pycircuit as ac

@ac.module
def Generic(value: ac.bits[W], *, W: int = 5) -> ac.bits[W]:
    return value | 1

@ac.module
def Required(value: ac.bits[W], *, W: int) -> ac.bits[W]:
    return value ^ 1

@ac.module
def Mapping(value: ac.bits[W], *, W: int = 5) -> {"result": ac.bits[W]}:
    return {"result": value}
"""


def _write(root: Path, name: str, text: str) -> Path:
    path = root / name
    path.write_text(text, encoding="utf-8")
    return path


def _parameters(path: Path, **bindings: int) -> Path:
    path.write_text(
        json.dumps(
            [{"name": name, "value": str(value)} for name, value in bindings.items()]
        )
        + "\n",
        encoding="utf-8",
    )
    return path


def _link(units, top: str, output: Path, parameters: Path | None = None, replace=False):
    arguments = ["link", *(str(unit) for unit in units), "--top", top]
    if parameters:
        arguments += ["--parameters", str(parameters)]
    arguments += ["-o", str(output)]
    if replace:
        arguments.append("--replace")
    return _cli(*arguments)


def _compile_replace(source: Path, filename: str, output: Path, providers=()):
    arguments = [
        "compile",
        "-c",
        str(source / filename),
        "--source-root",
        str(source),
        "--package-prefix",
        "checks",
    ]
    for provider in providers:
        arguments += ["-I", str(provider)]
    return _cli(*arguments, "-o", str(output), "--replace")


def _rejected(result, *fragments: str) -> None:
    assert result.returncode != 0, result.stdout + result.stderr
    diagnostic = result.stdout + result.stderr
    assert any(fragment in diagnostic for fragment in fragments), diagnostic


def test_scalar_generic_root_default_required_override_and_specialized_type(
    tmp_path: Path,
) -> None:
    source = tmp_path / "source"
    source.mkdir()
    _write(source, "generic.py", GENERIC)
    unit = tmp_path / "unit"
    _ok(_compile(source, "generic.py", unit))

    default = tmp_path / "default.ac"
    _ok(_link((unit,), "checks.generic.Generic", default))
    default_text = default.read_text()
    assert "bits[5]" in default_text or "#ac.math_int<5>" in default_text

    missing = tmp_path / "missing.ac"
    _rejected(
        _link((unit,), "checks.generic.Required", missing),
        "missing",
        "required static parameter",
        "requires a default",
        "requires a value or default",
    )
    assert not missing.exists()

    # Nondefault and required bindings belong to an ordinary caller's static
    # callsite. The public linker continues to reject nonempty root-parameter
    # files; C11 does not introduce a second binding interface.
    caller_text = (
        GENERIC
        + """
@ac.module
def RequiredCaller(value: ac.bits[6]) -> ac.bits[6]:
    return Required(value, W=6)
@ac.module
def OverrideCaller(value: ac.bits[130]) -> ac.bits[130]:
    return Generic(value, W=130)
"""
    )
    _write(source, "caller.py", caller_text)
    caller = tmp_path / "caller"
    _ok(_compile(source, "caller.py", caller))
    for name, width in (("RequiredCaller", 6), ("OverrideCaller", 130)):
        final = tmp_path / (name + ".ac")
        _ok(_link((caller,), "checks.caller." + name, final))
        assert (
            f"#ac.math_int<{width}>" in final.read_text()
            or f"bits[{width}]" in final.read_text()
        )

    # Mapping returns remain a compatibility control while scalar results gain
    # specialization; this is not a second generic implementation route.
    mapping = tmp_path / "mapping.ac"
    _ok(_link((unit,), "checks.generic.Mapping", mapping))


def test_scalar_generic_root_required_and_override_bindings(tmp_path: Path) -> None:
    source = tmp_path / "source"
    source.mkdir()
    _write(source, "generic.py", GENERIC)
    unit = tmp_path / "unit"
    _ok(_compile(source, "generic.py", unit))

    required = tmp_path / "required.ac"
    _ok(
        _link(
            (unit,),
            "checks.generic.Required",
            required,
            _parameters(tmp_path / "required.json", W=6),
        )
    )
    assert (
        "#ac.math_int<6>" in required.read_text() or "bits[6]" in required.read_text()
    )

    override = tmp_path / "override.ac"
    _ok(
        _link(
            (unit,),
            "checks.generic.Generic",
            override,
            _parameters(tmp_path / "override.json", W=130),
        )
    )
    assert (
        "#ac.math_int<130>" in override.read_text()
        or "bits[130]" in override.read_text()
    )


def test_scalar_generic_future_signature_and_concat_result_specialize(
    tmp_path: Path,
) -> None:
    source = tmp_path / "source"
    source.mkdir()
    _write(
        source,
        "grow.py",
        """import pycircuit as ac
@ac.module
def Future(value: ac.bits[W + 1], *, W: int = 5) -> ac.bits[W + 1]:
    return value
@ac.module
def Double(value: ac.bits[W], *, W: int = 5) -> ac.bits[W + W]:
    return ac.concat(value, value)
@ac.module
def Top(value: ac.bits[6], pair: ac.bits[5]) -> tuple[ac.bits[6], ac.bits[10]]:
    return Future(value, W=5), Double(pair, W=5)
""",
    )
    unit = tmp_path / "unit"
    _ok(_compile(source, "grow.py", unit))
    final = tmp_path / "grow.ac"
    _ok(_link((unit,), "checks.grow.Top", final))
    text = final.read_text()
    assert "#ac.math_int<6>" in text or "bits[6]" in text
    assert "#ac.math_int<10>" in text or "bits[10]" in text


def test_invalid_root_parameter_bindings_preserve_final_artifact(
    tmp_path: Path,
) -> None:
    source = tmp_path / "source"
    source.mkdir()
    _write(source, "generic.py", GENERIC)
    unit = tmp_path / "unit"
    _ok(_compile(source, "generic.py", unit))
    final = tmp_path / "generic.ac"
    _ok(_link((unit,), "checks.generic.Generic", final))
    before = final.read_bytes()
    cases = {
        "zero": [{"name": "W", "value": "0"}],
        "negative": [{"name": "W", "value": "-1"}],
        "unknown": [{"name": "Missing", "value": "5"}],
        "duplicate": [
            {"name": "W", "value": "5"},
            {"name": "W", "value": "6"},
        ],
        "boolean": [{"name": "W", "value": True}],
    }
    for name, bindings in cases.items():
        path = tmp_path / (name + ".json")
        path.write_text(json.dumps(bindings) + "\n", encoding="utf-8")
        result = _link(
            (unit,),
            "checks.generic.Generic",
            final,
            parameters=path,
            replace=True,
        )
        _rejected(
            result,
            "width",
            "unknown",
            "duplicate",
            "integer",
            "parameter",
            "positive and fit u64",
        )
        assert final.read_bytes() == before


def test_root_parameter_json_schema_and_native_modes_fail_closed(
    tmp_path: Path,
) -> None:
    source = tmp_path / "source"
    source.mkdir()
    _write(source, "generic.py", GENERIC)
    unit = tmp_path / "unit"
    _ok(_compile(source, "generic.py", unit))
    final = tmp_path / "generic.ac"
    _ok(_link((unit,), "checks.generic.Generic", final))
    before = final.read_bytes()
    cases = {
        "escaped-duplicate-key": '[{"name":"W","\\u006eame":"V","value":"5"}]',
        "extra-field": '[{"name":"W","value":"5","extra":"x"}]',
        "number": '[{"name":"W","value":5}]',
        "boolean": '[{"name":"W","value":true}]',
        "negative-zero": '[{"name":"W","value":"-0"}]',
        "leading-zero": '[{"name":"W","value":"01"}]',
        "plus": '[{"name":"W","value":"+1"}]',
        "unicode": '[{"name":"W","value":"١"}]',
        "missing-name": '[{"value":"5"}]',
        "empty-name": '[{"name":"","value":"5"}]',
    }
    for name, text in cases.items():
        path = tmp_path / (name + ".json")
        path.write_text(text + "\n", encoding="utf-8")
        result = _link(
            (unit,),
            "checks.generic.Generic",
            final,
            parameters=path,
            replace=True,
        )
        _rejected(result, "JSON", "exactly", "canonical", "nonempty", "duplicate")
        assert path.read_text(encoding="utf-8") == text + "\n"
        assert final.read_bytes() == before

    manifest = json.loads((unit / "unit.json").read_text())
    body = unit / manifest["files"]["body"]
    header = unit / manifest["files"]["interface"]
    valid = _parameters(tmp_path / "valid.json", W=5)
    linker = str(Path(os.environ["PYCIRCUIT_LINKER"]))
    for command in (
        [linker, "--design", str(final), "--parameters", str(valid), "--verify-only"],
        [
            linker,
            "--body",
            str(body),
            "--header",
            str(header),
            "--parameters",
            str(valid),
            "--verify-only",
        ],
    ):
        result = _run(command)
        _rejected(result, "--parameters requires source-unit link mode")


def test_root_dependent_defaults_follow_named_overrides(tmp_path: Path) -> None:
    source = tmp_path / "source"
    source.mkdir()
    _write(
        source,
        "dependent.py",
        """import pycircuit as ac
@ac.module
def Dependent(value: ac.bits[V], *, W: int = 8, V: int = W + 1) -> ac.bits[V]:
    return value
""",
    )
    unit = tmp_path / "unit"
    _ok(_compile(source, "dependent.py", unit))
    cases = {
        "default": (None, (8,), True),
        "w5-v-default6": ({"W": 5}, (5,), True),
        "v9-w-default8": ({"V": 9}, (8, 9), False),
        "order-independent": ({"V": 9, "W": 8}, (8, 9), False),
    }
    for name, (bindings, values, dependent) in cases.items():
        parameters = (
            _parameters(tmp_path / (name + ".json"), **bindings) if bindings else None
        )
        final = tmp_path / (name + ".ac")
        _ok(_link((unit,), "checks.dependent.Dependent", final, parameters=parameters))
        entry = next(
            line for line in final.read_text().splitlines() if '"ac.system"' in line
        )
        for value in values:
            assert f"#ac.math_int<{value}>" in entry
        assert ('operator = "add"' in entry) == dependent


SAME_SOURCE = """\
import pycircuit as ac

@ac.module
def Generic(value: ac.bits[W], *, W: int = 5) -> ac.bits[W]:
    return value | 1

@ac.module
def Top(a1: ac.u1, a5: ac.u5, a65: ac.bits[65], a130: ac.bits[130]) -> \
        tuple[ac.u1, ac.u5, ac.bits[65], ac.bits[130]]:
    # Positional and named hardware actuals share the one explicit static lane.
    return (Generic(a1, W=1), Generic(value=a5, W=5),
            Generic(a65, W=65), Generic(value=a130, W=130))
"""


def _imported_consumer(provider_name="Generic", static_name="WIDTH", forward=False):
    width = "W" if forward else "65"
    parameter = ",\n        *, W: int = 65" if forward else ""
    return f"""\
import pycircuit as ac
from checks.provider import {provider_name}

@ac.module
def Top(a1: ac.u1, a5: ac.u5, a65: ac.bits[65], a130: ac.bits[130]{parameter}) -> \
        tuple[ac.u1, ac.u5, ac.bits[65], ac.bits[130]]:
    return ({provider_name}(a1, {static_name}=1),
            {provider_name}(value=a5, {static_name}=5),
            {provider_name}(a65, {static_name}={width}),
            {provider_name}(value=a130, {static_name}=130))
"""


def test_same_source_and_imported_calls_specialize_each_result(tmp_path: Path) -> None:
    source = tmp_path / "source"
    source.mkdir()
    _write(source, "same.py", SAME_SOURCE)
    same = tmp_path / "same"
    _ok(_compile(source, "same.py", same))
    same_final = tmp_path / "same.ac"
    _ok(_link((same,), "checks.same.Top", same_final))

    provider_text = """import pycircuit as ac
@ac.module
def Provider(value: ac.bits[WIDTH], *, WIDTH: int = 5) -> ac.bits[WIDTH]:
    return value | 1
"""
    _write(source, "provider.py", provider_text)
    _write(source, "consumer.py", _imported_consumer("Provider", "WIDTH"))
    provider, consumer = tmp_path / "provider", tmp_path / "consumer"
    _ok(_compile(source, "provider.py", provider))
    _ok(_compile(source, "consumer.py", consumer, [provider]))
    imported = tmp_path / "imported.ac"
    _ok(_link((provider, consumer), "checks.consumer.Top", imported))

    for final in (same_final, imported):
        text = final.read_text()
        for width in (1, 5, 65, 130):
            assert f"#ac.math_int<{width}>" in text or f"bits[{width}]" in text


def test_imported_caller_forwards_differently_named_static_parameter(
    tmp_path: Path,
) -> None:
    source = tmp_path / "source"
    source.mkdir()
    _write(
        source,
        "provider.py",
        """import pycircuit as ac
@ac.module
def Provider(value: ac.bits[WIDTH], *, WIDTH: int = 5) -> ac.bits[WIDTH]:
    return value | 1
""",
    )
    _write(
        source,
        "consumer.py",
        """import pycircuit as ac
from checks.provider import Provider
@ac.module
def Top(value: ac.bits[W], *, W: int = 65) -> ac.bits[W]:
    return Provider(value, WIDTH=W)
""",
    )
    provider, consumer = tmp_path / "provider", tmp_path / "consumer"
    _ok(_compile(source, "provider.py", provider))
    _ok(_compile(source, "consumer.py", consumer, [provider]))
    final = tmp_path / "forwarded.ac"
    _ok(_link((provider, consumer), "checks.consumer.Top", final))
    assert "#ac.math_int<65>" in final.read_text() or "bits[65]" in final.read_text()


NEGATIVES = {
    "negative-width": """import pycircuit as ac
@ac.module
def Top(value: ac.bits[W], *, W: int = -1) -> ac.bits[W]: return value
""",
    "noninteger-static": """import pycircuit as ac
@ac.module
def Top(value: ac.bits[W], *, W: bool = False) -> ac.bits[W]: return value
""",
}


@pytest.mark.parametrize("case", sorted(NEGATIVES))
def test_invalid_scalar_generic_declarations_preserve_publication(
    tmp_path: Path, case: str
) -> None:
    source = tmp_path / "source"
    source.mkdir()
    path = _write(source, "generic.py", GENERIC)
    unit = tmp_path / "unit"
    _ok(_compile(source, "generic.py", unit))
    before = {
        entry.name: entry.read_bytes() for entry in unit.iterdir() if entry.is_file()
    }

    path.write_text(NEGATIVES[case], encoding="utf-8")
    result = _compile_replace(source, "generic.py", unit)
    _rejected(
        result,
        "width",
        "static parameter",
        "constant does not fit",
        "Boolean",
        "unsupported static expression",
    )
    assert before == {
        entry.name: entry.read_bytes() for entry in unit.iterdir() if entry.is_file()
    }


@pytest.mark.parametrize(
    "case,text",
    [
        (
            "zero-width",
            """import pycircuit as ac
@ac.module
def Top(value: ac.bits[W], *, W: int = 0) -> ac.bits[W]: return value
""",
        ),
        (
            "overflow-constant",
            """import pycircuit as ac
@ac.module
def Top(value: ac.bits[W], *, W: int = 5) -> ac.bits[W]: return value | 32
""",
        ),
    ],
)
def test_invalid_default_specialization_preserves_final_artifact(
    tmp_path: Path, case: str, text: str
) -> None:
    source = tmp_path / "source"
    source.mkdir()
    _write(source, "valid.py", GENERIC)
    valid = tmp_path / "valid"
    _ok(_compile(source, "valid.py", valid))
    final = tmp_path / "final.ac"
    _ok(_link((valid,), "checks.valid.Generic", final))
    before = final.read_bytes()

    _write(source, "invalid.py", text)
    invalid = tmp_path / "invalid"
    _ok(_compile(source, "invalid.py", invalid))
    result = _link((invalid,), "checks.invalid.Top", final, replace=True)
    _rejected(result, "width", "constant does not fit", "positive")
    assert final.read_bytes() == before


def test_invalid_scalar_generic_calls_fail_closed(tmp_path: Path) -> None:
    source = tmp_path / "source"
    source.mkdir()
    _write(source, "provider.py", GENERIC)
    provider = tmp_path / "provider"
    _ok(_compile(source, "provider.py", provider))
    cases = {
        "width-mismatch": "return Generic(value, W=5)",
        "unknown-static": "return Generic(value, Missing=6)",
        "duplicate-static": "return Generic(value, W=6, **{'W': 6})",
        "missing-static": "return Required(value)",
        "runtime-static": "return Required(value, W=runtime)",
        "runtime-boolean-static": "return Required(value, W=runtime)",
    }
    for name, statement in cases.items():
        extra = (
            ", runtime: bool"
            if name == "runtime-boolean-static"
            else ", runtime: ac.u6" if name == "runtime-static" else ""
        )
        width = 6 if name != "width-mismatch" else 6
        consumer_text = f"""import pycircuit as ac
from checks.provider import Generic, Required
@ac.module
def Top(value: ac.bits[{width}]{extra}) -> ac.bits[{width}]:
    {statement}
"""
        _write(source, "consumer.py", consumer_text)
        result = _compile(
            source, "consumer.py", tmp_path / ("invalid-" + name), [provider]
        )
        _rejected(
            result,
            "static parameter",
            "unknown keyword",
            "duplicate",
            "requires",
            "hardware type mismatch",
            "implicit narrowing",
            "unknown or repeated instance parameter",
            "missing required instance parameter",
            "unresolved hardware static expression",
        )


def test_imported_generic_header_tampering_is_rejected(tmp_path: Path) -> None:
    source = tmp_path / "source"
    source.mkdir()
    _write(source, "provider.py", GENERIC)
    _write(source, "consumer.py", _imported_consumer())
    provider = tmp_path / "provider"
    _ok(_compile(source, "provider.py", provider))
    manifest = json.loads((provider / "unit.json").read_text())
    header = provider / manifest["files"]["interface"]
    original = header.read_text()
    assert "W" in original
    header.write_text(original.replace("W", "BROKEN_W", 1), encoding="utf-8")
    result = _compile(source, "consumer.py", tmp_path / "consumer", [provider])
    _rejected(result, "header", "owner", "interface", "parameter", "static reference")


EXECUTABLE = """\
import pycircuit as ac
@ac.module
def Generic(value: ac.bits[W], *, W: int = 5) -> ac.bits[W]:
    return value | 1
@ac.module
def Root(value: ac.bits[130]) -> ac.bits[130]:
    return Generic(value, W=130)
"""


def test_scalar_generic_root_executes_known_and_four_state_cpp_rtl(
    tmp_path: Path,
) -> None:
    source = tmp_path / "source"
    source.mkdir()
    _write(source, "root.py", EXECUTABLE)
    unit = tmp_path / "unit"
    _ok(_compile(source, "root.py", unit))
    final = tmp_path / "root.ac"
    _ok(_link((unit,), "checks.root.Root", final))

    generated_cpp = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(generated_cpp)))
    (generated_cpp / "scalar_generic_probe.cpp").write_text(
        r"""#include "pycircuit_system.hpp"
#include <iostream>
using Wire=gfsim::wire<gfsim::Bits<130>>;
int main(){for(unsigned workers:{1u,2u}){pyc_dut dut(workers);dut.system().Build();for(unsigned mode=0;mode<3;++mode){gfsim::Bits<130> value{0},known{0},z{0};for(unsigned b=0;b<130;++b){if((b+mode)%3==0)value.setWord(b/64,value.word(b/64)|(std::uint64_t{1}<<(b%64)));if(mode==0||b!=64)known.setWord(b/64,known.word(b/64)|(std::uint64_t{1}<<(b%64)));if(mode==2&&b==64)z.setWord(1,1);}Wire input=Wire::fromPacked(gfsim::FourState<130>::fromMasks(value,known,z));pyc_dut::Inputs ports;ports.value=input;dut.drive(ports);dut.system().Reset();if(dut.system().Step()!=gfsim::SimStepResult::Running)return 2;auto out=dut.sample().result.packed();for(unsigned b=0;b<130;++b){if(b==0){if(!out.knownMask().bit(0)||!out.value().bit(0)||out.zMask().bit(0))return 3;}else if(out.knownMask().bit(b)!=known.bit(b)||out.zMask().bit(b)||out.knownMask().bit(b)&&out.value().bit(b)!=value.bit(b))return 4;}}}std::cout<<"SCALAR_GENERIC_CPP_OK\n";}
""",
        encoding="utf-8",
    )
    with (generated_cpp / "CMakeLists.txt").open("a") as cmake:
        cmake.write(
            "\nadd_executable(scalar_generic_probe scalar_generic_probe.cpp)\n"
            "target_link_libraries(scalar_generic_probe PRIVATE pycircuit_modules)\n"
        )
    cpp_build = tmp_path / "cpp-build"
    _ok(
        _run(
            [
                "cmake",
                "-S",
                str(generated_cpp),
                "-B",
                str(cpp_build),
                "-G",
                "Ninja",
                f"-Dpycircuit_DIR={_install() / 'share/pycircuit/cmake'}",
            ]
        )
    )
    _ok(_run(["cmake", "--build", str(cpp_build), "--target", "scalar_generic_probe"]))
    executable = next(cpp_build.rglob("scalar_generic_probe"))
    assert "SCALAR_GENERIC_CPP_OK" in _ok(_run([str(executable)]))

    generated_rtl = tmp_path / "rtl"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(generated_rtl)))
    bench = tmp_path / "tb.sv"
    bench.write_text(
        """module tb;logic [129:0] value;wire [129:0] result;pyc_root dut(.*);
initial begin value='0;#1;if(result!==130'b1)$fatal;value='x;#1;
if(result[0]!==1'b1||result[129:1]!==129'bx)$fatal;value='z;#1;
if(result[0]!==1'b1||result[129:1]!==129'bx)$fatal;
$display("SCALAR_GENERIC_RTL_OK");$finish;end endmodule
""",
        encoding="utf-8",
    )
    manifest = json.loads((generated_rtl / "generated.json").read_text())
    rtl = [
        generated_rtl / row["path"] for row in manifest["files"] if row["role"] == "rtl"
    ]
    binary = tmp_path / "scalar-generic.vvp"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "tb",
                "-o",
                str(binary),
                *map(str, rtl),
                str(bench),
            ]
        )
    )
    assert "SCALAR_GENERIC_RTL_OK" in _ok(_run(["vvp", str(binary)]))


METHOD_PROVIDER = """import pycircuit as ac
@ac.module
def Exchange(*, W: int):
    @ac.method
    def flip(value: ac.bits[W]) -> ac.bits[W]:
        return value ^ 1
@ac.module
def Reader(value: ac.bits[W], *, W: int = 7) -> ac.bits[W]:
    @ac.method
    def read() -> ac.bits[W]:
        return value
    return value
"""


@pytest.mark.parametrize("imported", [False, True], ids=["same-source", "imported"])
def test_method_generic_receivers_bind_each_instance(tmp_path: Path, imported: bool):
    source = tmp_path / "source"
    source.mkdir()
    units = []
    prefix = METHOD_PROVIDER
    if imported:
        provider_source = _write(source, "provider.py", METHOD_PROVIDER)
        provider = tmp_path / "provider"
        _ok(_compile(source, "provider.py", provider))
        provider_source.rename(tmp_path / "unavailable-provider.py")
        receipt = json.loads((provider / "unit.json").read_text())
        body = provider / receipt["files"]["body"]
        hidden = tmp_path / "unavailable-provider.ac"
        body.rename(hidden)
        units.append(provider)
        prefix = (
            "import pycircuit as ac\nfrom checks.provider import Exchange, Reader\n"
        )
    _write(
        source,
        "consumer.py",
        prefix
        + """
@ac.module
def Top(*, W: int = 7) -> tuple[ac.bits[W], ac.bits[69]]:
    a: ac.bits[W] = 1
    b: ac.bits[69] = 2
    small = Exchange(W=W)
    wide = Exchange(W=69)
    reader = Reader(b, W=69)
    @ac.rule
    def check() -> None:
        small_result: ac.bits[W] = small.flip(a)
        wide_result: ac.bits[69] = wide.flip(b)
        observed: ac.bits[69] = reader.read()
        assert small_result == (a ^ 1)
        assert wide_result == (b ^ 1)
        assert reader == b
        assert observed == b
    check()
    return a, b
""",
    )
    consumer = tmp_path / "consumer"
    try:
        _ok(_compile(source, "consumer.py", consumer, units))
    finally:
        if imported:
            hidden.rename(body)
    units.append(consumer)
    for width in (7, 11):
        final = tmp_path / f"width-{width}.ac"
        parameters = (
            _parameters(tmp_path / "parameters.json", W=width) if width != 7 else None
        )
        _ok(_link(units, "checks.consumer.Top", final, parameters))
        for target in ("cpp", "verilog"):
            output = tmp_path / f"width-{width}-{target}"
            _ok(_cli("emit", str(final), "--target", target, "-o", str(output)))
            ports = json.loads((output / "ports.json").read_text())
            # Closed root owns the resources; tuple exports preserve the two
            # independent interfaces without recreating legacy value inputs.
            assert [port["width"] for port in ports["outputs"]] == [width, 69]


def test_method_generic_closed_expression_matches_scalar_binding(tmp_path: Path):
    source = tmp_path / "source"
    source.mkdir()
    cases = {
        "scalar": """import pycircuit as ac
@ac.module
def Flip(value: ac.bits[W], *, W: int) -> ac.bits[W]:
    @ac.rule
    def flip() -> ac.bits[W]:
        return value ^ 1
    result = flip()
    return result
@ac.system
def Top():
    value: ac.bits[7 + 1] = 42
    flipped = Flip(value, W=7 + 1)
    @ac.rule
    def check(result: ac.bits[7 + 1]) -> None:
        assert result == 43
        assert result + 213 == 0
        ac.log("info", "closed expression", result, result + 213)
    check(flipped)
""",
        "method": METHOD_PROVIDER
        + """
@ac.system
def Top():
    value: ac.bits[7 + 1] = 42
    owner = Exchange(W=7 + 1)
    @ac.rule
    def check() -> None:
        result: ac.bits[7 + 1] = owner.flip(value)
        assert result == 43
        assert result + 213 == 0
        ac.log("info", "closed expression", result, result + 213)
    check()
""",
    }
    for name, text in cases.items():
        _write(source, name + ".py", text)
        unit = tmp_path / (name + "-unit")
        _ok(_compile(source, name + ".py", unit))
        final = tmp_path / (name + ".ac")
        _ok(_link([unit], "checks." + name + ".Top", final))
        for target in ("cpp", "verilog"):
            root = tmp_path / (name + "-" + target)
            root.mkdir()
            binary = _bundle(final, root, target)
            arguments = (
                ["--cycles", "3", "--workers", "1"]
                if target == "cpp"
                else ["+cycles=3"]
            )
            records = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            logs = [row for row in records if row["kind"] == "log"]
            # Queue production has no empty bypass; the direct Method has no
            # Queue stage. Both retain eight-bit arithmetic: 43 + 213 wraps.
            epochs = list(range(2 if name == "scalar" else 0, 6))
            assert [row["spec"]["event"] for row in logs] == [
                "closed expression"
            ] * len(epochs)
            assert [
                tuple(int(value["value"]) for value in row["values"]) for row in logs
            ] == [(43, 0)] * len(epochs)
            assert [int(row["evaluation_epoch"]) for row in logs] == epochs
            assert [int(row["commit_epoch"]) for row in logs] == [
                epoch + 1 for epoch in epochs
            ]
            terminal = [row for row in records if row["kind"] == "result"]
            assert len(terminal) == 1 and terminal[0]["status"] == "TERMINATED"


def test_method_generic_uncalled_body_checks_actuals_before_replacing_output(tmp_path):
    source = tmp_path / "source"
    source.mkdir()
    owner_source = _write(
        source,
        "owner.py",
        """import pycircuit as ac
@ac.module
def Owner(*, W: int = 7):
    @ac.method
    def unused(value: ac.bits[W]) -> ac.bits[W]:
        return value | 64
""",
    )
    unit = tmp_path / "unit"
    _ok(_compile(source, "owner.py", unit))
    final = tmp_path / "final.ac"
    _ok(_link([unit], "checks.owner.Owner", final))
    original = final.read_bytes()
    result = _link(
        [unit],
        "checks.owner.Owner",
        final,
        _parameters(tmp_path / "parameters.json", W=6),
        replace=True,
    )
    assert result.returncode != 0, "64 must not fit a six-bit Method operand"
    assert final.read_bytes() == original

    # Unselected concrete definitions are retained too. Their instances must
    # satisfy the same bound Method contract as the selected entry hierarchy.
    _write(
        source,
        "retained.py",
        owner_source.read_text()
        + """
@ac.module
def Wrapper() -> None:
    child = Owner(W=6)
@ac.module
def Top() -> ac.u7:
    value: ac.u7 = 0
    return value
""",
    )
    retained = tmp_path / "retained"
    _ok(_compile(source, "retained.py", retained))
    result = _link([retained], "checks.retained.Top", final, replace=True)
    assert result.returncode != 0, "retained instances need actual-bound validation"
    assert final.read_bytes() == original


def test_method_generic_system_preserves_state_and_four_state_payloads(tmp_path):
    source = tmp_path / "source"
    source.mkdir()
    _write(
        source,
        "counter.py",
        """import pycircuit as ac
@ac.module
def Counter(*, W: int = 7):
    count: ac.bits[W] = 0
    @ac.method
    def exchange(value: ac.bits[W]) -> tuple[ac.bits[W], ac.bits[W]]:
        return value, count
    @ac.method
    def advance(delta: ac.bits[W]) -> ac.bits[W]:
        proposed = count + delta
        count = proposed
        return proposed
    @ac.method
    def read() -> ac.bits[W]:
        return count
""",
    )
    small_pattern = "10xz010"
    wide_pattern = "1" + "0" * 64 + "xz01"
    _write(
        source,
        "bench.py",
        f"""import pycircuit as ac
from checks.counter import Counter
@ac.system
def Exercise():
    small = Counter(W=7)
    wide = Counter(W=69)
    @ac.rule
    def update() -> None:
        old_small: ac.u7 = small.read()
        old_wide: ac.bits[69] = wide.read()
        a, first = small.exchange(ac.four_state({small_pattern!r}))
        b, large = wide.exchange(ac.four_state({wide_pattern!r}))
        c, second = small.exchange(ac.four_state('1100101'))
        next_small: ac.u7 = small.advance(2)
        next_wide: ac.bits[69] = wide.advance(1)
        after_small: ac.u7 = small.read()
        after_wide: ac.bits[69] = wide.read()
        assert ac.case_eq(a, ac.four_state({small_pattern!r}))
        assert ac.case_eq(b, ac.four_state({wide_pattern!r}))
        assert ac.case_eq(c, ac.four_state('1100101'))
        ac.log("info", "states", old_small, old_wide, first, second, large,
               after_small, after_wide, next_small, next_wide)
    update()
""",
    )
    provider, bench = tmp_path / "provider", tmp_path / "bench"
    _ok(_compile(source, "counter.py", provider))
    (source / "counter.py").rename(tmp_path / "unavailable-counter-source.py")
    _ok(_compile(source, "bench.py", bench, [provider]))
    final = tmp_path / "system.ac"
    _ok(_link([provider, bench], "checks.bench.Exercise", final))
    original = final.read_bytes()
    # Retire candidate forwarding: two exchange calls preserve independent
    # payloads and old-Q snapshots. One write advances small by two and wide by
    # one; all state reads remain old Q, while explicit results carry next SSA.
    expected = [
        (
            2 * cycle,
            cycle,
            2 * cycle,
            2 * cycle,
            cycle,
            2 * cycle,
            cycle,
            2 * cycle + 2,
            cycle + 1,
        )
        for cycle in range(3)
        for _ in range(2)
    ]
    transcripts = []
    for target in ("cpp", "verilog"):
        directory = tmp_path / ("system-" + target)
        directory.mkdir()
        binary = _bundle(final, directory, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            args = (
                ["--cycles", "3", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=3"]
            )
            rows = _records(_ok(_run([str(binary), *args], timeout=30)))
            logs = [row for row in rows if row["kind"] == "log"]
            assert [row["spec"]["event"] for row in logs] == ["states"] * 6
            assert [
                tuple(int(v["value"]) for v in row["values"]) for row in logs
            ] == expected
            assert [int(row["evaluation_epoch"]) for row in logs] == list(range(6))
            terminal = [row for row in rows if row["kind"] == "result"]
            assert len(terminal) == 1 and terminal[0]["status"] == "TERMINATED"
            transcripts.append([row for row in rows if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert final.read_bytes() == original
