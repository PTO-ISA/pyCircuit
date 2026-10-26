"""Nominal Boolean fields retain logical authority and four-state transport.

These are small compiler conformance designs, not consumer ISA fixtures. The
oracles below follow the source values and never read generated DUT expressions.
"""

from __future__ import annotations

import json
import re
from pathlib import Path

import pytest
import test_cpp_source_parts as native
from test_source_system_execution import (
    _bundle,
    _cli,
    _compile,
    _install,
    _observations,
    _ok,
    _records,
    _run,
)

pytestmark = pytest.mark.system

TYPES = """import pycircuit as ac
@ac.struct
class Leaf:
    flag: bool = False
    payload: ac.u8 = 7
@ac.struct
class Envelope:
    inner: Leaf
    fixed: ac.u1 = 1
"""

HELPERS = """
def boolean_identity(value: bool) -> bool:
    return value

def leaf_identity(value: Leaf) -> Leaf:
    return value

def envelope_identity(value: Envelope) -> Envelope:
    return value

def rewrite_leaf(value: Leaf, flag: bool) -> Leaf:
    value.flag = flag
    return value
"""

SYSTEM = """
@ac.system
def Exercise():
    count: ac.u8 = 0
    @ac.rule
    def exercise():
        item = Envelope(inner=Leaf(flag=count == 1, payload=count), fixed=1)
        first = envelope_identity(item)
        second = leaf_identity(first.inner)
        answer = boolean_identity(second.flag)
        item.inner.payload = count + 5
        preserved = boolean_identity(item.inner.flag)
        if count == 0:
            selected = Leaf(flag=True, payload=3)
        else:
            selected = Leaf(flag=False, payload=4)
        joined = boolean_identity(selected.flag)
        selected.flag = boolean_identity(answer)
        selected = rewrite_leaf(selected, answer)
        ac.log("info", "dynamic", answer)
        ac.log("info", "preserved", preserved)
        ac.log("info", "joined", joined)
        ac.log("info", "updated", selected.flag)
        ac.log("info", "payload", item.inner.payload)
        ac.log("info", "fixed", item.fixed)
        unknown = Leaf(flag=ac.four_state('z') == 0, payload=8)
        ac.log("info", "unknown", boolean_identity(unknown.flag))
        count = count + 1
"""


def _write(source: Path, name: str, text: str) -> None:
    source.mkdir(parents=True, exist_ok=True)
    (source / name).write_text(text, encoding="utf-8")


def _link(units: list[Path], output: Path, top: str = "checks.bench.Exercise"):
    return _cli("link", *map(str, units), "--top", top, "-o", str(output))


def _snapshot(directory: Path) -> dict[str, bytes]:
    return {
        str(p.relative_to(directory)): p.read_bytes()
        for p in directory.rglob("*")
        if p.is_file()
    }


def _replace(source: Path, unit: Path, providers=()):
    arguments = [
        "compile",
        "-c",
        str(source / "bench.py"),
        "--source-root",
        str(source),
        "--package-prefix",
        "checks",
        "-o",
        str(unit),
        "--replace",
    ]
    for provider in providers:
        arguments.extend(["-I", str(provider)])
    return _cli(*arguments)


def _execute(
    final: Path,
    directory: Path,
    expected: dict[str, list[int]],
    *,
    expect_unknown: bool = False,
) -> None:
    traces = []
    for target in ("cpp", "verilog"):
        binary = _bundle(final, directory, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "3", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=3"]
            )
            rows = _records(_ok(_run([str(binary), *arguments], timeout=20)))
            for event, values in expected.items():
                assert _observations(rows, "log", event) == [
                    (epoch, epoch + 1, value) for epoch, value in enumerate(values)
                ]
            unknown = [
                row
                for row in rows
                if row["kind"] == "log" and row["spec"]["event"] == "unknown"
            ]
            if expect_unknown:
                assert len(unknown) == 6
                assert all(
                    row["values"]
                    == [
                        {
                            "kind": "bits4",
                            "width": 1,
                            "value": "0",
                            "known": "0",
                            "z": "0",
                        }
                    ]
                    for row in unknown
                )
            traces.append([row for row in rows if row["kind"] == "log"])
    assert traces[0] == traces[1] == traces[2]


@pytest.mark.parametrize("spelling", ["local", "imported", "reexported"])
def test_boolean_fields_nested_helpers_and_branch_execution(tmp_path: Path, spelling):
    source = tmp_path / "source"
    units = []
    if spelling == "local":
        prefix = TYPES
    else:
        _write(source, "provider.py", TYPES + HELPERS)
        provider = tmp_path / "provider"
        _ok(_compile(source, "provider.py", provider))
        units.append(provider)
        (source / "provider.py").unlink()
        receipt = json.loads((provider / "unit.json").read_text())
        body = provider / receipt["files"]["body"]
        saved_body = body.read_bytes()
        body.unlink()
        try:
            if spelling == "reexported":
                _write(
                    source,
                    "facade.py",
                    "from checks.provider import Leaf, Envelope, "
                    "boolean_identity, leaf_identity, envelope_identity, rewrite_leaf\n",
                )
                facade = tmp_path / "facade"
                _ok(_compile(source, "facade.py", facade, units))
                units.append(facade)
                (source / "facade.py").unlink()
            owner = "facade" if spelling == "reexported" else "provider"
            prefix = (
                f"import pycircuit as ac\nfrom checks.{owner} import Leaf, Envelope, "
                "boolean_identity, leaf_identity, envelope_identity, rewrite_leaf\n"
            )
            _write(source, "bench.py", prefix + SYSTEM)
            bench = tmp_path / "bench"
            _ok(_compile(source, "bench.py", bench, units))
        finally:
            body.write_bytes(saved_body)
        assert not (source / "provider.py").exists()
    if spelling == "local":
        _write(source, "bench.py", prefix + HELPERS + SYSTEM)
        bench = tmp_path / "bench"
        _ok(_compile(source, "bench.py", bench))
    units.append(bench)
    final = tmp_path / "boolean.ac"
    _ok(_link(units, final))
    _execute(
        final,
        tmp_path,
        {
            "dynamic": [0, 0, 1, 1, 0, 0],
            "preserved": [0, 0, 1, 1, 0, 0],
            "joined": [1, 1, 0, 0, 0, 0],
            "updated": [0, 0, 1, 1, 0, 0],
            "payload": [5, 5, 6, 6, 7, 7],
            "fixed": [1] * 6,
        },
        expect_unknown=True,
    )


def test_local_boolean_defaults_and_contextual_zero_execute(tmp_path: Path):
    text = (
        TYPES
        + HELPERS
        + """
@ac.system
def Exercise():
    @ac.rule
    def exercise():
        omitted = Leaf(payload=9)
        zero: Envelope = 0
        outer_default = Envelope()
        true_value = Leaf(flag=True, payload=2)
        false_value = Leaf(flag=False, payload=3)
        ac.log("info", "omitted", boolean_identity(omitted.flag))
        ac.log("info", "zero", boolean_identity(zero.inner.flag))
        ac.log("info", "nested_default", boolean_identity(outer_default.inner.flag))
        ac.log("info", "true", boolean_identity(true_value.flag))
        ac.log("info", "false", boolean_identity(false_value.flag))
"""
    )
    source, unit = tmp_path / "source", tmp_path / "unit"
    _write(source, "bench.py", text)
    _ok(_compile(source, "bench.py", unit))
    final = tmp_path / "defaults.ac"
    _ok(_link([unit], final))
    _execute(
        final,
        tmp_path,
        {
            name: [value] * 6
            for name, value in (
                ("omitted", 0),
                ("zero", 0),
                ("nested_default", 0),
                ("true", 1),
                ("false", 0),
            )
        },
    )


@pytest.mark.parametrize("expression", ["0", "1", "bits", "ac.four_state('z')"])
@pytest.mark.parametrize(
    "binding",
    [
        "constructor",
        "local-update",
        "persistent-update",
        "local-table-update",
        "persistent-table-update",
        "helper-update",
        "helper-table-update",
    ],
)
def test_integer_or_fixed_bits_cannot_bind_boolean_field(
    tmp_path: Path, expression, binding
):
    source, unit = tmp_path / "source", tmp_path / "unit"
    if binding in ("constructor", "local-update"):
        body = """    item = Leaf(flag=flag, payload=4)
    item.flag = flag
    return item.flag
"""
    elif binding == "persistent-update":
        body = """    item: Leaf = Leaf(flag=False, payload=4)
    @ac.rule
    def update():
        item.flag = flag
    return item.flag
"""
    elif binding == "local-table-update":
        body = """    @ac.rule
    def update() -> bool:
        items: ac.table[2, Leaf] = (Leaf(flag=False, payload=4), Leaf(flag=True, payload=5))
        items[0].flag = flag
        return items[0].flag
    return update
"""
    elif binding == "persistent-table-update":
        body = """    items: ac.table[2, Leaf] = (Leaf(flag=False, payload=4), Leaf(flag=True, payload=5))
    @ac.rule
    def update():
        items[0].flag = flag
    return items[0].flag
"""
    elif binding == "helper-update":
        body = """    return rewrite(Leaf(flag=False, payload=4), flag, bits).flag
"""
    else:
        body = """    @ac.rule
    def transform() -> bool:
        items: ac.table[2, Leaf] = (Leaf(flag=False, payload=4), Leaf(flag=True, payload=5))
        changed = rewrite(items, flag, bits)
        return changed[0].flag
    return transform
"""
    helper = ""
    if binding == "helper-update":
        helper = """
def rewrite(item: Leaf, flag: bool, bits: ac.u1) -> Leaf:
    item.flag = flag
    return item
"""
    elif binding == "helper-table-update":
        helper = """
def rewrite(items: ac.table[2, Leaf], flag: bool, bits: ac.u1) -> ac.table[2, Leaf]:
    items[0].flag = flag
    return items
"""
    good = (
        TYPES
        + helper
        + "\n@ac.module\ndef Top(flag: bool, bits: ac.u1) -> bool:\n"
        + body
    )
    _write(source, "bench.py", good)
    _ok(_compile(source, "bench.py", unit))
    before = _snapshot(unit)
    bad = (
        good.replace("Leaf(flag=flag", f"Leaf(flag={expression}")
        if binding == "constructor"
        else good.replace(".flag = flag", f".flag = {expression}")
    )
    _write(source, "bench.py", bad)
    rejected = _replace(source, unit)
    assert rejected.returncode != 0, rejected.stdout
    assert re.search(
        r"bool|kind|domain|binding", rejected.stderr, re.I
    ), rejected.stderr
    assert _snapshot(unit) == before


@pytest.mark.parametrize("default", ["0", "1", "ac.four_state('0')"])
@pytest.mark.parametrize("use", ["unused", "overridden"])
def test_invalid_boolean_defaults_reject_even_unused_or_overridden(
    tmp_path: Path, default, use
):
    source, unit = tmp_path / "source", tmp_path / "unit"
    tail = (
        "    return flag\n"
        if use == "unused"
        else "    return Leaf(flag=True, payload=2).flag\n"
    )
    good = TYPES + "\n@ac.module\ndef Top(flag: bool) -> bool:\n" + tail
    _write(source, "bench.py", good)
    _ok(_compile(source, "bench.py", unit))
    before = _snapshot(unit)
    _write(
        source,
        "bench.py",
        good.replace("flag: bool = False", f"flag: bool = {default}"),
    )
    rejected = _replace(source, unit)
    assert rejected.returncode != 0, rejected.stdout
    assert re.search(
        r"bool|kind|domain|default|binding", rejected.stderr, re.I
    ), rejected.stderr
    assert _snapshot(unit) == before


def test_imported_boolean_defaults_still_require_explicit_fields(tmp_path: Path):
    source = tmp_path / "source"
    _write(source, "provider.py", TYPES)
    provider, unit = tmp_path / "provider", tmp_path / "unit"
    _ok(_compile(source, "provider.py", provider))
    original = """import pycircuit as ac
from checks.provider import Leaf
@ac.module
def Top(flag: bool) -> bool:
    item = Leaf(flag=flag, payload=3)
    return item.flag
"""
    _write(source, "bench.py", original)
    _ok(_compile(source, "bench.py", unit, [provider]))
    before = _snapshot(unit)
    _write(
        source,
        "bench.py",
        original.replace("Leaf(flag=flag, payload=3)", "Leaf(payload=3)"),
    )
    rejected = _replace(source, unit, [provider])
    assert rejected.returncode != 0
    assert "field" in rejected.stderr.lower(), rejected.stderr
    assert _snapshot(unit) == before


def _field_constraint(text: str) -> str:
    # Select the field dictionary, not the scalar ports' unrelated constraints.
    match = re.search(r'name = "flag", source_constraint = ', text)
    assert match, text
    start = match.end()
    assert text[start] == "{"
    depth = 0
    for index in range(start, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start : index + 1]
    raise AssertionError("unterminated nominal field constraint")


def _native_unit(directory: Path):
    source = directory / "source"
    _write(
        source,
        "bench.py",
        TYPES
        + """
@ac.module
def Top(flag: bool) -> bool:
    item = Leaf(flag=flag, payload=9)
    item.payload = 4
    return item.flag
""",
    )
    return native._compile_source(
        source / "bench.py",
        source_root=source,
        output_dir=directory / "unit",
        package="checks",
    )


@pytest.mark.parametrize(
    "mutation",
    [
        "missing-domain",
        "wrong-domain",
        "wrong-width",
        "wrong-type",
        "unsupported-kind",
        "unsupported-source-kind",
        "extra-key",
        "scalar-marker",
    ],
)
def test_boolean_field_descriptor_rejects_malformed_authority(tmp_path: Path, mutation):
    unit = _native_unit(tmp_path)
    original = unit.body.read_text()
    constraint = _field_constraint(original)
    assert 'source_kind = "boolean"' in constraint
    assert 'kind = "hardware"' in constraint
    assert '#ac.source_domain<{kind = "bool"}>' in constraint
    replacements = {
        "missing-domain": re.sub(
            r'domain = #ac.source_domain<\{kind = "bool"\}>, ', "", constraint
        ),
        "wrong-domain": constraint.replace('kind = "bool"', 'kind = "bits"'),
        "wrong-width": constraint.replace("#ac.math_int<1>", "#ac.math_int<2>"),
        "wrong-type": constraint[: constraint.index("type = !ac.bits")] + "type = i1}",
        "unsupported-kind": constraint.replace('kind = "hardware"', 'kind = "integer"'),
        "unsupported-source-kind": constraint.replace(
            'source_kind = "boolean"', 'source_kind = "fixed_bits"'
        ),
        "extra-key": constraint[:-1] + ", rogue = true}",
        "scalar-marker": '"boolean"',
    }
    changed = replacements[mutation]
    assert changed != constraint, (mutation, constraint)
    # Mutate both snapshots to isolate schema validation from disagreement.
    for path in (unit.body, unit.header):
        text = path.read_text()
        assert constraint in text
        path.write_text(text.replace(constraint, changed))
    final = tmp_path / "malformed.ac"
    rejected = native._link([unit], final, top="checks.bench.Top", role=None)
    assert rejected.returncode != 0, rejected.stdout
    assert re.search(
        r"field|constraint|domain|type", rejected.stderr, re.I
    ), rejected.stderr
    assert not final.exists()


@pytest.mark.parametrize("side", ["body", "header"])
@pytest.mark.parametrize("mutation", ["remove", "change"])
def test_boolean_field_snapshot_disagreement_preserves_final(
    tmp_path: Path, side, mutation
):
    unit = _native_unit(tmp_path)
    final = tmp_path / "published.ac"
    _ok(native._link([unit], final, top="checks.bench.Top", role=None))
    before = final.read_bytes()
    path = getattr(unit, side)
    text = path.read_text()
    constraint = _field_constraint(text)
    if mutation == "remove":
        needle, replacement = f", source_constraint = {constraint}", ""
    else:
        needle = constraint
        replacement = constraint.replace(
            'source_kind = "boolean"', 'source_kind = "fixed_bits"'
        )
    assert needle in text
    path.write_text(text.replace(needle, replacement, 1))
    rejected = native._link([unit], final, top="checks.bench.Top", role=None)
    assert rejected.returncode != 0, rejected.stdout
    assert re.search(
        r"field|constraint|interface|declaration|mismatch", rejected.stderr, re.I
    ), rejected.stderr
    assert final.read_bytes() == before


def test_boolean_field_host_transport_preserves_all_four_states(tmp_path: Path):
    # A host-driven Boolean port can physically contain Z. A comparison against
    # four_state('z') would yield X and therefore cannot prove Z transport.
    source, unit = tmp_path / "source", tmp_path / "unit"
    _write(
        source,
        "bench.py",
        TYPES
        + HELPERS
        + """
@ac.module
def Transport(flag: bool) -> bool:
    item = Envelope(inner=Leaf(flag=flag, payload=9), fixed=1)
    item.inner.payload = 6
    result = envelope_identity(item)
    return boolean_identity(result.inner.flag)
""",
    )
    _ok(_compile(source, "bench.py", unit))
    final = tmp_path / "transport.ac"
    _ok(_link([unit], final, "checks.bench.Transport"))
    cpp = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(cpp)))
    (cpp / "transport_probe.cpp").write_text(
        r"""
#include "pycircuit_system.hpp"
#include <stdexcept>
using Bit = gfsim::wire<gfsim::Bits<1>>;
int main() {
  for (unsigned workers : {1u, 2u}) {
    pyc_dut dut(workers);
    dut.system().Build();
    dut.system().Reset();
    for (unsigned code : {0u, 1u, 2u, 3u, 1u, 0u}) {
      auto input = code < 2 ? Bit::known(gfsim::Bits<1>{code})
        : Bit::fromPacked(code == 2 ? gfsim::FourState<1>::unknown()
                                   : gfsim::FourState<1>::highImpedance());
      dut.drive({input});
      if (dut.system().Step() != gfsim::SimStepResult::Running)
        throw std::runtime_error("transport step failed");
      auto result = dut.sample().result.packed();
      if (result.value().value() != (code == 1)
          || result.knownMask().value() != (code < 2)
          || result.zMask().value() != (code == 3))
        throw std::runtime_error("Boolean field raw planes changed");
    }
  }
}
""",
        encoding="utf-8",
    )
    with (cpp / "CMakeLists.txt").open("a") as cmake:
        cmake.write(
            "\nadd_executable(transport_probe transport_probe.cpp)\n"
            "target_link_libraries(transport_probe PRIVATE pycircuit_modules)\n"
        )
    build = tmp_path / "cpp-build"
    _ok(
        _run(
            [
                "cmake",
                "-S",
                str(cpp),
                "-B",
                str(build),
                "-G",
                "Ninja",
                f"-Dpycircuit_DIR={_install() / 'share/pycircuit/cmake'}",
            ]
        )
    )
    _ok(_run(["cmake", "--build", str(build), "--target", "transport_probe"]))
    _ok(_run([str(next(build.rglob("transport_probe")))]))
    rtl = tmp_path / "rtl"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(rtl)))
    bench = tmp_path / "transport_tb.sv"
    bench.write_text(
        """module transport_tb;
reg flag; wire result;
pyc_root dut(.*);
initial begin
flag=1'b0; #1; if(result!==1'b0)$fatal;
flag=1'b1; #1; if(result!==1'b1)$fatal;
flag=1'bx; #1; if(result!==1'bx)$fatal;
flag=1'bz; #1; if(result!==1'bz)$fatal;
flag=1'b1; #1; if(result!==1'b1)$fatal;
flag=1'b0; #1; if(result!==1'b0)$fatal;
$finish;
end
endmodule
"""
    )
    manifest = json.loads((rtl / "generated.json").read_text())
    sources = [rtl / row["path"] for row in manifest["files"] if row["role"] == "rtl"]
    binary = tmp_path / "transport.vvp"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "transport_tb",
                "-o",
                str(binary),
                *map(str, sources),
                str(bench),
            ]
        )
    )
    _ok(_run(["vvp", str(binary)]))


def test_boolean_record_state_reset_hold_and_discard(tmp_path: Path):
    source, unit = tmp_path / "source", tmp_path / "unit"
    _write(
        source,
        "bench.py",
        TYPES
        + """
@ac.module
def Remember(flag: bool, enable: bool) -> Leaf:
    saved: Leaf = Leaf(flag=False, payload=3)
    @ac.rule(guard=lambda: enable)
    def update():
        saved = Leaf(flag=flag, payload=saved.payload + 1)
    return saved
""",
    )
    _ok(_compile(source, "bench.py", unit))
    final = tmp_path / "state.ac"
    _ok(_link([unit], final, "checks.bench.Remember"))
    cpp = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(cpp)))
    # Existing native conformance probes expose the coordinator root to exercise
    # Work/DiscardNext directly; source hardware and runtime stay unchanged.
    header = cpp / "pycircuit_system.hpp"
    header.write_text(header.read_text().replace("private:", "public:"))
    (cpp / "state_probe.cpp").write_text(
        r"""
#include "pycircuit_system.hpp"
#include <stdexcept>
using Bit = gfsim::wire<gfsim::Bits<1>>;
static Bit bit(bool value) { return Bit::known(gfsim::Bits<1>{value}); }
static void require(bool condition) {
  if (!condition) throw std::runtime_error("Boolean record state oracle mismatch");
}
int main() {
  for (unsigned workers : {1u, 2u}) {
    pyc_dut dut(workers);
    auto drive = [&](bool flag, bool enable, bool clock, bool reset) {
      dut.drive({bit(flag), bit(enable), bit(clock), bit(reset)});
    };
    auto step = [&](bool flag, unsigned payload) {
      require(dut.system().Step() == gfsim::SimStepResult::Running);
      auto result = dut.sample().result.value();
      require(result.flag.value() == flag && result.payload.value() == payload);
    };
    drive(false, true, false, false);
    dut.system().Build(); dut.system().Reset();
    step(false, 3);
    drive(true, true, true, false);
    dut.root_->Work(); dut.root_->DiscardNext(); dut.observations().DiscardNext();
    require(dut.system().cycle() == 1);
    step(false, 3); // discarded rising edge did not consume clock or commit data
    drive(true, true, false, false); step(true, 4);
    drive(false, false, true, false); step(true, 4);
    drive(false, false, false, false); step(true, 4); // disabled edge holds
    drive(false, true, true, true); step(true, 4);
    drive(false, true, false, false); step(false, 3); // reset restored default
    drive(false, true, true, false); step(false, 3);
    drive(false, true, false, false); step(false, 4);
  }
}
"""
    )
    with (cpp / "CMakeLists.txt").open("a") as cmake:
        cmake.write(
            "\nadd_executable(state_probe state_probe.cpp)\n"
            "target_link_libraries(state_probe PRIVATE pycircuit_modules)\n"
        )
    build = tmp_path / "cpp-build"
    _ok(
        _run(
            [
                "cmake",
                "-S",
                str(cpp),
                "-B",
                str(build),
                "-G",
                "Ninja",
                f"-Dpycircuit_DIR={_install() / 'share/pycircuit/cmake'}",
            ]
        )
    )
    _ok(_run(["cmake", "--build", str(build), "--target", "state_probe"]))
    _ok(_run([str(next(build.rglob("state_probe")))]))
    rtl = tmp_path / "rtl"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(rtl)))
    bench = tmp_path / "state_tb.sv"
    bench.write_text(
        """module state_tb;
logic flag=0, enable=1, clk=0, rst=0, permit=0;
logic [2:0] phase=0;
wire [8:0] result;
wire complete, sampling_error, error;
pyc_root dut(.flag(flag), .enable(enable), .result(result),
  .pyc_7079635f636c6b(clk), .pyc_7079635f727374(rst),
  .pyc_phase(phase), .pyc_root_commit_ok(permit),
  .pyc_check_complete(complete), .pyc_sampling_error(sampling_error),
  .pyc_local_error(error));
task prepare(input logic expected_flag, input logic[7:0] expected_payload);
  #1; phase=1; #1;
  if (complete!==1 || sampling_error!==0 || error!==0) $fatal;
  if (result!=={expected_flag,expected_payload}) $fatal(1,"state oracle mismatch");
endtask
task commit;
  permit=1; phase=2; #1; phase=0; permit=0; #1;
endtask
task discard;
  permit=0; phase=2; #1; phase=3; #1; phase=0; #1;
endtask
initial begin
  permit=1; phase=4; #1; phase=2; #1; phase=0; permit=0; #1;
  prepare(0,3); commit();
  flag=1; clk=1; prepare(0,3); discard();
  prepare(0,3); commit();
  clk=0; prepare(1,4); commit();
  flag=0; enable=0; clk=1; prepare(1,4); commit();
  clk=0; prepare(1,4); commit();
  enable=1; clk=1; rst=1; prepare(1,4); commit();
  clk=0; rst=0; prepare(0,3); commit();
  clk=1; prepare(0,3); commit();
  clk=0; prepare(0,4); commit();
  $finish;
end
endmodule
"""
    )
    manifest = json.loads((rtl / "generated.json").read_text())
    sources = [rtl / row["path"] for row in manifest["files"] if row["role"] == "rtl"]
    binary = tmp_path / "state.vvp"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "state_tb",
                "-o",
                str(binary),
                str(_install() / "include/verilog/dffe.v"),
                *map(str, sources),
                str(bench),
            ]
        )
    )
    _ok(_run(["vvp", str(binary)]))
